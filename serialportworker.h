#ifndef SERIALPORTWORKER_H
#define SERIALPORTWORKER_H

#include <QMutex>
#include <QThread>
#include <QList>
#include <QSerialPort>
#include "spdlog/spdlog.h"
#include "wake.h"


class SerialPortWorker : public QThread {
    Q_OBJECT

public:
    SerialPortWorker(bool isSimulator, QObject *parent = nullptr);
    ~SerialPortWorker();

    void startReceiver(const QString &portName, int waitTimeout);

signals:
    void error(const QString &s);
    void telemetryRecv(QList<double> current, double temperature, uint32_t status);

public slots:
    void recvValid(const QList<uint8_t> &data, uint8_t command);
    void recvInvalid(const QList<uint8_t> &data, uint8_t command);
    
    void setOutputVoltage(double voltagePercent);
    
protected:

private:
static const int TelementrySize = 90;

    void run() override;

    bool m_isSimulator;
    QString m_portName;
    int m_waitTimeout = 0;
    QMutex m_mutex;
    bool m_quit = false;

    std::shared_ptr<spdlog::logger> logger;

    void runSimulator();
    void runSerial();

    static constexpr uint32_t SinTableSize = 1024;
    static constexpr uint32_t SinTableModMask = SinTableSize - 1;
    QVector<double> m_phaseTable;
    
    QByteArray m_txDataPending;

    void ParseTelemetryRecord(const QList<uint8_t> &data);
};

#endif // SERIALPORTWORKER_H
