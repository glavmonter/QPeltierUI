
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

    m_widgetsInTabs.append(ui->tabPageCommon);
    m_widgetsInTabs.append(ui->tabPageCurrent);
    m_widgetsInTabs.append(ui->tabPageTemperature);
    m_widgetsInTabs.append(ui->tabPageDebug);
    for (auto w : m_widgetsInTabs) {
        w->setDisabled(true);
    }
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
    connect(m_serialPortWorker, &SerialPortWorker::commandExecute, this, &MainWindow::commandExecute, Qt::QueuedConnection);
    ConnectButtonsToSerialWorker();
    m_serialPortWorker->startReceiver(ui->cmbSerialPorts->currentData().toString(), 10);

    isConnected = true;
    ui->btnConnectDisconnect->setText("Disconnect");
    ui->cmbSerialPorts->setEnabled(false);

    for (auto w : m_widgetsInTabs) {
        w->setEnabled(true);
    }
}

void MainWindow::ConnectButtonsToSerialWorker() {
    connect(ui->btnCurrentPidPGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnCurrentPidIGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnDebugOutVoltageGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);

    connect(ui->btnCurrentPidPSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnCurrentPidISet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnDebugOutVoltageSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
}

void MainWindow::buttonGetClicked() {
    if (sender() == ui->btnDebugOutVoltageGet) {
        m_serialPortWorker->getOutputVoltage();
    } else if (sender() == ui->btnCurrentPidPGet) {
        m_serialPortWorker->getCurrentPid(PidVariableType::Proportional);
    } else if (sender() == ui->btnCurrentPidIGet) {
        m_serialPortWorker->getCurrentPid(PidVariableType::Integral);
    } else if (sender() == ui->btnCurrentPidWindUpGet) {
        m_serialPortWorker->getCurrentPid(PidVariableType::WindUp);
    }
}

void MainWindow::buttonSetClicked() {
    if (sender() == ui->btnDebugOutVoltageSet) {
        m_serialPortWorker->setOutputVoltage(ui->spinDebugOutVoltage->value());
    } else if (sender() == ui->btnCurrentPidPSet) {
        m_serialPortWorker->setCurrentPid(PidVariableType::Proportional, ui->spinCurrentPidP->value());
    } else if (sender() == ui->btnCurrentPidISet) {
        m_serialPortWorker->setCurrentPid(PidVariableType::Integral, ui->spinCurrentPidI->value());
    } else if (sender() == ui->btnCurrentPidWindUpSet) {
        m_serialPortWorker->setCurrentPid(PidVariableType::WindUp, ui->spinCurrentPidWindUp->value());
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

auto ports = SerialPortWorker::availablePorts();
    for (const auto &p : ports) {
        ui->cmbSerialPorts->addItem(p.first, p.second);
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

void MainWindow::commandExecute(SerialPortWorker::CommandError error, tec::Commands command, const QByteArray &data) {
    logger->info("Command {} executed: {}. Size {}", qToUnderlying(command), static_cast<int>(error), data.size());

    switch (error) {
        case SerialPortWorker::CommandError::Busy:
            for (auto w : m_widgetsInTabs) {
                w->setDisabled(true);
            }
            break;

        case SerialPortWorker::CommandError::NoError:
            ParseGetRequest(command, data);
        case SerialPortWorker::CommandError::Error:
        case SerialPortWorker::CommandError::TimeoutError:
            for (auto w : m_widgetsInTabs) {
                w->setEnabled(true);
            }
            break;
    }
}


void MainWindow::ParseGetRequest(tec::Commands command, const QByteArray &data) {
float f_value;

    switch (command) {
        case tec::Commands::VoltageGetSet:
            if (data.size() == sizeof(f_value)) {
                ::memcpy(&f_value, data.constData(), data.size());
                ui->spinDebugOutVoltage->setValue(f_value);
            }
            break;

        case tec::Commands::CurrentPidGetSet:
            if (data.size() == sizeof(f_value) + 1) {
                ::memcpy(&f_value, data.constData() + 1, data.size() - 1);
                switch (data[0]) {
                    case PidVariableType::Proportional:
                        ui->spinCurrentPidP->setValue(f_value);
                        break;
                    case PidVariableType::Integral:
                        ui->spinCurrentPidI->setValue(f_value);
                        break;
                    case PidVariableType::WindUp:
                        ui->spinCurrentPidWindUp->setValue(f_value);
                        break;
                }
            }
            break;

        default:
            break;
    }
}
