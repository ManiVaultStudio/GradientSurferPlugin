#include "Computation.h"

#include <iostream>
#include <set>
#include "LayerSurferPlugin.h"
#include "SettingsAction.h"
#include<string>  
#include <QFileDialog>
#include <QPageLayout>
#include <QWebEngineView>
#include <chrono>
#include <typeinfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QSet>
#include <QJsonValue>
#include <Eigen/Dense>
#include <QtConcurrent>
#include <QColor>
#include "util/StyledIcon.h"
#include <QLabel>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QToolTip>
#include <QElapsedTimer>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QVector>

using namespace mv;
using namespace mv::gui;

class FunctionTimer {
public:
    FunctionTimer(const QString& functionName)
        : _functionName(functionName)
    {
        _timer.start();
    }
    ~FunctionTimer()
    {
        qDebug() << _functionName << "took"
            << _timer.elapsed() / 1000.0 << "seconds";
    }
private:
    QString _functionName;
    QElapsedTimer _timer;
};
class CorrelationBarDelegate : public QStyledItemDelegate {
public:
    CorrelationBarDelegate(float minCorr, float maxCorr, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), minValue(minCorr), maxValue(maxCorr) {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override {
        if (index.column() == 1) { // Correlation column
            float value = index.data(Qt::UserRole + 1).toFloat();

            // --- Centered y-axis logic ---
            float maxAbs = std::max(std::abs(minValue), std::abs(maxValue));
            if (maxAbs < 1e-6f) maxAbs = 1.0f; // Avoid division by zero

            QRect barRect = option.rect.adjusted(4, 4, -4, -4);

            int centerX = barRect.left() + barRect.width() / 2;

            int barStartX = centerX;
            int barEndX = centerX + static_cast<int>((value / maxAbs) * (barRect.width() / 2));
            if (barEndX < barStartX) std::swap(barStartX, barEndX);

            QRect filledRect = barRect;
            filledRect.setLeft(barStartX);
            filledRect.setRight(barEndX);

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);

            // Draw selection or normal background
            QColor bgColor = (option.state & QStyle::State_Selected)
                ? option.palette.color(QPalette::Highlight)
                : option.palette.color(QPalette::Base);
            painter->fillRect(option.rect, bgColor);

            // Dotted y-axis line: white on black bg, black on white bg, else contrast
            QColor axisColor;
            auto isColorClose = [](const QColor& c1, const QColor& c2, int threshold = 16) {
                return (std::abs(c1.red() - c2.red()) < threshold &&
                    std::abs(c1.green() - c2.green()) < threshold &&
                    std::abs(c1.blue() - c2.blue()) < threshold);
                };
            bool bgIsBlack = isColorClose(bgColor, QColor(Qt::black));
            bool bgIsWhite = isColorClose(bgColor, QColor(Qt::white));
            if (bgIsBlack) {
                axisColor = QColor(Qt::white);
            }
            else if (bgIsWhite) {
                axisColor = QColor(Qt::black);
            }
            else {
                double luminance = (0.299 * bgColor.red() + 0.587 * bgColor.green() + 0.114 * bgColor.blue()) / 255.0;
                axisColor = (luminance > 0.5) ? QColor(Qt::black) : QColor(Qt::white);
            }

            QPen axisPen(axisColor);
            axisPen.setWidth(2);
            axisPen.setStyle(Qt::DotLine);

            int axisExtra = std::max(2, barRect.height() / 8);
            int axisTop = barRect.top() - axisExtra;
            int axisBottom = barRect.bottom() + axisExtra;

            painter->setPen(axisPen);
            painter->drawLine(centerX, axisTop, centerX, axisBottom);

            // Bar color: not blue, contrast to both axis and background
            QColor barColor;
            if (bgIsBlack || axisColor == QColor(Qt::white)) {
                barColor = QColor(255, 140, 0); // Orange
            }
            else if (bgIsWhite || axisColor == QColor(Qt::black)) {
                barColor = QColor(128, 0, 128); // Purple
            }
            else {
                double axisLuminance = (0.299 * axisColor.red() + 0.587 * axisColor.green() + 0.114 * axisColor.blue()) / 255.0;
                barColor = (axisLuminance > 0.5) ? QColor(0, 128, 0) : QColor(255, 165, 0);
            }

            // Draw the bar only if value is not zero
            if (std::abs(value) > 1e-6f) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(barColor);
                painter->drawRect(filledRect.normalized());
            }

            // Draw focus rect if needed
            if (option.state & QStyle::State_HasFocus) {
                QStyleOptionFocusRect focusOption;
                focusOption.QStyleOption::operator=(option);
                focusOption.rect = option.rect;
                focusOption.state |= QStyle::State_KeyboardFocusChange;
                focusOption.backgroundColor = bgColor;
                QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, painter);
            }

            painter->restore();
            // --- End: Centered y-axis logic ---
        }
        else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }

    bool helpEvent(QHelpEvent* event, QAbstractItemView* view,
        const QStyleOptionViewItem& option, const QModelIndex& index) override {
        if (index.column() == 1) {
            float value = index.data(Qt::UserRole + 1).toFloat();
            QToolTip::showText(event->globalPos(), QString::number(value, 'g', 4), view);
            return true;
        }
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }

private:
    float minValue, maxValue;
};
enum class NormalizationType {
    None,
    ZScore,
    MinMax,
    Log2
};
float computeGradientScore(const std::vector<float>& corrArray, const std::vector<float>& expArray) {
    if (corrArray.size() != expArray.size()) {
        qWarning() << "Correlation and expression arrays must have the same size for gradient score calculation.";
        return 0.0f;
    }
    float score = 0.0f;
    int N = std::min(corrArray.size(), expArray.size());
    for (int i = 0; i < N; ++i) {
        score += corrArray[i] * expArray[i];
    }
    return score;
}
void normalizeVector(std::vector<float>& vec, NormalizationType type) {
    if (vec.empty() || type == NormalizationType::None)
        return;

    const float EPSILON = 1e-8f; // Document this threshold

    switch (type) {
    case NormalizationType::ZScore: {
        float sum = std::accumulate(vec.begin(), vec.end(), 0.0f);
        float mean = sum / vec.size();

        // More numerically stable variance calculation
        float variance = 0.0f;
        for (float v : vec) {
            float diff = v - mean;
            variance += diff * diff;
        }
        variance /= vec.size();

        float stdev = std::sqrt(variance);
        if (stdev < EPSILON) stdev = 1.0f;

        for (auto& v : vec) {
            v = (v - mean) / stdev;
        }
        break;
    }
    case NormalizationType::MinMax: {
        auto [minIt, maxIt] = std::minmax_element(vec.begin(), vec.end());
        float minVal = *minIt;
        float maxVal = *maxIt;
        float range = maxVal - minVal;

        if (range < EPSILON) range = 1.0f;

        for (auto& v : vec) {
            v = (v - minVal) / range;
        }
        break;
    }
    case NormalizationType::Log2: {
        // Adding 1 before log2 to handle zeros - document this transformation
        for (auto& v : vec) {
            v = (v > 0.0f) ? std::log2(v + 1.0f) : 0.0f;
        }
        break;
    }
    default:
        // Should never reach here if all enum values are handled
        assert(false && "Unknown normalization type");
        break;
    }
}
static QString getContrastingTextColor(const QString& bgColor) {
    QColor color(bgColor);
    if (!color.isValid()) return "white";
    // Calculate luminance (perceived brightness)
    double luminance = (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) / 255.0;
    return (luminance > 0.5) ? "black" : "white";
}

QVector<LineData> convertToLineData(const std::vector<float>& vecAll, const std::vector<float>& vecCoord, int pointsPerLine, int numOfLines, const QVector<QColor>& colors, bool coordIsX, bool sortByX)
{
    QVector<LineData> lines;

    //qDebug() << "Conversion started. Total elements:" << vecAll.size()<< "Points per line:" << pointsPerLine<< "Number of lines:" << numOfLines<< "coordIsX:" << coordIsX;

    // Validate input
    if (pointsPerLine < 1 || numOfLines < 1) {
        qWarning() << "Invalid parameters - pointsPerLine:" << pointsPerLine
            << "numOfLines:" << numOfLines;
        return lines;
    }

    const size_t requiredSize = static_cast<size_t>(pointsPerLine * numOfLines);
    if (vecAll.size() < requiredSize || vecCoord.size() < static_cast<size_t>(pointsPerLine)) {
        qWarning() << "Insufficient data - Got:" << vecAll.size()
            << "Need:" << requiredSize << "and vecCoord size:" << vecCoord.size();
        return lines;
    }

    lines.reserve(numOfLines);

    for (int lineIdx = 0; lineIdx < numOfLines; ++lineIdx) {
        LineData line;

        // Set color
        if (!colors.isEmpty() && lineIdx < colors.size()) {
            line.color = colors[lineIdx];
        }
        else {
            static const QVector<QColor> defaultColors = {
                Qt::blue, Qt::green, Qt::red, Qt::cyan,
                Qt::magenta, Qt::yellow, Qt::gray
            };
            line.color = defaultColors[lineIdx % defaultColors.size()];
        }

        line.points.reserve(pointsPerLine);

        for (int pointIdx = 0; pointIdx < pointsPerLine; ++pointIdx) {
            size_t pos = static_cast<size_t>(pointIdx) * numOfLines + lineIdx;
            if (pos >= vecAll.size() || pointIdx >= static_cast<int>(vecCoord.size())) {
                qWarning() << "Index out of bounds at line" << lineIdx
                    << "point" << pointIdx;
                continue;
            }

            float x, y;
            if (!coordIsX) {
                x = vecAll[pos];
                y = vecCoord[pointIdx];
            }
            else {
                x = vecCoord[pointIdx];
                y = vecAll[pos];
            }

            if (!std::isfinite(x) || !std::isfinite(y)) {
                qWarning() << "Non-finite point at line" << lineIdx
                    << "point" << pointIdx << ":" << x << y;
                continue;
            }

            line.points.append(QPointF(x, y));
        }

        // Sort points if requested
        if (sortByX) {
            if (coordIsX) {
                std::sort(line.points.begin(), line.points.end(), [](const QPointF& a, const QPointF& b) {
                    return a.x() < b.x();
                    });
            }
            else {
                std::sort(line.points.begin(), line.points.end(), [](const QPointF& a, const QPointF& b) {
                    return a.y() < b.y();
                    });
            }
        }

        if (!line.points.isEmpty()) {
            lines.append(line);
            //qDebug() << "Added line" << lineIdx << "with" << line.points.size() << "points";
            if (!line.points.isEmpty()) {
                //qDebug() << "  First point:" << line.points.first().x() << line.points.first().y();
                //qDebug() << "  Last point:" << line.points.last().x() << line.points.last().y();
            }
        }
        else {
            //qWarning() << "Skipping empty line" << lineIdx;
        }
    }

    //qDebug() << "Conversion complete. Generated" << lines.size() << "lines";
    return lines;
}

static QString statusToIconName(const QString& status) {
    if (status == "C") return "circle-check";      // FontAwesome: check-circle
    if (status == "M") return "circle-exclamation"; // FontAwesome: exclamation-circle
    if (status == "E") return "circle-xmark";      // FontAwesome: times-circle
    if (status == "R") return "spinner";           // FontAwesome: spinner
    return "circle-question";                      // FontAwesome: question-circle
}

Computation::Computation(LayerSurferPlugin& LayerSurferPlugin, SettingsAction& SettingsAction)
    : WidgetAction(&LayerSurferPlugin, "Computation"),
    _viewerPlugin(LayerSurferPlugin),
    _settingsAction(SettingsAction)
{

    _correlationRangeDebounceTimer1.setSingleShot(true);
    _clusterSelectionDebounceTimer.setSingleShot(true);
    _correlationRangeDebounceTimer2.setSingleShot(true);

    const auto updateStatus = [this]() -> void {
        auto stringFull = _settingsAction.getStatusHolder().getStatusMessageAction().getString();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(stringFull.toUtf8(), &parseError);
        QString status, message; QColor color = "#6c757d";
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            status = obj.value("status").toString();
            message = obj.value("message").toString();
            color = obj.value("color").toString();
        }
        else {
            // fallback for legacy comma-separated string
            status = stringFull.split(",").first().trimmed();
            message = stringFull.split(",").last().trimmed();
        }

        QString labelText = message.isEmpty() ? "" : message;
        QString backgroundColor = "none";

        if (status == "C") {
            if (labelText.isEmpty()) labelText = "Updated";
            backgroundColor = "#28a745"; // Green
        }
        else if (status == "M") {
            if (labelText.isEmpty()) labelText = "Outdated";
            backgroundColor = "#ffc107"; // Gold
        }
        else if (status == "E") {
            if (labelText.isEmpty()) labelText = "Error";
            backgroundColor = "#dc3545"; // Red
        }
        else if (status == "R") {
            if (labelText.isEmpty()) labelText = "Processing";
            backgroundColor = "#007bff"; // Blue
        }
        else {
            if (labelText.isEmpty()) labelText = "Unknown";
            backgroundColor = "#6c757d"; // Grey
        }

        QString textColor = getContrastingTextColor(backgroundColor);

        QString iconName = statusToIconName(status);

        // Create a QLabel to get the font metrics for text height
        QLabel tempLabel;
        tempLabel.setText(labelText);
        QFont font = tempLabel.font();
        QFontMetrics fm(font);
        int textHeight = fm.height();
        int iconSize = std::max(12, textHeight - 2); // icon a little less than text height, min 12px

        // Set icon color to match text color
        QColor iconColor(textColor);
        mv::util::StyledIcon icon(iconName);
        icon.withColor(iconColor);

        QPixmap iconPixmap = QIcon(icon).pixmap(iconSize, iconSize);

        // Create a QLabel with icon and text
        QLabel* statusLabel = new QLabel;
        statusLabel->setTextFormat(Qt::RichText);

        // Use HTML to combine icon and text
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        iconPixmap.save(&buffer, "PNG");
        QString imgTag = QString("<img style='vertical-align:middle;' width='%1' height='%2' src='data:image/png;base64,%3'/>")
            .arg(iconSize)
            .arg(iconSize)
            .arg(QString::fromLatin1(ba.toBase64()));
        statusLabel->setText(imgTag + "&nbsp;" + labelText);
        statusLabel->setStyleSheet(
            "QLabel{padding-left:8px;background:" + backgroundColor + ";color:" + textColor + ";}"
        );
        statusLabel->setFont(font);

        // Clear previous widgets and set new label in status bar
        QStatusBar* statusBar = _settingsAction.getStatusHolder().getStatusBarAction();
        if (statusBar) {
            // Remove all previous widgets
            QList<QWidget*> children = statusBar->findChildren<QWidget*>();
            for (QWidget* child : children) {
                statusBar->removeWidget(child);
                child->deleteLater();
            }
            statusBar->addWidget(statusLabel, 1);
        }

        };
    connect(&_settingsAction.getStatusHolder().getStatusMessageAction(), &StringAction::stringChanged, this, updateStatus);
    const auto scatterplotSelectionOptions = [this]() -> void
        {
            QStringList scatterplotNames = {};
            QString selectedCurrentText1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
            QString selectedCurrentText2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
            auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
            if (scatterplotViewFactory)
            {

                for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
                {
                    scatterplotNames.append(plugin->getGuiName());
                }
            }

            if (!scatterplotNames.contains(selectedCurrentText1))
            {
                selectedCurrentText1 = "";
                if (scatterplotNames.size() > 0)
                {
                    selectedCurrentText1 = scatterplotNames[0];
                }
            }
            if (!scatterplotNames.contains(selectedCurrentText2))
            {
                selectedCurrentText2 = "";
                if (scatterplotNames.size() > 1)
                {
                    selectedCurrentText1 = scatterplotNames[1];
                }
                else if (scatterplotNames.size() > 0)
                {
                    selectedCurrentText2 = scatterplotNames[0];
                }
            }

            _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().setOptions(scatterplotNames);
            _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().setCurrentText(selectedCurrentText1);
            _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().setOptions(scatterplotNames);
            _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().setCurrentText(selectedCurrentText2);

        };

    connect(&mv::plugins(), &AbstractPluginManager::pluginAdded, this, scatterplotSelectionOptions);

    _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        bool validity = false;
        Dataset<Points> sibling = _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset();
        if (sibling.isValid())
        {
            if (sibling->getParent()== dataset->getParent() && dataset->getDataType() == ClusterType && (dataset->getGroupIndex()!=666 || dataset->getGroupIndex()!=667))
            {
                validity = true;
            }
        }
        
        
        return validity;
        });
     _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });
     _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
         bool validity = false;
         auto parent = dataset->getParent();
         bool checkGroup = false;
         if (dataset->getGroupIndex() != 666 && dataset->getGroupIndex() != 667)
         {
             checkGroup = true;
         }
         if (parent.isValid() && dataset->getDataType() == PointType && checkGroup)
         {
             validity = true;
         }
         
        return validity;
        });

     _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        bool validity = false;
        Dataset<Points> sibling = _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset();
        if (sibling.isValid())
        {
            if (sibling->getParent()== dataset->getParent() && dataset->getDataType() == ClusterType && (dataset->getGroupIndex()!=666 || dataset->getGroupIndex()!=667))
            {
                validity = true;
            }
        }


        return validity;
         });
     _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
         return dataset->getDataType() == PointType;
         });
     _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
         bool validity = false;
         auto parent = dataset->getParent();
         bool checkGroup = false;
         if (dataset->getGroupIndex() != 666 && dataset->getGroupIndex() != 667)
         {
             checkGroup = true;
         }
         if (parent.isValid() && dataset->getDataType() == PointType && checkGroup)
         {
             validity = true;
         }

         return validity;
         });

    const auto updateClusterDataset1 = [this]() -> void {
        updateClusterOptions();
        setStatusMessage("M", "Cluster dataset updated", "");
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset1);

    const auto updateClusterDataset2 = [this]() -> void {
        updateClusterOptions();
        setStatusMessage("M", "Cluster dataset updated", "");
        };

    connect(&_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset2);

    const auto updatePointDataset1 = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid())
        {
            auto dataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
            if (dataset.isValid())
            {
                 _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setPointsDataset(dataset);
                auto dimensions = dataset->getNumDimensions();
                
            }


        }
        else
        {
             _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setPointsDataset({});
        }
        setStatusMessage("M", "Point dataset 1 updated", "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset1);

    const auto updatePointDataset2 = [this]() -> void {
        if (_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().isValid())
        {
            auto dataset = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId());
            if (dataset.isValid())
            {
                _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setPointsDataset(dataset);
                auto dimensions = dataset->getNumDimensions();

            }


        }
        else
        {
            _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setPointsDataset({});
        }
        setStatusMessage("M", "Point dataset 2 updated", "");
        };

    connect(&_settingsAction.getDataset2OptionsHolder().getPointDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset2);

    const auto updateGeneSelection1 = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionIndex() == -1)
        {
            qWarning() << "No gene selected";
             _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }

        auto selectedGene =  _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene == "")
        {
            qWarning() << "No gene selected";
             _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }
        if ( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() &&  _filteredSelectionIndices.size() > 0)
        {

            auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
            if (lineplotViewFactory)
            {
                // qDebug() << "LinePlot View Factory found, populating lineplot view...";
                for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
                {

                    if (plugin->getGuiName() == "LinePlot View 1")
                    {
                        //qDebug() << "Processing plugin:" << plugin->getGuiName();
                        //plugin->printChildren();

                        mv::gui::DatasetPickerAction* pointDatasetAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Point dataset"));
                        mv::gui::DatasetPickerAction* clusterDatasetAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Cluster dataset"));
                        if (pointDatasetAction && _lineChartPointDataset1.isValid())
                        {
                            pointDatasetAction->setCurrentDataset(_lineChartPointDataset1);

                            if (clusterDatasetAction && _lineChartClusterDataset1.isValid())
                            {
                                clusterDatasetAction->setCurrentDataset(_lineChartClusterDataset1);
                            }
                            mv::gui::StringAction* chartTitle = dynamic_cast<StringAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Chart Title"));
                            if (chartTitle)
                            {
                                QString titleText = QString("%1 vs %2 expression of selected cells (%3)")
                                    .arg(QStringLiteral("Rotated Y Coordinates"))
                                    .arg(selectedGene)
                                    .arg(static_cast<int>(_filteredSelectionIndices.size()));
                                chartTitle->setString(titleText);

                            }


                            mv::gui::OptionAction* dimensionX = dynamic_cast<OptionAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Data Dimension X Selection/Select dimension"));
                            mv::gui::OptionAction* dimensionY = dynamic_cast<OptionAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Data Dimension Y Selection/Select dimension"));
                            if (dimensionX)
                            {
                                dimensionX->setCurrentText("Rotated Y Coordinates");
                            }
                            if (dimensionY)
                            {
                                dimensionY->setCurrentText(selectedGene);
                            }
                            events().notifyDatasetDataChanged(_lineChartPointDataset1);

                        }


                    }
                }
            }

        }
        else
        {
            qWarning() << "No point dataset selected";
        }


        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        DimensionPickerAction* geneSelectionAction;
         _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().setEnabled(true);
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText())
                {
                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
                            if (embeddingDataset.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
                                if (dataset.isValid())
                                {
                                    colorDatasetPickerAction->setCurrentDataset(dataset);
                                }

                            }
                            geneSelectionAction = dynamic_cast<DimensionPickerAction*>(plugin->findChildByPath("Settings/Coloring/Dimension"));
                            if (geneSelectionAction)
                            {
                                geneSelectionAction->setCurrentDimensionName(selectedGene);
                            }
                            else
                            {
                                qWarning() << "Gene selection action not found";
                            }
                        }



                    }

                }
            }
        }
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, updateGeneSelection1);


    const auto computationTypeChanged = [this]() {
        
        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        setSelectionAndAutoCompute();

        _settingsAction.getDataset1OptionsHolder().getFreezeSelectionAction().setChecked(false);
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        };
    connect(&_settingsAction.getCommonOptionsHolder().getComputationTypesAction(), &OptionAction::currentIndexChanged, this, computationTypeChanged);
    const auto updateGeneSelection2 = [this]() -> void {

        auto selectedGene2 = _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().getCurrentDimensionName();
        if (selectedGene2 == "")
        {
            qWarning() << "No gene selected";
            _settingsAction.getDataset2OptionsHolder().getDeSelectRow2Action().setEnabled(false);
            return;
        }

        auto selectedGene = _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene == "")
        {
            qWarning() << "No gene selected";
            _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        DimensionPickerAction* geneSelectionAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText())
                {
                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId());
                            if (embeddingDataset2.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset2);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId());
                                if (dataset2.isValid())
                                {
                                    colorDatasetPickerAction->setCurrentDataset(dataset2);
                                }

                            }
                            geneSelectionAction = dynamic_cast<DimensionPickerAction*>(plugin->findChildByPath("Settings/Coloring/Dimension"));
                            if (geneSelectionAction)
                            {
                                geneSelectionAction->setCurrentDimensionName(selectedGene2);
                            }
                            else
                            {
                                qWarning() << "Gene selection action not found";
                            }
                        }



                    }

                }
            }
        }

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getGeneSelection2Action(), &DimensionPickerAction::currentDimensionIndexChanged, this, updateGeneSelection2);


    const auto updateEmbeddingDataset1 = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().isValid())
        {


            _embeddingDataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());

            if (_embeddingDataset.isValid())
            {
                auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
                if (scatterplotViewFactory)
                {
                    auto scatterplotName = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
                    if (!scatterplotName.isEmpty())
                    {
                        for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
                        {
                            if (plugin->getGuiName() == scatterplotName)
                            {
                                mv::gui::DatasetPickerAction* pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                                if (pointDatasetPickerAction)
                                {


                                    pointDatasetPickerAction->setCurrentDataset(_embeddingDataset);

                                }
                            }
                        }
                    }
                }
                
                
                auto parent = _embeddingDataset->getParent();
                if (parent.isValid())
                {
                    _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().setCurrentDataset(parent);
                    auto children = parent->getChildren();
                    Datasets clusterDatasets;
                    for (const Dataset<Clusters>& child : children) {
                        if (child->getDataType() == ClusterType && (child->getGroupIndex() != 667 || child->getGroupIndex() != 666)) {
                            clusterDatasets.append(child);

                        }
                    }
                    auto selectedClusterDatasetName = _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset();

                    _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setDatasets(clusterDatasets);
                    if (clusterDatasets.contains(selectedClusterDatasetName))
                    {
                        _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setCurrentDataset(selectedClusterDatasetName);
                    }
                    else if (clusterDatasets.size() > 0)
                    {
                        _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setCurrentDataset(clusterDatasets.first());
                    }
                }
                else
                {
                    _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setDatasets({});
                }
                
                
                
                _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setPointsDataset(_embeddingDataset);
                 _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setPointsDataset(_embeddingDataset);
                 if (_embeddingDataset->getNumDimensions() >= 2)
                 {
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(0);
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(1);
                    }
                    else if (_embeddingDataset->getNumDimensions() == 1)
                    {
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(0);
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(0);
                    }
                    else
                 {
                     _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(-1);
                     _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(-1);
                 }
                const auto mainSelectionChanged = [this]() -> void {
                    //qDebug() << "Main selection changed in fullEmbeddingDataset";
                    setStatusMessage("M", "Selected indices changed", "");

                    auto fullclusterDataset = mv::data().getDataset<Clusters>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
                    if (fullclusterDataset.isValid() && _embeddingDataset.isValid())
                    {
                        auto clusters = fullclusterDataset->getClusters();
                        auto selectedClusters =  _settingsAction.getCommonOptionsHolder().getClusterSelectionAction().getSelectedOptions();
                        auto selectedIndices = _embeddingDataset->getSelectionIndices();
                        _settingsAction.getDataset1OptionsHolder().getFreezeSelectionAction().setChecked(true);
                        if (!selectedClusters.isEmpty())
                        {

                            for (const auto& cluster : clusters)
                            {
                                if (!selectedClusters.contains(cluster.getName()))
                                {

                                    auto clusterindices = cluster.getIndices();

                                    selectedIndices.erase(std::remove_if(selectedIndices.begin(), selectedIndices.end(),
                                        [&clusterindices](int index) {
                                            return std::find(clusterindices.begin(), clusterindices.end(), index) != clusterindices.end();
                                        }), selectedIndices.end());


                                }
                            }
                            _filteredSelectionIndices.clear();
                            _filteredSelectionIndices.reserve(selectedIndices.size());
                            for (const auto& index : selectedIndices)
                            {
                                _filteredSelectionIndices.push_back(index);
                            }
                            _embeddingDataset->setSelectionIndices(selectedIndices);
                            mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset);
                        }
                        else
                        {
                            qWarning() << "No clusters selected, clearing selection indices";
                            _embeddingDataset->setSelectionIndices({});
                            mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset);
                        }

                    }
                    else
                    {
                        qWarning() << "No cluster dataset selected or invalid cluster dataset";
                        _embeddingDataset->setSelectionIndices({});
                        mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset);
                    }
                    setStatusMessage("R", "Computation in progress", "");
                    _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
                    QApplication::processEvents();

                    setSelectionAndAutoCompute();
                    
                    _settingsAction.getDataset1OptionsHolder().getFreezeSelectionAction().setChecked(false);
                    _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);


                    };


                bool ok = connect(&_embeddingDataset, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged);
                if (!ok)
                {
                    qWarning() << "Failed to connect to dataSelectionChanged signal of fullEmbeddingDataset";
                }

            }


        }

        setStatusMessage("M", "Embedding dataset 1 updated", "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset1);

    const auto updateEmbeddingDataset2 = [this]() -> void {
        if (_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().isValid())
        {
            

            auto embeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId());
            
            if (embeddingDataset2.isValid())
            {
                auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
                if (scatterplotViewFactory)
                {
                    auto scatterplotName = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
                    if (!scatterplotName.isEmpty())
                    {
                        for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
                        {
                            if (plugin->getGuiName() == scatterplotName)
                            {
                                mv::gui::DatasetPickerAction* pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                                if (pointDatasetPickerAction)
                                {


                                    pointDatasetPickerAction->setCurrentDataset(embeddingDataset2);

                                }
                            }
                        }
                    }
                }
                auto parent = embeddingDataset2->getParent();
                if (parent.isValid())
                {
                    _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().setCurrentDataset(parent);
                    auto children = parent->getChildren();
                    Datasets clusterDatasets;
                    for (const Dataset<Clusters>& child : children) {
                        if (child->getDataType() == ClusterType && (child->getGroupIndex() != 667|| child->getGroupIndex() != 666)) {
                            clusterDatasets.append(child);
                            
                        }
                    }

                    auto selectedClusterDatasetName = _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset();
                    _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setDatasets(clusterDatasets);
                    if (clusterDatasets.contains(selectedClusterDatasetName))
                    {
                        _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setCurrentDataset(selectedClusterDatasetName);
                    }
                    else if (clusterDatasets.size() > 0)
                    {
                        _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setCurrentDataset(clusterDatasets.first());
                    }
                }
                else
                {
                    _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setDatasets({});
                }
                
                _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setPointsDataset(embeddingDataset2);
                _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setPointsDataset(embeddingDataset2);
                if (embeddingDataset2->getNumDimensions() >= 2)
                {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setCurrentDimensionIndex(0);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setCurrentDimensionIndex(1);
                }
                else if (embeddingDataset2->getNumDimensions() == 1)
                {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setCurrentDimensionIndex(0);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setCurrentDimensionIndex(0);
                }
                else
                {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setCurrentDimensionIndex(-1);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setCurrentDimensionIndex(-1);
                }
            }


        }

        setStatusMessage("M", "Embedding dataset 2 updated", "");
        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset2);

    const auto computeInfo = [this]() -> void {

        setStatusMessage("R", "Correlation computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        correlationAfterPCA(); 

        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        };

    connect(&_settingsAction.getCommonOptionsHolder().getComputeAction(), &TriggerAction::triggered, this, computeInfo);

    const auto correlationCalculationTypeActionChanged = [this]() -> void {

        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        setSelectionAndAutoCompute();

        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        };

    connect(&_settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction(), &OptionAction::currentIndexChanged, this, correlationCalculationTypeActionChanged);


    const auto computePCATrigger = [this]() -> void {

        setStatusMessage("R", "PCA Computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputePCAAction().setDisabled(true);
        QApplication::processEvents();
  
        ManualCompute();

        _settingsAction.getCommonOptionsHolder().getComputePCAAction().setDisabled(false);
        };

    connect(&_settingsAction.getCommonOptionsHolder().getComputePCAAction(), &TriggerAction::triggered, this, computePCATrigger);


    const auto pcaColorsToScatterplot = [this]() -> void {
        int selectedDimInd = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();
        QString selectedDimName = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionName();
        if (selectedDimInd < 0 || selectedDimName.isEmpty()) {
            qWarning() << "No PCA dimension selected for coloring";
            return;
        }
        auto options = _settingsAction.getCommonOptionsHolder().getDualPCAColoringAction().getSelectedOptions();
        if (options.isEmpty()) {
            qWarning() << "No PCA coloring options selected";
            return;
        }

        auto setScatterplotColorAndGene = [&](const QString& scatterplotName, mv::Dataset<Points>& dataset) {
            if (!dataset.isValid()) {
                qWarning() << "PCA common genes dataset is not valid";
                return false;
            }
            if (scatterplotName.isEmpty()) {
                qWarning() << "No scatterplot view selected";
                return false;
            }
            auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
            if (!scatterplotViewFactory)
                return false;
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
                if (plugin->getGuiName() == scatterplotName) {
                    auto pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction && pointDatasetPickerAction->getCurrentDataset().isValid()) {
                        auto colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                        if (colorDatasetPickerAction) {
                            colorDatasetPickerAction->setCurrentDataset(dataset);
                        }
                        auto geneSelectionAction = dynamic_cast<DimensionPickerAction*>(plugin->findChildByPath("Settings/Coloring/Dimension"));
                        if (geneSelectionAction) {
                            geneSelectionAction->setCurrentDimensionName(selectedDimName);
                        }
                        else {
                            qWarning() << "Gene selection action not found";
                        }
                    }
                    break;
                }
            }
            return true;
            };
        if (_settingsAction.getCommonOptionsHolder().getPcaColoringAction().isChecked()) {
            bool doDataset1 = options.contains("Dataset1");
            bool doDataset2 = options.contains("Dataset2");
            if (doDataset1) {
                setScatterplotColorAndGene(
                    _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(),
                    _pcaCommonGenesDataset1
                );
            }
            if (doDataset2) {
                setScatterplotColorAndGene(
                    _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(),
                    _pcaCommonGenesDataset2
                );
            }
        }
        else {
            _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().trigger();
            _settingsAction.getDataset2OptionsHolder().getDeSelectRow2Action().trigger();

            // Lambda to select the first row in a table view if available
            auto selectFirstRow = [](QTableView* tableView) {
                if (tableView) {
                    auto* model = tableView->model();
                    if (model && model->rowCount() > 0) {
                        QModelIndex firstIndex = model->index(0, 0);
                        if (firstIndex.isValid()) {
                            tableView->setCurrentIndex(firstIndex);
                            tableView->selectionModel()->select(
                                firstIndex,
                                QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                            );
                        }
                    }
                }
                };

            bool hasDataset1 = options.contains("Dataset1");
            bool hasDataset2 = options.contains("Dataset2");

            if (hasDataset1) {
                selectFirstRow(_settingsAction.getTableView1Action());
            }
            if (hasDataset2) {
                selectFirstRow(_settingsAction.getTableView2Action());
            }
        }


        };

    connect(&_settingsAction.getCommonOptionsHolder().getPcaColoringAction(), &ToggleAction::toggled, this, pcaColorsToScatterplot);
    connect(&_settingsAction.getCommonOptionsHolder().getDualPCAColoringAction(), &OptionsAction::selectedOptionsChanged, this, pcaColorsToScatterplot);

    connect(&_settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, pcaColorsToScatterplot);

    connect(& _settingsAction.getCommonOptionsHolder().getClusterSelectionAction(),
        &OptionsAction::selectedOptionsChanged,
        this,
        [this]() {
            _clusterSelectionDebounceTimer.start(800);
        });
    connect(&_clusterSelectionDebounceTimer, &QTimer::timeout, this, [this]() {
        //setStatusMessage("M", "Cluster selection updated", "");
        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        
        setSelectionAndAutoCompute();

        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        });



    connect(& _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction(),
        &DecimalRangeAction::rangeChanged,
        this,
        [this]() {
            _correlationRangeDebounceTimer1.start(1000);
        });
    connect(&_correlationRangeDebounceTimer1, &QTimer::timeout, this, [this]() {
        setStatusMessage("R", "Updating correlation 1 table", "");
        resetupCorrelationTable(1);
        //drawCombinedLines();
        setStatusMessage("C", "Correlation table 1 updated", "");
        if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked())
        {
            setStatusMessage("R", "Computation in progress", "");
            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
            QApplication::processEvents();

            ManualCompute();

            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        }
        });


    connect(&_settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action(),
        &DecimalRangeAction::rangeChanged,
        this,
        [this]() {
            _correlationRangeDebounceTimer2.start(300);
        });
    connect(&_correlationRangeDebounceTimer2, &QTimer::timeout, this, [this]() {
        setStatusMessage("R", "Updating correlation 2 table", "");
        resetupCorrelationTable(2);
        resetupCorrelationTable(3);
        setStatusMessage("C", "Correlation table 2 updated", "");
        });

    const auto showPointSelectionChanged1 = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();


        if (scatterplotName1.isEmpty())
        {
            qWarning() << "No scatterplot view 1 selected";

        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1)
                {
                    auto pointSelectionScaleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Scale"));
                    auto pointSelectionOpacityAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Opacity"));
                    if (pointSelectionScaleAction)
                    {
                        if ( _settingsAction.getDataset1OptionsHolder().getHidePointSelectionOutlineAction().isChecked())
                        {
                            pointSelectionScaleAction->setValue(100.0);
                        }
                        else
                        {
                            pointSelectionScaleAction->setValue(160.0);
                        }

                    }
                    else
                    {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {
                        if ( _settingsAction.getDataset1OptionsHolder().getHidePointSelectionOutlineAction().isChecked())
                        {
                            pointSelectionOpacityAction->setValue(0.0);
                        }
                        else
                        {
                            pointSelectionOpacityAction->setValue(100.0);
                        }

                    }
                    else
                    {
                        qWarning() << "Point selection opacity action not found";
                    }
                }
            }
        }
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getHidePointSelectionOutlineAction(), &ToggleAction::toggled, this, showPointSelectionChanged1);

    const auto freezeSelectionChanged1 = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();

 
        if (scatterplotName1.isEmpty())
        {
            qWarning() << "No scatterplot view 1 selected";

        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1 )
                {
                    auto freezeAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Selection/Freeze selection"));
                    if (freezeAction)
                    {
                        freezeAction->setChecked( _settingsAction.getDataset1OptionsHolder().getFreezeSelectionAction().isChecked());
                    }
                    else
                    {
                        qWarning() << "Freeze selection action not found";
                    }
                }
            }
        }

        };

    connect(& _settingsAction.getDataset1OptionsHolder().getFreezeSelectionAction(), &ToggleAction::toggled, this, freezeSelectionChanged1);


    const auto focusSelectionChanged1 = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();


        if (scatterplotName1.isEmpty())
        {
            qWarning() << "No scatterplot view 1 selected";

        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1 )
                {
                    auto focusAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Plot/Point/Focus selection"));
                    if (focusAction)
                    {
                        focusAction->setChecked(_settingsAction.getDataset1OptionsHolder().getFocusSelectionAction().isChecked());
                    }
                    else
                    {
                        qWarning() << "Focus selection action not found";
                    }
                }
            }
        }

        };

    connect(& _settingsAction.getDataset1OptionsHolder().getFocusSelectionAction(), &ToggleAction::toggled, this, focusSelectionChanged1);



    const auto showPointSelectionChanged2 = [this]() -> void {

        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();


        if (scatterplotName2.isEmpty())
        {
            qWarning() << "No scatterplot view 2 selected";

        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName2 )
                {
                    auto pointSelectionScaleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Scale"));
                    auto pointSelectionOpacityAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Opacity"));
                    if (pointSelectionScaleAction)
                    {
                        if (_settingsAction.getDataset2OptionsHolder().getHidePointSelectionOutline2Action().isChecked())
                        {
                            pointSelectionScaleAction->setValue(100.0);
                        }
                        else
                        {
                            pointSelectionScaleAction->setValue(160.0);
                        }

                    }
                    else
                    {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {
                        if (_settingsAction.getDataset2OptionsHolder().getHidePointSelectionOutline2Action().isChecked())
                        {
                            pointSelectionOpacityAction->setValue(0.0);
                        }
                        else
                        {
                            pointSelectionOpacityAction->setValue(100.0);
                        }

                    }
                    else
                    {
                        qWarning() << "Point selection opacity action not found";
                    }
                }
            }
        }
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getHidePointSelectionOutlineAction(), &ToggleAction::toggled, this, showPointSelectionChanged2);

    const auto freezeSelectionChanged2 = [this]() -> void {

        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();

        if (scatterplotName2.isEmpty())
        {
            qWarning() << "No scatterplot view 2 selected";

        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName2)
                {
                    auto freezeAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Selection/Freeze selection"));
                    if (freezeAction)
                    {
                        freezeAction->setChecked(_settingsAction.getDataset2OptionsHolder().getFreezeSelection2Action().isChecked());
                    }
                    else
                    {
                        qWarning() << "Freeze selection action not found";
                    }
                }
            }
        }

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getFreezeSelection2Action(), &ToggleAction::toggled, this, freezeSelectionChanged2);


    const auto focusSelectionChanged2 = [this]() -> void {

        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();

        if (scatterplotName2.isEmpty())
        {
            qWarning() << "No scatterplot view 2 selected";

        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName2)
                {
                    auto focusAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Plot/Point/Focus selection"));
                    if (focusAction)
                    {
                        focusAction->setChecked(_settingsAction.getDataset2OptionsHolder().getFocusSelection2Action().isChecked());
                    }
                    else
                    {
                        qWarning() << "Focus selection action not found";
                    }
                }
            }
        }

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getFocusSelection2Action(), &ToggleAction::toggled, this, focusSelectionChanged2);


    const auto embeddingDimensionYActionChanged1 = [this]() -> void {

        setStatusMessage("M", "Embedding 1 Y dimension changed", "");
        QString scatterplotName = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName.isEmpty())
        {
            qWarning() << "No scatterplot view selected";
            setStatusMessage("E", "No scatterplot view selected", "");
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName)
                {

                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
                            if (embeddingDataset.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition =  _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName();
                            auto yPosition =  _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName();
                            if (xPosition != "" && yPosition != "") {
                                auto embeddingXVal = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/X/Select dimension"));
                                auto embeddingYVal = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/Y/Select dimension"));
                                if (embeddingXVal && embeddingYVal) {
                                    if (embeddingXVal->getCurrentIndex() != -1) {
                                        embeddingXVal->setCurrentText("");
                                    }
                                    if (embeddingYVal->getCurrentIndex() != -1) {
                                        embeddingYVal->setCurrentText("");
                                    }

                                    auto xoptions = embeddingXVal->getOptions();
                                    if (xoptions.contains(xPosition)) {
                                        embeddingXVal->setCurrentText(xPosition);
                                    }
                                    else {
                                        qWarning() << "Embedding X value not found in options";
                                    }

                                    auto yoptions = embeddingYVal->getOptions();
                                    if (yoptions.contains(yPosition)) {
                                        embeddingYVal->setCurrentText(yPosition);
                                    }
                                    else {
                                        qWarning() << "Embedding Y value not found in options";
                                    }
                                }
                                else {
                                    qWarning() << "Embedding X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
                                if (dataset.isValid())
                                {
                                    //colorDatasetPickerAction->setCurrentDataset(dataset);
                                    //qDebug() << "Setting dataset to color dataset";
                                }

                            }

                        }



                    }

                }
            }
        }



        };

    connect(& _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionYActionChanged1);


    const auto embeddingDimensionYActionChanged2 = [this]() -> void {

        setStatusMessage("M", "Embedding 2 Y dimension changed", "");
        QString scatterplotName = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
        if (scatterplotName.isEmpty())
        {
            qWarning() << "No scatterplot view 2 selected";
            setStatusMessage("E", "No scatterplot view 2 selected", "");
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName)
                {

                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId());
                            if (embeddingDataset2.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset2);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition2 = _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName();
                            auto yPosition2 = _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName();
                            if (xPosition2 != "" && yPosition2 != "") {
                                auto embeddingXVal2 = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/X/Select dimension"));
                                auto embeddingYVal2 = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/Y/Select dimension"));
                                if (embeddingXVal2 && embeddingYVal2) {
                                    if (embeddingXVal2->getCurrentIndex() != -1) {
                                        embeddingXVal2->setCurrentText("");
                                    }
                                    if (embeddingYVal2->getCurrentIndex() != -1) {
                                        embeddingYVal2->setCurrentText("");
                                    }

                                    auto xoptions2 = embeddingXVal2->getOptions();
                                    if (xoptions2.contains(xPosition2)) {
                                        embeddingXVal2->setCurrentText(xPosition2);
                                    }
                                    else {
                                        qWarning() << "Embedding 2 X value not found in options";
                                    }

                                    auto yoptions2 = embeddingYVal2->getOptions();
                                    if (yoptions2.contains(yPosition2)) {
                                        embeddingYVal2->setCurrentText(yPosition2);
                                    }
                                    else {
                                        qWarning() << "Embedding 2 Y value not found in options";
                                    }
                                }
                                else {
                                    qWarning() << "Embedding 2 X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
                                if (dataset2.isValid())
                                {
                                    //colorDatasetPickerAction->setCurrentDataset(dataset2);
                                    //qDebug() << "Setting dataset to color dataset";
                                }

                            }

                        }



                    }

                }
            }
        }




        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionYActionChanged2);

    const auto embeddingDimensionXActionChanged1 = [this]() -> void {

        setStatusMessage("M", "Embedding 1 X dimension changed", "");
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText())
                {

                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
                            if (embeddingDataset.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition =  _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName();
                            auto yPosition =  _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName();
                            if (xPosition != "" && yPosition != "") {
                                auto embeddingXVal = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/X/Select dimension"));
                                auto embeddingYVal = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/Y/Select dimension"));
                                if (embeddingXVal && embeddingYVal) {
                                    if (embeddingXVal->getCurrentIndex() != -1) {
                                        embeddingXVal->setCurrentText("");
                                    }
                                    if (embeddingYVal->getCurrentIndex() != -1) {
                                        embeddingYVal->setCurrentText("");
                                    }

                                    auto xoptions = embeddingXVal->getOptions();
                                    if (xoptions.contains(xPosition)) {
                                        embeddingXVal->setCurrentText(xPosition);
                                    }
                                    else {
                                        qWarning() << "Embedding X value not found in options";
                                    }

                                    auto yoptions = embeddingYVal->getOptions();
                                    if (yoptions.contains(yPosition)) {
                                        embeddingYVal->setCurrentText(yPosition);
                                    }
                                    else {
                                        qWarning() << "Embedding Y value not found in options";
                                    }
                                }
                                else {
                                    qWarning() << "Embedding X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
                                if (dataset.isValid())
                                {
                                    //colorDatasetPickerAction->setCurrentDataset(dataset);
                                    //qDebug() << "Setting dataset to color dataset";
                                }

                            }

                        }



                    }

                }
            }
        }

        };

    connect(& _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionXActionChanged1);


    const auto embeddingDimensionXActionChanged2 = [this]() -> void {

        setStatusMessage("M", "Embedding 1 X dimension changed", "");
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText())
                {

                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId());
                            if (embeddingDataset2.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset2);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition2 = _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName();
                            auto yPosition2 = _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName();
                            if (xPosition2 != "" && yPosition2 != "") {
                                auto embeddingXVal2 = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/X/Select dimension"));
                                auto embeddingYVal2 = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/Y/Select dimension"));
                                if (embeddingXVal2 && embeddingYVal2) {
                                    if (embeddingXVal2->getCurrentIndex() != -1) {
                                        embeddingXVal2->setCurrentText("");
                                    }
                                    if (embeddingYVal2->getCurrentIndex() != -1) {
                                        embeddingYVal2->setCurrentText("");
                                    }

                                    auto xoptions2 = embeddingXVal2->getOptions();
                                    if (xoptions2.contains(xPosition2)) {
                                        embeddingXVal2->setCurrentText(xPosition2);
                                    }
                                    else {
                                        qWarning() << "Embedding 2 X value not found in options";
                                    }

                                    auto yoptions2 = embeddingYVal2->getOptions();
                                    if (yoptions2.contains(yPosition2)) {
                                        embeddingYVal2->setCurrentText(yPosition2);
                                    }
                                    else {
                                        qWarning() << "Embedding 2 Y value not found in options";
                                    }
                                }
                                else {
                                    qWarning() << "Embedding 2 X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
                                if (dataset2.isValid())
                                {
                                    //colorDatasetPickerAction->setCurrentDataset(dataset2);
                                    //qDebug() << "Setting dataset to color dataset";
                                }

                            }

                        }



                    }

                }
            }
        }

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionXActionChanged2);

    const auto deselectRow1 = [this]() -> void {
        auto selectedIndex = _settingsAction.getTableView1Action()->currentIndex();
        if (selectedIndex.isValid())
        {
            _settingsAction.getTableView1Action()->selectionModel()->clearSelection();
            _settingsAction.getTableView2Action()->selectionModel()->clearSelection();
            _settingsAction.getTableView3Action()->selectionModel()->clearSelection();
            _settingsAction.getTableView1Action()->setCurrentIndex(QModelIndex());
             _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName("");
             _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionName("");
        }
        else
        {
            qWarning() << "No row selected";
        }
         _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().setEnabled(false);
         _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);
         //TODO: Fix thisSS
         /*
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText())
                {

                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
                            if (embeddingDataset.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition =  _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName();
                            auto yPosition =  _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName();
                            if (xPosition != "" && yPosition != "") {
                                auto embeddingXVal = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/X/Select dimension"));
                                auto embeddingYVal = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/Y/Select dimension"));
                                if (embeddingXVal && embeddingYVal) {
                                    if (embeddingXVal->getCurrentIndex() != -1) {
                                        embeddingXVal->setCurrentText("");
                                    }
                                    if (embeddingYVal->getCurrentIndex() != -1) {
                                        embeddingYVal->setCurrentText("");
                                    }

                                    auto xoptions = embeddingXVal->getOptions();
                                    if (xoptions.contains(xPosition)) {
                                        embeddingXVal->setCurrentText(xPosition);
                                    }
                                    else {
                                        qDebug() << "Embedding X value not found in options";
                                    }

                                    auto yoptions = embeddingYVal->getOptions();
                                    if (yoptions.contains(yPosition)) {
                                        embeddingYVal->setCurrentText(yPosition);
                                    }
                                    else {
                                        qDebug() << "Embedding Y value not found in options";
                                    }
                                }
                                else {
                                    qDebug() << "Embedding X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
                                if (dataset.isValid())
                                {
                                    //colorDatasetPickerAction->setCurrentDataset(dataset);
                                    //qDebug() << "Setting dataset to color dataset";
                                }

                            }

                        }



                    }

                }
            }
        }

        */
        _numOfPointsInLine1 = 0;
        auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
        if (lineplotViewFactory)
        {
            // qDebug() << "LinePlot View Factory found, populating lineplot view...";
            for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
            {

                if (plugin->getGuiName() == "LinePlot View 1")
                {
                    //qDebug() << "Processing plugin:" << plugin->getGuiName();
                    //plugin->printChildren();

                    mv::gui::DatasetPickerAction* pointDatasetAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Point dataset"));
                    mv::gui::DatasetPickerAction* clusterDatasetAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Cluster dataset"));
                    if (clusterDatasetAction)
                    {
                        clusterDatasetAction->setCurrentIndex(-1);
                    }
                    if (pointDatasetAction)
                    {
                        pointDatasetAction->setCurrentIndex(-1);
                    }
                    if (_lineChartPointDataset1.isValid())
                    {
                        events().notifyDatasetDataChanged(_lineChartPointDataset1);
                    }

                }
            }
        }
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction(), &TriggerAction::triggered, this, deselectRow1);

    const auto deselectRow2 = [this]() -> void {
        auto selectedIndex = _settingsAction.getTableView1Action()->currentIndex();
        if (selectedIndex.isValid())
        {
            _settingsAction.getTableView1Action()->selectionModel()->clearSelection();
            _settingsAction.getTableView1Action()->setCurrentIndex(QModelIndex());
            
            _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionName("");
        }
        else
        {
            qWarning() << "No row selected";
        }
        _settingsAction.getDataset2OptionsHolder().getDeSelectRow2Action().setEnabled(false);
       
        _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionIndex(-1);

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
          
                 if (plugin->getGuiName() == _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText())
                {

                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId());
                            if (embeddingDataset2.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset2);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition2 = _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName();
                            auto yPosition2 = _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName();
                            if (xPosition2 != "" && yPosition2 != "") {
                                auto embeddingXVal2 = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/X/Select dimension"));
                                auto embeddingYVal2 = dynamic_cast<OptionAction*>(plugin->findChildByPath("Settings/Position/Y/Select dimension"));
                                if (embeddingXVal2 && embeddingYVal2) {
                                    if (embeddingXVal2->getCurrentIndex() != -1) {
                                        embeddingXVal2->setCurrentText("");
                                    }
                                    if (embeddingYVal2->getCurrentIndex() != -1) {
                                        embeddingYVal2->setCurrentText("");
                                    }

                                    auto xoptions2 = embeddingXVal2->getOptions();
                                    if (xoptions2.contains(xPosition2)) {
                                        embeddingXVal2->setCurrentText(xPosition2);
                                    }
                                    else {
                                        qWarning() << "Embedding 2 X value not found in options";
                                    }

                                    auto yoptions2 = embeddingYVal2->getOptions();
                                    if (yoptions2.contains(yPosition2)) {
                                        embeddingYVal2->setCurrentText(yPosition2);
                                    }
                                    else {
                                        qWarning() << "Embedding 2 Y value not found in options";
                                    }
                                }
                                else {
                                    qWarning() << "Embedding 2 X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
                                if (dataset2.isValid())
                                {
                                    colorDatasetPickerAction->setCurrentDataset(dataset2);
                                    qWarning() << "Setting dataset to color dataset";
                                }

                            }

                        }



                    }

                }
            }
        }
       
        _numOfPointsInLine2 = 0;
        };

    connect(&_settingsAction.getDataset2OptionsHolder().getDeSelectRow2Action(), &TriggerAction::triggered, this, deselectRow2);
    const auto scatterplotviewSelectionChanged1= [this]() -> void {

        setStatusMessage("M", "Scatterplotview selection 1 updated", "");

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName1.isEmpty())
        {
            qWarning() << "No scatterplot view selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1)
                {
                    mv::gui::ToggleAction* notifyDuringSelectionAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Selection/Point Selection/Notify during selection"));
                    mv::gui::ToggleAction* haloAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Selection/Halo"));

                    if (notifyDuringSelectionAction) {
                        notifyDuringSelectionAction->setChecked(false);
                    }
                    else
                    {
                        qWarning() << "Notify during selection action not found.";
                    }
                    if (haloAction) {
                        haloAction->setChecked(true);
                    }
                    else
                    {
                        qWarning() << "Halo action not found.";
                    }

                    auto pointSelectionScaleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Scale"));
                    auto pointSelectionOpacityAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Opacity"));
                    if (pointSelectionScaleAction)
                    {

                        pointSelectionScaleAction->setValue(160.0);

                    }
                    else
                    {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {

                        pointSelectionOpacityAction->setValue(100.0);



                    }
                    else
                    {
                        qWarning() << "Point selection opacity action not found";
                    }

                }
            }
        }

        };

    connect(&_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction(), &OptionAction::currentIndexChanged, this, scatterplotviewSelectionChanged1);


    const auto scatterplotviewSelectionChanged2 = [this]() -> void {

        setStatusMessage("M", "Scatterplotview selection 2 updated", "");

        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
        if (scatterplotName2.isEmpty())
        {
            qWarning() << "No scatterplot view 2 selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName2)
                {
                    mv::gui::ToggleAction* notifyDuringSelectionAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Selection/Point Selection/Notify during selection"));
                    mv::gui::ToggleAction* haloAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Selection/Halo"));

                    if (notifyDuringSelectionAction) {
                        notifyDuringSelectionAction->setChecked(false);
                    }
                    else
                    {
                        qWarning() << "Notify during selection action not found.";
                    }
                    if (haloAction) {
                        haloAction->setChecked(true);
                    }
                    else
                    {
                        qWarning() << "Halo action not found.";
                    }

                    auto pointSelectionScaleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Scale"));
                    auto pointSelectionOpacityAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Opacity"));
                    if (pointSelectionScaleAction)
                    {

                        pointSelectionScaleAction->setValue(160.0);

                    }
                    else
                    {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {

                        pointSelectionOpacityAction->setValue(100.0);



                    }
                    else
                    {
                        qWarning() << "Point selection opacity action not found";
                    }

                }
            }
        }

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action(), &OptionAction::currentIndexChanged, this, scatterplotviewSelectionChanged2);
    _pca1.setCompletionCallback(nullptr);
    _pca1.setCompletionCallback([this](bool success, const std::string& message) {
        if (success) {
            qDebug() << "PCA 1 finished successfully:" << message.c_str();
           
            if (_settingsAction.getCommonOptionsHolder().getComputationTypesAction().getCurrentText() == "PCA Loadings Per Component")
            {
                processAfterPCAComplete();
            }
            else if (_settingsAction.getCommonOptionsHolder().getComputationTypesAction().getCurrentText() == "PCA Loadings Total") 
            {
                processAfterPCACompleteAlt();
            }
        }
        else {
            setStatusMessage("E", "PCA Failed", message.c_str());
            qWarning() << "PCA 1 failed:" << message.c_str();
        }
        });
}


void Computation::AutoCompute()
{
    if (_isComputing) return;
    _isComputing = true;
    auto guard = qScopeGuard([&] { _isComputing = false; });
    try
    {
        // Remove all rows in one call if model exists
        clearTable(3);
        clearTable(2);
        clearTable(1);

        auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
        if (lineplotViewFactory)
        {
            // qDebug() << "LinePlot View Factory found, populating lineplot view...";
            for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
            {

                if (plugin->getGuiName() == "LinePlot View 1")
                {
                    //qDebug() << "Processing plugin:" << plugin->getGuiName();
                    //plugin->printChildren();

                    mv::gui::DatasetPickerAction* pointDatasetAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Point dataset"));
                    mv::gui::DatasetPickerAction* clusterDatasetAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Cluster dataset"));
                    if (clusterDatasetAction)
                    {
                        clusterDatasetAction->setCurrentIndex(-1);
                    }
                    if (pointDatasetAction)
                    {
                        pointDatasetAction->setCurrentIndex(-1);
                    }
                    if (_lineChartPointDataset1.isValid())
                    {
                        events().notifyDatasetDataChanged(_lineChartPointDataset1);
                    }

                }
            }
        }


        _numOfPointsInLine1 = 0;
        QApplication::processEvents();

        //Validate and retrieve datasets and indices
        mv::Dataset<Points> fullPointDataset;
        mv::Dataset<Clusters> fullClusterDataset;
        int xEmbeddingIndex = -1, yEmbeddingIndex = -1; float maxCorrelation = 1.0f, minCorrelation = 0.0f;
        std::vector<int> selectedIndices;
        QString scatterplotViewName;
        if (!validateAndRetrieveDatasets(fullPointDataset, fullClusterDataset, xEmbeddingIndex, yEmbeddingIndex, selectedIndices, scatterplotViewName, minCorrelation, maxCorrelation))
            return;

        // Create datasets if needed
        if (!createDatasetsIfNeeded())
            return;

        // Get rotation angle from plugin
        float rotationAngle = 0.0f;
        if (!getRotationAngle(scatterplotViewName, rotationAngle))
            return;

        // Populate embedding coordinates
        std::vector<float> XYCoordinates;
        if (!populateEmbeddingCoordinates(xEmbeddingIndex, yEmbeddingIndex, XYCoordinates))
            return;

        // Rotate and extract coordinates
        std::vector<float> rotatedXYCordinates;
        try {
            rotatedXYCordinates = rotateAxis(XYCoordinates, rotationAngle);
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in rotateAxis:" << ex.what();
            setStatusMessage("E", "Error occurred during point rotation", "");
            return;
        }

        std::vector<float> yCoordinates;
        try {
            yCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "Y");
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in getCoordinatesOfCells:" << ex.what();
            setStatusMessage("E", "Error occurred during fetching coordinates", "");
            return;
        }

        std::vector<int> sortedCellIndices;
        try {
            sortedCellIndices = getSortedCellIndices(yCoordinates);
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in getSortedCellIndices:" << ex.what();
            setStatusMessage("E", "Error occurred during sorting indices", "");
            return;
        }

        int noOfDimensions = fullPointDataset->getNumDimensions();
        const auto& dimensionNames = fullPointDataset->getDimensionNames();
        std::vector<int> columnIndices(noOfDimensions);
        std::iota(columnIndices.begin(), columnIndices.end(), 0);

        // Populate and update datasets
        if (!populateAndUpdateDatasets(fullPointDataset, rotatedXYCordinates, XYCoordinates,noOfDimensions, columnIndices, dimensionNames))
            return;

        // Min/max calculation and debug output
        const auto xCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "X");
        float minX = *std::min_element(xCoordinates.begin(), xCoordinates.end());
        float maxX = *std::max_element(xCoordinates.begin(), xCoordinates.end());
        float minY = *std::min_element(yCoordinates.begin(), yCoordinates.end());
        float maxY = *std::max_element(yCoordinates.begin(), yCoordinates.end());
        //qDebug() << "Rotated XY Coordinates minX:" << minX << "maxX:" << maxX << "minY:" << minY << "maxY:" << maxY;
        //qDebug() << "Rotated XY Coordinates size:" << rotatedXYCordinates.size() << "filteredSelectionIndices size:" << _filteredSelectionIndices.size() << "yCoordinates size:" << yCoordinates.size();

        // Correlation and gene filtering
        std::vector<float> corrGeneCoords;
        computeCorrelationGeneCoords(_filteredSelectionIndices, fullPointDataset, yCoordinates, corrGeneCoords);

        
        try {
            populateGenesByCorrelationRange(corrGeneCoords, minCorrelation, maxCorrelation, dimensionNames,1);
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            setStatusMessage("E", "Error occurred during filtering genes", "");
            return;
        }

        // Setup table model and connect selection
        setupTableModelAndSelection(1);
        //select the first row of the table

        if (lineplotViewFactory)
        {
           // qDebug() << "LinePlot View Factory found, populating lineplot view...";
            for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
            {
                
                if (plugin->getGuiName() == "LinePlot View 1")
                {
                    //qDebug() << "Processing plugin:" << plugin->getGuiName();
                    //plugin->printChildren();
                    
                   mv::gui::DatasetPickerAction* pointDatasetAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Point dataset"));
                   mv::gui::DatasetPickerAction* clusterDatasetAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("LinePlotViewPlugin/Dataset1 Options/Cluster dataset"));
                   if (pointDatasetAction && _lineChartPointDataset1.isValid())
                   {
                       pointDatasetAction->setCurrentDataset(_lineChartPointDataset1);
                       if (clusterDatasetAction && _lineChartClusterDataset1.isValid())
                       {
                           clusterDatasetAction->setCurrentDataset(_lineChartClusterDataset1);
                       }
                       events().notifyDatasetDataChanged(_lineChartPointDataset1);
                   }

                }
            }
        }

        //populate _lineChartPointDataset1 and _lineChartClusterDataset1 here TODO



        setStatusMessage("C", "Computation completed", "#28a745");

        if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked())
        {
            setStatusMessage("R", "Computation in progress", "");
            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
            QApplication::processEvents();

            ManualCompute();
            

            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        }

    }
    catch (const std::exception& ex)
    {
        qWarning() << "Exception in Computation:" << ex.what();
        setStatusMessage("E", "Error occurred during computation: " + QString::fromStdString(ex.what()), "");
    }
    catch (...)
    {
        qWarning() << "Unknown exception in Computation";
        setStatusMessage("E", "Unknown error occurred during computation", "");
    }
    _isComputing = false;
}

void Computation::ManualCompute()
{
    //mv::Dataset<Points> fullPointDataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());

    // Setup and trigger PCA
    
    if (_settingsAction.getCommonOptionsHolder().getComputationTypesAction().getCurrentText()=="Correlation")
    {
        processAlternativeToPCA();
    }
    else if(_settingsAction.getCommonOptionsHolder().getComputationTypesAction().getCurrentText()=="PCA Loadings Per Component")
    {
        bool complete = setupAndTriggerPCA();
        if (!complete)
        {
            clearTable(3);
            clearTable(2);
            return;
        }
    }
    else if(_settingsAction.getCommonOptionsHolder().getComputationTypesAction().getCurrentText() == "PCA Loadings Total")
    {
        bool complete = setupAndTriggerPCA();
        if (!complete)
        {
            clearTable(3);
            clearTable(2);
            return;
        }
    }
    else
    {
        qDebug() << "Unknown computation type selected:" << _settingsAction.getCommonOptionsHolder().getComputationTypesAction().getCurrentText();
        setStatusMessage("E", "Unknown computation type selected", "");
        return;
    }
    
}

std::vector<float> Computation::rotateAxis(const std::vector<float>& xyCoordinates, float degreeValue) {
    std::vector<float> rotated(xyCoordinates.size());
    const double angle = degreeValue * M_PI / 180.0;
    const double cosAngle = std::cos(angle);
    const double sinAngle = std::sin(angle);

    if (xyCoordinates.size() % 2 != 0) {
        throw std::invalid_argument("xyCoordinates vector must have an even number of elements (x, y pairs).");
    }

    // Calculate initial bounding rectangle
    float minX = xyCoordinates[0], maxX = xyCoordinates[0];
    float minY = xyCoordinates[1], maxY = xyCoordinates[1];
    for (std::size_t i = 0; i < xyCoordinates.size(); i += 2) {
        minX = std::min(minX, xyCoordinates[i]);
        maxX = std::max(maxX, xyCoordinates[i]);
        minY = std::min(minY, xyCoordinates[i + 1]);
        maxY = std::max(maxY, xyCoordinates[i + 1]);
    }

    // Calculate centroid
    const double centerX = (minX + maxX) / 2.0;
    const double centerY = (minY + maxY) / 2.0;

    // Rotate each point
    for (std::size_t i = 0; i < xyCoordinates.size(); i += 2) {
        const double originalX = xyCoordinates[i];
        const double originalY = xyCoordinates[i + 1];

        // Translate to origin and rotate
        const double x = originalX - centerX;
        const double y = originalY - centerY;
        const double rotatedX = x * cosAngle - y * sinAngle;
        const double rotatedY = x * sinAngle + y * cosAngle;

        // Translate back to original center
        rotated[i] = static_cast<float>(rotatedX + centerX);
        rotated[i + 1] = static_cast<float>(rotatedY + centerY);
    }

    return rotated;
}


std::vector<float> Computation::getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis) {
    std::vector<float> coords;
    int offset = (axis == "Y") ? 1 : 0;
    for (size_t i = offset; i < rotatedXYCoordinates.size(); i += 2) {
        coords.push_back(rotatedXYCoordinates[i]);
    }
    return coords;
}


std::vector<int> Computation::getSortedCellIndices(const std::vector<float>& yCoordinates) {
    std::vector<int> indices(yCoordinates.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return yCoordinates[a] < yCoordinates[b];
        });
    return indices;
}

void Computation::computeCorrelationGeneCoordsFast(
    const std::vector<int>& cellIndices,
    const mv::Dataset<Points> dataset,
    const std::vector<float>& coords,
    std::vector<float>& corrVector)
{
    FunctionTimer timer(Q_FUNC_INFO);

    const int numGenes = dataset->getNumDimensions();
    const int numCells = static_cast<int>(cellIndices.size());
    const int numCoords = static_cast<int>(coords.size());

    Q_ASSERT(numCells == numCoords);

    //qDebug() << "Number of genes:" << numGenes<< "Number of cells:" << numCells<< "Number of coordinates:" << numCoords;

    // Sort (coord, cellIndex) pairs by coordinate value
    std::vector<std::pair<float, int>> coordIndexPairs(numCells);
    for (int i = 0; i < numCells; ++i)
        coordIndexPairs[i] = { coords[i], cellIndices[i] };

    std::sort(coordIndexPairs.begin(), coordIndexPairs.end());

    std::vector<int> sortedCellIndices(numCells);
    std::vector<float> sortedCoords(numCells);
    for (int i = 0; i < numCells; ++i) {
        sortedCoords[i] = coordIndexPairs[i].first;
        sortedCellIndices[i] = coordIndexPairs[i].second;
    }

    // Center and normalize coordinates
    float meanCoord = std::accumulate(sortedCoords.begin(), sortedCoords.end(), 0.0f) / numCells;
    std::vector<float> centeredCoords(numCells);
    float normCoord = 0.0f;

    for (int i = 0; i < numCells; ++i) {
        float c = sortedCoords[i] - meanCoord;
        centeredCoords[i] = c;
        normCoord += c * c;
    }

    // Prepare gene indices
    QVector<int> geneIndices(numGenes);
    std::iota(geneIndices.begin(), geneIndices.end(), 0);

    // Correlation type
    QString corrType = _settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction().getCurrentText();

    auto computeCorrelation = [&](int geneIdx) -> float {
        float meanGene = 0.0f;
        std::vector<float> geneVals(numCells);

        for (int i = 0; i < numCells; ++i) {
            int rowIdx = sortedCellIndices[i];
            float val = dataset->getValueAt(rowIdx * numGenes + geneIdx);
            geneVals[i] = val;
            meanGene += val;
        }
        meanGene /= numCells;

        float normGene = 0.0f;
        float dotProd = 0.0f;
        for (int i = 0; i < numCells; ++i) {
            float centered = geneVals[i] - meanGene;
            normGene += centered * centered;
            dotProd += centered * centeredCoords[i];
        }

        float corr = (normGene > 0.0f && normCoord > 0.0f)
            ? (dotProd / std::sqrt(normGene * normCoord))
            : 0.0f;

        if (std::isnan(corr)) corr = 0.0f;

        // Apply correlation type
        if (corrType == "Absolute") {
            return std::abs(corr);
        }
        else if (corrType == "Positive") {
            return (corr > 0.0f) ? corr : 0.0f;
        }
        else if (corrType == "Negative") {
            return (corr < 0.0f) ? corr : 0.0f;
        }
        else {
            return corr;
        }
        };

    // Parallel map
    QFuture<float> future = QtConcurrent::mapped(geneIndices, computeCorrelation);
    QList<float> resultList = future.results();
    corrVector = std::vector<float>(resultList.begin(), resultList.end());

    //qDebug() << "Correlation computation completed";
}

void Computation::computeCorrelationGeneCoords(const std::vector<int>& cellIndices, const mv::Dataset<Points> dataset, const std::vector<float>& coords, std::vector<float>& corrVector)
{
    FunctionTimer timer(Q_FUNC_INFO);
    const int numGenes = dataset->getNumDimensions();
    const int numCells = cellIndices.size();
    const int numCoords = coords.size();
    //qDebug() << "Number of genes:" << numGenes << "Number of cells:" << numCells << "Number of coordinates:" << numCoords;
    assert(numCells == numCoords);


    // sort filteredSelectionIndices based on yCoordinates
    std::vector<std::pair<float, int>> coordIndexPairs;
    for (size_t i = 0; i < cellIndices.size(); ++i) {
        coordIndexPairs.emplace_back(coords[i], cellIndices[i]);
    }

    // Sort by coordinate (first of the pair)
    std::sort(coordIndexPairs.begin(), coordIndexPairs.end());

    // Rebuild sorted vectors
    std::vector<int> sortedCellIndices;
    std::vector<float> sortedCoords;
    sortedCellIndices.reserve(cellIndices.size());
    sortedCoords.reserve(coords.size());

    for (const auto& pair : coordIndexPairs) {
        sortedCoords.push_back(pair.first);
        sortedCellIndices.push_back(pair.second);
    }


    // start compute corr
    float meanCoord = 0.0f;
    for (float val : sortedCoords)
        meanCoord += val;
    meanCoord /= numCells;

    std::vector<float> centeredCoords(numCells);
    float normCoord = 0.0f;
    for (int i = 0; i < numCells; ++i) {
        float c = sortedCoords[i] - meanCoord;
        centeredCoords[i] = c;
        normCoord += c * c;
    }

    corrVector.clear();
    corrVector.resize(numGenes);

#pragma omp parallel for
    for (int geneIdx = 0; geneIdx < numGenes; ++geneIdx) {
        std::vector<float> geneVals(numCells);
        float meanGene = 0.0f;

        for (int i = 0; i < numCells; ++i) {
            int rowIdx = sortedCellIndices[i];
            float val = dataset->getValueAt(rowIdx * numGenes + geneIdx);
            geneVals[i] = val;
            meanGene += val;
        }
        meanGene /= numCells;

        float normGene = 0.0f;
        float dotProd = 0.0f;
        for (int i = 0; i < numCells; ++i) {
            float centered = geneVals[i] - meanGene;
            normGene += centered * centered;
            dotProd += centered * centeredCoords[i];
        }

        float corr = (normGene > 0.0f && normCoord > 0.0f)
            ? (dotProd / std::sqrt(normGene * normCoord))
            : 0.0f;

        if (std::isnan(corr)) corr = 0.0f;

        
        QString corrType = _settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction().getCurrentText();
        if (corrType == "Absolute") {
            corrVector[geneIdx] = std::abs(corr);
        }
        else if (corrType == "Positive") {
            corrVector[geneIdx] = (corr > 0.0f) ? corr : 0.0f;
        }
        else if (corrType == "Negative") {
            corrVector[geneIdx] = (corr < 0.0f) ? corr : 0.0f;
        }
        else { // "Raw" or fallback
            corrVector[geneIdx] = corr;
        }
        
    }

    //qDebug() << "Correlation computation completed";
}

void Computation::computeCorrelationGenePCFast(const mv::Dataset<Points>& fullDataset, const std::vector<float>& selectedPC, std::vector<float>& corrVector)
{
    FunctionTimer timer(Q_FUNC_INFO);
    const int numGenes = fullDataset->getNumDimensions();
    const int numCells = fullDataset->getNumPoints();
    const int numCellsInPC = static_cast<int>(selectedPC.size());
    Q_ASSERT(numCells == numCellsInPC);

    float meanCoord = std::accumulate(selectedPC.begin(), selectedPC.end(), 0.0f) / numCells;

    std::vector<float> centeredCoords(numCells);
    float normCoord = 0.0f;
    for (int i = 0; i < numCells; ++i) {
        float c = selectedPC[i] - meanCoord;
        centeredCoords[i] = c;
        normCoord += c * c;
    }

    QVector<int> geneIndices(numGenes);
    std::iota(geneIndices.begin(), geneIndices.end(), 0);

    auto computeCorrelation = [&](int geneIdx) -> float {
        std::vector<float> geneVals(numCells);
        fullDataset->extractDataForDimension(geneVals, geneIdx);
        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked())
        {
            NormalizationType normType;
            QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
            if (normTypeStr == "ZScore") {
                normType = NormalizationType::ZScore;
            }
            else if (normTypeStr == "MinMax") {
                normType = NormalizationType::MinMax;
            }
            else if (normTypeStr == "Log2") {
                normType = NormalizationType::Log2;
            }
            else {
                normType = NormalizationType::None; // Default case
            }

            // Normalize gene values if normalization is enabled

            normalizeVector(geneVals, normType);
        }
        float sumGene = std::accumulate(geneVals.begin(), geneVals.end(), 0.0f);
        float meanGene = sumGene / numCells;

        float normGene = 0.0f;
        float dotProd = 0.0f;
        for (int i = 0; i < numCells; ++i) {
            float centered = geneVals[i] - meanGene;
            normGene += centered * centered;
            dotProd += centered * centeredCoords[i];
        }

        float corr = (normGene > 0.0f && normCoord > 0.0f)
            ? (dotProd / std::sqrt(normGene * normCoord))
            : 0.0f;

        return std::isnan(corr) ? 0.0f : corr;
        };

    // Blocking parallel computation of correlations
    QFuture<float> future = QtConcurrent::mapped(geneIndices, computeCorrelation);
    QList<float> resultList = future.results();
    corrVector = std::vector<float>(resultList.begin(), resultList.end());

}



void Computation::computeCorrelationGenePC(const mv::Dataset<Points>& fullDataset, const std::vector<float>& selectedPC, std::vector<float>& corrVector)
{
    FunctionTimer timer(Q_FUNC_INFO);
    // check
    const int numGenes = fullDataset->getNumDimensions();
    const int numCells = fullDataset->getNumPoints();
    const int numCellsInPC = selectedPC.size();
    //qDebug() << "Number of genes:" << numGenes << "Number of cells:" << numCells << "Number of cells in PC:" << numCellsInPC;
    assert(numCells == numCellsInPC);

    float meanCoord = 0.0f;
    for (float val : selectedPC)
        meanCoord += val;
    meanCoord /= numCells;

    std::vector<float> centeredCoords(numCells);
    float normCoord = 0.0f;
    for (int i = 0; i < numCells; ++i) {
        float c = selectedPC[i] - meanCoord;
        centeredCoords[i] = c;
        normCoord += c * c;
    }

    corrVector.clear();
    corrVector.resize(numGenes);

    for (int geneIdx = 0; geneIdx < numGenes; ++geneIdx) {
        std::vector<float> geneVals(numCells);

        fullDataset->extractDataForDimension(geneVals, geneIdx);
        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked())
        {
            NormalizationType normType;
            QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
            if (normTypeStr == "ZScore") {
                normType = NormalizationType::ZScore;
            }
            else if (normTypeStr == "MinMax") {
                normType = NormalizationType::MinMax;
            }
            else if (normTypeStr == "Log2") {
                normType = NormalizationType::Log2;
            }
            else {
                normType = NormalizationType::None; // Default case
            }
            
            normalizeVector(geneVals, normType);
        }
        float sumGene = 0.0f;
        for (float v : geneVals) sumGene += v;
        float meanGene = sumGene / numCells;

        float normGene = 0.0f;
        float dotProd = 0.0f;
        for (int i = 0; i < numCells; ++i) {
            float centered = geneVals[i] - meanGene;
            normGene += centered * centered;
            dotProd += centered * centeredCoords[i];
        }

        float corr = (normGene > 0.0f && normCoord > 0.0f)
            ? (dotProd / std::sqrt(normGene * normCoord))
            : 0.0f;

        if (std::isnan(corr)) corr = 0.0f;

        corrVector[geneIdx] = corr;
    }

    //qDebug() << "Correlation between gene and selected PC completed";
}


void Computation::setStatusMessage(const QString& status, const QString& message, const QString& color) {
    QString resolvedColor = color;
    if (resolvedColor.isEmpty()) {
        if (status == "C") resolvedColor = "#28a745";      // Green
        else if (status == "M") resolvedColor = "#ffc107"; // Gold
        else if (status == "E") resolvedColor = "#dc3545"; // Red
        else if (status == "R") resolvedColor = "#007bff"; // Blue
        else resolvedColor = "#6c757d";                    // Grey
    }
    QJsonObject obj;
    obj["status"] = status;
    obj["message"] = message;
    obj["color"] = resolvedColor;
    _settingsAction.getStatusHolder().getStatusMessageAction().setString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

//Validate and retrieve datasets and indices
bool Computation::validateAndRetrieveDatasets(mv::Dataset<Points>& fullPointDataset, mv::Dataset<Clusters>& fullClusterDataset, int& xEmbeddingIndex, int& yEmbeddingIndex, std::vector<int>& selectedIndices, QString& scatterplotViewName, float& minCorrelation, float& maxCorrelation)
{

    fullPointDataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    fullClusterDataset = mv::data().getDataset<Clusters>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());

    if (!_embeddingDataset.isValid() || !fullPointDataset.isValid() || !fullClusterDataset.isValid()) {
        qWarning() << "Invalid datasets";
        setStatusMessage("E", "Error occured during dataset creation", "");
        return false;
    }

    xEmbeddingIndex = _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionIndex();
    yEmbeddingIndex = _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionIndex();
    if (xEmbeddingIndex == -1 || yEmbeddingIndex == -1) {
        qWarning() << "No embedding dimensions selected";
        setStatusMessage("E", "No embedding dataset dimensions selected", "");
        return false;
    }

    // Convert selection indices to std::vector<int>
    {
        auto indices = _embeddingDataset->getSelectionIndices();
        selectedIndices.assign(indices.begin(), indices.end());
    }
    if (selectedIndices.empty()) {
        qWarning() << "No selected indices";
        setStatusMessage("E", "No indices selected", "");
        return false;
    }

    auto selectedClusterValues = _settingsAction.getCommonOptionsHolder().getClusterSelectionAction().getSelectedOptions();
    if (selectedClusterValues.isEmpty()) {
        qWarning() << "No selected cluster values";
        setStatusMessage("E", "No clusters selected", "");
        return false;
    }

    scatterplotViewName = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
    if (scatterplotViewName.isEmpty()) {
        qWarning() << "No scatterplot view selected";
        setStatusMessage("E", "No scatterplot view selected", "");
        return false;
    }

    minCorrelation = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMinimum();
    maxCorrelation = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMaximum();
    if (minCorrelation < -1.0f || maxCorrelation > 1.0f || minCorrelation >= maxCorrelation) {
        qWarning() << "Invalid correlation range";
        setStatusMessage("E", "Invalid correlation range", "");
        return false;
    }
    qWarning() << "Selected indices size:" << selectedIndices.size() << "xEmbeddingIndex:" << xEmbeddingIndex << "yEmbeddingIndex:" << yEmbeddingIndex
        << "scatterplotViewName:" << scatterplotViewName << "minCorrelation:" << minCorrelation << "maxCorrelation:" << maxCorrelation;


    return true;
}

// Create datasets if needed
bool Computation::createDatasetsIfNeeded()
{
    if (!_selectedGenePointDataset1.isValid()) {
        _selectedGenePointDataset1 = mv::data().createDataset("Points", "Selected Gene Point Data 1");
        _selectedGenePointDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedGenePointDataset1);
    }

    if (!_selectedGenePointDataset2.isValid()) {
        _selectedGenePointDataset2 = mv::data().createDataset("Points", "Selected Gene Point Data 2");
        _selectedGenePointDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedGenePointDataset2);
    }
    
    if (!_selectedCellPointDataset.isValid()) {
        _selectedCellPointDataset = mv::data().createDataset("Points", "Selected Cell Point Data");
        _selectedCellPointDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_selectedCellPointDataset);
    }
    if (!_rotatedSelectedEmbeddingDataset.isValid()) {
        _rotatedSelectedEmbeddingDataset = mv::data().createDataset("Points", "Selected Rotated Embedding Data", _selectedCellPointDataset);
        _rotatedSelectedEmbeddingDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_rotatedSelectedEmbeddingDataset);
    }
    if (!_lineChartPointDataset1.isValid()) {
        _lineChartPointDataset1 = mv::data().createDataset("Points", "Line Chart Point Data 1");
        _lineChartPointDataset1->setGroupIndex(670);
        mv::events().notifyDatasetAdded(_lineChartPointDataset1);
    }
    if(!_lineChartClusterDataset1.isValid()) {
        _lineChartClusterDataset1 = mv::data().createDataset("Cluster", "Line Chart Cluster Data 1", _lineChartPointDataset1);
        _lineChartClusterDataset1->setGroupIndex(670);
        mv::events().notifyDatasetAdded(_lineChartClusterDataset1);
    }

    if (!_selectedEmbeddingDataset.isValid()) {
        _selectedEmbeddingDataset = mv::data().createDataset("Points", "Selected Embedding Data", _selectedCellPointDataset);
        _selectedEmbeddingDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_selectedEmbeddingDataset);
    }

    if (!_rotatedSelectedEmbeddingDataset.isValid() || !_selectedGenePointDataset1.isValid() || !_lineChartPointDataset1.isValid()||!_lineChartClusterDataset1.isValid() ||!_selectedGenePointDataset2.isValid() || !_selectedEmbeddingDataset.isValid() || !_selectedCellPointDataset.isValid()) {
        qWarning() << "Failed to create datasets";
        setStatusMessage("E", "Error occurred during dataset creation", "");
        return false;
    }

    return true;
}

void Computation::populateGenesByCorrelationRange(const std::vector<float>& correlationVector, float minCorr, float maxCorr, const std::vector<QString>& geneNameOrdered, int datasetNumber)
{
    constexpr float epsilon = 1e-6f;
    const int count = std::min(static_cast<int>(correlationVector.size()), static_cast<int>(geneNameOrdered.size()));
    if (count == 0) return;

    // Prepare vector of indices for sorting
    std::vector<int> indices(count);
    std::iota(indices.begin(), indices.end(), 0);

    // Sort indices by descending correlation value
    std::sort(indices.begin(), indices.end(),
        [&correlationVector](int a, int b) {
            return correlationVector[a] > correlationVector[b];
        });

    // Select appropriate output containers
    auto& geneCorrValues = (datasetNumber == 2) ? _geneCorrValues2
        : (datasetNumber == 1) ? _geneCorrValues1
        : _geneCorrValues3;
    auto& geneOfInterestIndices = (datasetNumber == 2) ? _geneOfInterestIndices2 : (datasetNumber == 1)?_geneOfInterestIndices1: _geneOfInterestIndices3;

    geneCorrValues.clear();
    geneCorrValues.reserve(count);
    geneOfInterestIndices.clear();
    geneOfInterestIndices.reserve(count);

    // Assign ranks and fill output in a single loop
    int rank = 1;
    float prevCorr = std::numeric_limits<float>::quiet_NaN();
    for (int i = 0; i < count; ++i) {
        int idx = indices[i];
        float corr = correlationVector[idx];
        // Mark as within range if in [minCorr, maxCorr] (inclusive, with epsilon)
        bool isWithinRange = (corr <= minCorr - epsilon) || (corr >= maxCorr + epsilon);

        // Dense ranking: same value gets same rank
        if (i == 0 || std::abs(corr - prevCorr) > epsilon) {
            rank = i + 1;
            prevCorr = corr;
        }

        geneCorrValues.emplace_back(CorrelationGeneCoords{
            idx,
            geneNameOrdered[idx],
            corr,
            rank,
            isWithinRange
            });
        if (isWithinRange) {
            geneOfInterestIndices.push_back(idx);
        }
    }
}

void Computation::updateClusterOptions()
{
    // Use QSet for efficient intersection and avoid unnecessary sorting
    QSet<QString> clusterNames1, clusterNames2;

    // Collect cluster names for dataset 1
    if (_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().isValid())
    {
        auto clusterFullDataset1 = mv::data().getDataset<Clusters>(
            _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
        const auto clusters1 = clusterFullDataset1->getClusters();
        for (const auto& cluster : clusters1)
            clusterNames1.insert(cluster.getName());
    }

    // Collect cluster names for dataset 2
    if (_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().isValid())
    {
        auto clusterFullDataset2 = mv::data().getDataset<Clusters>(
            _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
        const auto clusters2 = clusterFullDataset2->getClusters();
        for (const auto& cluster : clusters2)
            clusterNames2.insert(cluster.getName());
    }

    // Compute intersection and sort result
    QStringList clusterNamesCommon = (clusterNames1 & clusterNames2).values();
    // FIX: Use a lambda for case-insensitive sorting instead of Qt::CaseInsensitive enum
    std::sort(clusterNamesCommon.begin(), clusterNamesCommon.end(),
        [](const QString& a, const QString& b) {
            return a.compare(b, Qt::CaseInsensitive) < 0;
        });

    // Update options and selection
    auto& clusterSelectionAction = _settingsAction.getCommonOptionsHolder().getClusterSelectionAction();
    clusterSelectionAction.setOptions(clusterNamesCommon);
    clusterSelectionAction.setSelectedOptions(clusterNamesCommon);
}

void Computation::clearTable(int tableNumber)
{
    if (tableNumber == 1)
    {
        QStandardItemModel* model1 = qobject_cast<QStandardItemModel*>(_settingsAction.getTableView1Action()->model());
        if (model1) {
            model1->clear();

            //model1->setHorizontalHeaderLabels(QStringList() << "Gene" << "Correlation" << "Rank");
        }
    }
    else if (tableNumber == 2)
    {
        QStandardItemModel* model2 = qobject_cast<QStandardItemModel*>(_settingsAction.getTableView2Action()->model());
        if (model2) {
            model2->clear();
            //model2->setHorizontalHeaderLabels(QStringList() << "Gene" << "Correlation" << "Rank");
        }
    }
    else if (tableNumber == 3)
    {
        QStandardItemModel* model3 = qobject_cast<QStandardItemModel*>(_settingsAction.getTableView3Action()->model());
        if (model3) {
            model3->clear();
            //model3->setHorizontalHeaderLabels(QStringList() << "Gene" << "Correlation" << "Rank");
        }
    }
    else
    {
        qWarning() << "Invalid table number for clearing";
    }

}

void Computation::processAlternativeToPCA()
{
    FunctionTimer timer(Q_FUNC_INFO);

    auto fullPointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto fullPointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId());

    if (!_selectedGenePointDataset1.isValid() || !fullPointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() || !fullPointDataset2.isValid()) {
        qWarning() << "One or more datasets are invalid!";
        setStatusMessage("E", "Internal error: invalid dataset pointer", "");
        return;
    }

    const int numAllCells2 = fullPointDataset2->getNumPoints();
    const int numAllCells1 = fullPointDataset1->getNumPoints();
    const int numOfIntGenes = static_cast<int>(_geneOfInterestIndices1.size());

    const auto& dimensionNames1 = fullPointDataset1->getDimensionNames();
    const auto& dimensionNames2 = fullPointDataset2->getDimensionNames();

    if (numAllCells2 <= 0 || numOfIntGenes <= 0) {
        qWarning() << "Invalid dataset sizes: numAllCells1:" << numAllCells1 << "numAllCells2:" << numAllCells2 << "numOfIntGenes:" << numOfIntGenes;
        setStatusMessage("E", "Invalid dataset sizes", "");
        return;
    }
    if (numOfIntGenes > static_cast<int>(dimensionNames1.size()) ||
        numOfIntGenes > static_cast<int>(dimensionNames2.size())) {
        qWarning() << "Number of genes of interest exceeds dataset dimensions";
        setStatusMessage("E", "Number of genes of interest exceeds dataset dimensions", "");
        return;
    }

    // Build a hash for fast correlation lookup
    QHash<QString, float> corrLookup;
    for (const auto& gene : _geneCorrValues1)
        corrLookup.insert(gene.geneName, gene.correlationValue);

    std::vector<QString> dimensionNamesOfInterest;
    std::vector<int> geneDataset1, geneDataset2;
    std::vector<float> corrValuesForGeneOfInterest;
    dimensionNamesOfInterest.reserve(numOfIntGenes);
    geneDataset1.reserve(numOfIntGenes);
    geneDataset2.reserve(numOfIntGenes);
    corrValuesForGeneOfInterest.reserve(numOfIntGenes);

    for (int idx : _geneOfInterestIndices1) {
        if (idx < 0 || idx >= static_cast<int>(dimensionNames1.size()))
            continue;
        const QString& geneName = dimensionNames1[idx];
        auto it2 = std::find(dimensionNames2.begin(), dimensionNames2.end(), geneName);
        if (it2 != dimensionNames2.end()) {
            int geneId2 = static_cast<int>(std::distance(dimensionNames2.begin(), it2));
            dimensionNamesOfInterest.emplace_back(geneName);
            geneDataset1.emplace_back(idx);
            geneDataset2.emplace_back(geneId2);
            corrValuesForGeneOfInterest.emplace_back(corrLookup.value(geneName, 0.0f));
        }
    }

    const int numDimsOfInterest = static_cast<int>(dimensionNamesOfInterest.size());
    if (numDimsOfInterest <= 0) {
        qWarning() << "No genes of interest selected for PCA";
        setStatusMessage("E", "No genes of interest selected for PCA", "");
        return;
    }
    if (numDimsOfInterest > numOfIntGenes) {
        qWarning() << "Number of dimensions of interest exceeds number of genes of interest";
        setStatusMessage("E", "Number of dimensions of interest exceeds number of genes of interest", "");
        return;
    }
    if (dimensionNamesOfInterest.size() != geneDataset1.size() || dimensionNamesOfInterest.size() != geneDataset2.size()) {
        qWarning() << "Dimension names size mismatch with gene datasets";
        setStatusMessage("E", "Dimension names size mismatch with gene datasets", "");
        return;
    }

    std::vector<int> allCellIndices1(numAllCells1), allCellIndices2(numAllCells2);
    std::iota(allCellIndices1.begin(), allCellIndices1.end(), 0);
    std::iota(allCellIndices2.begin(), allCellIndices2.end(), 0);

    if (allCellIndices1.empty() || allCellIndices2.empty()) {
        qWarning() << "No valid cell indices found for datasets";
        setStatusMessage("E", "No valid cell indices found for datasets", "");
        return;
    }

    std::vector<float> cellByGeneMatrix2(numAllCells2 * numDimsOfInterest);
    std::vector<float> cellByGeneMatrix1(numAllCells1 * numDimsOfInterest);

    try {
        fullPointDataset2->populateDataForDimensions(cellByGeneMatrix2, geneDataset2, allCellIndices2);
        fullPointDataset1->populateDataForDimensions(cellByGeneMatrix1, geneDataset1, allCellIndices1);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return;
    }

    if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked())
    {
        NormalizationType normType;
        QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
        if (normTypeStr == "ZScore") {
            normType = NormalizationType::ZScore;
        }
        else if (normTypeStr == "MinMax") {
            normType = NormalizationType::MinMax;
        }
        else if (normTypeStr == "Log2") {
            normType = NormalizationType::Log2;
        }
        else {
            normType = NormalizationType::None; // Default case
        }
        normalizeVector(cellByGeneMatrix2, normType);
        normalizeVector(cellByGeneMatrix1, normType);
    }


    _selectedGenePointDataset2->setData(cellByGeneMatrix2.data(), numAllCells2, numDimsOfInterest);
    _selectedGenePointDataset2->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset2);

    _selectedGenePointDataset1->setData(cellByGeneMatrix1.data(), numAllCells1, numDimsOfInterest);
    _selectedGenePointDataset1->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset1);

    std::vector<float> gradientScore1(numAllCells1, 0.0f), gradientScore2(numAllCells2, 0.0f);

    // Compute gradient scores for dataset 1
    for (int i = 0; i < numAllCells1; ++i) {
        const float* rowPtr = cellByGeneMatrix1.data() + i * numDimsOfInterest;
        std::vector<float> rowData1(rowPtr, rowPtr + numDimsOfInterest);
        if (rowData1.size() != numDimsOfInterest) {
            qInfo() << "Row data size mismatch for PCA computation at index" << i << "Expected:" << numDimsOfInterest
                << "Got rowData1:" << rowData1.size();
            qWarning() << "Row data size mismatch for PCA computation at index" << i;
            setStatusMessage("E", "Row data size mismatch for PCA computation", "");
            return;
        }
        gradientScore1[i] = computeGradientScore(rowData1, corrValuesForGeneOfInterest);
    }
    // Compute gradient scores for dataset 2
    for (int i = 0; i < numAllCells2; ++i) {
        const float* rowPtr = cellByGeneMatrix2.data() + i * numDimsOfInterest;
        std::vector<float> rowData2(rowPtr, rowPtr + numDimsOfInterest);
        if (rowData2.size() != numDimsOfInterest) {
            qInfo() << "Row data size mismatch for PCA computation at index" << i << "Expected:" << numDimsOfInterest
                << "rowData2:" << rowData2.size();
            qWarning() << "Row data size mismatch for PCA computation at index" << i;
            setStatusMessage("E", "Row data size mismatch for PCA computation", "");
            return;
        }
        gradientScore2[i] = computeGradientScore(rowData2, corrValuesForGeneOfInterest);
    }

    // Ensure PCA common gene datasets exist
    if (!_pcaCommonGenesDataset1.isValid()) {
        _pcaCommonGenesDataset1 = mv::data().createDataset("Points", "PCA Dataset 1", _selectedGenePointDataset1);
        _pcaCommonGenesDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset1);
    }
    if (!_pcaCommonGenesDataset2.isValid()) {
        _pcaCommonGenesDataset2 = mv::data().createDataset("Points", "PCA Dataset 2", _selectedGenePointDataset2);
        _pcaCommonGenesDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset2);
    }

    _pcaCommonGenesDataset1->setData(gradientScore1.data(), numAllCells1, 1);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset1);
    _pcaCommonGenesDataset2->setData(gradientScore2.data(), numAllCells2, 1);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset2);

    _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setPointsDataset(_pcaCommonGenesDataset2);

    int numDimsForColoring = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getNumberOfDimensions();
    if (numDimsForColoring > 0) {
        _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setCurrentDimensionIndex(0);
    }
    else if (numDimsForColoring < 0) {
        qWarning() << "No PCA dimensions available for coloring";
        setStatusMessage("E", "No PCA dimensions available for coloring", "");
        clearTable(3);
        clearTable(2);
        return;
    }

    if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked()) {
        correlationAfterPCA();
    }
}

void Computation::processAfterPCACompleteAlt()

{
    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("C", "PCA analysis completed", "#28a745");
    qDebug() << "PCA analysis completed successfully";

    auto& scores1 = _pca1.scores();
    auto& loadings1 = _pca1.loadings();
    const int nRows = _pca1.numRows();
    const int nDims = _pca1.numDims();
    const int nComps = _pca1.numComponents();

    if (!_selectedGenePointDataset1.isValid() || !_selectedGenePointDataset2.isValid()) {
        qWarning() << "Selected gene point datasets are invalid";
        setStatusMessage("E", "Selected gene point datasets are invalid", "");
        return;
    }

    Dataset<Points> fullDataset2 = _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset();
    Dataset<Points> fullDataset1 = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset();
    if (!fullDataset1.isValid() || !fullDataset2.isValid()) {
        qWarning() << "Full datasets are invalid";
        setStatusMessage("E", "Full datasets are invalid", "");
        return;
    }

    // Ensure PCA common gene datasets exist
    if (!_pcaCommonGenesDataset1.isValid()) {
        _pcaCommonGenesDataset1 = mv::data().createDataset("Points", "PCA Dataset 1", _selectedGenePointDataset1);
        _pcaCommonGenesDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset1);
    }
    if (!_pcaCommonGenesDataset2.isValid()) {
        _pcaCommonGenesDataset2 = mv::data().createDataset("Points", "PCA Dataset 2", _selectedGenePointDataset2);
        _pcaCommonGenesDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset2);
    }

    setStatusMessage("R", "Computation after PCA", "");

    if (!_selectedGenePointDataset2.isValid()) {
        qWarning() << "Selected gene point dataset 2 is invalid";
        setStatusMessage("E", "Selected gene point dataset 2 is invalid", "");
        return;
    }
    if (_selectedGenePointDataset2->getNumDimensions() != nDims) {
        qWarning() << "Selected gene point dataset 2 dimensions do not match PCA dimensions";
        setStatusMessage("E", "Selected gene point dataset 2 dimensions do not match PCA dimensions", "");
        return;
    }

    if (loadings1.empty()) {
        qWarning() << "PCA loadings are empty";
        setStatusMessage("E", "PCA loadings are empty", "");
        return;
    }
    if (nDims != nComps) {
        qWarning() << "Loadings matrix is not square: nDims =" << nDims << ", nComps =" << nComps;
        setStatusMessage("E", "Loadings matrix is not square", "");
        return;
    }
    if (static_cast<int>(loadings1.size()) != nDims * nComps) {
        qWarning() << "Loadings size mismatch: loadings1.size() =" << loadings1.size() << ", expected =" << nDims * nComps;
        setStatusMessage("E", "Loadings size mismatch", "");
        return;
    }

    if (_geneOfInterestIndices1.empty()) {
        qWarning() << "Gene of interest indices 1 are empty";
        setStatusMessage("E", "Gene of interest indices 1 are empty", "");
        return;
    }

    // Build geneOfInterestIndices2 and commonGeneNames in a single pass
    std::vector<int> geneOfInterestIndices2;
    QStringList commonGeneNames;
    const auto& genedims1 = fullDataset1->getDimensionNames();
    const auto& genedims2 = fullDataset2->getDimensionNames();
    geneOfInterestIndices2.reserve(_geneOfInterestIndices1.size());
    commonGeneNames.reserve(_geneOfInterestIndices1.size());

    for (int inddx : _geneOfInterestIndices1) {
        if (inddx < 0 || inddx >= genedims1.size()) {
            qWarning() << "Invalid index in _geneOfInterestIndices1:" << inddx;
            setStatusMessage("E", "Invalid index in gene of interest indices 1", "");
            return;
        }
        const QString& geneName = genedims1[inddx];
        auto it = std::find(genedims2.begin(), genedims2.end(), geneName);
        if (it != genedims2.end()) {
            geneOfInterestIndices2.push_back(static_cast<int>(std::distance(genedims2.begin(), it)));
            commonGeneNames.push_back(geneName);
        }
        else {
            qWarning() << "Gene name not found in dataset 2:" << geneName;
            setStatusMessage("E", "Gene name not found in dataset 2", "");
            return;
        }
    }

    // Consistency checks
    const int nCommon = static_cast<int>(_geneOfInterestIndices1.size());
    if (geneOfInterestIndices2.size() != nCommon || commonGeneNames.size() != nCommon) {
        qWarning() << "Gene indices or common gene names size mismatch";
        setStatusMessage("E", "Gene indices or common gene names size mismatch", "");
        return;
    }
    if (nCommon != nDims) {
        qWarning() << "Gene indices size does not match nDims: indices1 =" << nCommon << ", nDims =" << nDims;
        setStatusMessage("E", "Gene indices size does not match PCA dimensions", "");
        return;
    }

    // Prepare loadings map
    struct LoadingContainer {
        int geneId1;
        int geneId2;
        QString geneName;
        QVector<float> loadings;
    };
    QVector<LoadingContainer> loadingsMap;
    loadingsMap.reserve(nDims);
    for (int id = 0; id < nDims; ++id) {
        loadingsMap.push_back({
            _geneOfInterestIndices1[id],
            geneOfInterestIndices2[id],
            commonGeneNames[id],
            QVector<float>(loadings1.begin() + id * nComps, loadings1.begin() + (id + 1) * nComps)
            });
    }

    // Preallocate output arrays
    const int numPoints1 = _selectedGenePointDataset1->getNumPoints();
    const int numPoints2 = _selectedGenePointDataset2->getNumPoints();
    // Prepare output: total contribution per cell per gene
    std::vector<float> totalContribution1(numPoints1* nDims, 0.0f);
    std::vector<float> totalContribution2(numPoints2* nDims, 0.0f);

    for (int geneitr = 0; geneitr < nDims; ++geneitr) {
        const auto& loadingsVals = loadingsMap[geneitr];
        std::vector<float> genePointVector1(numPoints1, 0.0f);
        std::vector<float> genePointVector2(numPoints2, 0.0f);
        _selectedGenePointDataset1->extractDataForDimension(genePointVector1, geneitr);
        _selectedGenePointDataset2->extractDataForDimension(genePointVector2, geneitr);
        NormalizationType normType;
        QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();

        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked())
        {
            /*if (normTypeStr == "ZScore") {
                normType = NormalizationType::ZScore;
            }
            else if (normTypeStr == "MinMax") {
                normType = NormalizationType::MinMax;
            }
            else if (normTypeStr == "Log2") {
                normType = NormalizationType::Log2;
            }
            else {
                normType = NormalizationType::None; // Default case
            }
            normalizeVector(genePointVector1, normType);
            normalizeVector(genePointVector2, normType);*/
        }

        const QVector<float>& loadings = loadingsVals.loadings;
        for (int i = 0; i < numPoints1; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < nDims; ++j) {
                sum += genePointVector1[i] * loadings[j];
            }
            totalContribution1[i * nDims + geneitr] = sum;
        }
        for (int i = 0; i < numPoints2; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < nDims; ++j) {
                sum += genePointVector2[i] * loadings[j];
            }
            totalContribution2[i * nDims + geneitr] = sum;
        }
    }

    qInfo() << "After conversion stats - Dataset 1 Min:" << *std::min_element(totalContribution1.begin(), totalContribution1.end())
        << "Max:" << *std::max_element(totalContribution1.begin(), totalContribution1.end())
        << "| Dataset 2 Min:" << *std::min_element(totalContribution2.begin(), totalContribution2.end())
        << "Max:" << *std::max_element(totalContribution2.begin(), totalContribution2.end());

    _pcaCommonGenesDataset1->setData(totalContribution1.data(), numPoints1, nDims);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset1);
    _pcaCommonGenesDataset2->setData(totalContribution2.data(), numPoints2, nDims);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset2);

    _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setPointsDataset(_pcaCommonGenesDataset2);

    int numDimsForColoring = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getNumberOfDimensions();
    if (numDimsForColoring > 0) {
        _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setCurrentDimensionIndex(0);
    }
    else if (numDimsForColoring < 0) {
        qWarning() << "No PCA dimensions available for coloring";
        setStatusMessage("E", "No PCA dimensions available for coloring", "");
        clearTable(3);
        clearTable(2);
        return;
    }

    if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked()) {
        correlationAfterPCA();
    }
}


void Computation::processAfterPCAComplete()
{
    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("C", "PCA analysis completed", "#28a745");
    qDebug() << "PCA analysis completed successfully";

    auto& scores1 = _pca1.scores();
    auto& loadings1 = _pca1.loadings();
    const int nRows = _pca1.numRows();
    const int nDims = _pca1.numDims();
    const int nComps = _pca1.numComponents();

    if (!_selectedGenePointDataset1.isValid() || !_selectedGenePointDataset2.isValid()) {
        qWarning() << "Selected gene point datasets are invalid";
        setStatusMessage("E", "Selected gene point datasets are invalid", "");
        return;
    }

    Dataset<Points> fullDataset2 = _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset();
    Dataset<Points> fullDataset1 = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset();
    if (!fullDataset1.isValid() || !fullDataset2.isValid()) {
        qWarning() << "Full datasets are invalid";
        setStatusMessage("E", "Full datasets are invalid", "");
        return;
    }

    // Ensure PCA common gene datasets exist
    if (!_pcaCommonGenesDataset1.isValid()) {
        _pcaCommonGenesDataset1 = mv::data().createDataset("Points", "PCA Dataset 1", _selectedGenePointDataset1);
        _pcaCommonGenesDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset1);
    }
    if (!_pcaCommonGenesDataset2.isValid()) {
        _pcaCommonGenesDataset2 = mv::data().createDataset("Points", "PCA Dataset 2", _selectedGenePointDataset2);
        _pcaCommonGenesDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset2);
    }

    setStatusMessage("R", "Computation after PCA", "");

    if (!_selectedGenePointDataset2.isValid()) {
        qWarning() << "Selected gene point dataset 2 is invalid";
        setStatusMessage("E", "Selected gene point dataset 2 is invalid", "");
        return;
    }
    if (_selectedGenePointDataset2->getNumDimensions() != nDims) {
        qWarning() << "Selected gene point dataset 2 dimensions do not match PCA dimensions";
        setStatusMessage("E", "Selected gene point dataset 2 dimensions do not match PCA dimensions", "");
        return;
    }

    if (loadings1.empty()) {
        qWarning() << "PCA loadings are empty";
        setStatusMessage("E", "PCA loadings are empty", "");
        return;
    }
    if (nDims != nComps) {
        qWarning() << "Loadings matrix is not square: nDims =" << nDims << ", nComps =" << nComps;
        setStatusMessage("E", "Loadings matrix is not square", "");
        return;
    }
    if (static_cast<int>(loadings1.size()) != nDims * nComps) {
        qWarning() << "Loadings size mismatch: loadings1.size() =" << loadings1.size() << ", expected =" << nDims * nComps;
        setStatusMessage("E", "Loadings size mismatch", "");
        return;
    }

    if (_geneOfInterestIndices1.empty()) {
        qWarning() << "Gene of interest indices 1 are empty";
        setStatusMessage("E", "Gene of interest indices 1 are empty", "");
        return;
    }

    // Build geneOfInterestIndices2 and commonGeneNames in a single pass
    std::vector<int> geneOfInterestIndices2;
    QStringList commonGeneNames;
    const auto& genedims1 = fullDataset1->getDimensionNames();
    const auto& genedims2 = fullDataset2->getDimensionNames();
    geneOfInterestIndices2.reserve(_geneOfInterestIndices1.size());
    commonGeneNames.reserve(_geneOfInterestIndices1.size());

    for (int inddx : _geneOfInterestIndices1) {
        if (inddx < 0 || inddx >= genedims1.size()) {
            qWarning() << "Invalid index in _geneOfInterestIndices1:" << inddx;
            setStatusMessage("E", "Invalid index in gene of interest indices 1", "");
            return;
        }
        const QString& geneName = genedims1[inddx];
        auto it = std::find(genedims2.begin(), genedims2.end(), geneName);
        if (it != genedims2.end()) {
            geneOfInterestIndices2.push_back(static_cast<int>(std::distance(genedims2.begin(), it)));
            commonGeneNames.push_back(geneName);
        }
        else {
            qWarning() << "Gene name not found in dataset 2:" << geneName;
            setStatusMessage("E", "Gene name not found in dataset 2", "");
            return;
        }
    }

    // Consistency checks
    const int nCommon = static_cast<int>(_geneOfInterestIndices1.size());
    if (geneOfInterestIndices2.size() != nCommon || commonGeneNames.size() != nCommon) {
        qWarning() << "Gene indices or common gene names size mismatch";
        setStatusMessage("E", "Gene indices or common gene names size mismatch", "");
        return;
    }
    if (nCommon != nDims) {
        qWarning() << "Gene indices size does not match nDims: indices1 =" << nCommon << ", nDims =" << nDims;
        setStatusMessage("E", "Gene indices size does not match PCA dimensions", "");
        return;
    }

    // Prepare loadings map
    struct LoadingContainer {
        int geneId1;
        int geneId2;
        QString geneName;
        QVector<float> loadings;
    };
    QVector<LoadingContainer> loadingsMap;
    loadingsMap.reserve(nDims);
    for (int id = 0; id < nDims; ++id) {
        loadingsMap.push_back({
            _geneOfInterestIndices1[id],
            geneOfInterestIndices2[id],
            commonGeneNames[id],
            QVector<float>(loadings1.begin() + id * nComps, loadings1.begin() + (id + 1) * nComps)
            });
    }

    // Preallocate output arrays
    const int numPoints1 = _selectedGenePointDataset1->getNumPoints();
    const int numPoints2 = _selectedGenePointDataset2->getNumPoints();
    std::vector<float> pcBestDataRowMajor1(numPoints1 * nDims, 0.0f);
    std::vector<float> pcBestDataRowMajor2(numPoints2 * nDims, 0.0f);

    // Compute dot products efficiently
    for (int geneitr = 0; geneitr < nDims; ++geneitr) {
        const auto& loadingsVals = loadingsMap[geneitr];
        std::vector<float> genePointVector1(numPoints1, 0.0f);
        std::vector<float> genePointVector2(numPoints2, 0.0f);
        _selectedGenePointDataset1->extractDataForDimension(genePointVector1, geneitr);
        _selectedGenePointDataset2->extractDataForDimension(genePointVector2, geneitr);
        
       
        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked()) 
        { 
            /*NormalizationType normType;
            QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
            if (normTypeStr == "ZScore") {
                normType = NormalizationType::ZScore;
            }
            else if (normTypeStr == "MinMax") {
                normType = NormalizationType::MinMax;
            }
            else if (normTypeStr == "Log2") {
                normType = NormalizationType::Log2;
            }
            else {
                normType = NormalizationType::None; // Default case
            }
            normalizeVector(genePointVector1, normType); 
            normalizeVector(genePointVector2, normType);*/
        }

        const QVector<float>& loadings = loadingsVals.loadings;
        for (int j = 0; j < nDims; ++j) {
            float loading = loadings[j];
            for (int i = 0; i < numPoints1; ++i) {
                pcBestDataRowMajor1[i * nDims + j] = genePointVector1[i] * loading;
            }
            for (int i = 0; i < numPoints2; ++i) {
                pcBestDataRowMajor2[i * nDims + j] = genePointVector2[i] * loading;
            }
        }
    }

    qInfo() << "After conversion stats - Dataset 1 Min:" << *std::min_element(pcBestDataRowMajor1.begin(), pcBestDataRowMajor1.end())
        << "Max:" << *std::max_element(pcBestDataRowMajor1.begin(), pcBestDataRowMajor1.end())
        << "| Dataset 2 Min:" << *std::min_element(pcBestDataRowMajor2.begin(), pcBestDataRowMajor2.end())
        << "Max:" << *std::max_element(pcBestDataRowMajor2.begin(), pcBestDataRowMajor2.end());

    _pcaCommonGenesDataset1->setData(pcBestDataRowMajor1.data(), numPoints1, nDims);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset1);
    _pcaCommonGenesDataset2->setData(pcBestDataRowMajor2.data(), numPoints2, nDims);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset2);

    _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setPointsDataset(_pcaCommonGenesDataset2);

    int numDimsForColoring = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getNumberOfDimensions();
    if (numDimsForColoring > 0) {
        _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setCurrentDimensionIndex(0);
    }
    else if (numDimsForColoring < 0) {
        qWarning() << "No PCA dimensions available for coloring";
        setStatusMessage("E", "No PCA dimensions available for coloring", "");
        clearTable(3);
        clearTable(2);
        return;
    }

    if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked()) {
        correlationAfterPCA();
    }
}

void Computation::correlationAfterPCA()
{
    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("R", "Running correlation between genes and PC", "#28a745");

    Dataset<Points> fullDataset2 = _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset();
    Dataset<Points> fullDataset1 = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset();
    if (!fullDataset2.isValid()) {
        setStatusMessage("E", "Point dataset 2 is invalid", "");
        return;
    }
    if (!fullDataset1.isValid()) {
        setStatusMessage("E", "Point dataset 1 is invalid", "");
        return;
    }
  

    int selectedPCDimension = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();


    if (selectedPCDimension < 0)
    {
        setStatusMessage("E", "No PCA dimension selected for dataset 2", "");
        return;
    }
    const int numAllCells2 = fullDataset2->getNumPoints();
    if (numAllCells2 <= 0) {
        setStatusMessage("E", "No cells in point dataset 2", "");
        return;
    }
    const int numAllCells1 = fullDataset1->getNumPoints();
    if (numAllCells1 <= 0) {
        setStatusMessage("E", "No cells in point dataset 1", "");
        return;
    }

    std::vector<float> selectedPC2(numAllCells2);
    _pcaCommonGenesDataset2->extractDataForDimension(selectedPC2, selectedPCDimension);

    std::vector<float> selectedPC1(numAllCells1);
    _pcaCommonGenesDataset1->extractDataForDimension(selectedPC1, selectedPCDimension);
    if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked()) 
    { 
        /*NormalizationType normType;
        QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
        if (normTypeStr == "ZScore") {
            normType = NormalizationType::ZScore;
        }
        else if (normTypeStr == "MinMax") {
            normType = NormalizationType::MinMax;
        }
        else if (normTypeStr == "Log2") {
            normType = NormalizationType::Log2;
        }
        else {
            normType = NormalizationType::None; // Default case
        }
        normalizeVector(selectedPC2, normType);
        normalizeVector(selectedPC2, normType); */
    }

    std::vector<float> correlationGenePC2;
    std::vector<float> correlationGenePC1;


    computeCorrelationGenePC(fullDataset2, selectedPC2, correlationGenePC2);
    computeCorrelationGenePC(fullDataset1, selectedPC1, correlationGenePC1);

    const float minCorrelation2 = _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMinimum();
    const float maxCorrelation2 = _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMaximum();
    const auto dimensionNames2 = fullDataset2->getDimensionNames();
    
    const float minCorrelation1 = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMinimum();
    const float maxCorrelation1 = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMaximum();
    const auto dimensionNames1 = fullDataset1->getDimensionNames();

    {
        
        
        
        /*
        // Prepare sorted indices for D2 and D1
        std::vector<int> sortedIndicesD2(correlationGenePC2.size());
        std::iota(sortedIndicesD2.begin(), sortedIndicesD2.end(), 0);
        std::sort(sortedIndicesD2.begin(), sortedIndicesD2.end(), [&](int a, int b) {
            return correlationGenePC2[a] > correlationGenePC2[b];
            });

        std::vector<int> sortedIndicesD1(correlationGenePC1.size());
        std::iota(sortedIndicesD1.begin(), sortedIndicesD1.end(), 0);
        std::sort(sortedIndicesD1.begin(), sortedIndicesD1.end(), [&](int a, int b) {
            return correlationGenePC1[a] > correlationGenePC1[b];
            });
        
        // Print table header
        qDebug() << QString("%1 | %2 | %3 | %4 | %5")
            .arg("Rank", 4)
            .arg("Gene (D2)", 20)
            .arg("Corr (D2)", 12)
            .arg("Gene (D1)", 20)
            .arg("Corr (D1)", 12);

        // Print a separator
        qDebug() << QString("%1-+-%2-+-%3-+-%4-+-%5")
            .arg(QString(4, '-'))
            .arg(QString(20, '-'))
            .arg(QString(12, '-'))
            .arg(QString(20, '-'))
            .arg(QString(12, '-'));

        // Print each row
        for (int i = 0; i < 10; i++) {
            int idxD2 = sortedIndicesD2[i];
            int idxD1 = sortedIndicesD1[i];

            qDebug() << QString("%1 | %2 | %3 | %4 | %5")
                .arg(i + 1, 4)
                .arg(dimensionNames2[idxD2], 20)
                .arg(QString::number(correlationGenePC2[idxD2], 'f', 6), 12)
                .arg(dimensionNames1[idxD1], 20)
                .arg(QString::number(correlationGenePC1[idxD1], 'f', 6), 12);
        }*/
    }


    try {
        populateGenesByCorrelationRange(correlationGenePC2, minCorrelation2, maxCorrelation2, dimensionNames2, 2);
        populateGenesByCorrelationRange(correlationGenePC1, minCorrelation1, maxCorrelation1, dimensionNames1, 3);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
        setStatusMessage("E", "Error occurred during filtering genes", "");
        return;
    }

    setupTableModelAndSelection(2);
    setupTableModelAndSelection(3); 

    setStatusMessage("C", "Correlation between genes and PC completed", "#28a745");
}
bool Computation::getRotationAngle(const QString& scatterplotViewName, float& rotationAngle)
{
    auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
    if (!scatterplotViewFactory) {
        qWarning() << "Scatterplot view factory not found";
        setStatusMessage("E", "Scatterplot view factory not found", "");
        return false;
    }

    // Use range-based for and early return for clarity and efficiency
    for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
        if (plugin->getGuiName() == scatterplotViewName) {
            auto* angleAction = dynamic_cast<DecimalAction*>(
                plugin->findChildByPath("Settings/Selection/Point Selection/Line angle"));
            if (!angleAction) {
                qWarning() << "Angle action not found";
                setStatusMessage("E", "Angle action not found", "");
                return false;
            }
            rotationAngle = angleAction->getValue();
            return true;
        }
    }

    qWarning() << "Scatterplot plugin not found";
    setStatusMessage("E", "Scatterplot plugin not found", "");
    return false;
}

bool Computation::populateEmbeddingCoordinates(int xEmbeddingIndex, int yEmbeddingIndex, std::vector<float>& XYCoordinates)
{
    if (_filteredSelectionIndices.empty()) {
        qWarning() << "No filtered selection indices";
        setStatusMessage("E", "No indices selected", "");
        return false;
    }

    const size_t numPoints = _filteredSelectionIndices.size();
    XYCoordinates.resize(numPoints * 2);

    // Avoid unnecessary size check (resize always sets correct size)
    // Use stack allocation for small fixed-size vector
    int embeddingIndices[2] = { xEmbeddingIndex, yEmbeddingIndex };

    // Directly use std::vector<int> constructor from array for clarity and efficiency
    _embeddingDataset->populateDataForDimensions(
        XYCoordinates,
        std::vector<int>(embeddingIndices, embeddingIndices + 2),
        _filteredSelectionIndices
    );

    return true;
}

bool Computation::populateAndUpdateDatasets(mv::Dataset<Points>& fullPointDataset, const std::vector<float>& rotatedXYCordinates, const std::vector<float>& XYCordinates, int noOfDimensions, const std::vector<int>& columnIndices, const std::vector<QString>& dimensionNames)
{
    // Validate datasets and input sizes
    if (!_selectedCellPointDataset.isValid() ||
        !_rotatedSelectedEmbeddingDataset.isValid() ||
        !_lineChartPointDataset1.isValid() ||
        !_lineChartClusterDataset1.isValid() ||
        !_selectedGenePointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() ||
        !_selectedEmbeddingDataset.isValid() ||
        _filteredSelectionIndices.empty() ||
        columnIndices.empty() ||
        dimensionNames.size() != static_cast<size_t>(noOfDimensions))
    {
        qWarning() << "Invalid dataset pointers or input sizes!";
        setStatusMessage("E", "Internal error: invalid dataset pointer or input sizes", "");
        return false;
    }

    // Prepare cell-by-gene matrix
    const size_t numCells = _filteredSelectionIndices.size();
    const size_t numGenes = columnIndices.size();
    std::vector<float> cellbygenematrix(numCells * numGenes);


    try {
        fullPointDataset->populateDataForDimensions(cellbygenematrix, columnIndices, _filteredSelectionIndices);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }
    if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked()) 
    { 
        NormalizationType normType;
        QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
        if (normTypeStr == "ZScore") {
            normType = NormalizationType::ZScore;
        }
        else if (normTypeStr == "MinMax") {
            normType = NormalizationType::MinMax;
        }
        else if (normTypeStr == "Log2") {
            normType = NormalizationType::Log2;
        }
        else {
            normType = NormalizationType::None; // Default case
        }
        normalizeVector(cellbygenematrix, normType);
    }
    // Update datasets in a cache-friendly order
    _selectedCellPointDataset->setData(cellbygenematrix.data(), static_cast<int>(numCells), noOfDimensions);
    _selectedCellPointDataset->setDimensionNames(dimensionNames);
    mv::events().notifyDatasetDataChanged(_selectedCellPointDataset);

    _rotatedSelectedEmbeddingDataset->setData(rotatedXYCordinates.data(), static_cast<int>(numCells), 2);
    _rotatedSelectedEmbeddingDataset->setDimensionNames({ QStringLiteral("X"), QStringLiteral("Y") });
    mv::events().notifyDatasetDataChanged(_rotatedSelectedEmbeddingDataset);
    
    
    _selectedEmbeddingDataset->setData(XYCordinates.data(), static_cast<int>(numCells), 2);
    _selectedEmbeddingDataset->setDimensionNames({ QStringLiteral("X"), QStringLiteral("Y") });
    mv::events().notifyDatasetDataChanged(_selectedEmbeddingDataset);


    std::vector<QString> dimensionNamesWithYCoords = { QStringLiteral("Rotated Y Coordinates") };
    dimensionNamesWithYCoords.insert(dimensionNamesWithYCoords.end(), dimensionNames.begin(), dimensionNames.end());


    std::vector<float> rotatedYCoord(numCells * 1);
    _rotatedSelectedEmbeddingDataset->extractDataForDimension(rotatedYCoord, 1);

    std::vector<float> linechartdata(numCells * (1 + numGenes));

    // Fill the linechartdata vector
    for (size_t i = 0; i < numCells; ++i) {
        // Set the first column with rotatedYCoord
        linechartdata[i * (1 + numGenes)] = rotatedYCoord[i];

        // Copy the rest of the row from cellbygenematrix
        for (size_t j = 0; j < numGenes; ++j) {
            linechartdata[i * (1 + numGenes) + 1 + j] = cellbygenematrix[i * numGenes + j];
        }
    }

    // Set data for line chart datasets
    _lineChartPointDataset1->setData(linechartdata.data(), static_cast<int>(numCells), noOfDimensions + 1);
    _lineChartPointDataset1->setDimensionNames(dimensionNamesWithYCoords);
    mv::events().notifyDatasetDataChanged(_lineChartPointDataset1);

    //populate the clusterDataset using _filteredSelectionIndices indices positions
    Dataset<Clusters> clusterDataset =_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset();
    if (clusterDataset.isValid() && _lineChartClusterDataset1.isValid())
    {
        _lineChartClusterDataset1->getClusters() = QVector<Cluster>();
        events().notifyDatasetDataChanged(_lineChartClusterDataset1);
        auto clusters = clusterDataset->getClusters();
        for (const auto& cluster : clusters)
        {
            std::vector<std::seed_seq::result_type> newClusterIndices;
            auto clusterIndices = cluster.getIndices();
            //check which indices from clusterIndices are in _filteredSelectionIndices and get their positions in  newClusterIndices
            for (int index : clusterIndices) {
                auto it = std::find(_filteredSelectionIndices.begin(), _filteredSelectionIndices.end(), index);
                if (it != _filteredSelectionIndices.end()) {
                    // Push the position (offset) instead of the value
                    newClusterIndices.push_back(static_cast<int>(std::distance(_filteredSelectionIndices.begin(), it)));
                }
            }

            if (newClusterIndices.size()>0)
            {
                Cluster remappedCluster;
                remappedCluster.setName(cluster.getName());
                remappedCluster.setColor(cluster.getColor());
                remappedCluster.setIndices(newClusterIndices);
                _lineChartClusterDataset1->addCluster(remappedCluster);
            }
           

        }
        events().notifyDatasetDataChanged(_lineChartClusterDataset1);
    }



    return true;
}

bool Computation::setupAndTriggerPCA()
{
    // Compute PCA on all cells with selected genes of interest
    FunctionTimer timer(Q_FUNC_INFO);

    auto fullPointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto fullPointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId());

    if (!_selectedGenePointDataset1.isValid() || !fullPointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() || !fullPointDataset2.isValid()) {
        qWarning() << "One or more datasets are invalid!";
        setStatusMessage("E", "Internal error: invalid dataset pointer", "");
        return false;
    }

    const int numCells0 = static_cast<int>(_filteredSelectionIndices.size());
    const int numAllCells2 = fullPointDataset2->getNumPoints();
    const int numAllCells1 = fullPointDataset1->getNumPoints();
    const int numOfIntGenes = static_cast<int>(_geneOfInterestIndices1.size());

    const auto& dimensionNames1 = fullPointDataset1->getDimensionNames();
    const auto& dimensionNames2 = fullPointDataset2->getDimensionNames();

    if (numCells0 <= 0 || numAllCells2 <= 0 || numOfIntGenes <= 0) {
        qWarning() << "Invalid dataset sizes: numAllCells1:" << numCells0 << "numAllCells2:" << numAllCells2 << "numOfIntGenes:" << numOfIntGenes;
        setStatusMessage("E", "Invalid dataset sizes", "");
        return false;
    }
    if (numOfIntGenes > static_cast<int>(dimensionNames1.size()) ||
        numOfIntGenes > static_cast<int>(dimensionNames2.size())) {
        qWarning() << "Number of genes of interest exceeds dataset dimensions";
        setStatusMessage("E", "Number of genes of interest exceeds dataset dimensions", "");
        return false;
    }

    // Build gene index mapping and names of interest in a single pass
    std::vector<QString> dimensionNamesOfInterest;
    std::vector<int> geneDataset0, geneDataset1, geneDataset2;
    dimensionNamesOfInterest.reserve(numOfIntGenes);
    geneDataset0.reserve(numOfIntGenes);
    geneDataset1.reserve(numOfIntGenes);
    geneDataset2.reserve(numOfIntGenes);

    for (int idx : _geneOfInterestIndices1) {
        if (idx < 0 || idx >= static_cast<int>(dimensionNames1.size()))
            continue;
        const QString& geneName = dimensionNames1[idx];
        auto it2 = std::find(dimensionNames2.begin(), dimensionNames2.end(), geneName);
        if (it2 != dimensionNames2.end()) {
            int geneId2 = static_cast<int>(std::distance(dimensionNames2.begin(), it2));
            dimensionNamesOfInterest.push_back(geneName);
            geneDataset0.push_back(idx);
            geneDataset1.push_back(idx);
            geneDataset2.push_back(geneId2);
        }
    }

    const int numDimsOfInterest = static_cast<int>(dimensionNamesOfInterest.size());
    if (numDimsOfInterest <= 0) {
        qWarning() << "No genes of interest selected for PCA";
        setStatusMessage("E", "No genes of interest selected for PCA", "");
        return false;
    }

    // Prepare cell indices
    std::vector<int> cellIndices0(_filteredSelectionIndices.begin(), _filteredSelectionIndices.end());

    std::vector<float> cellByGeneMatrix0(numCells0 * numDimsOfInterest);
    try {
        fullPointDataset1->populateDataForDimensions(cellByGeneMatrix0, geneDataset0, cellIndices0);
        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked())
        {
            NormalizationType normType;
            QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
            if (normTypeStr == "ZScore") {
                normType = NormalizationType::ZScore;
            }
            else if (normTypeStr == "MinMax") {
                normType = NormalizationType::MinMax;
            }
            else if (normTypeStr == "Log2") {
                normType = NormalizationType::Log2;
            }
            else {
                normType = NormalizationType::None; // Default case
            }
            normalizeVector(cellByGeneMatrix0, normType);
        }
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }
    auto [minVal, maxVal] = std::minmax_element(cellByGeneMatrix0.begin(), cellByGeneMatrix0.end());
    qInfo() << "PCA cellByGeneMatrix0 min:" << *minVal << "max:" << *maxVal;

    // Prepare all cell indices for both datasets
    std::vector<int> allCellIndices1(numAllCells1), allCellIndices2(numAllCells2);
    std::iota(allCellIndices1.begin(), allCellIndices1.end(), 0);
    std::iota(allCellIndices2.begin(), allCellIndices2.end(), 0);

    if (allCellIndices1.empty() || allCellIndices2.empty()) {
        qWarning() << "No valid cell indices found for datasets";
        setStatusMessage("E", "No valid cell indices found for datasets", "");
        return false;
    }

    // Prepare cell-by-gene matrices for both datasets
    std::vector<float> cellByGeneMatrix2(numAllCells2 * numDimsOfInterest);
    std::vector<float> cellByGeneMatrix1(numAllCells1 * numDimsOfInterest);

    try {
        fullPointDataset2->populateDataForDimensions(cellByGeneMatrix2, geneDataset2, allCellIndices2);
        fullPointDataset1->populateDataForDimensions(cellByGeneMatrix1, geneDataset1, allCellIndices1);

        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked()) 
        {
            NormalizationType normType;
            QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
            if (normTypeStr == "ZScore") {
                normType = NormalizationType::ZScore;
            }
            else if (normTypeStr == "MinMax") {
                normType = NormalizationType::MinMax;
            }
            else if (normTypeStr == "Log2") {
                normType = NormalizationType::Log2;
            }
            else {
                normType = NormalizationType::None; // Default case
            }
            normalizeVector(cellByGeneMatrix2, normType);
            normalizeVector(cellByGeneMatrix1, normType);
        }

    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }
    auto [minVal2, maxVal2] = std::minmax_element(cellByGeneMatrix2.begin(), cellByGeneMatrix2.end());
    auto [minVal1, maxVal1] = std::minmax_element(cellByGeneMatrix1.begin(), cellByGeneMatrix1.end());
    qInfo() << "PCA cellByGeneMatrix2 min:" << *minVal2 << "max:" << *maxVal2
        << "PCA cellByGeneMatrix1 min:" << *minVal1 << "max:" << *maxVal1;

    _selectedGenePointDataset2->setData(cellByGeneMatrix2.data(), numAllCells2, numDimsOfInterest);
    _selectedGenePointDataset2->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset2);

    _selectedGenePointDataset1->setData(cellByGeneMatrix1.data(), numAllCells1, numDimsOfInterest);
    _selectedGenePointDataset1->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset1);

    setStatusMessage("R", "PCA Started", "");
    _pca1.fit(cellByGeneMatrix0, numCells0, numDimsOfInterest, numOfIntGenes);
    return true;
}


void Computation::resetupCorrelationTable(int datasetNumber)
{
    float minRange, maxRange;
    if (datasetNumber == 2) {
        minRange = _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMinimum();
        maxRange = _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMaximum();
    }
    else {
        minRange = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMinimum();
        maxRange = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMaximum();
    }

    constexpr float epsilon = 1e-6f;

    // Use references to avoid code duplication
    auto& geneCorrValues = (datasetNumber == 2) ? _geneCorrValues2 : (datasetNumber == 1) ? _geneCorrValues1 : _geneCorrValues3;
    auto& geneOfInterestIndices = (datasetNumber == 2) ? _geneOfInterestIndices2 : (datasetNumber == 1) ? _geneOfInterestIndices1: _geneOfInterestIndices3;

    geneOfInterestIndices.clear();
    geneOfInterestIndices.reserve(geneCorrValues.size());

    // Use a single loop for marking and collecting indices
    for (auto& gene : geneCorrValues) {
        // Corrected logic: isWithinRange should be true if value is inside [minRange, maxRange]
        bool isWithinRange = (gene.correlationValue <= (minRange - epsilon) || gene.correlationValue >= (maxRange + epsilon));
        gene.isWithinRange = isWithinRange;
        if (isWithinRange) {
            geneOfInterestIndices.push_back(gene.geneIndex);
           
        }
    }

    setupTableModelAndSelection(datasetNumber);

    // Select the first row of the table if available
    QTableView* tableView = (datasetNumber == 2)
        ? _settingsAction.getTableView2Action()
        : (datasetNumber == 1)
        ? _settingsAction.getTableView1Action(): _settingsAction.getTableView3Action();

    if (tableView) {
        auto* model = tableView->model();
        if (model && model->rowCount() > 0) {
            QModelIndex firstIndex = model->index(0, 0);
            if (firstIndex.isValid()) {
                tableView->setCurrentIndex(firstIndex);
                tableView->selectionModel()->select(
                    firstIndex,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                );
            }
        }
    }
}

void Computation::setSelectionAndAutoCompute()
{

    AutoCompute();
}
void Computation::setupTableModelAndSelection(int datasetNumber)
{
    FunctionTimer timer(Q_FUNC_INFO);
    // Select appropriate data and view pointers
    const auto& geneCorrValues = (datasetNumber == 2) ? _geneCorrValues2 : (datasetNumber == 1) ? _geneCorrValues1: _geneCorrValues3;
    QTableView* tableView = (datasetNumber == 2)
        ? _settingsAction.getTableView2Action(): (datasetNumber == 1)?
         _settingsAction.getTableView1Action(): _settingsAction.getTableView3Action();

    // Create model
    auto rowCount = static_cast<int>(geneCorrValues.size());
    QStandardItemModel* model = new QStandardItemModel(rowCount, 3, tableView);
    model->setHorizontalHeaderLabels(QStringList() << "Gene" << "Correlation" << "Rank");

    // Find min/max correlation in a single pass
    float minCorr = std::numeric_limits<float>::max();
    float maxCorr = std::numeric_limits<float>::lowest();
    for (const auto& entry : geneCorrValues) {
        minCorr = std::min(minCorr, entry.correlationValue);
        maxCorr = std::max(maxCorr, entry.correlationValue);
    }
    _commonGeneNames.clear();
    // Preallocate row items for efficiency
    QVector<QStandardItem*> rowItems(3);
    for (int i = 0; i < rowCount; ++i) {
        const auto& entry = geneCorrValues[i];

        QStandardItem* geneItem = new QStandardItem(entry.geneName);
        QStandardItem* corrItem = new QStandardItem;
        QStandardItem* rankItem = new QStandardItem(QString::number(entry.rank));

        corrItem->setData(QVariant::fromValue(entry.correlationValue), Qt::UserRole + 1); // For delegate
        corrItem->setData(QVariant::fromValue(entry.correlationValue), Qt::DisplayRole);  // For sorting

        // Highlight and bold if isWithinRange
        if (entry.isWithinRange) {
            QFont fontG = geneItem->font();
            fontG.setBold(true);
            geneItem->setFont(fontG);
            if (datasetNumber == 1)
            {
                geneItem->setBackground(QColor("#FDCC0D"));
            }
            else
            {
                geneItem->setBackground(QColor("#FFB6C1"));
            }
            
            geneItem->setForeground(QColor("#000000"));
            QFont fontR = rankItem->font();
            fontR.setBold(true);
            rankItem->setFont(fontR);
            if (datasetNumber == 1)
            {
                rankItem->setBackground(QColor("#FDCC0D"));
            }
            else
            {
                rankItem->setBackground(QColor("#FFB6C1"));
            }
            
            
            rankItem->setForeground(QColor("#000000"));
            _commonGeneNames.append(entry.geneName);
        }
        corrItem->setForeground(QColor("#000000"));
        //TODO: Set background color based on datasetNumber if needed
        /*if (datasetNumber == 1)
        {
            corrItem->setBackground(QColor("#FDCC0D"));
        }
        else
        {
            corrItem->setBackground(QColor("#FFB6C1"));
        }*/

        rowItems[0] = geneItem;
        rowItems[1] = corrItem;
        rowItems[2] = rankItem;
        for (int col = 0; col < 3; ++col)
            model->setItem(i, col, rowItems[col]);
    }

    // Set up the view
    tableView->setModel(model);
    tableView->setItemDelegateForColumn(1, new CorrelationBarDelegate(minCorr, maxCorr, model));
    tableView->resizeColumnsToContents();
    tableView->resizeRowsToContents();
    tableView->setSortingEnabled(true);
    tableView->sortByColumn(1, Qt::DescendingOrder);

    // Disconnect previous selection handler if any
    if (tableView->selectionModel())
        disconnect(tableView->selectionModel(), nullptr, this, nullptr);

    // Connect selection handler
    connect(
        tableView->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this, datasetNumber](const QItemSelection&, const QItemSelection&) {
            QTableView* tv = (datasetNumber == 2)
                ? _settingsAction.getTableView2Action() : (datasetNumber == 1) ? _settingsAction.getTableView1Action()
                : _settingsAction.getTableView3Action();
            auto* mdl = tv->model();
            if (!mdl) return;
            QModelIndexList selectedIndexes = tv->selectionModel()->selectedIndexes();
            if (selectedIndexes.isEmpty()) return;
            const QModelIndex& index = selectedIndexes.first();
            if (!index.isValid()) return;
            QString value = mdl->data(mdl->index(index.row(), 0), Qt::DisplayRole).toString();
            if (datasetNumber == 2)
            {
                QString targetGeneName = value;
                QTableView* firstTableView = _settingsAction.getTableView1Action();
                QStandardItemModel* firstModel = qobject_cast<QStandardItemModel*>(firstTableView->model());
                if (firstModel) {
                    for (int row = 0; row < firstModel->rowCount(); ++row) {
                        QString geneName = firstModel->data(firstModel->index(row, 0), Qt::DisplayRole).toString();
                        if (geneName == targetGeneName) {
                            QModelIndex index = firstModel->index(row, 0);
                            if (index.isValid()) {
                                firstTableView->setCurrentIndex(index);
                                firstTableView->selectionModel()->select(
                                    index,
                                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                                );
                            }
                            break;
                        }
                    }
                }
                _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionName(value);
            }
            else if (datasetNumber == 1)
            {
                _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
                //select the gene in the third table  find the row and select
                QString targetGeneName = value; 
                QTableView* thirdTableView = _settingsAction.getTableView3Action();
                QStandardItemModel* thirdModel = qobject_cast<QStandardItemModel*>(thirdTableView->model());
                if (thirdModel) {
                    for (int row = 0; row < thirdModel->rowCount(); ++row) {
                        QString geneName = thirdModel->data(thirdModel->index(row, 0), Qt::DisplayRole).toString();
                        if (geneName == targetGeneName) {
                            QModelIndex index = thirdModel->index(row, 0);
                            if (index.isValid()) {
                                thirdTableView->setCurrentIndex(index);
                                thirdTableView->selectionModel()->select(
                                    index,
                                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                                );
                                _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
                            }
                            break;
                        }
                    }
                }
                if (_settingsAction.getCommonOptionsHolder().getLinkDataset1And2GeneTableAction().isChecked())
                {
                    QTableView* secondTableView = _settingsAction.getTableView2Action();
                    QStandardItemModel* secondModel = qobject_cast<QStandardItemModel*>(secondTableView->model());
                    if (secondModel) {
                        for (int row = 0; row < secondModel->rowCount(); ++row) {
                            QString geneName = secondModel->data(secondModel->index(row, 0), Qt::DisplayRole).toString();
                            if (geneName == targetGeneName) {
                                QModelIndex index = secondModel->index(row, 0);
                                if (index.isValid()) {
                                    secondTableView->setCurrentIndex(index);
                                    secondTableView->selectionModel()->select(
                                        index,
                                        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                                    );
                                    _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionName(value);
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                //_settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
                //select the gene in the first table in the first table find the row and select
                QString targetGeneName = value; 
                QTableView* firstTableView = _settingsAction.getTableView1Action();
                QStandardItemModel* firstModel = qobject_cast<QStandardItemModel*>(firstTableView->model());
                if (firstModel) {
                    for (int row = 0; row < firstModel->rowCount(); ++row) {
                        QString geneName = firstModel->data(firstModel->index(row, 0), Qt::DisplayRole).toString();
                        if (geneName == targetGeneName) {
                            QModelIndex index = firstModel->index(row, 0);
                            if (index.isValid()) {
                                firstTableView->setCurrentIndex(index);
                                firstTableView->selectionModel()->select(
                                    index,
                                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                                );
                                _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
                            }
                            break;
                        }
                    }
                }
                if (_settingsAction.getCommonOptionsHolder().getLinkDataset1And2GeneTableAction().isChecked())
                {
                    QTableView* secondTableView = _settingsAction.getTableView2Action();
                    QStandardItemModel* secondModel = qobject_cast<QStandardItemModel*>(secondTableView->model());
                    if (secondModel) {
                        for (int row = 0; row < secondModel->rowCount(); ++row) {
                            QString geneName = secondModel->data(secondModel->index(row, 0), Qt::DisplayRole).toString();
                            if (geneName == targetGeneName) {
                                QModelIndex index = secondModel->index(row, 0);
                                if (index.isValid()) {
                                    secondTableView->setCurrentIndex(index);
                                    secondTableView->selectionModel()->select(
                                        index,
                                        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                                    );
                                    _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionName(value);
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    );

    // Select the first row if available
    if (model->rowCount() > 0) {
        QModelIndex firstIndex = model->index(0, 0);
        if (firstIndex.isValid()) {
            tableView->setCurrentIndex(firstIndex);
            tableView->selectionModel()->select(
                firstIndex,
                QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
            );
        }
    }
}


