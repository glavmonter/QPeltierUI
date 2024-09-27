#ifndef SERIALPORTWORKER_H
#define SERIALPORTWORKER_H

#include <QMutex>
#include <QThread>
#include <QList>
#include "spdlog/spdlog.h"

class SerialPortWorker : public QThread {
    Q_OBJECT

public:
    SerialPortWorker(bool isSimulator, QObject *parent = nullptr);
    ~SerialPortWorker();

    void startReceiver(const QString &portName, int waitTimeout);

signals:
    void error(const QString &s);
    void telemetryRecv(QList<double> &current, double temperature, uint32_t status);

private:
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
};

#endif // SERIALPORTWORKER_H
