#ifndef RECORDERWIDGET_H
#define RECORDERWIDGET_H

#include <QChart>

QT_FORWARD_DECLARE_CLASS(QLineSeries);
QT_FORWARD_DECLARE_CLASS(QValueAxis);
QT_FORWARD_DECLARE_CLASS(QXYSeries);

class RecorderWidget : public QChart  {
    Q_OBJECT

public:
    RecorderWidget(int axis, QGraphicsItem *parent = nullptr);

    // QLineSeries *series() const { return m_series; }

    void addData(const QList<double> &data);
    void addData(const QList<double> &dataY1, const QList<double> &dataY2);
    void addData(double data);
    void addData(double dataY1, double dataY2);

    void clear();

    void setRecordParameters(double tick, double recordTime);
    double timebase() const { return m_tickTime; }
    void setVerticalRange(int axis, double range);
    void setVerticalOffset(int axis, double offset);

private:
    QLineSeries *m_series1;
    QLineSeries *m_series2;

    QValueAxis *m_axisX;
    QValueAxis *m_axisY1;
    QValueAxis *m_axisY2;

    QList<QPointF> m_buffer1;
    QList<QPointF> m_buffer2;

    int m_iNumAxis;
    double m_verticalRange1 = 0.1;
    double m_verticalRange2 = 0.1;
    double m_verticalRange2Offset = 0.5;

    double m_tickTime;          ///< Время одного тика, секунд
    double m_recordTime;        ///< Полное отображаемое время, секунд
    int m_bufferMaxSize;
    double m_currentTickTime = 0;

    void updateAsisX();

    double m_currentTime = std::numeric_limits<double>::lowest();
};


#endif // RECORDERWIDGET_H
