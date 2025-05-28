#include "HighPerfLineChart.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <random>
#include <QFont>
#include <QPen>

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

// Static member definition
std::unordered_map<int, QString> LineData::groupStringMap;

// --- Merged binary normalization helper ---
// Remove m_interchangeXY logic from here: normalization is always on original x/y, not swapped
QVector<QPointF> HighPerfLineChart::binaryNormalize(const QVector<QPointF>& points, BinaryNormAxis axis) const {
    bool enabled = (axis == BinaryNormAxis::X) ? m_binaryNormalizeX : m_binaryNormalizeY;
    if (!enabled)
        return points;
    QVector<QPointF> normed;
    normed.reserve(points.size());
    if (axis == BinaryNormAxis::X) {
        for (const QPointF& pt : points) {
            double x = (pt.x() > m_binaryNormalizeXThreshold) ? 1.0 : 0.0;
            normed.append(QPointF(x, pt.y()));
        }
    } else {
        for (const QPointF& pt : points) {
            double y = (pt.y() > m_binaryNormalizeYThreshold) ? 1.0 : 0.0;
            normed.append(QPointF(pt.x(), y));
        }
    }
    return normed;
}

HighPerfLineChart::HighPerfLineChart(QWidget* parent)
    : QOpenGLWidget(parent) {
    m_selectedLine = -1;
    m_hoveredLine = -1;
    m_xMin = 0; m_xMax = 1;
    m_yMin = 0; m_yMax = 1;
    m_groupAveragingEnabled = false;
    m_logTransformX = false;
    m_logTransformY = false;
    m_strongLogTransform = false;
    m_gradientLineEnabled = false;
    m_noiseRemovalEnabled = false;
    m_noiseRemovalWindow = 3;
    m_noiseRemovalThreshold = 0.0;
    m_lineWidth = 1.0f;
    m_pointSize = 6.0f;
    m_axisFont = QFont();
    m_axisColor = Qt::black;
    m_numXTicks = 5;
    m_numYTicks = 5;
    m_gridEnabled = false;
    m_gridColor = QColor(220,220,220);
    m_backgroundColor = Qt::white;
    m_showPoints = true;
    m_axisLineWidth = 1.0f;
    m_xAxisLineColor = Qt::black;
    m_yAxisLineColor = Qt::black;
    m_tickLabelFont = QFont();
    m_tickLabelColor = Qt::black;
    m_leftMargin = 50;
    m_rightMargin = 20;
    m_topMargin = 40;
    m_bottomMargin = 40;
    m_legendFont = QFont();
    m_legendColor = Qt::black;
    m_legendX = -1;
    m_legendY = -1;
    m_binaryNormalizeX = false;
    m_binaryNormalizeY = false;
    m_binaryNormalizeXThreshold = 0.5;
    m_binaryNormalizeYThreshold = 0.5;
    m_interchangeXY = false; // 
}

// Add log transform setter/getter
void HighPerfLineChart::setLogTransformX(bool enabled) {
    m_logTransformX = enabled;
    update();
}

void HighPerfLineChart::setLogTransformY(bool enabled) {
    m_logTransformY = enabled;
    update();
}

void HighPerfLineChart::setStrongLogTransform(bool enabled) {
    m_strongLogTransform = enabled;
    update();
}

// Gradient line and noise removal setters
void HighPerfLineChart::setGradientLineEnabled(bool enabled) {
    m_gradientLineEnabled = enabled;
    update();
}
void HighPerfLineChart::setNoiseRemovalEnabled(bool enabled) {
    m_noiseRemovalEnabled = enabled;
    update();
}
void HighPerfLineChart::setNoiseRemovalWindow(int window) {
    m_noiseRemovalWindow = window;
    update();
}
void HighPerfLineChart::setNoiseRemovalThreshold(double threshold) {
    m_noiseRemovalThreshold = threshold;
    update();
}

// --- Binary normalization setters/getters ---
void HighPerfLineChart::setBinaryNormalizeX(bool enabled) {
    m_binaryNormalizeX = enabled;
    update();
}
void HighPerfLineChart::setBinaryNormalizeY(bool enabled) {
    m_binaryNormalizeY = enabled;
    update();
}
void HighPerfLineChart::setBinaryNormalizeXThreshold(double threshold) {
    m_binaryNormalizeXThreshold = threshold;
    update();
}
void HighPerfLineChart::setBinaryNormalizeYThreshold(double threshold) {
    m_binaryNormalizeYThreshold = threshold;
    update();
}
bool HighPerfLineChart::binaryNormalizeX() const { return m_binaryNormalizeX; }
bool HighPerfLineChart::binaryNormalizeY() const { return m_binaryNormalizeY; }
double HighPerfLineChart::binaryNormalizeXThreshold() const { return m_binaryNormalizeXThreshold; }
double HighPerfLineChart::binaryNormalizeYThreshold() const { return m_binaryNormalizeYThreshold; }

// --- Add interchangeXY setter/getter ---
void HighPerfLineChart::setInterchangeXY(bool enabled) {
    m_interchangeXY = enabled;
    update();
}
bool HighPerfLineChart::interchangeXY() const {
    return m_interchangeXY;
}

// --- Customization setters/getters ---
void HighPerfLineChart::setLineWidth(float w) { m_lineWidth = w; update(); }
void HighPerfLineChart::setPointSize(float s) { m_pointSize = s; update(); }
void HighPerfLineChart::setAxisFont(const QFont& f) { m_axisFont = f; update(); }
void HighPerfLineChart::setAxisColor(const QColor& c) { m_axisColor = c; update(); }
void HighPerfLineChart::setNumXTicks(int n) { m_numXTicks = n; update(); }
void HighPerfLineChart::setNumYTicks(int n) { m_numYTicks = n; update(); }
void HighPerfLineChart::setGridEnabled(bool en) { m_gridEnabled = en; update(); }
void HighPerfLineChart::setGridColor(const QColor& c) { m_gridColor = c; update(); }
void HighPerfLineChart::setBackgroundColor(const QColor& c) { m_backgroundColor = c; update(); }
void HighPerfLineChart::setShowPoints(bool en) { m_showPoints = en; update(); }

// New customization setters/getters
void HighPerfLineChart::setAxisLineWidth(float w) { m_axisLineWidth = w; update(); }
void HighPerfLineChart::setXAxisLineColor(const QColor& c) { m_xAxisLineColor = c; update(); }
void HighPerfLineChart::setYAxisLineColor(const QColor& c) { m_yAxisLineColor = c; update(); }
void HighPerfLineChart::setTickLabelFont(const QFont& f) { m_tickLabelFont = f; update(); }
void HighPerfLineChart::setTickLabelColor(const QColor& c) { m_tickLabelColor = c; update(); }
void HighPerfLineChart::setChartMargins(int left, int right, int top, int bottom) {
    m_leftMargin = left; m_rightMargin = right; m_topMargin = top; m_bottomMargin = bottom; update();
}
void HighPerfLineChart::setLegendFont(const QFont& f) { m_legendFont = f; update(); }
void HighPerfLineChart::setLegendColor(const QColor& c) { m_legendColor = c; update(); }
void HighPerfLineChart::setLegendPosition(int x, int y) { m_legendX = x; m_legendY = y; update(); }

// Stronger log transform: log(log(x+e)+1), e = 1e-6 to avoid log(0)
double HighPerfLineChart::strongLog(double v) const {
    double e = 1e-6;
    if (v <= 0) return 0.0;
    return std::log(std::log(v + e) + 1.0);
}
double HighPerfLineChart::invStrongLog(double lv) const {
    // Inverse of log(log(x+e)+1): x = exp(exp(lv)-1)-e
    double e = 1e-6;
    return std::exp(std::exp(lv) - 1.0) - e;
}

// Log transform helpers
double HighPerfLineChart::logX(double x) const {
    if (m_strongLogTransform)
        return strongLog(x);
    // Use log10(x+1) to handle x=0 gracefully, and avoid negative/zero
    return (x > 0) ? std::log10(x + 1.0) : 0.0;
}
double HighPerfLineChart::invLogX(double lx) const {
    if (m_strongLogTransform)
        return invStrongLog(lx);
    return std::pow(10.0, lx) - 1.0;
}
double HighPerfLineChart::logY(double y) const {
    if (m_strongLogTransform)
        return strongLog(y);
    // Use log10(y+1) to handle y=0 gracefully, and avoid negative/zero
    return (y > 0) ? std::log10(y + 1.0) : 0.0;
}
double HighPerfLineChart::invLogY(double ly) const {
    if (m_strongLogTransform)
        return invStrongLog(ly);
    return std::pow(10.0, ly) - 1.0;
}

// Remove m_interchangeXY logic from filterNoise as well
QVector<QPointF> HighPerfLineChart::filterNoise(const QVector<QPointF>& points) const {
    if (!m_noiseRemovalEnabled || points.size() < 3)
        return points;
    QVector<QPointF> filtered;
    int w = m_noiseRemovalWindow;
    for (int i = 0; i < points.size(); ++i) {
        int start = static_cast<int>(std::max(0.0, static_cast<double>(i - w)));
        int end = static_cast<int>(std::min(static_cast<double>(points.size() - 1), static_cast<double>(i + w)));
        double sumX = 0, sumY = 0;
        int count = 0;
        for (int j = start; j <= end; ++j) {
            sumX += points[j].x();
            sumY += points[j].y();
            ++count;
        }
        double avgX = sumX / count;
        double avgY = sumY / count;
        // Optionally, threshold-based noise removal
        if (m_noiseRemovalThreshold > 0.0) {
            if (std::abs(points[i].y() - avgY) > m_noiseRemovalThreshold)
                continue; // skip noisy point
        }
        filtered.append(QPointF(avgX, avgY));
    }
    return filtered;
}

// Map slope to color (blue for negative, green for zero, red for positive)
QColor HighPerfLineChart::slopeToColor(double slope) const {
    // Clamp slope for color mapping
    double s = std::tanh(slope / 2.0); // -1 to 1
    int r = int(127 + 128 * s);
    int g = int(200 - 100 * std::abs(s));
    int b = int(127 - 128 * s);
    return QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
}

void HighPerfLineChart::setLines(const QVector<LineData>& lines) {
    m_lines = lines;
    qDebug() << "[HighPerfLineChart] setLines called. Number of lines:" << m_lines.size();

    // --- Detailed debug info ---
    int totalPoints = 0;
    QSet<int> uniqueGroups;
    double xMin = std::numeric_limits<double>::max();
    double xMax = std::numeric_limits<double>::lowest();
    double yMin = std::numeric_limits<double>::max();
    double yMax = std::numeric_limits<double>::lowest();

    for (int i = 0; i < m_lines.size(); ++i) {
        const auto& line = m_lines[i];
        int npts = line.points.size();
        totalPoints += npts;
        uniqueGroups.insert(line.group);
        qDebug() << "  Line" << i
                 << "points:" << npts
                 << "group:" << line.group
                 << "color:" << line.color
                 << "opacity:" << line.opacity
                 << "lineWidth:" << line.lineWidth
                 << "pointSize:" << line.pointSize;
        for (const QPointF& pt : line.points) {
            if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                continue;
            double xval, yval;
            if (m_interchangeXY) {
                xval = m_logTransformY ? logY(pt.y()) : pt.y();
                yval = m_logTransformX ? logX(pt.x()) : pt.x();
            } else {
                xval = m_logTransformX ? logX(pt.x()) : pt.x();
                yval = m_logTransformY ? logY(pt.y()) : pt.y();
            }
            xMin = std::min(xMin, xval);
            xMax = std::max(xMax, xval);
            yMin = std::min(yMin, yval);
            yMax = std::max(yMax, yval);
        }
    }
    qDebug() << "  Total points:" << totalPoints;
    qDebug() << "  Unique groups:" << uniqueGroups.size() << uniqueGroups;
    if (totalPoints > 0) {
        qDebug() << "  X range:" << xMin << "to" << xMax;
        qDebug() << "  Y range:" << yMin << "to" << yMax;
    }
    qDebug() << "  Chart customization:"
             << "defaultLineWidth:" << m_lineWidth
             << "defaultPointSize:" << m_pointSize
             << "axisFont:" << m_axisFont
             << "axisColor:" << m_axisColor
             << "numXTicks:" << m_numXTicks
             << "numYTicks:" << m_numYTicks
             << "gridEnabled:" << m_gridEnabled
             << "backgroundColor:" << m_backgroundColor
             << "showPoints:" << m_showPoints;

    // --- Apply binary normalization to all lines if enabled ---
    if (m_binaryNormalizeX || m_binaryNormalizeY) {
        for (auto& line : m_lines) {
            if (m_binaryNormalizeX)
                line.points = binaryNormalize(line.points, BinaryNormAxis::X);
            if (m_binaryNormalizeY)
                line.points = binaryNormalize(line.points, BinaryNormAxis::Y);
        }
    }

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
                double xval, yval;
                if (m_interchangeXY) {
                    xval = m_logTransformY ? logY(pt.y()) : pt.y();
                    yval = m_logTransformX ? logX(pt.x()) : pt.x();
                } else {
                    xval = m_logTransformX ? logX(pt.x()) : pt.x();
                    yval = m_logTransformY ? logY(pt.y()) : pt.y();
                }
                xMin = std::min(xMin, static_cast<double>(xval));
                xMax = std::max(xMax, static_cast<double>(xval));
                yMin = std::min(yMin, static_cast<double>(yval));
                yMax = std::max(yMax, static_cast<double>(yval));
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

void HighPerfLineChart::setGroupAveragingEnabled(bool enabled) {
    m_groupAveragingEnabled = enabled;
    update();
}

void HighPerfLineChart::setGroupStringMap(const std::unordered_map<int, QString>& groupMap) {
    m_groupStringMap = groupMap;
    updateGroupColors();
    update();
}

void HighPerfLineChart::updateGroupColors() {
    m_groupColorMap.clear();
    // Assign a unique color for each group
    std::vector<QColor> palette = {
        QColor(31,119,180), QColor(255,127,14), QColor(44,160,44), QColor(214,39,40),
        QColor(148,103,189), QColor(140,86,75), QColor(227,119,194), QColor(127,127,127),
        QColor(188,189,34), QColor(23,190,207)
    };
    int idx = 0;
    for (const auto& [group, str] : m_groupStringMap) {
        if (idx < int(palette.size()))
            m_groupColorMap[group] = palette[idx++];
        else {
            // Generate random color if palette runs out
            std::mt19937 rng(group);
            std::uniform_int_distribution<int> dist(0, 255);
            m_groupColorMap[group] = QColor(dist(rng), dist(rng), dist(rng));
        }
    }
}

int HighPerfLineChart::selectedLine() const { return m_selectedLine; }
int HighPerfLineChart::hoveredLine() const { return m_hoveredLine; }

void HighPerfLineChart::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(1, 1, 1, 1);
}

void HighPerfLineChart::paintGL() {
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), 1.0f);
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
    const int leftMargin = m_leftMargin;
    const int rightMargin = m_rightMargin;
    const int topMargin = m_topMargin;
    const int bottomMargin = m_bottomMargin;
    QRect chartRect(leftMargin, topMargin, width() - leftMargin - rightMargin, height() - topMargin - bottomMargin);

    // --- Draw grid lines (if enabled) ---
    if (m_gridEnabled) {
        QPainter gridPainter(this);
        gridPainter.setPen(QPen(m_gridColor, 1, Qt::DashLine));
        // Vertical grid lines
        for (int i = 0; i <= m_numXTicks; ++i) {
            double t = double(i) / m_numXTicks;
            int x = chartRect.left() + t * chartRect.width();
            gridPainter.drawLine(QPoint(x, chartRect.top()), QPoint(x, chartRect.bottom()));
        }
        // Horizontal grid lines
        for (int i = 0; i <= m_numYTicks; ++i) {
            double t = double(i) / m_numYTicks;
            int y = chartRect.bottom() - t * chartRect.height();
            gridPainter.drawLine(QPoint(chartRect.left(), y), QPoint(chartRect.right(), y));
        }
    }

    // Helper lambda: map chart (data) coordinates to chartRect (screen) coordinates
    auto chartToScreen = [&](const QPointF& pt) -> QPointF {
        double xval = m_logTransformX ? logX(pt.x()) : pt.x();
        double yval = m_logTransformY ? logY(pt.y()) : pt.y();
        double xFrac = (xval - m_xMin) / (m_xMax - m_xMin);
        double yFrac = (yval - m_yMin) / (m_yMax - m_yMin);
        double x = chartRect.left() + xFrac * chartRect.width();
        double y = chartRect.bottom() - yFrac * chartRect.height();
        return QPointF(x, y);
    };

    if (m_groupAveragingEnabled) {
        // --- Group averaging mode ---
        // Collect all points by group
        std::unordered_map<int, std::vector<QPointF>> groupPoints;
        for (const auto& line : m_lines) {
            int group = line.group;
            QVector<QPointF> pts = line.points;
            if (m_binaryNormalizeX)
                pts = binaryNormalize(pts, BinaryNormAxis::X);
            if (m_binaryNormalizeY)
                pts = binaryNormalize(pts, BinaryNormAxis::Y);
            for (const QPointF& pt : pts) {
                if (!qIsFinite(pt.x()) || !qIsFinite(pt.y()))
                    continue;
                groupPoints[group].push_back(pt);
            }
        }
        // Compute mean for each group
        std::unordered_map<int, QPointF> groupMeans;
        for (const auto& [group, pts] : groupPoints) {
            if (pts.empty()) continue;
            double sumX = 0, sumY = 0;
            for (const auto& pt : pts) {
                sumX += m_logTransformX ? logX(pt.x()) : pt.x();
                sumY += m_logTransformY ? logY(pt.y()) : pt.y();
            }
            groupMeans[group] = QPointF(sumX / pts.size(), sumY / pts.size());
        }
        // Draw points colored by group
        glPointSize(10.0f);
        glBegin(GL_POINTS);
        for (const auto& [group, meanPt] : groupMeans) {
            if (!qIsFinite(meanPt.x()) || !qIsFinite(meanPt.y()))
                continue;
            if (meanPt.x() < m_xMin || meanPt.x() > m_xMax || meanPt.y() < m_yMin || meanPt.y() > m_yMax)
                continue;
            QColor c = m_groupColorMap.count(group) ? m_groupColorMap[group] : Qt::black;
            glColor4f(c.redF(), c.greenF(), c.blueF(), 1.0f);
            // meanPt is already in log space if enabled
            QPointF screenPt = chartToScreen(QPointF(
                m_logTransformX ? invLogX(meanPt.x()) : meanPt.x(),
                m_logTransformY ? invLogY(meanPt.y()) : meanPt.y()
            ));
            float x = (screenPt.x() / width()) * 2.0f - 1.0f;
            float y = 1.0f - (screenPt.y() / height()) * 2.0f;
            glVertex2f(x, y);
        }
        glEnd();
    } else {
        // --- Draw lines and points (with line clipping to axes) ---
        for (int i = 0; i < m_lines.size(); ++i) {
            const auto& line = m_lines[i];
            if (line.points.isEmpty())
                continue;
            QVector<QPointF> pts = m_noiseRemovalEnabled ? filterNoise(line.points) : line.points;
            if (m_binaryNormalizeX)
                pts = binaryNormalize(pts, BinaryNormAxis::X);
            if (m_binaryNormalizeY)
                pts = binaryNormalize(pts, BinaryNormAxis::Y);
            QColor c = line.color.isValid() ? line.color : Qt::black;
            float alpha = (line.opacity >= 0.0f && line.opacity <= 1.0f) ? line.opacity : 1.0f;
            float lw = (line.lineWidth > 0) ? line.lineWidth : m_lineWidth;
            float ps = (line.pointSize > 0) ? line.pointSize : m_pointSize;
            if (i == m_selectedLine) { c = Qt::red; alpha = 1.0f; lw = std::max(lw, 3.0f); }
            else if (i == m_hoveredLine) { c = c.lighter(150); alpha = 1.0f; }

            // Set OpenGL line stipple for dash style
            if (line.dashStyle == LineDashStyle::Dash) {
                glEnable(GL_LINE_STIPPLE);
                glLineStipple(1, 0x00FF);
            } else if (line.dashStyle == LineDashStyle::Dot) {
                glEnable(GL_LINE_STIPPLE);
                glLineStipple(1, 0x0101);
            } else {
                glDisable(GL_LINE_STIPPLE);
            }

            // Draw points (all points inside axes)
            if (m_showPoints) {
                glPointSize(ps);
                glBegin(GL_POINTS);
                for (const QPointF& ptOrig : pts) {
                    QPointF pt = m_interchangeXY ? QPointF(ptOrig.y(), ptOrig.x()) : ptOrig;
                    double xval = m_logTransformX ? logX(pt.x()) : pt.x();
                    double yval = m_logTransformY ? logY(pt.y()) : pt.y();
                    if (xval < m_xMin || xval > m_xMax || yval < m_yMin || yval > m_yMax)
                        continue;
                    QPointF screenPt = chartToScreen(pt);
                    float x = (screenPt.x() / width()) * 2.0f - 1.0f;
                    float y = 1.0f - (screenPt.y() / height()) * 2.0f;
                    glColor4f(c.redF(), c.greenF(), c.blueF(), alpha);
                    glVertex2f(x, y);
                }
                glEnd();
            }

            // Draw lines (clip each segment to axes)
            if (pts.size() > 1) {
                if (m_gradientLineEnabled) {
                    for (int j = 1; j < pts.size(); ++j) {
                        QPointF p0Orig = pts[j - 1];
                        QPointF p1Orig = pts[j];
                        QPointF p0 = m_interchangeXY ? QPointF(p0Orig.y(), p0Orig.x()) : p0Orig;
                        QPointF p1 = m_interchangeXY ? QPointF(p1Orig.y(), p1Orig.x()) : p1Orig;
                        if (!qIsFinite(p0.x()) || !qIsFinite(p0.y()) || !qIsFinite(p1.x()) || !qIsFinite(p1.y()))
                            continue;
                        double x0 = m_logTransformX ? logX(p0.x()) : p0.x();
                        double y0 = m_logTransformY ? logY(p0.y()) : p0.y();
                        double x1 = m_logTransformX ? logX(p1.x()) : p1.x();
                        double y1 = m_logTransformY ? logY(p1.y()) : p1.y();
                        if (cohenSutherlandClip(x0, y0, x1, y1, m_xMin, m_xMax, m_yMin, m_yMax)) {
                            QPointF sp0 = chartToScreen(QPointF(
                                m_logTransformX ? invLogX(x0) : x0,
                                m_logTransformY ? invLogY(y0) : y0
                            ));
                            QPointF sp1 = chartToScreen(QPointF(
                                m_logTransformX ? invLogX(x1) : x1,
                                m_logTransformY ? invLogY(y1) : y1
                            ));
                            float sx0 = (sp0.x() / width()) * 2.0f - 1.0f;
                            float sy0 = 1.0f - (sp0.y() / height()) * 2.0f;
                            float sx1 = (sp1.x() / width()) * 2.0f - 1.0f;
                            float sy1 = 1.0f - (sp1.y() / height()) * 2.0f;
                            double dx = x1 - x0;
                            double dy = y1 - y0;
                            double slope = (dx != 0) ? dy / dx : 0.0;
                            QColor gradColor = slopeToColor(slope);
                            glLineWidth(lw);
                            glBegin(GL_LINES);
                            glColor4f(gradColor.redF(), gradColor.greenF(), gradColor.blueF(), alpha);
                            glVertex2f(sx0, sy0);
                            glColor4f(gradColor.redF(), gradColor.greenF(), gradColor.blueF(), alpha);
                            glVertex2f(sx1, sy1);
                            glEnd();
                        }
                    }
                } else {
                    for (int j = 1; j < pts.size(); ++j) {
                        QPointF p0Orig = pts[j - 1];
                        QPointF p1Orig = pts[j];
                        QPointF p0 = m_interchangeXY ? QPointF(p0Orig.y(), p0Orig.x()) : p0Orig;
                        QPointF p1 = m_interchangeXY ? QPointF(p1Orig.y(), p1Orig.x()) : p1Orig;
                        if (!qIsFinite(p0.x()) || !qIsFinite(p0.y()) || !qIsFinite(p1.x()) || !qIsFinite(p1.y()))
                            continue;
                        double x0 = m_logTransformX ? logX(p0.x()) : p0.x();
                        double y0 = m_logTransformY ? logY(p0.y()) : p0.y();
                        double x1 = m_logTransformX ? logX(p1.x()) : p1.x();
                        double y1 = m_logTransformY ? logY(p1.y()) : p1.y();
                        if (cohenSutherlandClip(x0, y0, x1, y1, m_xMin, m_xMax, m_yMin, m_yMax)) {
                            QPointF sp0 = chartToScreen(QPointF(
                                m_logTransformX ? invLogX(x0) : x0,
                                m_logTransformY ? invLogY(y0) : y0
                            ));
                            QPointF sp1 = chartToScreen(QPointF(
                                m_logTransformX ? invLogX(x1) : x1,
                                m_logTransformY ? invLogY(y1) : y1
                            ));
                            float sx0 = (sp0.x() / width()) * 2.0f - 1.0f;
                            float sy0 = 1.0f - (sp0.y() / height()) * 2.0f;
                            float sx1 = (sp1.x() / width()) * 2.0f - 1.0f;
                            float sy1 = 1.0f - (sp1.y() / height()) * 2.0f;
                            glLineWidth(lw);
                            glColor4f(c.redF(), c.greenF(), c.blueF(), alpha);
                            glBegin(GL_LINES);
                            glVertex2f(sx0, sy0);
                            glVertex2f(sx1, sy1);
                            glEnd();
                        }
                    }
                }
            }
            glDisable(GL_LINE_STIPPLE); // Always disable after drawing
        }
    }

    // --- Draw axes and labels using QPainter for text ---
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw X and Y axis lines with custom width and color
    QPen xPen(m_xAxisLineColor, m_axisLineWidth);
    QPen yPen(m_yAxisLineColor, m_axisLineWidth);

    // X axis
    painter.setPen(xPen);
    QPoint xAxisStart(chartRect.left(), chartRect.bottom());
    QPoint xAxisEnd(chartRect.right(), chartRect.bottom());
    painter.drawLine(xAxisStart, xAxisEnd);

    // Y axis
    painter.setPen(yPen);
    QPoint yAxisStart(chartRect.left(), chartRect.bottom());
    QPoint yAxisEnd(chartRect.left(), chartRect.top());
    painter.drawLine(yAxisStart, yAxisEnd);

    // Draw tick marks and values
    int numXTicks = m_numXTicks;
    int numYTicks = m_numYTicks;

    // Tick labels
    QFont tickFont = m_tickLabelFont.family().isEmpty() ? painter.font() : m_tickLabelFont;
    painter.setFont(tickFont);
    painter.setPen(m_tickLabelColor);
    QFontMetrics fmTick(tickFont);

    // X ticks and labels
    for (int i = 0; i <= numXTicks; ++i) {
        double t = double(i) / numXTicks;
        int x = chartRect.left() + t * chartRect.width();
        int y = chartRect.bottom();
        painter.drawLine(QPoint(x, y), QPoint(x, y + 6));
        double xValue = m_xMin + t * (m_xMax - m_xMin);
        QString xLabel;
        if (m_interchangeXY ? m_logTransformY : m_logTransformX)
            xLabel = QString::number(m_interchangeXY ? invLogY(xValue) : invLogX(xValue), 'g', 4);
        else
            xLabel = QString::number(xValue, 'g', 4);
        int labelWidth = fmTick.horizontalAdvance(xLabel);
        painter.drawText(x - labelWidth / 2, y + 6 + fmTick.ascent(), xLabel);
    }

    // Y ticks and labels
    for (int i = 0; i <= numYTicks; ++i) {
        double t = double(i) / numYTicks;
        int y = chartRect.bottom() - t * chartRect.height();
        int x = chartRect.left();
        painter.drawLine(QPoint(x, y), QPoint(x - 6, y));
        double yValue = m_yMin + t * (m_yMax - m_yMin);
        QString yLabel;
        if (m_interchangeXY ? m_logTransformX : m_logTransformY)
            yLabel = QString::number(m_interchangeXY ? invLogX(yValue) : invLogY(yValue), 'g', 4);
        else
            yLabel = QString::number(yValue, 'g', 4);
        int labelHeight = fmTick.height();
        painter.drawText(x - 8 - fmTick.horizontalAdvance(yLabel), y + labelHeight / 2 - 2, yLabel);
    }

    // Axis labels and title
    painter.setFont(m_axisFont);
    painter.setPen(m_axisColor);

    // Draw chart title at the top center
    if (!m_chartTitle.isEmpty()) {
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        titleFont.setPointSize(titleFont.pointSize() + 2);
        painter.setFont(titleFont);
        painter.drawText(QRect(0, 0, width(), topMargin), Qt::AlignHCenter | Qt::AlignVCenter, m_chartTitle);
        painter.setFont(m_axisFont);
    }

    // Draw x-axis label
    QString xAxisLabel = m_interchangeXY ? m_yAxisName : m_xAxisName;
    QString yAxisLabel = m_interchangeXY ? m_xAxisName : m_yAxisName;
    if (!xAxisLabel.isEmpty())
        painter.drawText(QRect(chartRect.left(), chartRect.bottom() + 22, chartRect.width(), 20), Qt::AlignHCenter | Qt::AlignVCenter, xAxisLabel);

    // Draw y-axis label (rotated)
    if (!yAxisLabel.isEmpty()) {
        painter.save();
        painter.translate(leftMargin - 35, chartRect.top() + chartRect.height() / 2);
        painter.rotate(-90);
        painter.drawText(QRect(0, 0, chartRect.height(), 20), Qt::AlignHCenter | Qt::AlignVCenter, yAxisLabel);
        painter.restore();
    }

    // --- Draw legend for group colors ---
    if (m_groupAveragingEnabled && !m_groupStringMap.empty()) {
        QFont legendFont = m_legendFont.family().isEmpty() ? painter.font() : m_legendFont;
        painter.setFont(legendFont);
        painter.setPen(m_legendColor);

        int legendX = (m_legendX >= 0) ? m_legendX : width() - 160;
        int legendY = (m_legendY >= 0) ? m_legendY : 50;
        int boxSize = 16;
        int spacing = 6;
        int textOffset = 22;
        int i = 0;
        QFontMetrics fmLegend(legendFont);
        painter.setBrush(Qt::NoBrush);
        painter.drawText(legendX, legendY - 10, "Group Legend:");
        for (const auto& [group, groupStr] : m_groupStringMap) {
            QColor c = m_groupColorMap.count(group) ? m_groupColorMap[group] : Qt::black;
            QRect colorRect(legendX, legendY + i * (boxSize + spacing), boxSize, boxSize);
            painter.fillRect(colorRect, c);
            painter.drawRect(colorRect);
            painter.drawText(legendX + textOffset, legendY + i * (boxSize + spacing) + boxSize - 2, groupStr);
            ++i;
        }
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
    if (m_interchangeXY)
        return QPointF(y, x);
    else
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