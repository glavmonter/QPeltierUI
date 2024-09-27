
#ifdef __WIN32__
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#else
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include <QTimer>
#include <QRandomGenerator>
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

    connect(ui->btnAddData, &QPushButton::clicked, this, &MainWindow::AddTestData);
    connect(ui->btnAxisXApply, &QPushButton::clicked, this, &MainWindow::UpdateAxisX);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::ConfigureCharts() {
    m_chartCurrent = new RecorderWidget();
    m_chartCurrent->legend()->hide();
    m_chartCurrent->axisY()->setTitleText("Current, A");
    m_chartCurrent->axisY()->setRange(-1, 1);
    m_chartCurrent->setRecordParameters(500e-6, 10); // 500 мкс/тик, 10 секунд записи

    m_chartTemperature = new RecorderWidget();
    m_chartTemperature->legend()->hide();
    m_chartTemperature->axisY()->setTitleText("Temperature, C");
    m_chartTemperature->axisY()->setRange(-1, 1);
    m_chartTemperature->setRecordParameters(20e-3, 30); // 20 мс/тик, 30 секунд записи

    ui->chartViewCurrent->setChart(m_chartCurrent);
    ui->chartViewTemperature->setChart(m_chartTemperature);
}

void MainWindow::SetConnected() {
    if (m_serialPortWorker) {
        delete m_serialPortWorker;
    }

    m_chartCurrent->clear();
    m_chartTemperature->clear();

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


void MainWindow::Telemetry(const QList<double> &current, double temperature, uint32_t status) {
    m_chartCurrent->addData(current);
    m_chartTemperature->addData(temperature);
}

void MainWindow::AddTestData() {
QVector<double> data;
QRandomGenerator rng;
static int last_data = 0;

    for (int i(0); i < 16; ++i) {
        data.append(i + last_data);
    }

    last_data += 16;
    m_chartCurrent->addData(data);
}

void MainWindow::UpdateAxisX() {
    auto min = ui->spinAxisXMin->value();
    auto max = ui->spinAxisXMax->value();
    logger->info("Set range to [{} : {}]", min, max);
    m_chartCurrent->axisX()->setRange(min, max);
}
