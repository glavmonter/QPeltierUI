
#ifdef __WIN32__
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#else
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include <QTimer>
#include <QSerialPortInfo>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(bool isSimulator, QWidget *parent) : isSimulator(isSimulator), QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    
    std::vector<spdlog::sink_ptr> sinks;
#ifdef __WIN32__
    sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(L"logs/QPeltierUI.log", 1024*1024*50, 10, true));
#else
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/QPeltierUI.log", 1024*1024*50, 10, true));
#endif
    
    logger = std::make_shared<spdlog::logger>("IO", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);

    logger = std::make_shared<spdlog::logger>("Serial", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);
    
    logger = std::make_shared<spdlog::logger>("Wake", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);
    
    logger = std::make_shared<spdlog::logger>("QPeltierUI", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);

    logger->info("Init QPeltierUI");

    ConfigureCharts();

    PopulateSerialPorts();
    connect(ui->cmbSerialPorts, &QComboBox::activated, [=](int index) {
        if (ui->cmbSerialPorts->itemData(index).toString() == "__refresh__") {
            PopulateSerialPorts();
        } else {
            ui->btnConnectDisconnect->setEnabled(true);
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
    connect(ui->btnCheckWake, &QPushButton::clicked, this, &MainWindow::CheckWake);

    m_widgetsInTabs.append(ui->spinDebugOutVoltage);
    m_widgetsInTabs.append(ui->btnDebugOutVoltageSet);
    for (auto w : m_widgetsInTabs) {
        w->setDisabled(true);
    }

    connect(ui->btnDebugOutVoltageSet, &QPushButton::clicked, [this](){ m_serialPortWorker->setOutputVoltage(ui->spinDebugOutVoltage->value()); });
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
    connect(m_serialPortWorker, &SerialPortWorker::telemetryRecv, this, &MainWindow::Telemetry, Qt::QueuedConnection);
    m_serialPortWorker->startReceiver(ui->cmbSerialPorts->currentData().toString(), 10);

    isConnected = true;
    ui->btnConnectDisconnect->setText("Disconnect");
    ui->cmbSerialPorts->setEnabled(false);

    for (auto w : m_widgetsInTabs) {
        w->setEnabled(true);
    }
}

void MainWindow::SetDisconnected() {
    if (m_serialPortWorker) {
        delete m_serialPortWorker;
        m_serialPortWorker = nullptr;
    }

    isConnected = false;
    ui->btnConnectDisconnect->setText("Connect");
    ui->cmbSerialPorts->setEnabled(true);

    for (auto w : m_widgetsInTabs) {
        w->setDisabled(true);
    }
}

void MainWindow::PopulateSerialPorts() {
    logger->debug("Populate serial ports");
    ui->cmbSerialPorts->clear();
    if (isSimulator) {
        logger->info("Set simulator mode");
        ui->cmbSerialPorts->addItem("Simulator");
        return;
    }

const auto serialPortInfos = QSerialPortInfo::availablePorts();
QStringList serials;
    for (const auto &portInfo : serialPortInfos) {
        logger->info("Found port {}: {}", portInfo.portName().toStdString(), portInfo.description().toStdString());
        if (portInfo.description().contains("CH340")) {
            serials << portInfo.portName();
        }
    }

    serials.sort();
    for (const auto p : serials) {
        ui->cmbSerialPorts->addItem(p, p);
    }
    ui->cmbSerialPorts->addItem("Refresh", "__refresh__");
    ui->btnConnectDisconnect->setDisabled(ui->cmbSerialPorts->count() == 1);
}

void MainWindow::SerialError(const QString &s) {
    logger->error("{}", s.toStdString());
    SetDisconnected();
}


void MainWindow::Telemetry(const QList<double> &current, double temperature, uint32_t status) {
    m_chartCurrent->addData(current);
    m_chartTemperature->addData(temperature);

    auto cur_mean = std::accumulate(current.begin(), current.end(), 0.0) / current.size();
    ui->labelTemperature->setText(tr("Temperature %1 °C").arg(temperature, 3, 'g', 3, '0'));    
    ui->labelCurrent->setText(tr("Current: %1 A").arg(cur_mean, 3, 'g', 3, '0'));
}

void MainWindow::AddTestData() {
QList<double> data;
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


void MainWindow::CheckWake() {
Wake *wake = new Wake(nullptr);

QList<QByteArray> datas {
        QByteArray::fromHex(QString("c0:55:5a:93:2d:08:00:10:00:10:00:10:00:08:00:08:00:10:00:10:00:08:00:08:00:10:00:10:00:10:00:08:00:08:00:10:00:08:00:00:00:08:00:08:00:08:00:08:00:08:00:08:00:10:00:08:00:08:00:10:00:10:00:10:00:08:00:08:00:08:00:10:00:08:00:08:00:08:00:08:00:10:00:10:00:00:00:f0:c1:00:00:00:00:35").replace(":", "").toLocal8Bit()),
        QByteArray::fromHex(QString("c0:55:5a:99:2d:08:00:08:00:10:00:10:00:10:00:08:00:08:00:08:00:08:00:10:00:10:00:08:00:10:00:08:00:10:00:08:00:08:00:10:00:10:00:10:00:00:00:08:00:10:00:08:00:00:00:08:00:10:00:10:00:08:00:08:00:08:00:08:00:08:00:08:00:08:00:08:00:08:00:10:00:08:00:08:00:00:00:db:dc:c1:00:00:00:00:31").replace(":", "").toLocal8Bit())
    };

    connect(wake, &Wake::recvInvalid, [this](const QList<uint8_t> &data, uint8_t command) {
        QByteArray a((const char *)data.constData(), data.size());
        this->logger->warn("Recv invalid command {}, size {}: {}", command, data.size(), a.toHex(':').toStdString());
    });

    connect(wake, &Wake::recvValid, [this](const QList<uint8_t> &data, uint8_t command) {
        QByteArray a((const char *)data.constData(), data.size());
        this->logger->info("Recv valid command {}, size {}: {}", command, data.size(), a.toHex(':').toStdString());
    });

    for (auto &arr : datas) {
        while (!arr.isEmpty()) {
            auto b = (uint8_t)arr[0];
            arr.removeFirst();
            wake->ProcessInByte(b);
        }
    }

    delete wake;
}
