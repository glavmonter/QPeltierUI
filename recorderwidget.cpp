#include <QLineSeries>
#include <QValueAxis>

#include "recorderwidget.h"


RecorderWidget::RecorderWidget(QGraphicsItem *parent) : QChart(QChart::ChartTypeCartesian, parent, Qt::Widget) {

    m_series = new QLineSeries();
    m_series->setUseOpenGL(true);
    addSeries(m_series);

    m_axisX = new QValueAxis();
    m_axisX->setLabelFormat("%g");
    m_axisX->setTitleText("Samples");

    m_axisY = new QValueAxis();
    m_axisY->setRange(-2, 2);
    m_axisY->setTitleText("Y");

    addAxis(m_axisX, Qt::AlignBottom);
    addAxis(m_axisY, Qt::AlignLeft);

    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);
}

void RecorderWidget::clear() {
    m_buffer.clear();
    m_currentTickTime = 0;
    m_currentTime = std::numeric_limits<double>::lowest();
    updateAsisX();
}


void RecorderWidget::addData(const QList<double> &data) {
static const int resolution = 1;
    int free_elements = m_bufferMaxSize - m_buffer.size();
    if (free_elements >= data.size()) {
        // Места много, добавляем к буферу
        for (int i(0); i < data.size(); ++i) {
            m_buffer.append(QPointF(m_currentTickTime, data[i]));
            m_currentTickTime += m_tickTime;
        }
    } else {
        // Буфера не хватает на полный блок дата, смещаем и добиваем до буфера
        if (free_elements > 0) {
            for (int i(0); i < free_elements; ++i) {
                m_buffer.append(QPointF(m_currentTickTime, data[i]));
            }
        }

        int start = 0;
        const int availableSamples = int(data.size()) / resolution;
        if (availableSamples < m_bufferMaxSize) {
            start = m_bufferMaxSize - availableSamples;
            const int offset = data.size() - free_elements;
            for (int s = 0; s < start; ++s) {
                m_buffer[s].setY(m_buffer.at(s + offset).y());
                m_buffer[s].setX(m_buffer.at(s + offset).x());
            }

            for (int s = start; s < m_bufferMaxSize; ++s) {
                m_buffer[s].setX(m_currentTickTime);
                m_currentTickTime += m_tickTime;
            }
        }
        
        int sample = 0;
        for (int s = start; s < m_bufferMaxSize; ++s, sample += resolution) {
            m_buffer[s].setY(data[sample]);
        }
    }

    auto left = m_buffer[0].x();
    auto right = m_buffer[m_buffer.size() - 1].x();
    if (left > -0.0001) {
        left = -m_recordTime + right;
    }

    if (right - m_currentTime > 0.1) {
        m_series->replace(m_buffer);
        m_axisX->setRange(left, right);
        m_currentTime = right;

        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        for (const auto &p : m_buffer) {
            if (p.y() < min) {
                min = p.y();
            }

            if (p.y() > max) {
                max = p.y();
            }
        }

        m_axisY->setRange(min - m_vericalRange, max + m_vericalRange);
    }
}

void RecorderWidget::addData(double data) {
QVector<double> d({data});
    addData(d);
}

void RecorderWidget::setVerticalRange(double range) {
    m_vericalRange = range;    
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
    m_buffer.clear();
    m_buffer.reserve(m_bufferMaxSize);
    m_currentTime = 0;
}

void RecorderWidget::updateAsisX() {
    if (m_currentTime < m_recordTime) {
        m_currentTime = -m_recordTime;
    }
    m_axisX->setRange(m_currentTime, m_currentTime + m_recordTime);
}
