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
    m_showLines = true; // NEW: default to show lines
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
    m_showAverageLine = false;
    m_showMedianLine = false;
    m_showPercentileLines = false;
    m_percentileLower = 0.25;
    m_percentileUpper = 0.75;
    m_showMinLine = false;
    m_showMaxLine = false;
    m_showStddevLines = false;
    m_showAllLines = true;
    m_globalLineColor = QColor();
    m_globalLineWidth = 0.0f;
    m_globalLineOpacity = 1.0f;
    m_showLegend = true; // NEW: default to show legend

    // Initialize statistical line labels
    m_averageLineLabel = "Average";
    m_medianLineLabel = "Median";
    m_percentileLowerLabel = "25th Percentile";
    m_percentileUpperLabel = "75th Percentile";
    m_minLineLabel = "Min";
    m_maxLineLabel = "Max";
    m_stddevLineLabel = "Stddev";

    // Initialize additional statistical line options
    m_showModeLine = false;
    m_showVarianceLine = false;
    m_showLowerQuartileLine = false;
    m_showUpperQuartileLine = false;
    m_showMADLine = false;
    m_showSkewnessLine = false;
    m_showKurtosisLine = false;

    // Initialize additional statistical line labels
    m_modeLineLabel = "Mode";
    m_varianceLineLabel = "Variance";
    m_lowerQuartileLineLabel = "Lower Quartile";
    m_upperQuartileLineLabel = "Upper Quartile";
    m_madLineLabel = "MAD";
    m_skewnessLineLabel = "Skewness";
    m_kurtosisLineLabel = "Kurtosis";

    // Initialize connect stats line options
    m_connectStatsLineEnabled = false;
    m_connectStatsLineN = 5;
    m_connectStatsLineType = ConnectStatsType::None;
}

// log transform setter/getter
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

// --- interchangeXY setter/getter ---
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
bool HighPerfLineChart::showPoints() const { return m_showPoints; }

// NEW: show/hide lines connecting points
void HighPerfLineChart::setShowLines(bool en) { m_showLines = en; update(); }
bool HighPerfLineChart::showLines() const { return m_showLines; }

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
            // small margin for better visualization
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

    // --- Draw lines and points (with line clipping to axes) ---
    if (m_showAllLines) {
        for (int i = 0; i < m_lines.size(); ++i) {
            const auto& line = m_lines[i];
            if (line.points.isEmpty())
                continue;
            QVector<QPointF> pts = m_noiseRemovalEnabled ? filterNoise(line.points) : line.points;
            if (m_binaryNormalizeX)
                pts = binaryNormalize(pts, BinaryNormAxis::X);
            if (m_binaryNormalizeY)
                pts = binaryNormalize(pts, BinaryNormAxis::Y);

            QColor c = m_globalLineColor.isValid() ? m_globalLineColor : (line.color.isValid() ? line.color : Qt::black);
            float alpha = m_globalLineOpacity;
            float lw = (m_globalLineWidth > 0) ? m_globalLineWidth : ((line.lineWidth > 0) ? line.lineWidth : m_lineWidth);
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
            if (m_showLines && pts.size() > 1) { // <--- Only draw lines if m_showLines is true
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

    // --- Draw line between statistic of first n and last n points ---
    if (m_connectStatsLineEnabled && m_connectStatsLineType != ConnectStatsType::None && !m_lines.isEmpty()) {
        // Use the first line for simplicity (or could average across all lines)
        const auto& line = m_lines[0];
        if (line.points.size() >= 2 * m_connectStatsLineN) {
            auto statFunc = [](const QVector<QPointF>& pts, ConnectStatsType type) -> QPointF {
                if (pts.isEmpty()) return QPointF();
                QVector<double> xs, ys;
                for (const auto& pt : pts) { xs.append(pt.x()); ys.append(pt.y()); }
                double x = 0, y = 0;
                switch (type) {
                    case ConnectStatsType::Average:
                        x = std::accumulate(xs.begin(), xs.end(), 0.0) / xs.size();
                        y = std::accumulate(ys.begin(), ys.end(), 0.0) / ys.size();
                        break;
                    case ConnectStatsType::Median: {
                        std::sort(xs.begin(), xs.end());
                        std::sort(ys.begin(), ys.end());
                        int n = xs.size();
                        x = (n % 2 == 1) ? xs[n/2] : 0.5 * (xs[n/2-1] + xs[n/2]);
                        y = (n % 2 == 1) ? ys[n/2] : 0.5 * (ys[n/2-1] + ys[n/2]);
                        break;
                    }
                    case ConnectStatsType::Mode: {
                        auto mode = [](const QVector<double>& vals) -> double {
                            std::map<double, int> freq;
                            for (double v : vals) freq[v]++;
                            auto it = std::max_element(freq.begin(), freq.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
                            return (it != freq.end()) ? it->first : vals[0];
                        };
                        x = mode(xs);
                        y = mode(ys);
                        break;
                    }
                    default: break;
                }
                return QPointF(x, y);
            };
            QVector<QPointF> firstN(line.points.mid(0, m_connectStatsLineN));
            QVector<QPointF> lastN(line.points.mid(line.points.size() - m_connectStatsLineN, m_connectStatsLineN));
            QPointF statFirst = statFunc(firstN, m_connectStatsLineType);
            QPointF statLast = statFunc(lastN, m_connectStatsLineType);

            // Draw the connecting line
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            QPen pen(Qt::darkCyan, 2, Qt::DashDotLine);
            painter.setPen(pen);
            QPointF p1 = chartToScreen(statFirst);
            QPointF p2 = chartToScreen(statLast);
            painter.drawLine(p1, p2);

            // Optionally, draw points at the ends
            painter.setBrush(Qt::darkCyan);
            painter.drawEllipse(p1, 5, 5);
            painter.drawEllipse(p2, 5, 5);
        }
    }

    // --- Draw average/median/percentile/min/max/stddev lines using QPainter ---
    if ((!m_lines.isEmpty()) && (m_showAverageLine || m_showMedianLine || m_showPercentileLines || m_showMinLine || m_showMaxLine || m_showStddevLines || m_showModeLine || m_showVarianceLine || m_showLowerQuartileLine || m_showUpperQuartileLine || m_showMADLine || m_showSkewnessLine || m_showKurtosisLine)) {
        const int N = 200;
        QVector<QPointF> avgPoints, medianPoints, percLowPoints, percHighPoints, minPoints, maxPoints, stddevLowPoints, stddevHighPoints;
        QVector<QPointF> modePoints, variancePoints, lowerQuartilePoints, upperQuartilePoints, madPoints, skewnessPoints, kurtosisPoints;
        double xStart = m_xMin, xEnd = m_xMax;
        for (int i = 0; i < N; ++i) {
            double x = xStart + (xEnd - xStart) * i / (N - 1);
            QVector<double> yVals;
            double ySum = 0;
            int yCount = 0;
            // For each line, interpolate y at this x (if possible)
            for (const auto& line : m_lines) {
                const QVector<QPointF>& pts = line.points;
                if (pts.size() < 2) continue;
                for (int j = 1; j < pts.size(); ++j) {
                    double x0 = m_logTransformX ? logX(pts[j-1].x()) : pts[j-1].x();
                    double x1 = m_logTransformX ? logX(pts[j].x()) : pts[j].x();
                    double y0 = m_logTransformY ? logY(pts[j-1].y()) : pts[j-1].y();
                    double y1 = m_logTransformY ? logY(pts[j].y()) : pts[j].y();
                    if ((x0 <= x && x <= x1) || (x1 <= x && x <= x0)) {
                        double t = (x1 - x0) != 0 ? (x - x0) / (x1 - x0) : 0.0;
                        double y = y0 + t * (y1 - y0);
                        yVals.append(y);
                        ySum += y;
                        yCount++;
                        break;
                    }
                }
            }
            if (yCount > 0) {
                if (m_showAverageLine)
                    avgPoints.append(QPointF(x, ySum / yCount));
                if (m_showMedianLine || m_showPercentileLines || m_showMinLine || m_showMaxLine || m_showStddevLines || m_showModeLine || m_showVarianceLine || m_showLowerQuartileLine || m_showUpperQuartileLine || m_showMADLine || m_showSkewnessLine || m_showKurtosisLine) {
                    std::sort(yVals.begin(), yVals.end());
                    int n = yVals.size();
                    // Median
                    if (m_showMedianLine) {
                        double med = (n % 2 == 1) ? yVals[n/2] : 0.5 * (yVals[n/2-1] + yVals[n/2]);
                        medianPoints.append(QPointF(x, med));
                    }
                    // Percentiles
                    if (m_showPercentileLines) {
                        int idxLow = std::clamp(int(m_percentileLower * n), 0, n-1);
                        int idxHigh = std::clamp(int(m_percentileUpper * n), 0, n-1);
                        percLowPoints.append(QPointF(x, yVals[idxLow]));
                        percHighPoints.append(QPointF(x, yVals[idxHigh]));
                    }
                    // Min/Max
                    if (m_showMinLine)
                        minPoints.append(QPointF(x, yVals.first()));
                    if (m_showMaxLine)
                        maxPoints.append(QPointF(x, yVals.last()));
                    // Stddev
                    if (m_showStddevLines) {
                        double mean = ySum / yCount;
                        double var = 0;
                        for (double y : yVals) var += (y - mean) * (y - mean);
                        double stddev = std::sqrt(var / yCount);
                        stddevLowPoints.append(QPointF(x, mean - stddev));
                        stddevHighPoints.append(QPointF(x, mean + stddev));
                    }
                    // Mode
                    if (m_showModeLine) {
                        std::map<double, int> freqMap;
                        for (double y : yVals) freqMap[y]++;
                        auto modeIt = std::max_element(freqMap.begin(), freqMap.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
                        if (modeIt != freqMap.end())
                            modePoints.append(QPointF(x, modeIt->first));
                    }
                    // Variance
                    if (m_showVarianceLine) {
                        double mean = ySum / yCount;
                        double var = 0;
                        for (double y : yVals) var += (y - mean) * (y - mean);
                        variancePoints.append(QPointF(x, var / yCount));
                    }
                    // Lower/Upper Quartiles
                    if (m_showLowerQuartileLine) {
                        int idxLQ = std::clamp(int(0.25 * n), 0, n-1);
                        lowerQuartilePoints.append(QPointF(x, yVals[idxLQ]));
                    }
                    if (m_showUpperQuartileLine) {
                        int idxUQ = std::clamp(int(0.75 * n), 0, n-1);
                        upperQuartilePoints.append(QPointF(x, yVals[idxUQ]));
                    }
                    // MAD
                    if (m_showMADLine) {
                        double mean = ySum / yCount;
                        QVector<double> absDevs;
                        for (double y : yVals) absDevs.append(std::abs(y - mean));
                        std::sort(absDevs.begin(), absDevs.end());
                        double mad = (n % 2 == 1) ? absDevs[n/2] : 0.5 * (absDevs[n/2-1] + absDevs[n/2]);
                        madPoints.append(QPointF(x, mad));
                    }
                    // Skewness
                    if (m_showSkewnessLine) {
                        double mean = ySum / yCount;
                        double m3 = 0, m2 = 0;
                        for (double y : yVals) {
                            double dev = y - mean;
                            m3 += dev * dev * dev;
                            m2 += dev * dev;
                        }
                        m3 /= yCount;
                        m2 = std::pow(m2 / yCount, 1.5);
                        double skewness = (m2 != 0) ? m3 / m2 : 0.0;
                        skewnessPoints.append(QPointF(x, skewness));
                    }
                    // Kurtosis
                    if (m_showKurtosisLine) {
                        double mean = ySum / yCount;
                        double m4 = 0, m2 = 0;
                        for (double y : yVals) {
                            double dev = y - mean;
                            m4 += dev * dev * dev * dev;
                            m2 += dev * dev;
                        }
                        m4 /= yCount;
                        m2 = std::pow(m2 / yCount, 2.0);
                        double kurtosis = (m2 != 0) ? m4 / m2 : 0.0;
                        kurtosisPoints.append(QPointF(x, kurtosis));
                    }
                }
            }
        }
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        // Average line
        if (m_showAverageLine && avgPoints.size() > 1) {
            QPen avgPen(QColor(0, 0, 200), 2, Qt::DotLine);
            painter.setPen(avgPen);
            QPolygonF poly;
            for (const QPointF& pt : avgPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Median line
        if (m_showMedianLine && medianPoints.size() > 1) {
            QPen medPen(QColor(200, 0, 200), 2, Qt::DashLine);
            painter.setPen(medPen);
            QPolygonF poly;
            for (const QPointF& pt : medianPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Percentile lines
        if (m_showPercentileLines && percLowPoints.size() > 1 && percHighPoints.size() > 1) {
            QPen percPen(QColor(0, 180, 0), 1, Qt::DashDotLine);
            painter.setPen(percPen);
            QPolygonF polyLow, polyHigh;
            for (const QPointF& pt : percLowPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                polyLow << screenPt;
            }
            for (const QPointF& pt : percHighPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                polyHigh << screenPt;
            }
            painter.drawPolyline(polyLow);
            painter.drawPolyline(polyHigh);
        }
        // Min line
        if (m_showMinLine && minPoints.size() > 1) {
            QPen minPen(QColor(0, 150, 255), 1, Qt::DotLine);
            painter.setPen(minPen);
            QPolygonF poly;
            for (const QPointF& pt : minPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Max line
        if (m_showMaxLine && maxPoints.size() > 1) {
            QPen maxPen(QColor(255, 100, 0), 1, Qt::DotLine);
            painter.setPen(maxPen);
            QPolygonF poly;
            for (const QPointF& pt : maxPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Stddev lines
        if (m_showStddevLines && stddevLowPoints.size() > 1 && stddevHighPoints.size() > 1) {
            QPen stddevPen(QColor(120, 120, 120), 1, Qt::DashLine);
            painter.setPen(stddevPen);
            QPolygonF polyLow, polyHigh;
            for (const QPointF& pt : stddevLowPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                polyLow << screenPt;
            }
            for (const QPointF& pt : stddevHighPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                polyHigh << screenPt;
            }
            painter.drawPolyline(polyLow);
            painter.drawPolyline(polyHigh);
        }
        // Mode line
        if (m_showModeLine && modePoints.size() > 1) {
            QPen modePen(QColor(0, 0, 0), 1, Qt::SolidLine);
            painter.setPen(modePen);
            QPolygonF poly;
            for (const QPointF& pt : modePoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Variance line
        if (m_showVarianceLine && variancePoints.size() > 1) {
            QPen varPen(QColor(100, 100, 255), 1, Qt::DashDotDotLine);
            painter.setPen(varPen);
            QPolygonF poly;
            for (const QPointF& pt : variancePoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Lower Quartile line
        if (m_showLowerQuartileLine && lowerQuartilePoints.size() > 1) {
            QPen lqPen(QColor(0, 180, 180), 1, Qt::DotLine);
            painter.setPen(lqPen);
            QPolygonF poly;
            for (const QPointF& pt : lowerQuartilePoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Upper Quartile line
        if (m_showUpperQuartileLine && upperQuartilePoints.size() > 1) {
            QPen uqPen(QColor(180, 180, 0), 1, Qt::DotLine);
            painter.setPen(uqPen);
            QPolygonF poly;
            for (const QPointF& pt : upperQuartilePoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // MAD line
        if (m_showMADLine && madPoints.size() > 1) {
            QPen madPen(QColor(180, 0, 180), 1, Qt::DashDotLine);
            painter.setPen(madPen);
            QPolygonF poly;
            for (const QPointF& pt : madPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Skewness line
        if (m_showSkewnessLine && skewnessPoints.size() > 1) {
            QPen skewPen(QColor(0, 120, 120), 1, Qt::DashLine);
            painter.setPen(skewPen);
            QPolygonF poly;
            for (const QPointF& pt : skewnessPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
        }
        // Kurtosis line
        if (m_showKurtosisLine && kurtosisPoints.size() > 1) {
            QPen kurtPen(QColor(120, 0, 120), 1, Qt::DashLine);
            painter.setPen(kurtPen);
            QPolygonF poly;
            for (const QPointF& pt : kurtosisPoints) {
                if (pt.x() < m_xMin || pt.x() > m_xMax || pt.y() < m_yMin || pt.y() > m_yMax)
                    continue;
                QPointF screenPt(
                    chartRect.left() + ((pt.x() - m_xMin) / (m_xMax - m_xMin)) * chartRect.width(),
                    chartRect.bottom() - ((pt.y() - m_yMin) / (m_yMax - m_yMin)) * chartRect.height()
                );
                poly << screenPt;
            }
            painter.drawPolyline(poly);
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

    // --- Draw legend for group colors and statistical lines ---
    if (m_showLegend) {
        // --- Compute average slope of main stat line (average or median) ---
        double statSlope = 0.0;
        int statSlopeCount = 0;
        QVector<QPointF> statLinePoints;
        if (m_showAverageLine || m_showMedianLine) {
            // Use whichever is enabled (prefer average)
            const int N = 200;
            QVector<QPointF> avgPoints, medianPoints;
            double xStart = m_xMin, xEnd = m_xMax;
            for (int i = 0; i < N; ++i) {
                double x = xStart + (xEnd - xStart) * i / (N - 1);
                QVector<double> yVals;
                double ySum = 0;
                int yCount = 0;
                for (const auto& line : m_lines) {
                    const QVector<QPointF>& pts = line.points;
                    if (pts.size() < 2) continue;
                    for (int j = 1; j < pts.size(); ++j) {
                        double x0 = m_logTransformX ? logX(pts[j-1].x()) : pts[j-1].x();
                        double x1 = m_logTransformX ? logX(pts[j].x()) : pts[j].x();
                        double y0 = m_logTransformY ? logY(pts[j-1].y()) : pts[j-1].y();
                        double y1 = m_logTransformY ? logY(pts[j].y()) : pts[j].y();
                        if ((x0 <= x && x <= x1) || (x1 <= x && x <= x0)) {
                            double t = (x1 - x0) != 0 ? (x - x0) / (x1 - x0) : 0.0;
                            double y = y0 + t * (y1 - y0);
                            yVals.append(y);
                            ySum += y;
                            yCount++;
                            break;
                        }
                    }
                }
                if (yCount > 0) {
                    if (m_showAverageLine)
                        avgPoints.append(QPointF(x, ySum / yCount));
                    if (m_showMedianLine) {
                        std::sort(yVals.begin(), yVals.end());
                        int n = yVals.size();
                        double med = (n % 2 == 1) ? yVals[n/2] : 0.5 * (yVals[n/2-1] + yVals[n/2]);
                        medianPoints.append(QPointF(x, med));
                    }
                }
            }
            if (m_showAverageLine && avgPoints.size() > 1)
                statLinePoints = avgPoints;
            else if (m_showMedianLine && medianPoints.size() > 1)
                statLinePoints = medianPoints;
        }
        if (statLinePoints.size() > 1) {
            for (int i = 1; i < statLinePoints.size(); ++i) {
                double dx = statLinePoints[i].x() - statLinePoints[i-1].x();
                double dy = statLinePoints[i].y() - statLinePoints[i-1].y();
                if (std::abs(dx) > 1e-8) {
                    statSlope += dy / dx;
                    ++statSlopeCount;
                }
            }
        }
        double avgSlope = (statSlopeCount > 0) ? statSlope / statSlopeCount : 0.0;

        // --- Legend positioning logic ---
        int legendMargin = 10;
        int statBoxSize = 16;
        int statSpacing = 6;
        int statTextOffset = 22;
        int statLegendLine = 0;
        int statLegendWidth = 160;
        int statLegendHeight = 0;
        int statLegendX = 0, statLegendY = 0;

        // Count number of stat lines to compute legend height
        int statLineCount = 0;
        if (m_showAverageLine) ++statLineCount;
        if (m_showMedianLine) ++statLineCount;
        if (m_showPercentileLines) statLineCount += 2;
        if (m_showMinLine) ++statLineCount;
        if (m_showMaxLine) ++statLineCount;
        if (m_showStddevLines) ++statLineCount;
        // Additional statistical lines
        if (m_showModeLine) ++statLineCount;
        if (m_showVarianceLine) ++statLineCount;
        if (m_showLowerQuartileLine) ++statLineCount;
        if (m_showUpperQuartileLine) ++statLineCount;
        if (m_showMADLine) ++statLineCount;
        if (m_showSkewnessLine) ++statLineCount;
        if (m_showKurtosisLine) ++statLineCount;
        bool anyStatLine = statLineCount > 0;

        statLegendHeight = (statLineCount + 1) * (statBoxSize + statSpacing);

        // Place legend to left or right of chart area, not overlapping y axis
        if (avgSlope < 0) {
            // Downward: put legend on right
            statLegendX = chartRect.right() + legendMargin;
            if (statLegendX + statLegendWidth > width())
                statLegendX = width() - statLegendWidth - legendMargin;
        } else {
            // Upward or flat: put legend to the right of y axis (not overlapping)
            statLegendX = chartRect.left() + legendMargin;
            // Ensure it doesn't overlap chart area too much
            if (statLegendX + statLegendWidth > chartRect.right() - legendMargin)
                statLegendX = std::max(legendMargin, chartRect.right() - statLegendWidth - legendMargin);
        }
        // Place legend vertically near top of chart area, but not above
        statLegendY = chartRect.top() + legendMargin;

        QFont statLegendFont = m_legendFont.family().isEmpty() ? painter.font() : m_legendFont;
        painter.setFont(statLegendFont);
        painter.setPen(m_legendColor);

        // Draw header
        if (anyStatLine) {
            painter.drawText(statLegendX, statLegendY - 10, "Statistical Lines:");
        }

        // Helper lambda to draw a legend swatch with a given pen and label
        auto drawStatLegendEntry = [&](const QPen& pen, const QString& label) {
            int y = statLegendY + statLegendLine * (statBoxSize + statSpacing);
            QRect swatchRect(statLegendX, y, statBoxSize, statBoxSize);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(swatchRect);
            // Draw line style in the box
            int midY = y + statBoxSize / 2;
            QPen penCopy = pen;
            penCopy.setWidth(2);
            painter.setPen(penCopy);
            painter.drawLine(statLegendX + 2, midY, statLegendX + statBoxSize - 2, midY);
            painter.setPen(m_legendColor);
            painter.drawText(statLegendX + statTextOffset, y + statBoxSize - 2, label);
            ++statLegendLine;
        };

        // Draw entries for each enabled statistical line, matching the style in the chart
        if (m_showAverageLine) {
            QPen avgPen(QColor(0, 0, 200), 2, Qt::DotLine);
            drawStatLegendEntry(avgPen, m_averageLineLabel);
        }
        if (m_showMedianLine) {
            QPen medPen(QColor(200, 0, 200), 2, Qt::DashLine);
            drawStatLegendEntry(medPen, m_medianLineLabel);
        }
        if (m_showPercentileLines) {
            QPen percPen(QColor(0, 180, 0), 1, Qt::DashDotLine);
            drawStatLegendEntry(percPen, m_percentileLowerLabel);
            drawStatLegendEntry(percPen, m_percentileUpperLabel);
        }
        if (m_showMinLine) {
            QPen minPen(QColor(0, 150, 255), 1, Qt::DotLine);
            drawStatLegendEntry(minPen, m_minLineLabel);
        }
        if (m_showMaxLine) {
            QPen maxPen(QColor(255, 100, 0), 1, Qt::DotLine);
            drawStatLegendEntry(maxPen, m_maxLineLabel);
        }
        if (m_showStddevLines) {
            QPen stddevPen(QColor(120, 120, 120), 1, Qt::DashLine);
            drawStatLegendEntry(stddevPen, m_stddevLineLabel + " (±1σ)");
        }
        // Additional statistical lines
        if (m_showModeLine) {
            QPen modePen(QColor(0, 0, 0), 1, Qt::SolidLine);
            drawStatLegendEntry(modePen, m_modeLineLabel.isEmpty() ? "Mode" : m_modeLineLabel);
        }
        if (m_showVarianceLine) {
            QPen varPen(QColor(100, 100, 255), 1, Qt::DashDotDotLine);
            drawStatLegendEntry(varPen, m_varianceLineLabel.isEmpty() ? "Variance" : m_varianceLineLabel);
        }
        if (m_showLowerQuartileLine) {
            QPen lqPen(QColor(0, 180, 180), 1, Qt::DotLine);
            drawStatLegendEntry(lqPen, m_lowerQuartileLineLabel.isEmpty() ? "Lower Quartile" : m_lowerQuartileLineLabel);
        }
        if (m_showUpperQuartileLine) {
            QPen uqPen(QColor(180, 180, 0), 1, Qt::DotLine);
            drawStatLegendEntry(uqPen, m_upperQuartileLineLabel.isEmpty() ? "Upper Quartile" : m_upperQuartileLineLabel);
        }
        if (m_showMADLine) {
            QPen madPen(QColor(180, 0, 180), 1, Qt::DashDotLine);
            drawStatLegendEntry(madPen, m_madLineLabel.isEmpty() ? "MAD" : m_madLineLabel);
        }
        if (m_showSkewnessLine) {
            QPen skewPen(QColor(0, 120, 120), 1, Qt::DashLine);
            drawStatLegendEntry(skewPen, m_skewnessLineLabel.isEmpty() ? "Skewness" : m_skewnessLineLabel);
        }
        if (m_showKurtosisLine) {
            QPen kurtPen(QColor(120, 0, 120), 1, Qt::DashLine);
            drawStatLegendEntry(kurtPen, m_kurtosisLineLabel.isEmpty() ? "Kurtosis" : m_kurtosisLineLabel);
        }

        // --- Draw group legend below statistical legend if enabled ---
        if (m_groupAveragingEnabled && !m_groupStringMap.empty()) {
            QFont legendFont = m_legendFont.family().isEmpty() ? painter.font() : m_legendFont;
            painter.setFont(legendFont);
            painter.setPen(m_legendColor);

            int groupLegendX = statLegendX;
            int groupLegendY = statLegendY + statLegendHeight + legendMargin;
            int boxSize = 16;
            int spacing = 6;
            int textOffset = 22;
            int i = 0;
            QFontMetrics fmLegend(legendFont);
            painter.setBrush(Qt::NoBrush);
            painter.drawText(groupLegendX, groupLegendY - 10, "Group Legend:");
            for (const auto& [group, groupStr] : m_groupStringMap) {
                QColor c = m_groupColorMap.count(group) ? m_groupColorMap[group] : Qt::black;
                QRect colorRect(groupLegendX, groupLegendY + i * (boxSize + spacing), boxSize, boxSize);
                painter.fillRect(colorRect, c);
                painter.drawRect(colorRect);
                painter.drawText(groupLegendX + textOffset, groupLegendY + i * (boxSize + spacing) + boxSize - 2, groupStr);
                ++i;
            }
        }

        // Add legend entry for connect stats line if enabled
        if (m_connectStatsLineEnabled && m_connectStatsLineType != ConnectStatsType::None) {
            QPen pen(Qt::darkCyan, 2, Qt::DashDotLine);
            QString label = "Connect ";
            switch (m_connectStatsLineType) {
                case ConnectStatsType::Average: label += "Avg"; break;
                case ConnectStatsType::Median: label += "Median"; break;
                case ConnectStatsType::Mode: label += "Mode"; break;
                default: break;
            }
            label += " (first/last n)";
            drawStatLegendEntry(pen, label);
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

void HighPerfLineChart::setShowAverageLine(bool show) {
    m_showAverageLine = show;
    update();
}

bool HighPerfLineChart::showAverageLine() const {
    return m_showAverageLine;
}

void HighPerfLineChart::setShowMedianLine(bool show) {
    m_showMedianLine = show;
    update();
}

bool HighPerfLineChart::showMedianLine() const {
    return m_showMedianLine;
}

void HighPerfLineChart::setShowPercentileLines(bool show, double lower, double upper) {
    m_showPercentileLines = show;
    m_percentileLower = lower;
    m_percentileUpper = upper;
    update();
}

bool HighPerfLineChart::showPercentileLines() const {
    return m_showPercentileLines;
}

double HighPerfLineChart::percentileLower() const {
    return m_percentileLower;
}

double HighPerfLineChart::percentileUpper() const {
    return m_percentileUpper;
}

void HighPerfLineChart::setShowMinLine(bool show) {
    m_showMinLine = show;
    update();
}

bool HighPerfLineChart::showMinLine() const {
    return m_showMinLine;
}

void HighPerfLineChart::setShowMaxLine(bool show) {
    m_showMaxLine = show;
    update();
}

bool HighPerfLineChart::showMaxLine() const {
    return m_showMaxLine;
}

void HighPerfLineChart::setShowStddevLines(bool show) {
    m_showStddevLines = show;
    update();
}

bool HighPerfLineChart::showStddevLines() const {
    return m_showStddevLines;
}

void HighPerfLineChart::setShowAllLines(bool show) {
    m_showAllLines = show;
    update();
}

bool HighPerfLineChart::showAllLines() const {
    return m_showAllLines;
}

void HighPerfLineChart::setGlobalLineColor(const QColor& color) {
    m_globalLineColor = color;
    update();
}

QColor HighPerfLineChart::globalLineColor() const {
    return m_globalLineColor;
}

void HighPerfLineChart::setGlobalLineWidth(float width) {
    m_globalLineWidth = width;
    update();
}

float HighPerfLineChart::globalLineWidth() const {
    return m_globalLineWidth;
}

void HighPerfLineChart::setGlobalLineOpacity(float opacity) {
    m_globalLineOpacity = opacity;
    update();
}

float HighPerfLineChart::globalLineOpacity() const {
    return m_globalLineOpacity;
}

// --- Statistical line legend label setters ---
void HighPerfLineChart::setAverageLineLabel(const QString& label) { m_averageLineLabel = label; update(); }
void HighPerfLineChart::setMedianLineLabel(const QString& label) { m_medianLineLabel = label; update(); }
void HighPerfLineChart::setPercentileLineLabels(const QString& lowerLabel, const QString& upperLabel) {
    m_percentileLowerLabel = lowerLabel; m_percentileUpperLabel = upperLabel; update();
}
void HighPerfLineChart::setMinLineLabel(const QString& label) { m_minLineLabel = label; update(); }
void HighPerfLineChart::setMaxLineLabel(const QString& label) { m_maxLineLabel = label; update(); }
void HighPerfLineChart::setStddevLineLabel(const QString& label) { m_stddevLineLabel = label; update(); }

// --- Additional statistical line setters/getters ---
void HighPerfLineChart::setShowModeLine(bool show) { m_showModeLine = show; update(); }
bool HighPerfLineChart::showModeLine() const { return m_showModeLine; }
void HighPerfLineChart::setShowVarianceLine(bool show) { m_showVarianceLine = show; update(); }
bool HighPerfLineChart::showVarianceLine() const { return m_showVarianceLine; }
void HighPerfLineChart::setShowLowerQuartileLine(bool show) { m_showLowerQuartileLine = show; update(); }
bool HighPerfLineChart::showLowerQuartileLine() const { return m_showLowerQuartileLine; }
void HighPerfLineChart::setShowUpperQuartileLine(bool show) { m_showUpperQuartileLine = show; update(); }
bool HighPerfLineChart::showUpperQuartileLine() const { return m_showUpperQuartileLine; }
void HighPerfLineChart::setShowMADLine(bool show) { m_showMADLine = show; update(); }
bool HighPerfLineChart::showMADLine() const { return m_showMADLine; }
void HighPerfLineChart::setShowSkewnessLine(bool show) { m_showSkewnessLine = show; update(); }
bool HighPerfLineChart::showSkewnessLine() const { return m_showSkewnessLine; }
void HighPerfLineChart::setShowKurtosisLine(bool show) { m_showKurtosisLine = show; update(); }
bool HighPerfLineChart::showKurtosisLine() const { return m_showKurtosisLine; }

// --- Additional statistical line legend label setters ---
void HighPerfLineChart::setModeLineLabel(const QString& label) { m_modeLineLabel = label; update(); }
void HighPerfLineChart::setVarianceLineLabel(const QString& label) { m_varianceLineLabel = label; update(); }
void HighPerfLineChart::setLowerQuartileLineLabel(const QString& label) { m_lowerQuartileLineLabel = label; update(); }
void HighPerfLineChart::setUpperQuartileLineLabel(const QString& label) { m_upperQuartileLineLabel = label; update(); }
void HighPerfLineChart::setMADLineLabel(const QString& label) { m_madLineLabel = label; update(); }
void HighPerfLineChart::setSkewnessLineLabel(const QString& label) { m_skewnessLineLabel = label; update(); }
void HighPerfLineChart::setKurtosisLineLabel(const QString& label) { m_kurtosisLineLabel = label; update(); }

// --- Connect stats line setters/getters ---
void HighPerfLineChart::setConnectStatsLineEnabled(bool enabled) { m_connectStatsLineEnabled = enabled; update(); }
bool HighPerfLineChart::connectStatsLineEnabled() const { return m_connectStatsLineEnabled; }
void HighPerfLineChart::setConnectStatsLineN(int n) { m_connectStatsLineN = std::max(1, n); update(); }
int HighPerfLineChart::connectStatsLineN() const { return m_connectStatsLineN; }
void HighPerfLineChart::setConnectStatsLineType(ConnectStatsType type) { m_connectStatsLineType = type; update(); }
HighPerfLineChart::ConnectStatsType HighPerfLineChart::connectStatsLineType() const { return m_connectStatsLineType; }

// --- Legend visibility setters/getters ---
void HighPerfLineChart::setShowLegend(bool show) { m_showLegend = show; update(); }
bool HighPerfLineChart::showLegend() const { return m_showLegend; }