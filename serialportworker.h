#ifndef SERIALPORTWORKER_H
#define SERIALPORTWORKER_H

#include <QMutex>
#include <QThread>
#include <QList>
#include <QSerialPort>
#include "spdlog/spdlog.h"
#include "wake.h"
#include <commands.hpp>

class SerialPortWorker : public QThread {
    Q_OBJECT

public:
    SerialPortWorker(bool isSimulator, QObject *parent = nullptr);
    ~SerialPortWorker();

    void startReceiver(const QString &portName, int waitTimeout);
    static QList<QPair<QString, QString>> availablePorts();

    enum CommandError {
        Busy = 0,           ///< Начало выполнения команды
        NoError = 1,        ///< Команда выполнена
        Error = 3,          ///< Команда вернула ошибку
        TimeoutError = 4    ///< Команда прервана по таймауту
    };
    Q_ENUM(CommandError);

signals:
    void error(const QString &s);
    void telemetryRecv(QList<double> current, double temperature, uint32_t status);
    void commandExecute(CommandError error, tec::Commands command, const QByteArray &data);

public slots:
    void recvValid(const QList<uint8_t> &data, uint8_t command);
    void recvInvalid(const QList<uint8_t> &data, uint8_t command);
    
    void sendFrame(tec::Commands cmd, const QByteArray &data);

    void setOutputVoltage(double voltagePercent);
    void getOutputVoltage();
    void setCommandTimeout(qint64 timeoutMs) { m_commandTimeout = timeoutMs; }
    
    void setCurrentPid(PidVariableType type, double value);
    void getCurrentPid(PidVariableType type);

private:
static const int TelementrySize = 90;

    void run() override;

    void commandTransmit(tec::Commands cmd, const QByteArray &tx);

    bool m_isSimulator;
    QString m_portName;
    int m_waitTimeout = 0;
    qint64 m_commandTimeout = 1000;

    QMutex m_mutex;
    bool m_quit = false;

    std::shared_ptr<spdlog::logger> logger;

    void runSimulator();
    void runSerial();

    static constexpr uint32_t SinTableSize = 1024;
    static constexpr uint32_t SinTableModMask = SinTableSize - 1;
    QVector<double> m_phaseTable;
    
    QByteArray m_txDataPending;
    uint8_t m_commandPending = qToUnderlying(tec::Commands::Invalid);
    
    void ParseTelemetryRecord(const QList<uint8_t> &data);
};

#endif // SERIALPORTWORKER_H
