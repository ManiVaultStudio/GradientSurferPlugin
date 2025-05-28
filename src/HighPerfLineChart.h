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

// Add this enum for axis selection
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
    void setChartTitle(const QString& title); // Add this setter

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

private:
    QVector<LineData> m_lines;
    QString m_xAxisName, m_yAxisName;
    double m_xMin = 0, m_xMax = 1, m_yMin = 0, m_yMax = 1;
    int m_selectedLine = -1;
    int m_hoveredLine = -1;
    QString m_emptyMessage;
    QString m_chartTitle; // Add this member for the chart title

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
    bool m_showPoints;

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
};