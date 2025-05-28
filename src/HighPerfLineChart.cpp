#include "HighPerfLineChart.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <QDebug>

namespace {
enum OutCode {
    INSIDE = 0, // 0000
    LEFT   = 1, // 0001
    RIGHT  = 2, // 0010
    BOTTOM = 4, // 0100
    TOP    = 8  // 1000
};

int computeOutCode(double x, double y, double xmin, double xmax, double ymin, double ymax) {
    int code = OutCode::INSIDE;
    if (x < xmin)       code |= OutCode::LEFT;
    else if (x > xmax)  code |= OutCode::RIGHT;
    if (y < ymin)       code |= OutCode::BOTTOM;
    else if (y > ymax)  code |= OutCode::TOP;
    return code;
}

// Returns true if a clipped segment exists, and modifies (x0,y0,x1,y1) to the clipped segment
bool cohenSutherlandClip(double& x0, double& y0, double& x1, double& y1, double xmin, double xmax, double ymin, double ymax) {
    int outcode0 = computeOutCode(x0, y0, xmin, xmax, ymin, ymax);
    int outcode1 = computeOutCode(x1, y1, xmin, xmax, ymin, ymax);
    bool accept = false;

    while (true) {
        if (!(outcode0 | outcode1)) {
            accept = true;
            break;
        } else if (outcode0 & outcode1) {
            break;
        } else {
            double x, y;
            int outcodeOut = outcode0 ? outcode0 : outcode1;
            if (outcodeOut & OutCode::TOP) {
                x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
                y = ymax;
            } else if (outcodeOut & OutCode::BOTTOM) {
                x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
                y = ymin;
            } else if (outcodeOut & OutCode::RIGHT) {
                y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
                x = xmax;
            } else { // LEFT
                y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
                x = xmin;
            }
            if (outcodeOut == outcode0) {
                x0 = x; y0 = y;
                outcode0 = computeOutCode(x0, y0, xmin, xmax, ymin, ymax);
            } else {
                x1 = x; y1 = y;
                outcode1 = computeOutCode(x1, y1, xmin, xmax, ymin, ymax);
            }
        }
    }
    return accept;
}
} // end anonymous namespace

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

void HighPerfLineChart::setChartTitle(const QString& title) {
    m_chartTitle = title;
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

    // --- Chart area margins and rect ---
    const int leftMargin = 50;
    const int rightMargin = 20;
    const int topMargin = 40;
    const int bottomMargin = 40;
    QRect chartRect(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);

    // Helper lambda: map chart (data) coordinates to chartRect (screen) coordinates
    auto chartToScreen = [&](const QPointF& pt) -> QPointF {
        double xFrac = (pt.x() - m_xMin) / (m_xMax - m_xMin);
        double yFrac = (pt.y() - m_yMin) / (m_yMax - m_yMin);
        double x = chartRect.left() + xFrac * chartRect.width();
        double y = chartRect.bottom() - yFrac * chartRect.height();
        return QPointF(x, y);
    };

    // --- Draw lines and points (with line clipping to axes) ---
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

        // Draw points (all points inside axes)
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        for (const QPointF& pt : line.points) {
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
            if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                continue;
            QPointF screenPt = chartToScreen(pt);
            float x = (screenPt.x() / width()) * 2.0f - 1.0f;
            float y = 1.0f - (screenPt.y() / height()) * 2.0f;
            glVertex2f(x, y);
        }
        glEnd();

        // Draw lines (clip each segment to axes)
        if (line.points.size() > 1) {
            for (int j = 1; j < line.points.size(); ++j) {
                QPointF p0 = line.points[j - 1];
                QPointF p1 = line.points[j];
                if (!qIsFinite(p0.x()) || !qIsFinite(p0.y()) || !qIsFinite(p1.x()) || !qIsFinite(p1.y()))
                    continue;
                double x0 = p0.x(), y0 = p0.y(), x1 = p1.x(), y1 = p1.y();
                if (cohenSutherlandClip(x0, y0, x1, y1, m_xMin, m_xMax, m_yMin, m_yMax)) {
                    QPointF sp0 = chartToScreen(QPointF(x0, y0));
                    QPointF sp1 = chartToScreen(QPointF(x1, y1));
                    float sx0 = (sp0.x() / width()) * 2.0f - 1.0f;
                    float sy0 = 1.0f - (sp0.y() / height()) * 2.0f;
                    float sx1 = (sp1.x() / width()) * 2.0f - 1.0f;
                    float sy1 = 1.0f - (sp1.y() / height()) * 2.0f;
                    glBegin(GL_LINES);
                    glVertex2f(sx0, sy0);
                    glVertex2f(sx1, sy1);
                    glEnd();
                }
            }
        }
    }

    // --- Draw axes and labels using QPainter for text ---
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::black);

    // Draw X and Y axis lines
    QPoint xAxisStart(chartRect.left(), chartRect.bottom());
    QPoint xAxisEnd(chartRect.right(), chartRect.bottom());
    painter.drawLine(xAxisStart, xAxisEnd);
    QPoint yAxisStart(chartRect.left(), chartRect.bottom());
    QPoint yAxisEnd(chartRect.left(), chartRect.top());
    painter.drawLine(yAxisStart, yAxisEnd);

    // Draw tick marks and values
    int numXTicks = 5;
    int numYTicks = 5;
    QFontMetrics fm(painter.font());

    // X ticks and labels
    for (int i = 0; i <= numXTicks; ++i) {
        double t = double(i) / numXTicks;
        int x = chartRect.left() + t * chartRect.width();
        int y = chartRect.bottom();
        painter.drawLine(QPoint(x, y), QPoint(x, y + 6));
        double xValue = m_xMin + t * (m_xMax - m_xMin);
        QString xLabel = QString::number(xValue, 'g', 4);
        int labelWidth = fm.horizontalAdvance(xLabel);
        painter.drawText(x - labelWidth / 2, y + 6 + fm.ascent(), xLabel);
    }

    // Y ticks and labels
    for (int i = 0; i <= numYTicks; ++i) {
        double t = double(i) / numYTicks;
        int y = chartRect.bottom() - t * chartRect.height();
        int x = chartRect.left();
        painter.drawLine(QPoint(x, y), QPoint(x - 6, y));
        double yValue = m_yMin + t * (m_yMax - m_yMin);
        QString yLabel = QString::number(yValue, 'g', 4);
        int labelHeight = fm.height();
        painter.drawText(x - 8 - fm.horizontalAdvance(yLabel), y + labelHeight / 2 - 2, yLabel);
    }

    // Draw chart title at the top center
    if (!m_chartTitle.isEmpty()) {
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        painter.setFont(titleFont);
        painter.drawText(QRect(0, 0, width(), topMargin), Qt::AlignHCenter | Qt::AlignVCenter, m_chartTitle);
        painter.setFont(QFont());
    }

    // Draw x-axis label
    if (!m_xAxisName.isEmpty())
        painter.drawText(QRect(chartRect.left(), chartRect.bottom() + 22, chartRect.width(), 20), Qt::AlignHCenter | Qt::AlignVCenter, m_xAxisName);

    // Draw y-axis label (rotated)
    if (!m_yAxisName.isEmpty()) {
        painter.save();
        painter.translate(leftMargin - 35, chartRect.top() + chartRect.height() / 2);
        painter.rotate(-90);
        painter.drawText(QRect(0, 0, chartRect.height(), 20), Qt::AlignHCenter | Qt::AlignVCenter, m_yAxisName);
        painter.restore();
    }
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