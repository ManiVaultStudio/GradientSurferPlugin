#include "HighPerfLineChart.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <QDebug>

HighPerfLineChart::HighPerfLineChart(QWidget* parent)
    : QOpenGLWidget(parent) {
    m_selectedLine = -1;
    m_hoveredLine = -1;
    m_xMin = 0; m_xMax = 1;
    m_yMin = 0; m_yMax = 1;
}

void HighPerfLineChart::setLines(const QVector<LineData>& lines) {
    m_lines = lines;
    qDebug() << "[HighPerfLineChart] setLines called. Number of lines:" << m_lines.size();

    // Auto-range axes if not explicitly set or if data is out of range
    if (!m_lines.isEmpty()) {
        double xMin = std::numeric_limits<double>::max();
        double xMax = std::numeric_limits<double>::lowest();
        double yMin = std::numeric_limits<double>::max();
        double yMax = std::numeric_limits<double>::lowest();
        bool found = false;
        for (const auto& line : m_lines) {
            if (line.points.isEmpty())
                continue;
            for (const QPointF& pt : line.points) {
                if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                    continue;
                xMin = std::min(xMin, pt.x());
                xMax = std::max(xMax, pt.x());
                yMin = std::min(yMin, pt.y());
                yMax = std::max(yMax, pt.y());
                found = true;
            }
        }
        if (found && qIsFinite(xMin) && qIsFinite(xMax) && qIsFinite(yMin) && qIsFinite(yMax)) {
            // If all points are the same or only one point, expand the range by a default margin
            if (xMin == xMax) {
                xMin -= 0.5;
                xMax += 0.5;
            }
            if (yMin == yMax) {
                yMin -= 0.5;
                yMax += 0.5;
            }
            // Add a small margin for better visualization
            double xMargin = (xMax - xMin) * 0.05 + 1e-6;
            double yMargin = (yMax - yMin) * 0.05 + 1e-6;
            m_xMin = xMin - xMargin;
            m_xMax = xMax + xMargin;
            m_yMin = yMin - yMargin;
            m_yMax = yMax + yMargin;
            qDebug() << "[HighPerfLineChart] Auto-ranged x:" << m_xMin << m_xMax << "y:" << m_yMin << m_yMax;
        } else {
            // Fallback if no valid points
            m_xMin = 0; m_xMax = 1;
            m_yMin = 0; m_yMax = 1;
        }
    } else {
        // If no data, reset axis ranges
        m_xMin = 0; m_xMax = 1;
        m_yMin = 0; m_yMax = 1;
    }

    update();
}

void HighPerfLineChart::setXAxisName(const QString& name) { m_xAxisName = name; update(); }
void HighPerfLineChart::setYAxisName(const QString& name) { m_yAxisName = name; update(); }
void HighPerfLineChart::setXRange(double min, double max) { 
    if (!qIsFinite(min) || !qIsFinite(max) || min == max) {
        qWarning() << "[HighPerfLineChart] setXRange: Invalid range, ignoring.";
        return;
    }
    m_xMin = min; m_xMax = max; 
    qDebug() << "[HighPerfLineChart] setXRange:" << m_xMin << m_xMax;
    update(); 
}
void HighPerfLineChart::setYRange(double min, double max) { 
    if (!qIsFinite(min) || !qIsFinite(max) || min == max) {
        qWarning() << "[HighPerfLineChart] setYRange: Invalid range, ignoring.";
        return;
    }
    m_yMin = min; m_yMax = max; 
    qDebug() << "[HighPerfLineChart] setYRange:" << m_yMin << m_yMax;
    update(); 
}

int HighPerfLineChart::selectedLine() const { return m_selectedLine; }
int HighPerfLineChart::hoveredLine() const { return m_hoveredLine; }

void HighPerfLineChart::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(1, 1, 1, 1);
}

void HighPerfLineChart::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    qDebug() << "[HighPerfLineChart] paintGL called. m_lines.size():" << m_lines.size()
             << "xRange:" << m_xMin << m_xMax << "yRange:" << m_yMin << m_yMax;

    // Guard: avoid division by zero if range is invalid or widget size is zero
    if (m_xMax == m_xMin || m_yMax == m_yMin || width() <= 0 || height() <= 0) {
        QPainter painter(this);
        painter.setPen(Qt::red);
        painter.drawText(rect(), Qt::AlignCenter, "Invalid axis range");
        qDebug() << "[HighPerfLineChart] Invalid axis range or widget size!";
        return;
    }

    if (m_lines.isEmpty()) {
        QPainter painter(this);
        painter.setPen(Qt::darkGray);
        painter.drawText(rect(), Qt::AlignCenter, m_emptyMessage.isEmpty() ? "No data to display" : m_emptyMessage);
        qDebug() << "[HighPerfLineChart] No data to display!";
        return;
    }

    bool anyPoints = false;
    int totalPoints = 0;
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& line = m_lines[i];
        if (line.points.isEmpty())
            continue;
        for (const QPointF& pt : line.points) {
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
            anyPoints = true;
            ++totalPoints;
        }
    }
    if (!anyPoints) {
        QPainter painter(this);
        painter.setPen(Qt::darkGray);
        painter.drawText(rect(), Qt::AlignCenter, m_emptyMessage.isEmpty() ? "No points in line data" : m_emptyMessage);
        qDebug() << "[HighPerfLineChart] No points in line data!";
        return;
    }

    // If only a single point in all lines, show a message and draw the point
    if (totalPoints == 1) {
        QPainter painter(this);
        painter.setPen(Qt::darkGray);
        painter.drawText(rect(), Qt::AlignCenter, "Only one data point");
        // Draw the point
        for (const auto& line : m_lines) {
            if (line.points.isEmpty())
                continue;
            const QPointF& pt = line.points[0];
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
            QColor c = line.color.isValid() ? line.color : Qt::black;
            float alpha = (line.opacity >= 0.0f && line.opacity <= 1.0f) ? line.opacity : 1.0f;
            if (line.selected) { c = Qt::red; alpha = 1.0f; }
            glColor4f(c.redF(), c.greenF(), c.blueF(), alpha);
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            float x = (pt.x() - m_xMin) / (m_xMax - m_xMin) * 2 - 1;
            float y = 1 - (pt.y() - m_yMin) / (m_yMax - m_yMin) * 2;
            glVertex2f(x, y);
            glEnd();
        }
        return;
    }

    // Draw lines and points
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& line = m_lines[i];
        if (line.points.isEmpty())
            continue;
        QColor c = line.color.isValid() ? line.color : Qt::black;
        float alpha = (line.opacity >= 0.0f && line.opacity <= 1.0f) ? line.opacity : 1.0f;
        if (i == m_selectedLine) { c = Qt::red; alpha = 1.0f; }
        else if (i == m_hoveredLine) { c = c.lighter(150); alpha = 1.0f; }

        glColor4f(c.redF(), c.greenF(), c.blueF(), alpha);
        glLineWidth(line.selected ? 3.0f : 1.0f);

        // Always draw points for all lines
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        for (const QPointF& pt : line.points) {
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
            float x = (pt.x() - m_xMin) / (m_xMax - m_xMin) * 2 - 1;
            float y = 1 - (pt.y() - m_yMin) / (m_yMax - m_yMin) * 2;
            glVertex2f(x, y);
        }
        glEnd();

        // Draw line if more than one point
        if (line.points.size() > 1) {
            glBegin(GL_LINE_STRIP);
            for (const QPointF& pt : line.points) {
                if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                    continue;
                float x = (pt.x() - m_xMin) / (m_xMax - m_xMin) * 2 - 1;
                float y = 1 - (pt.y() - m_yMin) / (m_yMax - m_yMin) * 2;
                glVertex2f(x, y);
            }
            glEnd();
        }
    }

    // Draw axes and labels using QPainter for text
    QPainter painter(this);
    painter.setPen(Qt::black);
    if (!m_xAxisName.isEmpty())
        painter.drawText(rect().adjusted(10, height() - 30, -10, -10), Qt::AlignLeft, m_xAxisName);
    painter.save();
    painter.translate(10, 30);
    painter.rotate(-90);
    if (!m_yAxisName.isEmpty())
        painter.drawText(QRect(0, 0, height() - 40, 20), Qt::AlignLeft, m_yAxisName);
    painter.restore();
}

void HighPerfLineChart::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

QPointF HighPerfLineChart::screenToChart(const QPoint& p) const {
    if (width() <= 0 || height() <= 0 || m_xMax == m_xMin || m_yMax == m_yMin)
        return QPointF(0, 0);
    float x = m_xMin + (float(p.x()) / width()) * (m_xMax - m_xMin);
    float y = m_yMax - (float(p.y()) / height()) * (m_yMax - m_yMin);
    return QPointF(x, y);
}

int HighPerfLineChart::findLineAt(const QPoint& pos) const {
    if (m_lines.isEmpty() || width() <= 0)
        return -1;
    QPointF chartPt = screenToChart(pos);
    double minDist = 5.0 * (m_xMax - m_xMin) / width(); // tolerance in chart units
    int found = -1;
    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& line = m_lines[i];
        if (line.points.isEmpty())
            continue;
        for (const QPointF& pt : line.points) {
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
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
    m_xMin = 0; m_xMax = 1;
    m_yMin = 0; m_yMax = 1;
    m_emptyMessage = "No data to display"; // Ensure the no data message is shown
    update();
}

void HighPerfLineChart::setEmptyMessage(const QString& msg) {
    m_emptyMessage = msg;
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