#include <QSerialPort>
#include <QSerialPortInfo>
#include <commands.hpp>
#include "serialportworker.h"

#define SIMULATOR_CURRENT_COUNTER   (40)    ///< 40 измерений тока


SerialPortWorker::SerialPortWorker(bool isSimulator, QObject *parent) : m_isSimulator(isSimulator), QThread(parent) {
    logger = spdlog::get("Serial");
    logger->info("Create");
    if (isSimulator) {
        double pstide = 2.0 * M_PI / SinTableSize;
        for (auto i = 0; i < SinTableSize; i++) {
            m_phaseTable.append(::sin(pstide * i));
        }
    }
}

SerialPortWorker::~SerialPortWorker() {
    logger->debug("Shutdown");
    m_mutex.lock();
    m_quit = true;
    m_mutex.unlock();
    wait();
    logger->info("Shutdown successfully");
}

void SerialPortWorker::startReceiver(const QString &portName, int waitTimeout) {
const QMutexLocker locker(&m_mutex);
    m_portName = portName;
    m_waitTimeout = waitTimeout;
    if (!isRunning()) {
        start();
    }
}

void SerialPortWorker::run() {
    if (m_isSimulator) {
        runSimulator();
    } else {
        runSerial();
    }
}

void SerialPortWorker::runSimulator() {
    logger->info("Using simulator");
    
    size_t itable = 0;
    size_t itable_temperature = 128;
QList<double> current(40);
    while (!m_quit) {
        QThread::msleep(20);
        for (int i = 0; i < 40; i++) {
            current[i] = m_phaseTable[itable];
            itable = (itable + 1) & SinTableModMask;
        }

        double temperature = m_phaseTable[itable_temperature];
        itable_temperature = (itable_temperature + 1) & SinTableModMask;

        emit telemetryRecv(current, temperature, 0);
    }
}

void SerialPortWorker::runSerial() {
    logger->info("Using real serial port");
    bool currentPortNameChanged = false;

    m_mutex.lock();
    QString currentPortName;
    if (currentPortName != m_portName) {
        currentPortName = m_portName;
        currentPortNameChanged = true;
    }
    
    int currentWaitTimeout = m_waitTimeout;
    m_mutex.unlock();

    Wake wake;
    QSerialPort serial;
    QByteArray recvData;
    QDeadlineTimer dealineTimer;

    connect(&serial, &QSerialPort::errorOccurred, [this, &serial](QSerialPort::SerialPortError err) {
        switch (err) {
            case QSerialPort::DeviceNotFoundError:
            case QSerialPort::PermissionError:
            case QSerialPort::WriteError:
            case QSerialPort::ReadError:
            case QSerialPort::ResourceError:
            case QSerialPort::UnsupportedOperationError:
            case QSerialPort::UnknownError:
                emit error(QString("Serial port error occurred {%1} ('%2')").arg(serial.error()).arg(serial.errorString()));
                break;
        }
    });
    
    while (!m_quit) {
        if (currentPortName.isEmpty()) {
            logger->error("Serial port name must be defined");
            emit error("Port not set");
            return;
        } else if (currentPortNameChanged) {
            recvData.clear();

            serial.close();
            serial.setPortName(currentPortName);
            serial.setBaudRate(921600);
            serial.setReadBufferSize(1024 * 1024);
            if (!serial.open(QIODevice::ReadWrite)) {
                QString err = QString("Cannot open %1, error code %2").arg(currentPortName).arg(serial.error());
                logger->error(err.toStdString());
                emit error(err);
                return;
            }
        }

        if (serial.waitForReadyRead(currentWaitTimeout)) {
            recvData += serial.readAll();
            while (serial.waitForReadyRead(2)) {
                recvData += serial.readAll();
            }
        }

        while (!recvData.isEmpty()) {
            auto b = (uint8_t)recvData[0];
            recvData.removeAt(0);
            if (wake.ProcessInByte(b) == Wake::Status::READY) {
                if (wake.command() != qToUnderlying(tec::Commands::Telemetry)) {
                    logger->info("Received Wake {}. Remain {} bytes", wake.command(), recvData.size());
                    
                    m_mutex.lock();
                    if (m_commandPending == wake.command()) {
                        m_commandPending = qToUnderlying(tec::Commands::Invalid);
                        emit commandExecute(CommandError::NoError, static_cast<tec::Commands>(wake.command()), wake.dataArray());
                    }
                    m_mutex.unlock();
                }

                if (wake.command() == qToUnderlying(tec::Commands::Telemetry)) {
                    ParseTelemetryRecord(wake.data());
                }
                break;
            }
        }

        m_mutex.lock();
        if (currentPortName != m_portName) {
            currentPortName = m_portName;
            currentPortNameChanged = true;
        } else {
            currentPortNameChanged = false;
        }
        currentWaitTimeout = m_waitTimeout;

        if (m_txDataPending.size() > 0) {
            logger->info("Transmit");
            serial.write(m_txDataPending);
            m_txDataPending.clear();
            dealineTimer.setRemainingTime(m_commandTimeout);
        }

        if (m_commandPending != qToUnderlying(tec::Commands::Invalid) && dealineTimer.hasExpired()) {
            logger->warn("Command: {} timeout", m_commandPending);
            emit commandExecute(CommandError::TimeoutError, static_cast<tec::Commands>(m_commandPending), QByteArray());
            m_commandPending = qToUnderlying(tec::Commands::Invalid);
        }
        m_mutex.unlock();
    }
}


void SerialPortWorker::commandTransmit(tec::Commands cmd, const QByteArray &tx) {
    if (m_mutex.tryLock(100)) {
        m_commandPending = qToUnderlying(cmd);
        m_txDataPending = tx;
        m_mutex.unlock();
    }
    emit commandExecute(CommandError::Busy, cmd, QByteArray());
}


void SerialPortWorker::setOutputVoltage(double voltagePercent) {
float v = static_cast<float>(voltagePercent);
auto cmd = tec::Commands::VoltageGetSet;
QByteArray arr;
    arr.append(reinterpret_cast<const char *>(&v), 4);
    arr = Wake::PrepareTx(qToUnderlying(cmd), arr);
    commandTransmit(cmd, arr);    
}

void SerialPortWorker::getOutputVoltage() {
auto cmd = tec::Commands::VoltageGetSet;
    auto arr = Wake::PrepareTx(qToUnderlying(cmd), QByteArray());
    commandTransmit(cmd, arr);
}

void SerialPortWorker::setCurrentPid(PidVariableType type, double value) {
auto cmd = tec::Commands::CurrentPidGetSet;
float v = static_cast<float>(value);
QByteArray arr;
    arr.append(type);
    arr.append(reinterpret_cast<const char *>(&v), 4);
    commandTransmit(cmd, Wake::PrepareTx(qToUnderlying(cmd), arr));
}

void SerialPortWorker::getCurrentPid(PidVariableType type) {
auto cmd = tec::Commands::CurrentPidGetSet;
QByteArray arr;
    arr.append(type);
    commandTransmit(cmd, Wake::PrepareTx(qToUnderlying(cmd), arr));
}

void SerialPortWorker::sendFrame(tec::Commands cmd, const QByteArray &data) {
    commandTransmit(cmd, Wake::PrepareTx(qToUnderlying(cmd), data));
}

void SerialPortWorker::recvValid(const QList<uint8_t> &data, uint8_t command) {
QByteArray a((const char *)data.constData(), data.size());
    logger->trace("Recv valid command {}, size {}: {}", command, data.size(), a.toHex(':').toStdString());
    if (command == 0x55) {
        ParseTelemetryRecord(data);
    }
}

void SerialPortWorker::recvInvalid(const QList<uint8_t> &data, uint8_t command) {
QByteArray a((const char *)data.constData(), data.size());
    logger->warn("Recv invalid command {}, size {}: {}", command, data.size(), a.toHex(':').toStdString());
}


/**
 * @brief Разбор сигнала телеметрии
 * @param[in] data 90 байт сырых данных от контроллера Пельтье 
 */
void SerialPortWorker::ParseTelemetryRecord(const QList<uint8_t> &data) {
    if (data.size() != TelementrySize) {
        return;
    }

auto p_data = static_cast<const uint8_t *>(data.constData());
auto p_current = reinterpret_cast<const int16_t *>(p_data + 2);
auto p_temperature = reinterpret_cast<const float *>(p_current + 40);
auto p_status = reinterpret_cast<const uint32_t *>(p_temperature + 1);

    // 2 байта: порядковый номер фрейма
uint16_t cnt;
    ::memcpy(&cnt, p_data, sizeof(cnt));

    // 40 int16_t с током в мА
QList<double> current;
int16_t c;
    for (int i = 0; i < 40; i++) {
        current.append((*(p_current + i)) / 1000.0);
    }

float temperature = *p_temperature;
    emit telemetryRecv(current, temperature, 2);
}


QList<QPair<QString, QString>> SerialPortWorker::availablePorts() {
QList<QPair<QString, QString>> ret;
const auto serialPortInfos = QSerialPortInfo::availablePorts();
QStringList serials;

    for (const auto &portInfo : serialPortInfos) {
        if (portInfo.description().contains("CH340")) {
            serials << portInfo.portName();
        }
    }

    serials.sort();
    for (const auto p : serials) {
        ret.append(QPair(p, p));    
    }

    return ret;
}
