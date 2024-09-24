#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include "serialportworker.h"
#include "qcustomplot.h"

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

    double m_lastPointKey = 0;
    double m_key = 0;

    void ConfigurePlot(QCustomPlot *plot, const QString &xCaption, const QString &yCaption);

public slots:
    void SerialError(const QString &s);
    void Telemetry(QVector<double> current, double temperature, uint32_t status);

};
#endif // MAINWINDOW_H
