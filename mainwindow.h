#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QChart>
#include <QValueAxis>
#include <QXYSeries>
#include <QLineSeries>

#include "serialportworker.h"
#include "recorderwidget.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(bool isSimulator, QWidget *parent = nullptr);
    ~MainWindow();

    bool isSimulator;
    bool isConnected = false;

    void SetConnected();
    void SetDisconnected();
    void PopulateSerialPorts();
    std::shared_ptr<spdlog::logger> logger;
    static QString toVersion(uint32_t version);
    
private:
    Ui::MainWindow *ui;
    SerialPortWorker *m_serialPortWorker = nullptr;

    void ConnectButtonsToSerialWorker();
    double m_graphTemperatureShowTime = 30;     ///< Длина отображения графика температуры, секунд
    double m_graphCurrentShowTime = 10;         ///< Длина отображения графика тока, секунд

    void ConfigureCharts();

    RecorderWidget *m_chartCurrent;
    RecorderWidget *m_chartTemperature;
    QList<QWidget *> m_widgetsInTabs;

    void ParseGetRequest(tec::Commands command, const QByteArray &data);
    QString m_recordFileName;
    QFile *m_recordFile = nullptr;
    qint64 m_recordIndex = -1;
    void RecordTelemetry(const QList<double> &current, double temperature);
    QString RecordIndexToTime(qint64 index, double timebase = 500e-6);
    
public slots:
    void SerialError(const QString &s);
    void Telemetry(const QList<double> &current, double temperature, uint32_t status);
    void commandExecute(SerialPortWorker::CommandError error, tec::Commands command, const QByteArray &data);
    
    void buttonGetClicked();
    void buttonSetClicked();
    void buttonRecordClicked();
};

#endif // MAINWINDOW_H
