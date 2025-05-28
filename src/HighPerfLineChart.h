#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QVector>
#include <QColor>
#include <QString>
#include <QMap>

struct LineData {
    QVector<QPointF> points;
    QColor color = Qt::black;
    float opacity = 1.0f;
    bool selected = false;
};

class HighPerfLineChart : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit HighPerfLineChart(QWidget* parent = nullptr);

    // Data and appearance
    void setLines(const QVector<LineData>& lines);
    void setXAxisName(const QString& name);
    void setYAxisName(const QString& name);
    void setXRange(double min, double max);
    void setYRange(double min, double max);
    void clear();
    void setEmptyMessage(const QString& msg);
    void setChartTitle(const QString& title); // Add this setter

    // Selection/hover
    int selectedLine() const;
    int hoveredLine() const;

signals:
    void lineSelected(int index);
    void lineHovered(int index);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QVector<LineData> m_lines;
    QString m_xAxisName, m_yAxisName;
    double m_xMin = 0, m_xMax = 1, m_yMin = 0, m_yMax = 1;
    int m_selectedLine = -1;
    int m_hoveredLine = -1;
    QString m_emptyMessage;
    QString m_chartTitle; // Add this member for the chart title

    QPointF screenToChart(const QPoint& p) const;
    int findLineAt(const QPoint& pos) const;
};