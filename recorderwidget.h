#ifndef RECORDERWIDGET_H
#define RECORDERWIDGET_H

#include <QChart>

QT_FORWARD_DECLARE_CLASS(QLineSeries);
QT_FORWARD_DECLARE_CLASS(QValueAxis);
QT_FORWARD_DECLARE_CLASS(QXYSeries);

class RecorderWidget : public QChart  {
    Q_OBJECT

public:
    explicit RecorderWidget(QGraphicsItem *parent = nullptr);

    QLineSeries *series() const { return m_series; }

    void addData(const QList<double> &data);
    void addData(double data);
    void clear();

    void setRecordParameters(double tick, double recordTime);

private:
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;

    QList<QPointF> m_buffer;
    
    double m_tickTime;          ///< Время одного тика, секунд
    double m_recordTime;        ///< Полное отображаемое время, секунд
    int m_bufferMaxSize;
    double m_currentTickTime = 0;

    void updateAsisX();

    double m_currentTime = std::numeric_limits<double>::lowest();
};


#endif // RECORDERWIDGET_H
