#include "xyseriesiodevice.h"
#include <QXYSeries>

XYSeriesIODevice::XYSeriesIODevice(QXYSeries *series, QObject *parent) : QObject(parent), m_series(series) {
    logger = spdlog::get("IO");
}

qint64 XYSeriesIODevice::readData(char *data, qint64 maxSize) {
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}

qint64 XYSeriesIODevice::writeData(const QVector<double> &data) {
    static const int resolution = 1;

    if (m_buffer.isEmpty()) {
        m_buffer.reserve(sampleCount);
        for (int i = 0; i < sampleCount; ++i) {
            m_buffer.append(QPointF(i, 0));
        }
    }

    int start = 0;
    const int availableSamples = int(data.size()) / resolution;
    if (availableSamples < sampleCount) {
        start = sampleCount - availableSamples;
        for (int s = 0; s < start; ++s) {
            m_buffer[s].setY(m_buffer.at(s + availableSamples).y());
        }
    }

    int sample = 0;
    for (int s = start; s < sampleCount; ++s, sample += resolution) {
        m_buffer[s].setY(data[sample]);
    }

    m_series->replace(m_buffer);
    return (sampleCount - start) * resolution;
}

qint64 XYSeriesIODevice::writeData(double data) {
QVector<double> d({data});
    return writeData(d);
}
