#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QChart>
#include <QValueAxis>
#include <QXYSeries>
#include <QLineSeries>

#include "serialportworker.h"
#include "xyseriesiodevice.h"


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

    QChart *m_chartCurrent;
    QLineSeries *m_seriesCurrent;
    XYSeriesIODevice *m_deviceCurrent;

    QChart *m_chartTemperature;
    QLineSeries *m_seriesTemperature;
    XYSeriesIODevice *m_deviceTemperature;

public slots:
    void SerialError(const QString &s);
    void Telemetry(QVector<double> current, double temperature, uint32_t status);

};
#endif // MAINWINDOW_H
