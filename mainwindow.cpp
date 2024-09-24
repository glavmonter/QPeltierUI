
#ifdef __WIN32__
#include <spdlog/sinks/wincolor_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#else
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

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
    
    logger = std::make_shared<spdlog::logger>("Serial", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);

    logger = std::make_shared<spdlog::logger>("QPeltierUI", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::trace);
    spdlog::register_logger(logger);

    logger->info("Init QPeltierUI");
    
QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    ConfigurePlot(ui->plotCurrent, "Current", "A");
    ConfigurePlot(ui->plotTemperature, "Temperature", "C");
    ui->plotCurrent->xAxis->setTicker(timeTicker);
    ui->plotTemperature->xAxis->setTicker(timeTicker);
    PopulateSerialPorts();

    connect(ui->cmbSerialPorts, &QComboBox::activated, [=](int index) {
        if (ui->cmbSerialPorts->itemData(index).toString() == "__refresh__") {
            PopulateSerialPorts();
        }
    });

    connect(ui->btnConnectDisconnect, &QPushButton::clicked, [=]() {
        if (isConnected) {
            SetDisconnected();
        } else {
            SetConnected();
        }
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::ConfigurePlot(QCustomPlot *plot, const QString &xCaption, const QString &yCaption) {
    plot->setMinimumHeight(200);
    plot->setInteractions(QCP::iRangeZoom | QCP::iRangeDrag);
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0, new QCPTextElement(plot, xCaption));
    plot->addGraph();

QPen pen(Qt::red);
    pen.setWidth(2);
    plot->graph(0)->setPen(pen);
    plot->xAxis->setLabel("samples");
    plot->yAxis->setLabel(yCaption);

    connect(plot->xAxis, qOverload<const QCPRange &>(&QCPAxis::rangeChanged), plot->xAxis2, qOverload<const QCPRange &>(&QCPAxis::setRange));
    connect(plot->yAxis, qOverload<const QCPRange &>(&QCPAxis::rangeChanged), plot->yAxis2, qOverload<const QCPRange &>(&QCPAxis::setRange));
}

void MainWindow::SetConnected() {
    if (m_serialPortWorker) {
        delete m_serialPortWorker;
    }

    m_lastPointKey = 0;
    m_key = 0;
    m_serialPortWorker = new SerialPortWorker(isSimulator);
    connect(m_serialPortWorker, &SerialPortWorker::error, this, &MainWindow::SerialError, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));
    connect(m_serialPortWorker, &SerialPortWorker::telemetryRecv, this, &MainWindow::Telemetry, Qt::QueuedConnection);
    m_serialPortWorker->startReceiver(ui->cmbSerialPorts->currentData().toString(), 10);

    ui->plotCurrent->graph(0)->data()->clear();
    ui->plotCurrent->xAxis->setRange(0, m_graphTemperatureShowTime, Qt::AlignRight);
    ui->plotCurrent->replot();

    ui->plotTemperature->graph(0)->data()->clear();
    ui->plotTemperature->xAxis->setRange(0, m_graphCurrentShowTime, Qt::AlignRight);
    ui->plotTemperature->replot();

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
        ui->plotCurrent->graph(0)->addData(m_key, current[i]); // 500 мкс на отсчет
        m_key += 500e-6;
    }

    ui->plotTemperature->graph(0)->addData(m_key, temperature);

    if (m_key - m_lastPointKey > 0.1) {
        logger->trace("TLM[I]: {}", ui->plotCurrent->graph(0)->data()->size());
        m_lastPointKey = m_key;

        ui->plotCurrent->graph(0)->data()->removeBefore(m_key - m_graphCurrentShowTime);
        ui->plotCurrent->xAxis->setRange(m_key, m_graphCurrentShowTime, Qt::AlignRight);
        ui->plotCurrent->yAxis->rescale(true);
        ui->plotCurrent->replot();
        
        logger->trace("TLM[T]: {}", ui->plotTemperature->graph(0)->data()->size());
        ui->plotTemperature->graph(0)->data()->removeBefore(m_key - m_graphTemperatureShowTime);
        ui->plotTemperature->xAxis->setRange(m_key, m_graphTemperatureShowTime, Qt::AlignRight);
        ui->plotTemperature->yAxis->rescale(true);
        ui->plotTemperature->replot();
    }
}
