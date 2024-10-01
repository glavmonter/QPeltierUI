#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

private:
    Ui::MainWindow *ui;
    SerialPortWorker *m_serialPortWorker = nullptr;


    double m_graphTemperatureShowTime = 30;     ///< Длина отображения графика температуры, секунд
    double m_graphCurrentShowTime = 10;         ///< Длина отображения графика тока, секунд

    void ConfigureCharts();

    RecorderWidget *m_chartCurrent;
    RecorderWidget *m_chartTemperature;
    QList<QWidget *> m_widgetsInTabs;

    void ParseGetRequest(tec::Commands command, const QByteArray &data);

public slots:
    void SerialError(const QString &s);
    void Telemetry(const QList<double> &current, double temperature, uint32_t status);
    void commandExecute(SerialPortWorker::CommandError error, tec::Commands command, const QByteArray &data);    
};

#endif // MAINWINDOW_H
