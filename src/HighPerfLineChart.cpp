#include "HighPerfLineChart.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

HighPerfLineChart::HighPerfLineChart(QWidget* parent)
    : QOpenGLWidget(parent) {}

void HighPerfLineChart::setLines(const QVector<LineData>& lines) {
    m_lines = lines;
    update();
}

void HighPerfLineChart::setXAxisName(const QString& name) { m_xAxisName = name; update(); }
void HighPerfLineChart::setYAxisName(const QString& name) { m_yAxisName = name; update(); }
void HighPerfLineChart::setXRange(double min, double max) { m_xMin = min; m_xMax = max; update(); }
void HighPerfLineChart::setYRange(double min, double max) { m_yMin = min; m_yMax = max; update(); }

int HighPerfLineChart::selectedLine() const { return m_selectedLine; }
int HighPerfLineChart::hoveredLine() const { return m_hoveredLine; }

void HighPerfLineChart::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(1, 1, 1, 1);
}

void HighPerfLineChart::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw lines
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& line = m_lines[i];
        QColor c = line.color;
        float alpha = line.opacity;
        if (i == m_selectedLine) { c = Qt::red; alpha = 1.0f; }
        else if (i == m_hoveredLine) { c = c.lighter(150); alpha = 1.0f; }

        glColor4f(c.redF(), c.greenF(), c.blueF(), alpha);
        glLineWidth(line.selected ? 3.0f : 1.0f);

        glBegin(GL_LINE_STRIP);
        for (const QPointF& pt : line.points) {
            float x = (pt.x() - m_xMin) / (m_xMax - m_xMin) * 2 - 1;
            float y = 1 - (pt.y() - m_yMin) / (m_yMax - m_yMin) * 2;
            glVertex2f(x, y);
        }
        glEnd();
    }

    // Draw axes and labels using QPainter for text
    QPainter painter(this);
    painter.setPen(Qt::black);
    painter.drawText(rect().adjusted(10, height() - 30, -10, -10), Qt::AlignLeft, m_xAxisName);
    painter.save();
    painter.translate(10, 30);
    painter.rotate(-90);
    painter.drawText(QRect(0, 0, height() - 40, 20), Qt::AlignLeft, m_yAxisName);
    painter.restore();
}

void HighPerfLineChart::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

QPointF HighPerfLineChart::screenToChart(const QPoint& p) const {
    float x = m_xMin + (float(p.x()) / width()) * (m_xMax - m_xMin);
    float y = m_yMax - (float(p.y()) / height()) * (m_yMax - m_yMin);
    return QPointF(x, y);
}

int HighPerfLineChart::findLineAt(const QPoint& pos) const {
    QPointF chartPt = screenToChart(pos);
    double minDist = 5.0 * (m_xMax - m_xMin) / width(); // tolerance in chart units
    int found = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& line = m_lines[i];
        for (const QPointF& pt : line.points) {
            if (qAbs(pt.x() - chartPt.x()) < minDist && qAbs(pt.y() - chartPt.y()) < minDist) {
                found = i;
                break;
            }
        }
        if (found != -1) break;
    }
    return found;
}

void HighPerfLineChart::clear() {
    m_lines.clear();
    m_selectedLine = -1;
    m_hoveredLine = -1;
    update();
}

void HighPerfLineChart::mouseMoveEvent(QMouseEvent* event) {
    int idx = findLineAt(event->pos());
    if (idx != m_hoveredLine) {
        m_hoveredLine = idx;
        emit lineHovered(idx);
        update();
    }
}

void HighPerfLineChart::mousePressEvent(QMouseEvent* event) {
    int idx = findLineAt(event->pos());
    if (idx != m_selectedLine) {
        m_selectedLine = idx;
        emit lineSelected(idx);
        update();
    }
}