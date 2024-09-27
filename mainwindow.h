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
    
private:
    Ui::MainWindow *ui;
    SerialPortWorker *m_serialPortWorker = nullptr;

    std::shared_ptr<spdlog::logger> logger;

    double m_graphTemperatureShowTime = 30;     ///< Длина отображения графика температуры, секунд
    double m_graphCurrentShowTime = 10;         ///< Длина отображения графика тока, секунд

    void ConfigureCharts();

    RecorderWidget *m_chartCurrent;
    RecorderWidget *m_chartTemperature;

public slots:
    void SerialError(const QString &s);
    void Telemetry(const QList<double> &current, double temperature, uint32_t status);

    void AddTestData();
    void UpdateAxisX();
};

#endif // MAINWINDOW_H
