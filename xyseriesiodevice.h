#ifndef XYSERIESIODEVICE_H
#define XYSERIESIODEVICE_H

#include <QIODevice>
#include <spdlog/spdlog.h>

QT_FORWARD_DECLARE_CLASS(QXYSeries);

class XYSeriesIODevice : public QObject {
    Q_OBJECT

public:
    explicit XYSeriesIODevice(QXYSeries *series, QObject *parent = nullptr);

    static const int sampleCount = 10000;

public:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const QVector<double> &data);
    qint64 writeData(double data);
    
private:
    QXYSeries *m_series = nullptr;
    QList<QPointF> m_buffer;

    std::shared_ptr<spdlog::logger> logger;
};


#endif // XYSERIESIODEVICE_H
