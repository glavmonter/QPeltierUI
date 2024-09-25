
#ifdef __WIN32__
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#else
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include <QTimer>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(bool isSimulator, QWidget *parent) : isSimulator(isSimulator), QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    
    std::vector<spdlog::sink_ptr> sinks;
#ifdef __WIN32__
    sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
#endif
    
    logger = std::make_shared<spdlog::logger>("IO", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);

    logger = std::make_shared<spdlog::logger>("Serial", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);

    logger = std::make_shared<spdlog::logger>("QPeltierUI", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    spdlog::register_logger(logger);

    logger->info("Init QPeltierUI");


    ConfigureCharts();

    PopulateSerialPorts();
    connect(ui->cmbSerialPorts, &QComboBox::activated, [=](int index) {
        if (ui->cmbSerialPorts->itemData(index).toString() == "__refresh__") {
            PopulateSerialPorts();
        }
    });

    connect(ui->btnConnectDisconnect, &QPushButton::clicked, [=]() {
        if (isConnected) {
            disconnect(m_serialPortWorker, &SerialPortWorker::telemetryRecv, this, &MainWindow::Telemetry);
            QTimer::singleShot(0, this, [this]() {
                SetDisconnected();
            });
        } else {
            SetConnected();
        }
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::ConfigureCharts() {
    m_chartCurrent = new QChart();
    m_seriesCurrent = new QLineSeries();
    m_chartCurrent->addSeries(m_seriesCurrent);

auto axisX = new QValueAxis();
    axisX->setRange(0, XYSeriesIODevice::sampleCount/2);
    axisX->setLabelFormat("%g");
    axisX->setTitleText("Samples");

auto axisY = new QValueAxis();
    axisY->setRange(-2, 2);
    axisY->setTitleText("Current, A");

    m_chartCurrent->addAxis(axisX, Qt::AlignBottom);
    m_seriesCurrent->attachAxis(axisX);
    m_chartCurrent->addAxis(axisY, Qt::AlignLeft);
    m_seriesCurrent->attachAxis(axisY);
    m_seriesCurrent->setUseOpenGL(true);
    m_chartCurrent->legend()->hide();
    m_chartCurrent->setTitle("Current");

    m_chartTemperature = new QChart();
    m_seriesTemperature = new QLineSeries();
    m_chartTemperature->addSeries(m_seriesTemperature);

    axisX = new QValueAxis();
    axisX->setRange(0, XYSeriesIODevice::sampleCount);
    axisX->setLabelFormat("%g");
    axisX->setTitleText("Samples");

    axisY = new QValueAxis();
    axisY->setRange(-1.5, 1.5);
    axisY->setTitleText("Temperature, C");
    
    m_chartTemperature->addAxis(axisX, Qt::AlignBottom);
    m_seriesTemperature->attachAxis(axisX);
    m_chartTemperature->addAxis(axisY, Qt::AlignLeft);
    m_seriesTemperature->attachAxis(axisY);
    m_seriesTemperature->setUseOpenGL(true);
    m_chartTemperature->legend()->hide();
    m_chartTemperature->setTitle("Temperature");

    ui->chartViewCurrent->setChart(m_chartCurrent);
    ui->chartViewTemperature->setChart(m_chartTemperature);

    m_deviceCurrent = new XYSeriesIODevice(m_seriesCurrent, this);
    m_deviceTemperature = new XYSeriesIODevice(m_seriesTemperature, this);
}

void MainWindow::SetConnected() {
    if (m_serialPortWorker) {
        delete m_serialPortWorker;
    }

    m_serialPortWorker = new SerialPortWorker(isSimulator);
    connect(m_serialPortWorker, &SerialPortWorker::error, this, &MainWindow::SerialError, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    connect(m_serialPortWorker, &SerialPortWorker::telemetryRecv, this, &MainWindow::Telemetry, Qt::BlockingQueuedConnection);
    m_serialPortWorker->startReceiver(ui->cmbSerialPorts->currentData().toString(), 10);

    isConnected = true;
    ui->btnConnectDisconnect->setText("Disconnect");
    ui->cmbSerialPorts->setEnabled(false);
}

void MainWindow::SetDisconnected() {
    if (m_serialPortWorker) {
        delete m_serialPortWorker;
        m_serialPortWorker = nullptr;
    }

    isConnected = false;
    ui->btnConnectDisconnect->setText("Connect");
    ui->cmbSerialPorts->setEnabled(true);
}

void MainWindow::PopulateSerialPorts() {
    logger->debug("Populate serial ports");
    ui->cmbSerialPorts->clear();
    if (isSimulator) {
        logger->info("Set simulator mode");
        ui->cmbSerialPorts->addItem("Simulator");
        return;
    }

    ui->cmbSerialPorts->addItem("COM1", "COM1");
    ui->cmbSerialPorts->addItem("COM2", "COM2");
    ui->cmbSerialPorts->addItem("Refresh", "__refresh__");
}

void MainWindow::SerialError(const QString &s) {
    SetDisconnected();
}


void MainWindow::Telemetry(QVector<double> current, double temperature, uint32_t status) {
    QCoreApplication::processEvents();

    for (int i = 0; i < current.size(); i++) {
        // ui->plotCurrent->graph(0)->addData(m_key, current[i]); // 500 мкс на отсчет
    }

    m_deviceCurrent->writeData(current);
    // m_deviceTemperature->writeData(temperature);
}
