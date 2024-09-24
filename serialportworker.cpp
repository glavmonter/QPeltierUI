#include <QSerialPort>
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

    while (!m_quit) {
        QThread::msleep(20);

        QVector<double> current;
        for (int i = 0; i < 40; i++) {
            current.append(m_phaseTable[itable]);
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

    QSerialPort serial;
    QByteArray recvData;

    while (!m_quit) {
        if (currentPortName.isEmpty()) {
            logger->error("Serial port name must be defined");
            emit error("Port not set");
            return;
        } else if (currentPortNameChanged) {
            recvData.clear();

            serial.close();
            serial.setPortName(currentPortName);
            if (!serial.open(QIODevice::ReadWrite)) {
                QString err = QString("Cannot open %1, error code %2").arg(currentPortName).arg(serial.error());
                logger->error(err.toStdString());
                emit error(err);
                return;
            }
        }

        if (serial.waitForReadyRead(currentWaitTimeout)) {
            recvData += serial.readAll();
            while (serial.waitForReadyRead(5)) {
                recvData += serial.readAll();
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
        m_mutex.unlock();
    }
}
