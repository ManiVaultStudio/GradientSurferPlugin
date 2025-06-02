#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QVector>
#include <QColor>
#include <QString>
#include <QMap>
#include <map>
#include <unordered_map>
#include <QFont>
#include <QPen>

enum class LineDashStyle {
    Solid,
    Dash,
    Dot
};

struct LineData {
    QVector<QPointF> points;
    QColor color = Qt::black;
    float opacity = 1.0f;
    bool selected = false;
    int group = -1; // Group number for this line
    // Map from group number to group string (shared across all lines)
    static std::unordered_map<int, QString> groupStringMap;

    // --- Customization options per line ---
    float lineWidth = 0.0f;  // If >0, overrides chart default line width
    float pointSize = 0.0f;  // If >0, overrides chart default point size
    LineDashStyle dashStyle = LineDashStyle::Solid; // New: per-line dash style
};


enum class BinaryNormAxis { X, Y };

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
    void setChartTitle(const QString& title); 

    // Selection/hover
    int selectedLine() const;
    int hoveredLine() const;

    // Option to enable group averaging mode
    void setGroupAveragingEnabled(bool enabled);
    bool groupAveragingEnabled() const { return m_groupAveragingEnabled; }
    // Set the group string map (for legend)
    void setGroupStringMap(const std::unordered_map<int, QString>& groupMap);

    // Log transform option for x coordinates
    void setLogTransformX(bool enabled);
    bool logTransformX() const { return m_logTransformX; }

    // Log transform option for y coordinates
    void setLogTransformY(bool enabled);
    bool logTransformY() const { return m_logTransformY; }

    // Strong log transform option
    void setStrongLogTransform(bool enabled);
    bool strongLogTransform() const { return m_strongLogTransform; }

    // Gradient line and noise removal options
    void setGradientLineEnabled(bool enabled);
    bool gradientLineEnabled() const { return m_gradientLineEnabled; }

    void setNoiseRemovalEnabled(bool enabled);
    bool noiseRemovalEnabled() const { return m_noiseRemovalEnabled; }
    void setNoiseRemovalWindow(int window);
    int noiseRemovalWindow() const { return m_noiseRemovalWindow; }
    void setNoiseRemovalThreshold(double threshold);
    double noiseRemovalThreshold() const { return m_noiseRemovalThreshold; }

    // --- Customization API ---
    void setLineWidth(float w);
    void setPointSize(float s);
    void setAxisFont(const QFont& f);
    void setAxisColor(const QColor& c);
    void setNumXTicks(int n);
    void setNumYTicks(int n);
    void setGridEnabled(bool en);
    void setGridColor(const QColor& c);
    void setBackgroundColor(const QColor& c);
    void setShowPoints(bool en);
    bool showPoints() const;

    // NEW: show/hide lines connecting points
    void setShowLines(bool en);
    bool showLines() const;

    // New customization options
    void setAxisLineWidth(float w);
    void setXAxisLineColor(const QColor& c);
    void setYAxisLineColor(const QColor& c);
    void setTickLabelFont(const QFont& f);
    void setTickLabelColor(const QColor& c);
    void setChartMargins(int left, int right, int top, int bottom);
    void setLegendFont(const QFont& f);
    void setLegendColor(const QColor& c);
    void setLegendPosition(int x, int y);

    // --- Binary normalization API ---
    void setBinaryNormalizeX(bool enabled);
    void setBinaryNormalizeY(bool enabled);
    void setBinaryNormalizeXThreshold(double threshold);
    void setBinaryNormalizeYThreshold(double threshold);
    bool binaryNormalizeX() const;
    bool binaryNormalizeY() const;
    double binaryNormalizeXThreshold() const;
    double binaryNormalizeYThreshold() const;

    // --- Interchange X and Y API ---
    void setInterchangeXY(bool enabled);
    bool interchangeXY() const;

    // Show/hide average line
    void setShowAverageLine(bool show);
    bool showAverageLine() const;

    // Add for median/percentile lines
    void setShowMedianLine(bool show);
    bool showMedianLine() const;
    void setShowPercentileLines(bool show, double lower=0.25, double upper=0.75);
    bool showPercentileLines() const;
    double percentileLower() const;
    double percentileUpper() const;

    // New: min/max/stddev lines
    void setShowMinLine(bool show);
    bool showMinLine() const;
    void setShowMaxLine(bool show);
    bool showMaxLine() const;
    void setShowStddevLines(bool show);
    bool showStddevLines() const;

    // New: show/hide all lines
    void setShowAllLines(bool show);
    bool showAllLines() const;

    // New: global line style
    void setGlobalLineColor(const QColor& color);
    QColor globalLineColor() const;
    void setGlobalLineWidth(float width);
    float globalLineWidth() const;
    void setGlobalLineOpacity(float opacity);
    float globalLineOpacity() const;

    // Statistical line legend label setters (optional, for customization)
    void setAverageLineLabel(const QString& label);
    void setMedianLineLabel(const QString& label);
    void setPercentileLineLabels(const QString& lowerLabel, const QString& upperLabel);
    void setMinLineLabel(const QString& label);
    void setMaxLineLabel(const QString& label);
    void setStddevLineLabel(const QString& label);

    // --- Additional statistical lines ---
    void setShowModeLine(bool show);
    bool showModeLine() const;
    void setShowVarianceLine(bool show);
    bool showVarianceLine() const;
    void setShowLowerQuartileLine(bool show);
    bool showLowerQuartileLine() const;
    void setShowUpperQuartileLine(bool show);
    bool showUpperQuartileLine() const;
    void setShowMADLine(bool show);
    bool showMADLine() const;
    void setShowSkewnessLine(bool show);
    bool showSkewnessLine() const;
    void setShowKurtosisLine(bool show);
    bool showKurtosisLine() const;

    // Additional statistical line legend label setters
    void setModeLineLabel(const QString& label);
    void setVarianceLineLabel(const QString& label);
    void setLowerQuartileLineLabel(const QString& label);
    void setUpperQuartileLineLabel(const QString& label);
    void setMADLineLabel(const QString& label);
    void setSkewnessLineLabel(const QString& label);
    void setKurtosisLineLabel(const QString& label);

    // Legend show/hide
    void setShowLegend(bool show);
    bool showLegend() const;

    // Option to draw a line between the statistic (avg/median/mode) of first n and last n points
    enum class ConnectStatsType { None, Average, Median, Mode };

    void setConnectStatsLineEnabled(bool enabled);
    bool connectStatsLineEnabled() const;
    void setConnectStatsLineN(int n);
    int connectStatsLineN() const;
    void setConnectStatsLineType(ConnectStatsType type);
    ConnectStatsType connectStatsLineType() const;

    // Add setter/getter for the label visibility
    void setShowAvgNumPointsLabel(bool show);
    bool showAvgNumPointsLabel() const;

    // --- Moving average smoothing ---
    void setShowMovingAverageLine(bool show);
    bool showMovingAverageLine() const;
    void setMovingAverageWindow(int window);
    int movingAverageWindow() const;

signals:
    void lineSelected(int index);
    void lineHovered(int index);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

    // --- Binary normalization helpers ---
    QVector<QPointF> binaryNormalize(const QVector<QPointF>& points, BinaryNormAxis axis) const;

    // Statistical line legend labels
    QString m_averageLineLabel = "Average";
    QString m_medianLineLabel = "Median";
    QString m_percentileLowerLabel = "25th Percentile";
    QString m_percentileUpperLabel = "75th Percentile";
    QString m_minLineLabel = "Min";
    QString m_maxLineLabel = "Max";
    QString m_stddevLineLabel = "Stddev";
    QString m_modeLineLabel = "Mode";
    QString m_varianceLineLabel = "Variance";
    QString m_lowerQuartileLineLabel = "Lower Quartile";
    QString m_upperQuartileLineLabel = "Upper Quartile";
    QString m_madLineLabel = "MAD";
    QString m_skewnessLineLabel = "Skewness";
    QString m_kurtosisLineLabel = "Kurtosis";

    // For connecting stats line
    bool m_connectStatsLineEnabled = false;
    int m_connectStatsLineN = 5;
    ConnectStatsType m_connectStatsLineType = ConnectStatsType::None;

    // Add a flag to show/hide average number of points label
    bool m_showAvgNumPointsLabel = true;

private:
    QVector<LineData> m_lines;
    QString m_xAxisName, m_yAxisName;
    double m_xMin = 0, m_xMax = 1, m_yMin = 0, m_yMax = 1;
    int m_selectedLine = -1;
    int m_hoveredLine = -1;
    QString m_emptyMessage;
    QString m_chartTitle; 

    bool m_groupAveragingEnabled = false;
    std::unordered_map<int, QString> m_groupStringMap;
    std::unordered_map<int, QColor> m_groupColorMap;
    void updateGroupColors();

    // Log transform for x and y
    bool m_logTransformX = false;
    bool m_logTransformY = false;
    bool m_strongLogTransform = false;
    double logX(double x) const;
    double invLogX(double lx) const;
    double logY(double y) const;
    double invLogY(double ly) const;
    double strongLog(double v) const;
    double invStrongLog(double lv) const;

    // Gradient line and noise removal options
    bool m_gradientLineEnabled = false;
    bool m_noiseRemovalEnabled = false;
    int m_noiseRemovalWindow = 3; // for moving average/median
    double m_noiseRemovalThreshold = 0.0; // for threshold-based removal

    QVector<QPointF> filterNoise(const QVector<QPointF>& points) const;
    QColor slopeToColor(double slope) const;

    QPointF screenToChart(const QPoint& p) const;
    int findLineAt(const QPoint& pos) const;

    // --- Customization members ---
    float m_lineWidth;
    float m_pointSize;
    QFont m_axisFont;
    QColor m_axisColor;
    int m_numXTicks;
    int m_numYTicks;
    bool m_gridEnabled;
    QColor m_gridColor;
    QColor m_backgroundColor;
    bool m_showPoints = true;
    bool m_showLines = true; // NEW: show/hide lines connecting points

    // New customization members
    float m_axisLineWidth = 1.0f;
    QColor m_xAxisLineColor = Qt::black;
    QColor m_yAxisLineColor = Qt::black;
    QFont m_tickLabelFont;
    QColor m_tickLabelColor = Qt::black;
    int m_leftMargin = 50, m_rightMargin = 20, m_topMargin = 40, m_bottomMargin = 40;
    QFont m_legendFont;
    QColor m_legendColor = Qt::black;
    int m_legendX = -1, m_legendY = -1; // -1 means auto

    // --- Binary normalization members ---
    bool m_binaryNormalizeX;
    bool m_binaryNormalizeY;
    double m_binaryNormalizeXThreshold;
    double m_binaryNormalizeYThreshold;

    // --- Interchange X and Y member ---
    bool m_interchangeXY;

    // --- Average line member ---
    bool m_showAverageLine = false;

    // --- Median and percentile lines members ---
    bool m_showMedianLine = false;
    bool m_showPercentileLines = false;
    double m_percentileLower = 0.25;
    double m_percentileUpper = 0.75;

    // --- Min, max, and stddev lines members ---
    bool m_showMinLine = false;
    bool m_showMaxLine = false;
    bool m_showStddevLines = false;

    // --- Show/hide all lines member ---
    bool m_showAllLines = true;

    // --- Global line style members ---
    QColor m_globalLineColor = Qt::black;
    float m_globalLineWidth = 1.0f;
    float m_globalLineOpacity = 1.0f;

    // --- Additional statistical line flags ---
    bool m_showModeLine = false;
    bool m_showVarianceLine = false;
    bool m_showLowerQuartileLine = false;
    bool m_showUpperQuartileLine = false;
    bool m_showMADLine = false;
    bool m_showSkewnessLine = false;
    bool m_showKurtosisLine = false;

    // Legend show/hide
    bool m_showLegend = true; // Show/hide legend

    // --- Moving average smoothing ---
    bool m_showMovingAverageLine = false;
    int m_movingAverageWindow = 5;
    QVector<QPointF> movingAverage(const QVector<QPointF>& points, int window) const;
    QString m_movingAverageLineLabel;
};