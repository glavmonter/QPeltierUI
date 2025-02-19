
#ifdef __WIN32__
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#else
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#endif

#include <QDateTime>
#include <QStringBuilder>
#include <QTimer>
#include <QSerialPortInfo>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <proto.hpp>

static QString FormatFloat(double d, int leading, int precision) {
    QLocale locale(QLocale::English);
    QString str = locale.toString(d, 'f', precision);
    QStringList parts = str.split('.');

    // Добавим лидирующие пробелы перед целой частью
    if (parts[0].length() < leading) {
        QString leadingEmpty(leading - parts[0].length(), ' ');
        str = leadingEmpty + str;
    }
    return str;
}

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
    connect(ui->cmbSerialPorts, &QComboBox::activated, [this](int index) {
        if (ui->cmbSerialPorts->itemData(index).toString() == "__refresh__") {
            PopulateSerialPorts();
        } else {
            ui->btnConnectDisconnect->setEnabled(true);
        }
    });

    connect(ui->btnConnectDisconnect, &QPushButton::clicked, [this]() {
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
    m_widgetsInTabs.append(ui->tabPageLimits);
    m_widgetsInTabs.append(ui->tabPageDebug);
    m_widgetsInTabs.append(ui->tabPageTemperatureAutomat);
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
    auto axisY = m_chartCurrent->axes(Qt::Vertical);
    axisY[0]->setTitleText("Current, A");
    axisY[0]->setRange(-1, 1);
    m_chartCurrent->setRecordParameters(500e-6, 10); // 500 мкс/тик, 10 секунд записи

    m_chartTemperature = new RecorderWidget();
    m_chartTemperature->legend()->hide();
    axisY = m_chartTemperature->axes(Qt::Vertical);
    axisY[0]->setTitleText("Temperature, C");
    axisY[0]->setRange(-1, 1);
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

#define CONNECT(list, w, lambda)  do { \
                                connect((w), &QPushButton::clicked, (lambda));\
                                (list) << (w); \
                            } while(0)

void MainWindow::ConnectButtonsToSerialWorker() {
    m_widgetsButtons.clear();

    CONNECT(m_widgetsButtons, ui->btnCurrentPidPGet,          [this]() { m_serialPortWorker->getCurrentPid(PidVariableType::Proportional); });
    CONNECT(m_widgetsButtons, ui->btnCurrentPidIGet,          [this]() { m_serialPortWorker->getCurrentPid(PidVariableType::Integral); });
    CONNECT(m_widgetsButtons, ui->btnCurrentPidDGet,          [this]() { m_serialPortWorker->getCurrentPid(PidVariableType::Derivative); });
    CONNECT(m_widgetsButtons, ui->btnCurrentPidWindUpGet,     [this]() { m_serialPortWorker->getCurrentPid(PidVariableType::WindUp); });
    CONNECT(m_widgetsButtons, ui->btnDebugOutVoltageGet,      [this]() { m_serialPortWorker->getOutputVoltage(); });
    CONNECT(m_widgetsButtons, ui->btnWorkModeGet,             [this]() { m_serialPortWorker->getWorkMode(); });
    CONNECT(m_widgetsButtons, ui->btnDebugCurrentGet,         [this]() { m_serialPortWorker->getDebugCurrent(); });
    CONNECT(m_widgetsButtons, ui->btnVersionGet,              [this]() { m_serialPortWorker->getVersion(); });

    CONNECT(m_widgetsButtons, ui->btnTemperaturePidPGet,      [this]() { m_serialPortWorker->getTemperaturePid(PidVariableType::Proportional); });
    CONNECT(m_widgetsButtons, ui->btnTemperaturePidIGet,      [this]() { m_serialPortWorker->getTemperaturePid(PidVariableType::Integral); });
    CONNECT(m_widgetsButtons, ui->btnTemperaturePidDGet,      [this]() { m_serialPortWorker->getTemperaturePid(PidVariableType::Derivative); });
    CONNECT(m_widgetsButtons, ui->btnTemperaturePidWindupGet, [this]() { m_serialPortWorker->getTemperaturePid(PidVariableType::WindUp); });
    CONNECT(m_widgetsButtons, ui->btnTemperatureGet,          [this]() { m_serialPortWorker->getTemperature(); });

    CONNECT(m_widgetsButtons, ui->btnLimitVoltageLowGet,      [this]() { m_serialPortWorker->getLimits(Limits::VoltageLow); logger->info("Limits::VoltageLow"); });
    CONNECT(m_widgetsButtons, ui->btnLimitVoltageHighGet,     [this]() { m_serialPortWorker->getLimits(Limits::VoltageHigh); });
    CONNECT(m_widgetsButtons, ui->btnLimitCurrentLowGet,      [this]() { m_serialPortWorker->getLimits(Limits::CurrentLow); });
    CONNECT(m_widgetsButtons, ui->btnLimitCurrentHighGet,     [this]() { m_serialPortWorker->getLimits(Limits::CurrentHigh); });
    CONNECT(m_widgetsButtons, ui->btnLimitCurrentGet,         [this]() { m_serialPortWorker->getLimits(Limits::CurrentLimitSW); });

    CONNECT(m_widgetsButtons, ui->btnAutomatCoolingDownGet,   [this]() { m_serialPortWorker->getTemperatureAutomat(Automat::CoolingDown); });
    CONNECT(m_widgetsButtons, ui->btnAutomatHeatingUpGet,     [this]() { m_serialPortWorker->getTemperatureAutomat(Automat::HeatingUp); });

    CONNECT(m_widgetsButtons, ui->btnCurrentPidPSet,          [this]() { m_serialPortWorker->setCurrentPid(PidVariableType::Proportional, ui->spinCurrentPidP->value()); });
    CONNECT(m_widgetsButtons, ui->btnCurrentPidISet,          [this]() { m_serialPortWorker->setCurrentPid(PidVariableType::Integral, ui->spinCurrentPidI->value()); });
    CONNECT(m_widgetsButtons, ui->btnCurrentPidDSet,          [this]() { m_serialPortWorker->setCurrentPid(PidVariableType::Derivative, ui->spinCurrentPidD->value()); });
    CONNECT(m_widgetsButtons, ui->btnCurrentPidWindUpSet,     [this]() { m_serialPortWorker->setCurrentPid(PidVariableType::WindUp, ui->spinCurrentPidWindUp->value()); });
    CONNECT(m_widgetsButtons, ui->btnDebugCurrentSet,         [this]() { m_serialPortWorker->setDebugCurrent(ui->spinDebugCurrent->value()); });
    
    CONNECT(m_widgetsButtons, ui->btnTemperaturePidPSet,      [this]() { m_serialPortWorker->setTemperaturePid(PidVariableType::Proportional, ui->spinTemperaturePidP->value()); });
    CONNECT(m_widgetsButtons, ui->btnTemperaturePidISet,      [this]() { m_serialPortWorker->setTemperaturePid(PidVariableType::Integral, ui->spinTemperaturePidI->value()); });
    CONNECT(m_widgetsButtons, ui->btnTemperaturePidDSet,      [this]() { m_serialPortWorker->setTemperaturePid(PidVariableType::Derivative, ui->spinTemperaturePidD->value()); });
    CONNECT(m_widgetsButtons, ui->btnTemperaturePidWindupSet, [this]() { m_serialPortWorker->setTemperaturePid(PidVariableType::WindUp, ui->spinTemperaturePidWindup->value()); });
    CONNECT(m_widgetsButtons, ui->btnTemperatureSet,          [this]() { m_serialPortWorker->setTemperature(ui->spinTemperature->value()); });

    CONNECT(m_widgetsButtons, ui->btnLimitVoltageLowSet,      [this]() { m_serialPortWorker->setLimits(Limits::VoltageLow,  ui->spinLimitVoltageLow->value()); });
    CONNECT(m_widgetsButtons, ui->btnLimitVoltageHighSet,     [this]() { m_serialPortWorker->setLimits(Limits::VoltageHigh, ui->spinLimitVoltageHigh->value()); });
    CONNECT(m_widgetsButtons, ui->btnLimitCurrentLowSet,      [this]() { m_serialPortWorker->setLimits(Limits::CurrentLow,  ui->spinLimitCurrentLow->value()); });
    CONNECT(m_widgetsButtons, ui->btnLimitCurrentHighSet,     [this]() { m_serialPortWorker->setLimits(Limits::CurrentHigh, ui->spinLimitCurrentHigh->value()); });
    CONNECT(m_widgetsButtons, ui->btnLimitCurrentSet,         [this]() { m_serialPortWorker->setLimits(Limits::CurrentLimitSW, ui->spinLimitCurrent->value()); });

    CONNECT(m_widgetsButtons, ui->btnAutomatCoolingDownSet,   [this]() { m_serialPortWorker->setTemperatureAutomat(Automat::CoolingDown, ui->spinAutomatCoolingDown->value()); });
    CONNECT(m_widgetsButtons, ui->btnAutomatHeatingUpSet,     [this]() { m_serialPortWorker->setTemperatureAutomat(Automat::HeatingUp, ui->spinAutomatHeatingUp->value()); });

    CONNECT(m_widgetsButtons, ui->btnDebugOutVoltageSet,      [this]() { m_serialPortWorker->setOutputVoltage(ui->spinDebugOutVoltage->value()); });
    CONNECT(m_widgetsButtons, ui->btnDebugMessageSet,         [this]() { m_serialPortWorker->setDebugMessage(ui->editDebugMessage->text()); });
    CONNECT(m_widgetsButtons, ui->btnWorkModeSet,             [this]() { m_serialPortWorker->setWorkMode(static_cast<WorkMode>(ui->cmbWorkMode->currentData().toInt())); });

    CONNECT(m_widgetsButtons, ui->btnSaveSettings,            [this]() { m_serialPortWorker->saveSettingsToEeprom(); });
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

    for (auto w : m_widgetsButtons) {
        w->disconnect();
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
    ui->labelTemperature->setText(tr("Temperature %1 °C").arg(FormatFloat(temperature, 3, 3)));
    ui->labelCurrent->setText(tr("Current: %1 A").arg(FormatFloat(cur_mean, 3, 2)));
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

        case tec::Commands::LimitsGetSet:
            if (data.size() == sizeof(f_value) + 1) {
                ::memcpy(&f_value, data.constData() + 1, data.size() - 1);
                switch (static_cast<Limits>(data[0])) {
                    case Limits::VoltageLow:
                        ui->spinLimitVoltageLow->setValue(f_value);
                        break;
                    case Limits::VoltageHigh:
                        ui->spinLimitVoltageHigh->setValue(f_value);
                        break;
                    case Limits::CurrentLow:
                        ui->spinLimitCurrentLow->setValue(f_value);
                        break;
                    case Limits::CurrentHigh:
                        ui->spinLimitCurrentHigh->setValue(f_value);
                        break;
                    case Limits::CurrentLimitSW:
                        ui->spinLimitCurrent->setValue(f_value);
                        break;
                    default:
                        logger->warn("Unknown Limit reply: {}", data[0]);
                }
            }
            break;

        case tec::Commands::AutomatGetSet:
            if (data.size() == sizeof(f_value) + 1) {
                ::memcpy(&f_value, data.constData() + 1, data.size() - 1);
                switch (static_cast<Automat>(data[0])) {
                    case Automat::HeatingUp:
                        ui->spinAutomatHeatingUp->setValue(f_value);
                        break;
                    case Automat::CoolingDown:
                        ui->spinAutomatCoolingDown->setValue(f_value);
                        break;
                    default:
                        logger->warn("Unkown Automat reply: {}", data[0]);
                }
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
