
#ifdef __WIN32__
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#else
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include <QDateTime>
#include <QStringBuilder>
#include <QTimer>
#include <QSerialPortInfo>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <proto.hpp>

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

    ui->cmbWorkMode->addItem("Stopped", qToUnderlying(WorkMode::Stopped));
    ui->cmbWorkMode->addItem("Current Source", qToUnderlying(WorkMode::CurrentSource));
    ui->cmbWorkMode->addItem("Temperature", qToUnderlying(WorkMode::TemperatureStab));
    ui->cmbWorkMode->addItem("Debug", qToUnderlying(WorkMode::Debug));
    ui->cmbWorkMode->setCurrentIndex(0);

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

    connect(ui->btnRecordCurrent, &QPushButton::clicked, this, &MainWindow::buttonRecordClicked);

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
    m_chartTemperature->setVerticalRange(0.05);
    
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
    connect(ui->btnCurrentPidDGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnCurrentPidWindUpGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnDebugOutVoltageGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnWorkModeGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnDebugCurrentGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnVersionGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);

    connect(ui->btnTemperaturePidPGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnTemperaturePidIGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnTemperaturePidDGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnTemperaturePidWindupGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);
    connect(ui->btnTemperatureGet, &QPushButton::clicked, this, &MainWindow::buttonGetClicked);

    connect(ui->btnCurrentPidPSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnCurrentPidISet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnCurrentPidDSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnCurrentPidWindUpSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnDebugCurrentSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    
    connect(ui->btnTemperaturePidPSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnTemperaturePidISet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnTemperaturePidDSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnTemperaturePidWindupSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnTemperatureSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);

    connect(ui->btnDebugOutVoltageSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
    connect(ui->btnWorkModeSet, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);

    connect(ui->btnSaveSettings, &QPushButton::clicked, this, &MainWindow::buttonSetClicked);
}

void MainWindow::buttonGetClicked() {
    if (sender() == ui->btnDebugOutVoltageGet) {
        m_serialPortWorker->getOutputVoltage();
    } else if (sender() == ui->btnCurrentPidPGet) {
        m_serialPortWorker->getCurrentPid(PidVariableType::Proportional);
    } else if (sender() == ui->btnCurrentPidIGet) {
        m_serialPortWorker->getCurrentPid(PidVariableType::Integral);
    } else if (sender() == ui->btnCurrentPidDGet) {
        m_serialPortWorker->getCurrentPid(PidVariableType::Derivative);
    } else if (sender() == ui->btnCurrentPidWindUpGet) {
        m_serialPortWorker->getCurrentPid(PidVariableType::WindUp);
    } else if (sender() == ui->btnWorkModeGet) {
        m_serialPortWorker->getWorkMode();
    } else if (sender() == ui->btnDebugCurrentGet) {
        m_serialPortWorker->getDebugCurrent();
    } else if (sender() == ui->btnTemperaturePidPGet) {
        m_serialPortWorker->getTemperaturePid(PidVariableType::Proportional);
    } else if (sender() == ui->btnTemperaturePidIGet) {
        m_serialPortWorker->getTemperaturePid(PidVariableType::Integral);
    } else if (sender() == ui->btnTemperaturePidDGet) {
        m_serialPortWorker->getTemperaturePid(PidVariableType::Derivative);
    } else if (sender() == ui->btnTemperaturePidWindupGet) {
        m_serialPortWorker->getTemperaturePid(PidVariableType::WindUp);
    } else if (sender() == ui->btnTemperatureGet) {
        m_serialPortWorker->getTemperature();
    } else if (sender() == ui->btnVersionGet) {
        m_serialPortWorker->getVersion();
    }
}

void MainWindow::buttonSetClicked() {
    if (sender() == ui->btnDebugOutVoltageSet) {
        m_serialPortWorker->setOutputVoltage(ui->spinDebugOutVoltage->value());
    } else if (sender() == ui->btnCurrentPidPSet) {
        m_serialPortWorker->setCurrentPid(PidVariableType::Proportional, ui->spinCurrentPidP->value());
    } else if (sender() == ui->btnCurrentPidISet) {
        m_serialPortWorker->setCurrentPid(PidVariableType::Integral, ui->spinCurrentPidI->value());
    } else if (sender() == ui->btnCurrentPidDSet) {
        m_serialPortWorker->setCurrentPid(PidVariableType::Derivative, ui->spinCurrentPidD->value());
    } else if (sender() == ui->btnCurrentPidWindUpSet) {
        m_serialPortWorker->setCurrentPid(PidVariableType::WindUp, ui->spinCurrentPidWindUp->value());
    } else if (sender() == ui->btnWorkModeSet) {
        m_serialPortWorker->setWorkMode(static_cast<WorkMode>(ui->cmbWorkMode->currentData().toInt()));
    } else if (sender() == ui->btnDebugCurrentSet) {
        m_serialPortWorker->setDebugCurrent(ui->spinDebugCurrent->value());
    } else if (sender() == ui->btnTemperaturePidPSet) {
        m_serialPortWorker->setTemperaturePid(PidVariableType::Proportional, ui->spinTemperaturePidP->value());
    } else if (sender() == ui->btnTemperaturePidISet) {
        m_serialPortWorker->setTemperaturePid(PidVariableType::Integral, ui->spinTemperaturePidI->value());
    } else if (sender() == ui->btnTemperaturePidDSet) {
        m_serialPortWorker->setTemperaturePid(PidVariableType::Derivative, ui->spinTemperaturePidD->value());
    } else if (sender() == ui->btnTemperaturePidWindupSet) {
        m_serialPortWorker->setTemperaturePid(PidVariableType::WindUp, ui->spinTemperaturePidWindup->value());
    } else if (sender() == ui->btnTemperatureSet) {
        m_serialPortWorker->setTemperature(ui->spinTemperature->value());
    } else if (sender() == ui->btnSaveSettings) {
        m_serialPortWorker->saveSettingsToEeprom();
    }
}

void MainWindow::buttonRecordClicked() {
    if (m_recordFileName.isEmpty()) {
        ui->btnRecordCurrent->setText("Stop Record");
        m_recordFileName = QString("Record-%1.csv").arg(QDateTime::currentDateTime().toString("dd.MM.yy-hh_mm_ss_zzz"));
        ui->lblRecordCurrentFileName->setText(QString("`%1`").arg(m_recordFileName));
        if (m_recordFile) {
            m_recordFile->close();
            m_recordFile = nullptr;
        }

        m_recordFile = new QFile(m_recordFileName);
        m_recordFile->open(QIODevice::WriteOnly | QIODevice::Truncate);
        m_recordFile->write(QString("Index; Time [s]; Current[A]\n").toLatin1());
        m_recordIndex = 0;
    } else {
        ui->btnRecordCurrent->setText("Start Record");
        ui->lblRecordCurrentFileName->setText(QString("`%1` stopped, %2 s").arg(m_recordFileName).arg(RecordIndexToTime(m_recordIndex, m_chartCurrent->timebase())));
        m_recordFileName.clear();
        m_recordFile->close();
        m_recordFile = nullptr;
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

    if (!m_recordFileName.isEmpty()) {
        buttonRecordClicked();
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
    RecordTelemetry(current, temperature);

    auto cur_mean = std::accumulate(current.begin(), current.end(), 0.0) / current.size();
    ui->labelTemperature->setText(tr("Temperature %1 °C").arg(temperature, 0, 'g', 4, '0'));    
    ui->labelCurrent->setText(tr("Current: %1 A").arg(cur_mean, 0, 'g', 3, '0'));
}

QString MainWindow::RecordIndexToTime(qint64 index, double timebase) {
    return QString("%1").arg(index * timebase, 4, 'g', 5, ' ').replace('.', ',');
}
    
void MainWindow::RecordTelemetry(const QList<double> &current, double temperature) {
    if (m_recordFile == nullptr) {
        return;
    }

    QString time;
    for (int i = 0; i < current.size(); i++) {
        time = RecordIndexToTime(m_recordIndex, m_chartCurrent->timebase());
        QString value = QString("%1").arg(current[i], 4, 'g', 5, '0').replace('.', ',');
        QString rec;
        if (i != 0) {
            rec = QString("%1; %2; %3\n").arg(m_recordIndex).arg(time).arg(value);
        } else {
            QString value_t = QString("%1").arg(temperature, 4, 'g', 5, '0').replace('.', ',');
            rec = QString("%1; %2; %3; %4\n").arg(m_recordIndex).arg(time).arg(value).arg(value_t);
        }
        m_recordFile->write(rec.toLatin1());
        m_recordIndex++;
    }

    ui->lblRecordCurrentFileName->setText(QString("`%1` - %2 s").arg(m_recordFileName).arg(time));
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
uint32_t ui32_value;
WorkMode wm;

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
                switch (static_cast<PidVariableType>(data[0])) {
                    case PidVariableType::Proportional:
                        ui->spinCurrentPidP->setValue(f_value);
                        break;
                    case PidVariableType::Integral:
                        ui->spinCurrentPidI->setValue(f_value);
                        break;
                    case PidVariableType::Derivative:
                        ui->spinCurrentPidD->setValue(f_value);
                        break;
                    case PidVariableType::WindUp:
                        ui->spinCurrentPidWindUp->setValue(f_value);
                        break;
                }
            }
            break;

        case tec::Commands::WorkModeSetGet:
            if (data.size() == 1) {
                int index = ui->cmbWorkMode->findData(data[0]);
                if (index >= 0) {
                    ui->cmbWorkMode->setCurrentIndex(data[0]);
                }
            }
            break;

        case tec::Commands::CurrentStabGetSet:
            if (data.size() == 4) {
                ::memcpy(&f_value, data.constData(), data.size());
                ui->spinDebugCurrent->setValue(f_value);
            }
            break;

        case tec::Commands::TemperaturePidGetSet:
            if (data.size() == sizeof(f_value) + 1) {
                ::memcpy(&f_value, data.constData() + 1, data.size() - 1);
                switch (static_cast<PidVariableType>(data[0])) {
                    case PidVariableType::Proportional:
                        ui->spinTemperaturePidP->setValue(f_value);
                        break;
                    case PidVariableType::Integral:
                        ui->spinTemperaturePidI->setValue(f_value);
                        break;
                    case PidVariableType::Derivative:
                        ui->spinTemperaturePidD->setValue(f_value);
                        break;
                    case PidVariableType::WindUp:
                        ui->spinTemperaturePidWindup->setValue(f_value);
                        break;
                }
            }
            break;

        case tec::Commands::TemperatureStabGetSet:
            if (data.size() == 4) {
                ::memcpy(&f_value, data.constData(), data.size());
                ui->spinTemperature->setValue(f_value);
            }
            break;

        case tec::Commands::VersionGet:
            if (data.size() == 8) {
                uint32_t hw_ver;
                uint32_t sw_ver;
                ::memcpy(&hw_ver, reinterpret_cast<const void *>(data.constData()    ), 4);
                ::memcpy(&sw_ver, reinterpret_cast<const void *>(data.constData() + 4), 4);
                QString ver = QString("Version HW: %1, SW: %2").arg(toVersion(hw_ver)).arg(toVersion(sw_ver));
                ui->lblVersion->setText(ver);
            }
            break;

        default:
            break;
    }
}

QString MainWindow::toVersion(uint32_t version) {
    uint32_t major = version / 10000;
    uint32_t minor = (version - major * 10000) / 100;
    uint32_t patch = (version - major * 10000 - minor * 100);
    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}
