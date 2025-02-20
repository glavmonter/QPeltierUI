#include <QLineSeries>
#include <QValueAxis>

#include "recorderwidget.h"


RecorderWidget::RecorderWidget(int axis, QGraphicsItem *parent) : QChart(QChart::ChartTypeCartesian, parent, Qt::Widget), m_iNumAxis(axis) {

    m_series1 = new QLineSeries();
    m_series1->setUseOpenGL(true);
    addSeries(m_series1);

    m_axisX = new QValueAxis();
    m_axisX->setLabelFormat("%g");
    m_axisX->setTitleText("Time, seconds");

    m_axisY1 = new QValueAxis();
    m_axisY1->setRange(-2, 2);
    m_axisY1->setTitleText("Y");

    addAxis(m_axisX, Qt::AlignBottom);
    addAxis(m_axisY1, Qt::AlignLeft);

    m_series1->attachAxis(m_axisX);
    m_series1->attachAxis(m_axisY1);

    if (axis == 2) {
        m_series2 = new QLineSeries();
        m_series2->setUseOpenGL(true);
        addSeries(m_series2);

        m_axisY2 = new QValueAxis();
        m_axisY2->setRange(-2, 2);
        m_axisY2->setTitleText("Y2");
        addAxis(m_axisY2, Qt::AlignRight);

        m_series2->attachAxis(m_axisX);
        m_series2->attachAxis(m_axisY2);
    }
}

void RecorderWidget::clear() {
    m_buffer1.clear();
    m_buffer2.clear();

    m_currentTickTime = 0;
    m_currentTime = std::numeric_limits<double>::lowest();
    updateAsisX();
}


void RecorderWidget::addData(const QList<double> &data) {
static const int resolution = 1;
    int free_elements = m_bufferMaxSize - m_buffer1.size();
    if (free_elements >= data.size()) {
        // Места много, добавляем к буферу
        for (int i(0); i < data.size(); ++i) {
            m_buffer1.append(QPointF(m_currentTickTime, data[i]));
            m_currentTickTime += m_tickTime;
        }
    } else {
        // Буфера не хватает на полный блок дата, смещаем и добиваем до буфера
        if (free_elements > 0) {
            for (int i(0); i < free_elements; ++i) {
                m_buffer1.append(QPointF(m_currentTickTime, data[i]));
            }
        }

        int start = 0;
        const int availableSamples = int(data.size()) / resolution;
        if (availableSamples < m_bufferMaxSize) {
            start = m_bufferMaxSize - availableSamples;
            const int offset = data.size() - free_elements;
            for (int s = 0; s < start; ++s) {
                m_buffer1[s].setY(m_buffer1.at(s + offset).y());
                m_buffer1[s].setX(m_buffer1.at(s + offset).x());
            }

            for (int s = start; s < m_bufferMaxSize; ++s) {
                m_buffer1[s].setX(m_currentTickTime);
                m_currentTickTime += m_tickTime;
            }
        }
        
        int sample = 0;
        for (int s = start; s < m_bufferMaxSize; ++s, sample += resolution) {
            m_buffer1[s].setY(data[sample]);
        }
    }

    auto left = m_buffer1[0].x();
    auto right = m_buffer1[m_buffer1.size() - 1].x();
    if (left > -0.0001) {
        left = -m_recordTime + right;
    }

    if (right - m_currentTime > 0.1) {
        m_series1->replace(m_buffer1);
        m_axisX->setRange(left, right);
        m_currentTime = right;

        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        for (const auto &p : m_buffer1) {
            if (p.y() < min) {
                min = p.y();
            }

            if (p.y() > max) {
                max = p.y();
            }
        }

        m_axisY1->setRange(min - m_verticalRange1, max + m_verticalRange1);
    }
}

void RecorderWidget::addData(const QList<double> &dataY1, const QList<double> &dataY2) {
    if (m_iNumAxis != 2)
        return;

    if (dataY1.size() != dataY2.size())
        return;

static const int resolution = 1;
    int free_elements = m_bufferMaxSize - m_buffer1.size();
    if (free_elements >= dataY1.size()) {
        // Места много, добавляем к буферу
        for (int i(0); i < dataY1.size(); ++i) {
            m_buffer1.append(QPointF(m_currentTickTime, dataY1[i]));
            m_buffer2.append(QPointF(m_currentTickTime, dataY2[i]));
            m_currentTickTime += m_tickTime;
        }
    } else {
        // Буфера не хватает на полный блок дата, смещаем и добиваем до буфера
        if (free_elements > 0) {
            for (int i(0); i < free_elements; ++i) {
                m_buffer1.append(QPointF(m_currentTickTime, dataY1[i]));
                m_buffer2.append(QPointF(m_currentTickTime, dataY2[i]));
            }
        }

        int start = 0;
        const int availableSamples = int(dataY1.size()) / resolution;
        if (availableSamples < m_bufferMaxSize) {
            start = m_bufferMaxSize - availableSamples;
            const int offset = dataY1.size() - free_elements;
            for (int s = 0; s < start; ++s) {
                m_buffer1[s].setY(m_buffer1.at(s + offset).y());
                m_buffer1[s].setX(m_buffer1.at(s + offset).x());

                m_buffer2[s].setY(m_buffer2.at(s + offset).y());
                m_buffer2[s].setX(m_buffer2.at(s + offset).x());
            }

            for (int s = start; s < m_bufferMaxSize; ++s) {
                m_buffer1[s].setX(m_currentTickTime);
                m_buffer2[s].setX(m_currentTickTime);
                m_currentTickTime += m_tickTime;
            }
        }
        
        int sample = 0;
        for (int s = start; s < m_bufferMaxSize; ++s, sample += resolution) {
            m_buffer1[s].setY(dataY1[sample]);
            m_buffer2[s].setY(dataY2[sample]);
        }
    }

    auto left = m_buffer1[0].x();
    auto right = m_buffer1[m_buffer1.size() - 1].x();
    if (left > -0.0001) {
        left = -m_recordTime + right;
    }

    if (right - m_currentTime > 0.1) {
        m_series1->replace(m_buffer1);
        m_series2->replace(m_buffer2);
        m_axisX->setRange(left, right);
        m_currentTime = right;

        double minY1, minY2;
        minY1 = minY2 = std::numeric_limits<double>::max();
        double maxY1, maxY2;
        maxY1 = maxY2 = std::numeric_limits<double>::lowest();

        for (int i = 0; i < m_buffer1.size(); i++) {
            if (m_buffer1[i].y() < minY1)
                minY1 = m_buffer1[i].y();
            if (m_buffer2[i].y() < minY2)
                minY2 = m_buffer2[i].y();
            
            if (m_buffer1[i].y() > maxY1)
                maxY1 = m_buffer1[i].y();
            if (m_buffer2[i].y() > maxY2)
                maxY2 = m_buffer2[i].y();
        }

        m_axisY1->setRange(minY1 - m_verticalRange1, maxY1 + m_verticalRange1);
        m_axisY2->setRange(minY2 - m_verticalRange2 + m_verticalRange2Offset, maxY2 + m_verticalRange2 + m_verticalRange2Offset);
    }
}

void RecorderWidget::addData(double data) {
QVector<double> d({data});
    addData(d);
}

void RecorderWidget::addData(double dataY1, double dataY2) {
QVector<double> dY1({dataY1});
QVector<double> dY2({dataY2});
    addData(dY1, dY2);
}

void RecorderWidget::setVerticalRange(int axis, double range) {
    if (axis == 0) {
        m_verticalRange1 = range;
    } else {
        m_verticalRange2 = range;
    }
}

void RecorderWidget::setVerticalOffset(int axis, double offset) {
    if (axis != 0) {
        m_verticalRange2Offset = offset;
    }
}

/**
 * @brief Установить параметры самописца
 * @param[in] tick - длительность тика, секунд 
 * @param[in] recordTime - время записи, секунд
 */
void RecorderWidget::setRecordParameters(double tick, double recordTime) {
    m_tickTime = tick;
    m_recordTime = recordTime;
    
    updateAsisX();
    m_bufferMaxSize = ::round(m_recordTime / m_tickTime) + 1;
    m_buffer1.clear();
    m_buffer1.reserve(m_bufferMaxSize);
    if (m_iNumAxis == 2) {
        m_buffer2.clear();
        m_buffer2.reserve(m_bufferMaxSize);
    }
    m_currentTime = 0;
}

void RecorderWidget::updateAsisX() {
    if (m_currentTime < m_recordTime) {
        m_currentTime = -m_recordTime;
    }
    m_axisX->setRange(m_currentTime, m_currentTime + m_recordTime);
}
