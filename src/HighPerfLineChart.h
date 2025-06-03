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


    // New: show/hide all lines
    void setShowAllLines(bool show);
    bool showAllLines() const;



    // Legend show/hide
    void setShowLegend(bool show);
    bool showLegend() const;

    // Option to draw a line between the statistic (avg/median/mode) of first n and last n points
    enum class ConnectStatsType { None, Average, Median, Mode, Mean };

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

    std::unordered_map<int, QString> m_groupStringMap;
    std::unordered_map<int, QColor> m_groupColorMap;
    void updateGroupColors();


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


    // --- Show/hide all lines member ---
    bool m_showAllLines = true;

    // --- Global line style members ---
    QColor m_globalLineColor = Qt::black;
    float m_globalLineWidth = 1.0f;
    float m_globalLineOpacity = 1.0f;

    // Legend show/hide
    bool m_showLegend = true; // Show/hide legend

    // --- Moving average smoothing ---
    bool m_showMovingAverageLine = false;
    int m_movingAverageWindow = 5;
    QVector<QPointF> movingAverage(const QVector<QPointF>& points, int window) const;
    QString m_movingAverageLineLabel;

    // --- New helper methods for refactored paintGL ---
    void drawChartWithOpenGL();
    void drawAxesAndLabels(QPainter& painter);
    void drawLegend(QPainter& painter);
    void drawAverageLabel(QPainter& painter);

    // Add missing log/antilog methods
    double logX(double x) const;
    double invLogX(double lx) const;
    double logY(double y) const;
    double invLogY(double ly) const;
};