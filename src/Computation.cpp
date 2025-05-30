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
using namespace mv;
using namespace mv::gui;


class CorrelationBarDelegate : public QStyledItemDelegate {
public:
    CorrelationBarDelegate(float minCorr, float maxCorr, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), minValue(minCorr), maxValue(maxCorr) {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override {
        if (index.column() == 1) { // Correlation column
            float value = index.data(Qt::UserRole + 1).toFloat();
            float norm = (maxValue > minValue) ? (value - minValue) / (maxValue - minValue) : 0.0f;
            int barWidth = static_cast<int>(norm * (option.rect.width() - 8));
            // Make the bar taller by reducing the vertical margins (e.g., 4px top/bottom)
            QRect barRect = option.rect.adjusted(4, 4, -4, -4);

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);

            // Draw selection or normal background
            QColor bgColor;
            if (option.state & QStyle::State_Selected) {
                bgColor = option.palette.color(QPalette::Highlight);
            }
            else {
                bgColor = option.palette.color(QPalette::Base);
            }
            painter->fillRect(option.rect, bgColor);

            // Bar color: use Highlight for normal, or a contrasting color if selected
            QColor barColor;
            if (option.state & QStyle::State_Selected) {
                // Use a color that contrasts with the selection background
                barColor = option.palette.color(QPalette::HighlightedText);
                if (barColor == bgColor) barColor = QColor("#ffffff");
            }
            else {
                barColor = option.palette.color(QPalette::Highlight);
                if (barColor == bgColor) barColor = QColor("#87ceeb");
            }

            QRect filledRect = barRect;
            filledRect.setWidth(barWidth);
            painter->setPen(Qt::NoPen);
            painter->setBrush(barColor);
            painter->drawRect(filledRect);

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


static QString getContrastingTextColor(const QString& bgColor) {
    QColor color(bgColor);
    if (!color.isValid()) return "white";
    // Calculate luminance (perceived brightness)
    double luminance = (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) / 255.0;
    return (luminance > 0.5) ? "black" : "white";
}
QVector<QPointF> convertToQPointF(const std::vector<float>& vec,
    int pointsPerLine,
    int numOfLines) {
    QVector<QPointF> points;

    if (pointsPerLine < 1) {
        throw std::invalid_argument("At least 1 point per line is required.");
    }

    if (static_cast<size_t>(pointsPerLine * numOfLines * 2) > vec.size()) {
        throw std::invalid_argument("Input vector size is smaller than expected.");
    }

    // Total points = numOfLines * pointsPerLine
    points.reserve(numOfLines * pointsPerLine);

    for (int pointIdx = 0; pointIdx < pointsPerLine; ++pointIdx) {
        for (int lineIdx = 0; lineIdx < numOfLines; ++lineIdx) {
            // Calculate position in the flat array
            size_t pos = pointIdx * numOfLines * 2 + lineIdx * 2;
            float x = vec[pos];
            float y = vec[pos + 1];
            points.append(QPointF(x, y));
        }
    }

    return points;
}
QVector<LineData> convertToLineData(const std::vector<float>& vec,
    int pointsPerLine,
    int numOfLines,
    const QVector<QColor>& colors = QVector<QColor>()) {
    QVector<LineData> lines;

    qDebug() << "Conversion started. Total elements:" << vec.size()
        << "Points per line:" << pointsPerLine
        << "Number of lines:" << numOfLines;

    // Validate input
    if (pointsPerLine < 1 || numOfLines < 1) {
        qWarning() << "Invalid parameters - pointsPerLine:" << pointsPerLine
            << "numOfLines:" << numOfLines;
        return lines;
    }

    const size_t requiredSize = static_cast<size_t>(pointsPerLine * numOfLines * 2);
    if (vec.size() < requiredSize) {
        qWarning() << "Insufficient data - Got:" << vec.size()
            << "Need:" << requiredSize;
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
            size_t pos = pointIdx * numOfLines * 2 + lineIdx * 2;

            // Bounds checking
            if (pos + 1 >= vec.size()) {
                qWarning() << "Index out of bounds at line" << lineIdx
                    << "point" << pointIdx;
                continue;
            }

            float x = vec[pos];
            float y = vec[pos + 1];

            if (!std::isfinite(x) || !std::isfinite(y)) {
                qWarning() << "Non-finite point at line" << lineIdx
                    << "point" << pointIdx << ":" << x << y;
                continue;
            }

            line.points.append(QPointF(x, y));
        }

        if (!line.points.isEmpty()) {
            lines.append(line);
            qDebug() << "Added line" << lineIdx << "with" << line.points.size() << "points";
        }
        else {
            qWarning() << "Skipping empty line" << lineIdx;
        }
    }

    qDebug() << "Conversion complete. Generated" << lines.size() << "lines";
    return lines;
}

QVector<LineData> convertToLineData(const std::vector<float>& vecAll, const std::vector<float>& vecCoord,
    int pointsPerLine,
    int numOfLines,
    const QVector<QColor>& colors,
    bool coordIsX,
    bool sortByX)
{
    QVector<LineData> lines;

    qDebug() << "Conversion started. Total elements:" << vecAll.size()
        << "Points per line:" << pointsPerLine
        << "Number of lines:" << numOfLines
        << "coordIsX:" << coordIsX;

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

    // Debug print first few values to verify data organization
    if (!coordIsX) {
        qDebug() << "First few x values (column order):";
        for (int i = 0; i < std::min(5, pointsPerLine); ++i) {
            for (int j = 0; j < std::min(5, numOfLines); ++j) {
                size_t pos = static_cast<size_t>(i) * numOfLines + j;
                qDebug() << "x[" << i << "][" << j << "]: pos" << pos << "=" << vecAll[pos];
            }
        }
        qDebug() << "First few y values:";
        for (int i = 0; i < std::min(5, pointsPerLine); ++i) {
            qDebug() << "y[" << i << "]:" << vecCoord[i];
        }
    }
    else {
        qDebug() << "First few y values (column order):";
        for (int i = 0; i < std::min(5, pointsPerLine); ++i) {
            for (int j = 0; j < std::min(5, numOfLines); ++j) {
                size_t pos = static_cast<size_t>(i) * numOfLines + j;
                qDebug() << "y[" << i << "][" << j << "]: pos" << pos << "=" << vecAll[pos];
            }
        }
        qDebug() << "First few x values:";
        for (int i = 0; i < std::min(5, pointsPerLine); ++i) {
            qDebug() << "x[" << i << "]:" << vecCoord[i];
        }
    }

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
            qDebug() << "Added line" << lineIdx << "with" << line.points.size() << "points";
            if (!line.points.isEmpty()) {
                qDebug() << "  First point:" << line.points.first().x() << line.points.first().y();
                qDebug() << "  Last point:" << line.points.last().x() << line.points.last().y();
            }
        }
        else {
            qWarning() << "Skipping empty line" << lineIdx;
        }
    }

    qDebug() << "Conversion complete. Generated" << lines.size() << "lines";
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

    _correlationRangeDebounceTimer.setSingleShot(true);
    _clusterSelectionDebounceTimer.setSingleShot(true);


    const auto updateStatus = [this]() -> void {
        auto stringFull = _settingsAction.getSelectionOptionsHolder().getStatusMessageAction().getString();

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
        QStatusBar* statusBar = _settingsAction.getComputeHolder().getStatusBarAction();
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
    connect(&_settingsAction.getSelectionOptionsHolder().getStatusMessageAction(), &StringAction::stringChanged, this, updateStatus);
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
        return dataset->getDataType() == ClusterType;
        });
     _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });
     _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });

     _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
         return dataset->getDataType() == ClusterType;
         });
     _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
         return dataset->getDataType() == PointType;
         });
     _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
         return dataset->getDataType() == PointType;
         });

    const auto updateClusterDataset1 = [this]() -> void {
        if (_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().isValid())
        {
            auto clusterFullDataset = mv::data().getDataset<Clusters>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
            auto clusters = clusterFullDataset->getClusters();
            QStringList clusterNames = {};
            if (!clusters.isEmpty())
            {
                for (auto cluster : clusters)
                {
                    clusterNames.append(cluster.getName());
                }
            }

             _settingsAction.getSelectionOptionsHolder().getClusterSelectionAction().setOptions(clusterNames);
             _settingsAction.getSelectionOptionsHolder().getClusterSelectionAction().setSelectedOptions(clusterNames);

        }
        else
        {
             _settingsAction.getSelectionOptionsHolder().getClusterSelectionAction().setOptions({});
        }
        setStatusMessage("M", "Cluster dataset updated", "");
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset1);

    const auto updateClusterDataset2 = [this]() -> void {

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset2);

    const auto updatePointDataset1 = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid())
        {
            auto dataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
            if (dataset.isValid())
            {
                 _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().setPointsDataset(dataset);
                auto dimensions = dataset->getNumDimensions();
                
            }


        }
        else
        {
             _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().setPointsDataset({});
        }
        setStatusMessage("M", "Point dataset updated", "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset1);

    const auto updatePointDataset2 = [this]() -> void {

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getPointDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset2);

    const auto updateGeneSelection = [this]() -> void {
        if ( _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().getCurrentDimensionIndex() == -1)
        {
            qDebug() << "No gene selected";
             _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }
        if (_settingsAction.getLineChartWidget())
        {
            _settingsAction.getLineChartWidget()->clear();

        }
        auto selectedGene =  _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene == "")
        {
            qDebug() << "No gene selected";
             _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }
        if ( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() && _settingsAction.getLineChartWidget() && _filteredSelectionIndices.size() > 0)
        {


            auto pointDataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
            if (pointDataset.isValid() && _rotatedSelectedEmbeddingDataset.isValid())
            {

                auto dimensionNames = pointDataset->getDimensionNames();
                //get the index of the selected gene QString in the dimension names std::vector<QString>
                auto it = std::find(dimensionNames.begin(), dimensionNames.end(), selectedGene);
                if (it == dimensionNames.end())
                {
                    qDebug() << "Selected gene not found in point dataset dimensions";

                }
                else
                {

                    int geneIndex = std::distance(dimensionNames.begin(), it);
                    LineData selectedGeneLineData;

                    std::vector<int> geneIndicesExp = { geneIndex };
                    std::vector<float> expresionValues(_filteredSelectionIndices.size() * geneIndicesExp.size());
                    QVector<QPointF> points;
                    
                    int geneIndicesCoord =  1 ;
                    std::vector<float> coordValues(_filteredSelectionIndices.size()* geneIndicesCoord);


                    pointDataset->populateDataForDimensions(expresionValues, geneIndicesExp, _filteredSelectionIndices);
                    _rotatedSelectedEmbeddingDataset->extractDataForDimension(coordValues, geneIndicesCoord);

                    qDebug() << "Raw data size:" << expresionValues.size();
                    if (!expresionValues.empty()) {
                        qDebug() << "First 10 elements:";
                        for (size_t i = 0; i < std::min(size_t(10), expresionValues.size()); ++i) {
                            qDebug() << i << ":" << expresionValues[i];
                        }
                    }

                    // 2. Verify your indices
                    qDebug() << "Points per line:" << _filteredSelectionIndices.size();
                    qDebug() << "Number of lines:" << geneIndicesExp.size();

                    // 3. Convert with debugging
                    auto lineData = convertToLineData(
                        expresionValues,
                        coordValues,
                        static_cast<int>(_filteredSelectionIndices.size()),
                        static_cast<int>(geneIndicesExp.size()),
                        QVector<QColor>(), // colors
                        true,              // coordIsX
                        true               // sortByX
                    );
                    // 4. Verify output
                    qDebug() << "Converted line data size:" << lineData.size();
                    if (!lineData.empty()) {
                        qDebug() << "First line points:" << lineData.first().points.size();
                        if (!lineData.first().points.empty()) {
                            qDebug() << "Sample point:" << lineData.first().points.first();
                        }
                    }
                    
                    // 5. Send to chart
                    _settingsAction.getLineChartWidget()->setLines(lineData);
                    /*_settingsAction.getLineChartWidget()->setBinaryNormalizeXThreshold(0.95f);
                    _settingsAction.getLineChartWidget()->setBinaryNormalizeX(true);*/
                    //_settingsAction.getLineChartWidget()->setShowAverageLine(true);
                    _settingsAction.getLineChartWidget()->setShowMedianLine(true);
                    //_settingsAction.getLineChartWidget()->setShowPercentileLines(true);
                    //_settingsAction.getLineChartWidget()->setShowStddevLines(true);
                    //_settingsAction.getLineChartWidget()->setShowMaxLine(true);
                    //_settingsAction.getLineChartWidget()->setShowMinLine(true);
                    _settingsAction.getLineChartWidget()->setShowPoints(false);
                    _settingsAction.getLineChartWidget()->setShowLines(false);
                    //_settingsAction.getLineChartWidget()->setShowKurtosisLine(true);
                    //_settingsAction.getLineChartWidget()->setShowLowerQuartileLine(true);
                    //_settingsAction.getLineChartWidget()->setShowMADLine(true);
                    //_settingsAction.getLineChartWidget()->setShowModeLine(true);
                    //_settingsAction.getLineChartWidget()->setShowSkewnessLine(true);
                    //_settingsAction.getLineChartWidget()->setShowUpperQuartileLine(true);
                    //_settingsAction.getLineChartWidget()->setShowVarianceLine(true);
                    //_settingsAction.getLineChartWidget()->setGradientLineEnabled(true);
                    _settingsAction.getLineChartWidget()->setConnectStatsLineEnabled(true);
                    _settingsAction.getLineChartWidget()->setConnectStatsLineN((lineData.first().points.size() /100)*50);
                    _settingsAction.getLineChartWidget()->setConnectStatsLineType(HighPerfLineChart::ConnectStatsType::Median);
                    _settingsAction.getLineChartWidget()->setShowAvgNumPointsLabel(true);
                    
                }




            }
            else
            {
                qDebug() << "Point dataset is not valid";
            }

        }
        else
        {
            qDebug() << "No point dataset selected";
        }


        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        DimensionPickerAction* geneSelectionAction;
         _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction().setEnabled(true);
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
                                qDebug() << "Gene selection action not found";
                            }
                        }



                    }

                }
            }
        }
        };

    connect(& _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, updateGeneSelection);


    const auto updateEmbeddingDataset1 = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().isValid())
        {


            _embeddingDataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());

            if (_embeddingDataset.isValid())
            {
                 _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setPointsDataset(_embeddingDataset);
                 _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setPointsDataset(_embeddingDataset);
                const auto mainSelectionChanged = [this]() -> void {
                    //qDebug() << "Main selection changed in fullEmbeddingDataset";
                    setStatusMessage("M", "Selected indices changed", "");

                    auto fullclusterDataset = mv::data().getDataset<Clusters>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
                    if (fullclusterDataset.isValid() && _embeddingDataset.isValid())
                    {
                        auto clusters = fullclusterDataset->getClusters();
                        auto selectedClusters =  _settingsAction.getSelectionOptionsHolder().getClusterSelectionAction().getSelectedOptions();
                        auto selectedIndices = _embeddingDataset->getSelectionIndices();
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
                            qDebug() << "No clusters selected, clearing selection indices";
                            _embeddingDataset->setSelectionIndices({});
                            mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset);
                        }

                    }
                    else
                    {
                        qDebug() << "No cluster dataset selected or invalid cluster dataset";
                        _embeddingDataset->setSelectionIndices({});
                        mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset);
                    }
                    setStatusMessage("R", "Computation in progress", "");
                    _settingsAction.getComputeHolder().getComputeAction().setDisabled(true);
                    QApplication::processEvents();

                    AutoCompute();

                    _settingsAction.getComputeHolder().getComputeAction().setDisabled(false);


                    };


                bool ok = connect(&_embeddingDataset, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged);
                if (!ok)
                {
                    qDebug() << "Failed to connect to dataSelectionChanged signal of fullEmbeddingDataset";
                }

            }


        }

        setStatusMessage("M", "Embedding dataset updated", "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset1);

    const auto updateEmbeddingDataset2 = [this]() -> void {

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset2);

    const auto computeInfo = [this]() -> void {

        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getComputeHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        ManualCompute();

        _settingsAction.getComputeHolder().getComputeAction().setDisabled(false);
        };

    connect(&_settingsAction.getComputeHolder().getComputeAction(), &TriggerAction::triggered, this, computeInfo);


    connect(& _settingsAction.getSelectionOptionsHolder().getClusterSelectionAction(),
        &OptionsAction::selectedOptionsChanged,
        this,
        [this]() {
            _clusterSelectionDebounceTimer.start(800);
        });
    connect(&_clusterSelectionDebounceTimer, &QTimer::timeout, this, [this]() {
        //setStatusMessage("M", "Cluster selection updated", "");
        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getComputeHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        
        setSelectionAndAutoCompute();

        _settingsAction.getComputeHolder().getComputeAction().setDisabled(false);
        });



    connect(& _settingsAction.getSelectionOptionsHolder().getCorrelationRangeAction(),
        &DecimalRangeAction::rangeChanged,
        this,
        [this]() {
            _correlationRangeDebounceTimer.start(300);
        });
    connect(&_correlationRangeDebounceTimer, &QTimer::timeout, this, [this]() {
        setStatusMessage("R", "Updating correlation table", "");
        resetupCorrelationTable();
        setStatusMessage("C", "Correlation table updated", "");
        });


    const auto showPointSelectionChanged = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
        if (scatterplotName2.isEmpty() && scatterplotName1.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            return;
        }
        if (scatterplotName1.isEmpty())
        {
            qDebug() << "No scatterplot view 1 selected";

        }
        if (scatterplotName2.isEmpty())
        {
            qDebug() << "No scatterplot view 2 selected";

        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1 || plugin->getGuiName() == scatterplotName2)
                {
                    auto pointSelectionScaleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Scale"));
                    auto pointSelectionOpacityAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Opacity"));
                    if (pointSelectionScaleAction)
                    {
                        if ( _settingsAction.getScatterplotOptionsHolder().getHidePointSelectionOutlineAction().isChecked())
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
                        qDebug() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {
                        if ( _settingsAction.getScatterplotOptionsHolder().getHidePointSelectionOutlineAction().isChecked())
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
                        qDebug() << "Point selection opacity action not found";
                    }
                }
            }
        }
        };

    connect(& _settingsAction.getScatterplotOptionsHolder().getHidePointSelectionOutlineAction(), &ToggleAction::toggled, this, showPointSelectionChanged);

    const auto freezeSelectionChanged = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
        if (scatterplotName2.isEmpty() && scatterplotName1.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            return;
        }
        if (scatterplotName1.isEmpty())
        {
            qDebug() << "No scatterplot view 1 selected";

        }
        if (scatterplotName2.isEmpty())
        {
            qDebug() << "No scatterplot view 2 selected";

        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1 || plugin->getGuiName() == scatterplotName2)
                {
                    auto freezeAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Selection/Freeze selection"));
                    if (freezeAction)
                    {
                        freezeAction->setChecked( _settingsAction.getScatterplotOptionsHolder().getFreezeSelectionAction().isChecked());
                    }
                    else
                    {
                        qDebug() << "Freeze selection action not found";
                    }
                }
            }
        }

        };

    connect(& _settingsAction.getScatterplotOptionsHolder().getFreezeSelectionAction(), &ToggleAction::toggled, this, freezeSelectionChanged);


    const auto focusSelectionChanged = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
        if (scatterplotName2.isEmpty() && scatterplotName1.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            return;
        }
        if (scatterplotName1.isEmpty())
        {
            qDebug() << "No scatterplot view 1 selected";

        }
        if (scatterplotName2.isEmpty())
        {
            qDebug() << "No scatterplot view 2 selected";

        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1 || plugin->getGuiName() == scatterplotName2)
                {
                    auto focusAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Plot/Point/Focus selection"));
                    if (focusAction)
                    {
                        focusAction->setChecked(_settingsAction.getScatterplotOptionsHolder().getFocusSelectionAction().isChecked());
                    }
                    else
                    {
                        qDebug() << "Focus selection action not found";
                    }
                }
            }
        }

        };

    connect(& _settingsAction.getScatterplotOptionsHolder().getFocusSelectionAction(), &ToggleAction::toggled, this, focusSelectionChanged);


    const auto embeddingDimensionYActionChanged1 = [this]() -> void {

        setStatusMessage("M", "Embedding Y dimension changed", "");
        QString scatterplotName = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
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
                                    colorDatasetPickerAction->setCurrentDataset(dataset);
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




        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionYActionChanged2);

    const auto embeddingDimensionXActionChanged1 = [this]() -> void {

        setStatusMessage("M", "Embedding X dimension changed", "");
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
                                    colorDatasetPickerAction->setCurrentDataset(dataset);
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



        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionXActionChanged2);

    const auto deselectRow = [this]() -> void {
        auto selectedIndex = _settingsAction.getTableViewAction()->currentIndex();
        if (selectedIndex.isValid())
        {
            _settingsAction.getTableViewAction()->selectionModel()->clearSelection();
            _settingsAction.getTableViewAction()->setCurrentIndex(QModelIndex());
             _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().setCurrentDimensionName("");
        }
        else
        {
            qDebug() << "No row selected";
        }
         _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction().setEnabled(false);
         _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);

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
                                    colorDatasetPickerAction->setCurrentDataset(dataset);
                                    qDebug() << "Setting dataset to color dataset";
                                }

                            }

                        }



                    }

                }
            }
        }
        _settingsAction.getLineChartWidget()->clear();
        };

    connect(& _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction(), &TriggerAction::triggered, this, deselectRow);
    const auto scatterplotviewSelectionChanged1= [this]() -> void {

        setStatusMessage("M", "Scatterplotview selection 1 updated", "");

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName1.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
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
                        qDebug() << "Notify during selection action not found.";
                    }
                    if (haloAction) {
                        haloAction->setChecked(true);
                    }
                    else
                    {
                        qDebug() << "Halo action not found.";
                    }

                    auto pointSelectionScaleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Scale"));
                    auto pointSelectionOpacityAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Opacity"));
                    if (pointSelectionScaleAction)
                    {

                        pointSelectionScaleAction->setValue(160.0);

                    }
                    else
                    {
                        qDebug() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {

                        pointSelectionOpacityAction->setValue(100.0);



                    }
                    else
                    {
                        qDebug() << "Point selection opacity action not found";
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
            qDebug() << "No scatterplot view 2 selected";
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
                        qDebug() << "Notify during selection action not found.";
                    }
                    if (haloAction) {
                        haloAction->setChecked(true);
                    }
                    else
                    {
                        qDebug() << "Halo action not found.";
                    }

                    auto pointSelectionScaleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Scale"));
                    auto pointSelectionOpacityAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Opacity"));
                    if (pointSelectionScaleAction)
                    {

                        pointSelectionScaleAction->setValue(160.0);

                    }
                    else
                    {
                        qDebug() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {

                        pointSelectionOpacityAction->setValue(100.0);



                    }
                    else
                    {
                        qDebug() << "Point selection opacity action not found";
                    }

                }
            }
        }

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action(), &OptionAction::currentIndexChanged, this, scatterplotviewSelectionChanged2);

}

void Computation::removeDatasets()
{
    const std::vector<int>& groupIds = { 666 };
    auto allDatasets = mv::data().getAllDatasets();

    Datasets datasetsFilteredAndSorted;

    for (int groupId : groupIds) {
        std::copy_if(allDatasets.begin(), allDatasets.end(), std::back_inserter(datasetsFilteredAndSorted), [groupId](Dataset<DatasetImpl> dataset) {
            return dataset->getGroupIndex() == groupId;
            });
    }

    std::sort(datasetsFilteredAndSorted.begin(), datasetsFilteredAndSorted.end(), [](Dataset<DatasetImpl> lhs, Dataset<DatasetImpl> rhs) -> bool {
        return rhs->getDataHierarchyItem().getDepth() < lhs->getDataHierarchyItem().getDepth();
        });

    for (auto dataset : datasetsFilteredAndSorted)
    {
        if (dataset.isValid())
            mv::data().removeDataset(dataset);
    }
}


void Computation::AutoCompute()
{
    try
    {
        // Remove all rows in one call if model exists
        if (auto* model = _settingsAction.getTableViewAction()->model()) {
            int rowCount = model->rowCount();
            if (rowCount > 0)
                model->removeRows(0, rowCount);
        }
        _settingsAction.getLineChartWidget()->clear();
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
            qDebug() << "Exception in rotateAxis:" << ex.what();
            setStatusMessage("E", "Error occurred during point rotation", "");
            return;
        }

        std::vector<float> yCoordinates;
        try {
            yCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "Y");
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in getCoordinatesOfCells:" << ex.what();
            setStatusMessage("E", "Error occurred during fetching coordinates", "");
            return;
        }

        std::vector<int> sortedCellIndices;
        try {
            sortedCellIndices = getSortedCellIndices(yCoordinates);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in getSortedCellIndices:" << ex.what();
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
        qDebug() << "Rotated XY Coordinates minX:" << minX << "maxX:" << maxX << "minY:" << minY << "maxY:" << maxY;
        qDebug() << "Rotated XY Coordinates size:" << rotatedXYCordinates.size() << "filteredSelectionIndices size:" << _filteredSelectionIndices.size() << "yCoordinates size:" << yCoordinates.size();

        // Correlation and gene filtering
        std::vector<float> corrGeneCoords;
        computeCorrelationGeneCoords(_filteredSelectionIndices, fullPointDataset, yCoordinates, corrGeneCoords);

        

        /*
        * // TODO: integrate the following part with setupTableModelAndSelection(), to avoid repetitive code
        * 
        * 
        * 
        * 
        // Sort genes by correlation and output top 5
        int numGenes = corrGeneCoords.size();
        _geneCorrPairs.clear();
        _geneCorrPairs.reserve(numGenes);
        for (int i = 0; i < numGenes; ++i) {
            _geneCorrPairs[i] = std::make_pair(corrGeneCoords[i], i);
        }
        std::sort(_geneCorrPairs.begin(), _geneCorrPairs.end(), std::greater<std::pair<float, int>>());
        qDebug() << "Top 5 genes with highest correlation:";
        for (int i = 0; i < 5 && i < numGenes; ++i) {
            int geneIdx = _geneCorrPairs[i].second;
            QString geneSymbol = fullPointDataset->getDimensionNames()[geneIdx];
            qDebug() << "Gene:" << geneSymbol << "Correlation:" << _geneCorrPairs[i].first;
        }
        // Filter top N genes by correlation and store the gene symbols
        int tempNumGenesOfInterest = 20; // FIXME: this is a hardcoded value, should be replaced with a proper setting
        _geneOfInterestIndices.clear();
        for (int i = 0; i < tempNumGenesOfInterest; ++i) {
            int geneIdx = geneCorrPairs[i].second;
            _geneOfInterestIndices.push_back(geneIdx);
        }
        qDebug() << "Gene of interest indices:" << _geneOfInterestIndices;
        */

        
        try {
            populateGenesByCorrelationRange(corrGeneCoords, minCorrelation, maxCorrelation, dimensionNames);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            setStatusMessage("E", "Error occurred during filtering genes", "");
            return;
        }

        // Setup table model and connect selection
        setupTableModelAndSelection();
        //select the first row of the table
        {
            auto* tableView = _settingsAction.getTableViewAction();
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

        setStatusMessage("C", "Computation completed", "#28a745");
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Exception in Computation:" << ex.what();
        setStatusMessage("E", "Error occurred during computation: " + QString::fromStdString(ex.what()), "");
    }
    catch (...)
    {
        qDebug() << "Unknown exception in Computation";
        setStatusMessage("E", "Unknown error occurred during computation", "");
    }
}

void Computation::ManualCompute()
{
    mv::Dataset<Points> fullPointDataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());

    // Setup and trigger PCA
    if (!setupAndTriggerPCA())
        return;
}

// Rotates the XY coordinates by a given angle
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
// Extracts either X or Y coordinates from the rotated coordinates
std::vector<float> Computation::getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis) {
    std::vector<float> coords;
    int offset = (axis == "Y") ? 1 : 0;
    for (size_t i = offset; i < rotatedXYCoordinates.size(); i += 2) {
        coords.push_back(rotatedXYCoordinates[i]);
    }
    return coords;
}

// Sorts gene indices based on the provided Y coordinates
std::vector<int> Computation::getSortedCellIndices(const std::vector<float>& yCoordinates) {
    std::vector<int> indices(yCoordinates.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return yCoordinates[a] < yCoordinates[b];
        });
    return indices;
}

// Computes a correlation matrix from a cell-by-gene matrix
std::vector<int> Computation::computeCorrelation(const std::vector<float>& cellbygenematrix, int numOfRows, int noOfCols) {
    // Pearson correlation between each gene (column)
    // Output: vector of correlation scores (e.g., sum of absolute correlations for each gene)
    // The result is a vector<int> for compatibility with downstream code, so we will scale and cast the float scores.

    std::vector<float> means(noOfCols, 0.0f);
    std::vector<float> stddevs(noOfCols, 0.0f);

    // Compute mean for each gene
    for (int col = 0; col < noOfCols; ++col) {
        float sum = 0.0f;
        for (int row = 0; row < numOfRows; ++row) {
            sum += cellbygenematrix[row * noOfCols + col];
        }
        means[col] = sum / static_cast<float>(numOfRows);
    }

    // Compute stddev for each gene
    for (int col = 0; col < noOfCols; ++col) {
        float sumSq = 0.0f;
        for (int row = 0; row < numOfRows; ++row) {
            float v = cellbygenematrix[row * noOfCols + col] - means[col];
            sumSq += v * v;
        }
        stddevs[col] = std::sqrt(sumSq / static_cast<float>(numOfRows));
        if (stddevs[col] == 0.0f) stddevs[col] = 1.0f; // Prevent division by zero
    }

    // For each gene, compute the sum of absolute Pearson correlations with all other genes
    std::vector<float> geneCorrSum(noOfCols, 0.0f);

    for (int i = 0; i < noOfCols; ++i) {
        for (int j = 0; j < noOfCols; ++j) {
            if (i == j) continue;
            float num = 0.0f;
            for (int row = 0; row < numOfRows; ++row) {
                float xi = cellbygenematrix[row * noOfCols + i];
                float xj = cellbygenematrix[row * noOfCols + j];
                num += (xi - means[i]) * (xj - means[j]);
            }
            float denom = static_cast<float>(numOfRows) * stddevs[i] * stddevs[j];
            float corr = (denom != 0.0f) ? (num / denom) : 0.0f;
            geneCorrSum[i] += std::abs(corr);
        }
    }

    // Convert to int vector (e.g., scale by 1000 for ranking)
    std::vector<int> correlation(noOfCols, 0);
    for (int i = 0; i < noOfCols; ++i) {
        correlation[i] = static_cast<int>(geneCorrSum[i] * 1000.0f);
    }

    return correlation;
}

void Computation::computeCorrelationGeneCoords(const std::vector<int>& cellIndices, const mv::Dataset<Points> dataset, const std::vector<float>& coords, std::vector<float>& corrVector)
{
    const int numGenes = dataset->getNumDimensions();
    const int numCells = cellIndices.size();
    const int numCoords = coords.size();
    qDebug() << "Number of genes:" << numGenes << "Number of cells:" << numCells << "Number of coordinates:" << numCoords;
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

        //corrVector[geneIdx] = corr;
        corrVector[geneIdx] = std::abs(corr); // Use absolute correlation for ranking
    }

    qDebug() << "Correlation computation completed";
}

// Filters the top N genes by correlation
QStringList Computation::filterTopNGenesByCorrelation(const std::vector<float>& correlationVector, int N, std::vector<QString>  geneNameOrdered) {
    QStringList topGenes;


    // Defensive: correlationMatrix and geneNames should have the same size
    int count = std::min<int>(std::min<int>(N, static_cast<int>(correlationVector.size())), geneNameOrdered.size());
    if (count == 0) return topGenes;

    // Create a vector of indices
    std::vector<int> indices(correlationVector.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Sort indices by descending correlation value
    std::partial_sort(
        indices.begin(), indices.begin() + count, indices.end(),
        [&](int a, int b) { return correlationVector[a] > correlationVector[b]; }
    );

    // Map top indices to gene names
    for (int i = 0; i < count; ++i) {
        int idx = indices[i];
        if (idx >= 0 && idx < geneNameOrdered.size())
            topGenes << geneNameOrdered[idx];
    }

    return topGenes;
}

QStringList Computation::filterGenesByCorrelationRange(
    const std::vector<float>& correlationVector,
    float min,
    float max,
    std::vector<QString> geneNameOrdered)
{
    QStringList filteredGenes;

    // Defensive: correlationVector and geneNameOrdered should have the same size
    int count = std::min(static_cast<int>(correlationVector.size()), static_cast<int>(geneNameOrdered.size()));
    if (count == 0) return filteredGenes;

    for (int i = 0; i < count; ++i) {
        float corr = correlationVector[i];
        if (corr >= min && corr <= max) {
            filteredGenes << geneNameOrdered[i];
        }
    }

    return filteredGenes;
}

void Computation::populateGenesByCorrelationRange(
    const std::vector<float>& correlationVector,
    float min,
    float max,
    const std::vector<QString>& geneNameOrdered)
{
    _geneOfInterestIndices.clear();
    constexpr float epsilon = 1e-6f;
    const int count = std::min(static_cast<int>(correlationVector.size()),
        static_cast<int>(geneNameOrdered.size()));
    if (count == 0) return;

    // Prepare vector of pointers to original indices for sorting
    std::vector<int> indices(count);
    std::iota(indices.begin(), indices.end(), 0);

    // Sort indices by descending correlation value
    std::sort(indices.begin(), indices.end(),
        [&correlationVector](int a, int b) {
            return correlationVector[a] > correlationVector[b];
        });

    _geneCorrValues.clear();
    _geneCorrValues.reserve(count);

    // Assign ranks: same correlation value gets same rank (dense ranking)
    int rank = 1;
    float prevCorr = std::numeric_limits<float>::quiet_NaN();
    for (int i = 0; i < count; ++i) {
        int idx = indices[i];
        float corr = correlationVector[idx];
        bool inRange = (corr > min - epsilon) && (corr < max + epsilon);

        // If this is the first entry or the correlation value has changed, update the rank
        if (i == 0 || std::abs(corr - prevCorr) > epsilon) {
            rank = i + 1;
            prevCorr = corr;
        }

        _geneCorrValues.emplace_back(CorrelationGeneCoords{
            idx,
            geneNameOrdered[idx],
            corr,
            rank,
            inRange
        });
        if (inRange) {
            _geneOfInterestIndices.push_back(idx);
        }
    }
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
    _settingsAction.getSelectionOptionsHolder().getStatusMessageAction().setString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

//Validate and retrieve datasets and indices
bool Computation::validateAndRetrieveDatasets(
    mv::Dataset<Points>& fullPointDataset,
    mv::Dataset<Clusters>& fullClusterDataset,
    int& xEmbeddingIndex,
    int& yEmbeddingIndex,
    std::vector<int>& selectedIndices,
    QString& scatterplotViewName,
    float& minCorrelation,
    float& maxCorrelation
)
{

    fullPointDataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    fullClusterDataset = mv::data().getDataset<Clusters>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());

    if (!_embeddingDataset.isValid() || !fullPointDataset.isValid() || !fullClusterDataset.isValid()) {
        qDebug() << "Invalid datasets";
        setStatusMessage("E", "Error occured during dataset creation", "");
        return false;
    }

    xEmbeddingIndex = _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionIndex();
    yEmbeddingIndex = _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionIndex();
    if (xEmbeddingIndex == -1 || yEmbeddingIndex == -1) {
        qDebug() << "No embedding dimensions selected";
        setStatusMessage("E", "No embedding dataset dimensions selected", "");
        return false;
    }

    // Convert selection indices to std::vector<int>
    {
        auto indices = _embeddingDataset->getSelectionIndices();
        selectedIndices.assign(indices.begin(), indices.end());
    }
    if (selectedIndices.empty()) {
        qDebug() << "No selected indices";
        setStatusMessage("E", "No indices selected", "");
        return false;
    }

    auto selectedClusterValues = _settingsAction.getSelectionOptionsHolder().getClusterSelectionAction().getSelectedOptions();
    if (selectedClusterValues.isEmpty()) {
        qDebug() << "No selected cluster values";
        setStatusMessage("E", "No clusters selected", "");
        return false;
    }

    scatterplotViewName = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
    if (scatterplotViewName.isEmpty()) {
        qDebug() << "No scatterplot view selected";
        setStatusMessage("E", "No scatterplot view selected", "");
        return false;
    }

    minCorrelation = _settingsAction.getSelectionOptionsHolder().getCorrelationRangeAction().getMinimum();
    maxCorrelation = _settingsAction.getSelectionOptionsHolder().getCorrelationRangeAction().getMaximum();
    if (minCorrelation < 0.0f || maxCorrelation > 1.0f || minCorrelation >= maxCorrelation) {
        qDebug() << "Invalid correlation range";
        setStatusMessage("E", "Invalid correlation range", "");
        return false;
    }
    qDebug() << "Selected indices size:" << selectedIndices.size() << "xEmbeddingIndex:" << xEmbeddingIndex << "yEmbeddingIndex:" << yEmbeddingIndex
        << "scatterplotViewName:" << scatterplotViewName << "minCorrelation:" << minCorrelation << "maxCorrelation:" << maxCorrelation;


    return true;
}

// Create datasets if needed
bool Computation::createDatasetsIfNeeded()
{
    if (!_selectedGenePointDataset.isValid()) {
        _selectedGenePointDataset = mv::data().createDataset("Points", "Selected Gene Point Data");
        _selectedGenePointDataset->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedGenePointDataset);
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
    if (!_selectedEmbeddingDataset.isValid()) {
        _selectedEmbeddingDataset = mv::data().createDataset("Points", "Selected Embedding Data", _selectedCellPointDataset);
        _selectedEmbeddingDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_selectedEmbeddingDataset);
    }

    if (!_rotatedSelectedEmbeddingDataset.isValid() || !_selectedGenePointDataset.isValid() || !_selectedEmbeddingDataset.isValid() || !_selectedCellPointDataset.isValid()) {
        qDebug() << "Failed to create datasets";
        setStatusMessage("E", "Error occurred during dataset creation", "");
        return false;
    }

    return true;
}

//Retrieve rotation angle from plugin
bool Computation::getRotationAngle(const QString& scatterplotViewName, float& rotationAngle)
{
    auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
    if (scatterplotViewFactory) {
        bool foundPlugin = false;
        for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
            if (plugin->getGuiName() == scatterplotViewName) {
                foundPlugin = true;
                auto* angleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Point Selection/Line angle"));
                if (angleAction) {
                    rotationAngle = angleAction->getValue();
                }
                else {
                    qDebug() << "Angle action not found";
                    setStatusMessage("E", "Angle action not found", "");
                    return false;
                }
                break;
            }
        }
        if (!foundPlugin) {
            qDebug() << "Scatterplot plugin not found";
            setStatusMessage("E", "Scatterplot plugin not found", "");
            return false;
        }
    }
    else {
        qDebug() << "Scatterplot view factory not found";
        setStatusMessage("E", "Scatterplot view factory not found", "");
        return false;
    }
    return true;
}

// Populate embedding coordinates
bool Computation::populateEmbeddingCoordinates(
    int xEmbeddingIndex, int yEmbeddingIndex,
    std::vector<float>& XYCoordinates)
{
    if (_filteredSelectionIndices.empty()) {
        qDebug() << "No filtered selection indices";
        setStatusMessage("E", "No indices selected", "");
        return false;
    }
    XYCoordinates.resize(_filteredSelectionIndices.size() * 2);
    std::vector<int> preComputedEmbeddingColumnIndices{ xEmbeddingIndex, yEmbeddingIndex };
    if (XYCoordinates.size() != _filteredSelectionIndices.size() * 2) {
        qDebug() << "XYCoordinates size mismatch";
        setStatusMessage("E", "XYCoordinates size mismatch", "");
        return false;
    }
    _embeddingDataset->populateDataForDimensions(XYCoordinates, preComputedEmbeddingColumnIndices, _filteredSelectionIndices);
    return true;
}

//Populate and update datasets
bool Computation::populateAndUpdateDatasets(
    mv::Dataset<Points>& fullPointDataset,
    const std::vector<float>& rotatedXYCordinates,
    const std::vector<float>& XYCordinates,
    int noOfDimensions,
    const std::vector<int>& columnIndices,
    const std::vector<QString>& dimensionNames)
{

    if (!_selectedCellPointDataset.isValid() ||
        !_rotatedSelectedEmbeddingDataset.isValid() || !_selectedGenePointDataset.isValid() ||
        !_selectedEmbeddingDataset.isValid()) {
        qDebug() << "One or more datasets are invalid!";
        setStatusMessage("E", "Internal error: invalid dataset pointer", "");
        return false;
    }
    if (_filteredSelectionIndices.empty() ||
        columnIndices.empty() ||
        dimensionNames.size() != static_cast<size_t>(noOfDimensions)) {
        qDebug() << "Invalid input sizes!";
        setStatusMessage("E", "Internal error: invalid input sizes", "");
        return false;
    }

    std::vector<float> cellbygenematrix(_filteredSelectionIndices.size() * columnIndices.size());
    try {
        fullPointDataset->populateDataForDimensions(cellbygenematrix, columnIndices, _filteredSelectionIndices);
    }
    catch (const std::exception& ex) {
        qDebug() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }

    _selectedCellPointDataset->setData(cellbygenematrix.data(), _filteredSelectionIndices.size(), noOfDimensions);
    _selectedCellPointDataset->setDimensionNames(dimensionNames);
    mv::events().notifyDatasetDataChanged(_selectedCellPointDataset);

    _rotatedSelectedEmbeddingDataset->setData(rotatedXYCordinates.data(), _filteredSelectionIndices.size(), 2);
    _rotatedSelectedEmbeddingDataset->setDimensionNames({ "X","Y" });
    mv::events().notifyDatasetDataChanged(_rotatedSelectedEmbeddingDataset);

    _selectedEmbeddingDataset->setData(XYCordinates.data(), _filteredSelectionIndices.size(), 2);
    _selectedEmbeddingDataset->setDimensionNames({ "X","Y" });
    mv::events().notifyDatasetDataChanged(_selectedEmbeddingDataset);

    return true;
}

// Setup and trigger PCA
//bool Computation::setupAndTriggerPCA(int noOfDimensions)
//{
//    
//    if (!_pcaDataset.isValid()) {
//        _pcaDataset = mv::data().createDataset("Points", "PCA Data");
//        _pcaDataset->setGroupIndex(666);
//        mv::events().notifyDatasetAdded(_pcaDataset);
//    }
//
//    if (!_pcaDataset.isValid()) {
//        qDebug() << "Failed to create PCA dataset";
//        setStatusMessage("E", "Error occurred during dataset creation", "");
//        return false;
//    }
//
//
//    std::vector<float> emptyData(_filteredSelectionIndices.size() * noOfDimensions, 0.0f);
//    _pcaDataset->setData(emptyData.data(), _filteredSelectionIndices.size(), noOfDimensions);
//    mv::events().notifyDatasetDataChanged(_pcaDataset);
//
//    if (!_pcaAnalysisPlugin) {
//        _pcaAnalysisPlugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedPointDataset }, { _pcaDataset });
//    }
//
//    if (!_pcaAnalysisPlugin) {
//        qDebug() << "PCA Analysis Plugin not found";
//        setStatusMessage("E", "PCA plugin not found", "");
//        return false;
//    }
//
//
//    
//    auto componentsAction = dynamic_cast<IntegralAction*>(_pcaDataset->findChildByPath("PCA/Number of PCA components"));
//    if (!componentsAction) {
//        qDebug() << "Number of PCA components action not found";
//        setStatusMessage("E", "PCA components action not found", "");
//        return false;
//    }
//    componentsAction->setValue(noOfDimensions);
//
//    auto startAction = dynamic_cast<TriggerAction*>(_pcaDataset->findChildByPath("PCA/Start analysis"));
//    if (startAction) {
//        startAction->trigger();
//        qDebug() << "PCA analysis started";
//        QEventLoop loop;
//        loop.processEvents();
//        _pcaDataset->setSelectionIndices({});
//    }
//    else {
//        qDebug() << "Start analysis action not found in PCA dataset";
//        setStatusMessage("E", "PCA start action not found", "");
//        return false;
//    }
//    qDebug() << "PCA analysis triggered successfully";
//    setStatusMessage("C", "PCA analysis completed", "#28a745");
//    return true;
//}

bool Computation::setupAndTriggerPCA()
{
    // noOfDimensions is the number of genes of interest, not the full dataset dimensions
    // compute PCA on all cells w. ith selected genes

    Dataset<Points> fullPointDataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());

    if (!_selectedGenePointDataset.isValid() || !fullPointDataset.isValid()) {
        qDebug() << "One or more datasets are invalid!";
        setStatusMessage("E", "Internal error: invalid dataset pointer", "");
        return false;
    }

    auto numAllCells = fullPointDataset->getNumPoints();
    auto numofIntGenes = _geneOfInterestIndices.size();
    qDebug() << "Number of all cells:" << numAllCells << "Number of dimensions of interest:" << numofIntGenes;

    // create point dataset subset with all cells and genes of interest
    std::vector<int> allCellIndices(numAllCells);
    std::iota(allCellIndices.begin(), allCellIndices.end(), 0);
    std::vector<float> cellbygenematrix(numAllCells * numofIntGenes);
    try {
        fullPointDataset->populateDataForDimensions(cellbygenematrix, _geneOfInterestIndices, allCellIndices);
    }
    catch (const std::exception& ex) {
        qDebug() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }

    _selectedGenePointDataset->setData(cellbygenematrix.data(), numAllCells, numofIntGenes);
    std::vector<QString> dimensionNamesOfInterest;
    for (int idx : _geneOfInterestIndices) {
        if (idx >= 0 && idx < fullPointDataset->getNumDimensions()) {
            dimensionNamesOfInterest.push_back(fullPointDataset->getDimensionNames()[idx]);
        }
    }

    _selectedGenePointDataset->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset);

    if (!_pcaDataset.isValid()) {
        _pcaDataset = mv::data().createDataset("Points", "PCA Data", _selectedGenePointDataset);
        _pcaDataset->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaDataset);
        _pcaAnalysisPlugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset }, { _pcaDataset });
    }

    std::vector<float> emptyData(numAllCells * numofIntGenes, 0.0f);
    _pcaDataset->setData(emptyData.data(), numAllCells, numofIntGenes);
    mv::events().notifyDatasetDataChanged(_pcaDataset);

    if (numofIntGenes <= 0) {
        qDebug() << "No genes of interest selected for PCA";
        setStatusMessage("E", "No genes of interest selected for PCA", "");
        return false;
    }

    if (!_pcaAnalysisPlugin) {
        _pcaAnalysisPlugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset }, { _pcaDataset });
    }

    if (!_pcaAnalysisPlugin) {
        qDebug() << "PCA Analysis Plugin not found";
        setStatusMessage("E", "PCA plugin not found", "");
        return false;
    }

    auto componentsAction = dynamic_cast<IntegralAction*>(_pcaDataset->findChildByPath("PCA/Number of PCA components"));
    if (!componentsAction) {
        qDebug() << "Number of PCA components action not found";
        setStatusMessage("E", "PCA components action not found", "");
        return false;
    }
    componentsAction->setMaximum(numofIntGenes);
    componentsAction->setValue(numofIntGenes);

    QApplication::processEvents();

    auto startAction = dynamic_cast<TriggerAction*>(_pcaDataset->findChildByPath("PCA/Start analysis"));
    if (startAction) {
        startAction->trigger();
        qDebug() << "PCA analysis started";
        QEventLoop loop;
        loop.processEvents();
        _pcaDataset->setSelectionIndices({});
        mv::events().notifyDatasetDataSelectionChanged(_pcaDataset);
        
    }
    else {
        qDebug() << "Start analysis action not found in PCA dataset";
        setStatusMessage("E", "PCA start action not found", "");
        return false;
    }
    QApplication::processEvents();
    qDebug() << "PCA analysis triggered successfully";
    setStatusMessage("C", "PCA analysis completed", "#28a745");
    return true;
}

void Computation::resetupCorrelationTable()
{
    auto minRange =  _settingsAction.getSelectionOptionsHolder().getCorrelationRangeAction().getMinimum();
    auto maxRange =  _settingsAction.getSelectionOptionsHolder().getCorrelationRangeAction().getMaximum();

    _geneOfInterestIndices.clear();
    //change the _geneCorrValues isWithinRange flag based on the min and max range
    for (auto& gene : _geneCorrValues) {
        bool isWithinRange = (gene.correlationValue >= minRange && gene.correlationValue <= maxRange);

        gene.isWithinRange = isWithinRange;
        if (isWithinRange) {
            _geneOfInterestIndices.push_back(gene.geneIndex);
        }
    }

    setupTableModelAndSelection();
    //select the first row of the table
    {
        auto* tableView = _settingsAction.getTableViewAction();
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


// Setup table model and connect selection
void Computation::setupTableModelAndSelection(const QStringList& topGenes)
{
    QStandardItemModel* model = new QStandardItemModel(topGenes.size(), 1, _settingsAction.getTableViewAction());
    model->setHorizontalHeaderLabels(QStringList() << "Gene");
    for (int i = 0; i < topGenes.size(); ++i) {
        model->setItem(i, 0, new QStandardItem(topGenes.at(i)));
    }
    _settingsAction.getTableViewAction()->setModel(model);
    _settingsAction.getTableViewAction()->resizeColumnsToContents();
    _settingsAction.getTableViewAction()->resizeRowsToContents();
    _settingsAction.getTableViewAction()->setColumnWidth(0, 200);

    if (_settingsAction.getTableViewAction()->selectionModel())
        disconnect(_settingsAction.getTableViewAction()->selectionModel(), nullptr, this, nullptr);

    connect(
        _settingsAction.getTableViewAction()->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this](const QItemSelection& selected, const QItemSelection&) {
            auto model = _settingsAction.getTableViewAction()->model();
            if (!model) return;
            QModelIndexList selectedIndexes = _settingsAction.getTableViewAction()->selectionModel()->selectedIndexes();
            if (selectedIndexes.isEmpty()) return;
            const QModelIndex& index = selectedIndexes.first();
            if (!index.isValid()) return;
            QString value = model->data(index, Qt::DisplayRole).toString();
             _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
        }
    );
}

void Computation::setupTableModelAndSelection(
    const QStringList& topGenes,
    const std::vector<QString>& allGenes,
    const std::vector<float>& correlationVector)
{
    // Pair each gene with its correlation
    std::vector<std::pair<float, QString>> geneCorrPairs;
    for (size_t i = 0; i < allGenes.size(); ++i) {
        float corr = (i < correlationVector.size()) ? correlationVector[i] : 0.0f;
        geneCorrPairs.emplace_back(corr, allGenes[i]);
    }

    // Sort by correlation descending
    std::sort(geneCorrPairs.begin(), geneCorrPairs.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    QStandardItemModel* model = new QStandardItemModel(static_cast<int>(geneCorrPairs.size()), 2, _settingsAction.getTableViewAction());
    model->setHorizontalHeaderLabels(QStringList() << "Gene" << "Correlation");

    QSet<QString> topGeneSet(topGenes.begin(), topGenes.end());

    float minCorr = std::numeric_limits<float>::max();
    float maxCorr = std::numeric_limits<float>::lowest();
    for (const auto& pair : geneCorrPairs) {
        minCorr = std::min(minCorr, pair.first);
        maxCorr = std::max(maxCorr, pair.first);
    }

    for (int i = 0; i < static_cast<int>(geneCorrPairs.size()); ++i) {
        const QString& gene = geneCorrPairs[i].second;
        float corr = geneCorrPairs[i].first;

        QStandardItem* geneItem = new QStandardItem(gene);
        QStandardItem* corrItem = new QStandardItem;
        corrItem->setData(QVariant::fromValue(corr), Qt::UserRole + 1); // Store value for delegate
        corrItem->setData(QVariant::fromValue(corr), Qt::DisplayRole);  // Set value for sorting

        if (topGeneSet.contains(gene)) {
            QFont font = geneItem->font();
            font.setBold(true);
            geneItem->setFont(font);
            geneItem->setBackground(QColor("#FFFACD"));
            geneItem->setForeground(QColor("#000000"));
        }

        model->setItem(i, 0, geneItem);
        model->setItem(i, 1, corrItem);
    }

    _settingsAction.getTableViewAction()->setModel(model);
    _settingsAction.getTableViewAction()->setItemDelegateForColumn(1, new CorrelationBarDelegate(minCorr, maxCorr, model));
    _settingsAction.getTableViewAction()->resizeColumnsToContents();
    _settingsAction.getTableViewAction()->resizeRowsToContents();
    _settingsAction.getTableViewAction()->setColumnWidth(0, 200);

    // Enable sorting
    _settingsAction.getTableViewAction()->setSortingEnabled(true);
    // Sort by correlation column (descending) by default
    _settingsAction.getTableViewAction()->sortByColumn(1, Qt::DescendingOrder);

    if (_settingsAction.getTableViewAction()->selectionModel())
        disconnect(_settingsAction.getTableViewAction()->selectionModel(), nullptr, this, nullptr);

    connect(
        _settingsAction.getTableViewAction()->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this](const QItemSelection& selected, const QItemSelection&) {
            auto model = _settingsAction.getTableViewAction()->model();
            if (!model) return;
            QModelIndexList selectedIndexes = _settingsAction.getTableViewAction()->selectionModel()->selectedIndexes();
            if (selectedIndexes.isEmpty()) return;
            const QModelIndex& index = selectedIndexes.first();
            if (!index.isValid()) return;
            QString value = model->data(model->index(index.row(), 0), Qt::DisplayRole).toString();
             _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
        }
    );
}



void Computation::setupTableModelAndSelection()
{
    QStandardItemModel* model = new QStandardItemModel(static_cast<int>(_geneCorrValues.size()), 3, _settingsAction.getTableViewAction());
    model->setHorizontalHeaderLabels(QStringList() << "Gene" << "Correlation" << "Rank");

    float minCorr = std::numeric_limits<float>::max();
    float maxCorr = std::numeric_limits<float>::lowest();

    for (const auto& entry : _geneCorrValues) {
        minCorr = std::min(minCorr, entry.correlationValue);
        maxCorr = std::max(maxCorr, entry.correlationValue);
    }

    for (int i = 0; i < static_cast<int>(_geneCorrValues.size()); ++i) {
        const auto& entry = _geneCorrValues[i];

        QStandardItem* geneItem = new QStandardItem(entry.geneName);
        QStandardItem* corrItem = new QStandardItem;
        QStandardItem* rankItem = new QStandardItem(QString::number(entry.rank));

        corrItem->setData(QVariant::fromValue(entry.correlationValue), Qt::UserRole + 1); // For delegate
        corrItem->setData(QVariant::fromValue(entry.correlationValue), Qt::DisplayRole);  // For sorting

        // Consistent formatting: only highlight and bold if isWithinRange
        if (entry.isWithinRange) {
            QFont fontG = geneItem->font();
            fontG.setBold(true);
            geneItem->setFont(fontG);
            geneItem->setBackground(QColor("#FFFACD")); // LemonChiffon (same as reference)
            geneItem->setForeground(QColor("#000000"));
            QFont fontR = rankItem->font();
            fontR.setBold(true);
            rankItem->setFont(fontR);
            rankItem->setBackground(QColor("#FFFACD")); // LemonChiffon (same as reference)
            rankItem->setForeground(QColor("#000000"));


        }
        // Always set text color for visibility
        corrItem->setForeground(QColor("#000000"));

        model->setItem(i, 0, geneItem);
        model->setItem(i, 1, corrItem);
        model->setItem(i, 2, rankItem);
    }

    _settingsAction.getTableViewAction()->setModel(model);
    _settingsAction.getTableViewAction()->setItemDelegateForColumn(1, new CorrelationBarDelegate(minCorr, maxCorr, model));
    _settingsAction.getTableViewAction()->resizeColumnsToContents();
    _settingsAction.getTableViewAction()->resizeRowsToContents();
    _settingsAction.getTableViewAction()->setColumnWidth(0, 200);

    // Enable sorting and set default sort order
    _settingsAction.getTableViewAction()->setSortingEnabled(true);
    _settingsAction.getTableViewAction()->sortByColumn(1, Qt::DescendingOrder);

    if (_settingsAction.getTableViewAction()->selectionModel())
        disconnect(_settingsAction.getTableViewAction()->selectionModel(), nullptr, this, nullptr);

    connect(
        _settingsAction.getTableViewAction()->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this](const QItemSelection& selected, const QItemSelection&) {
            auto model = _settingsAction.getTableViewAction()->model();
            if (!model) return;
            QModelIndexList selectedIndexes = _settingsAction.getTableViewAction()->selectionModel()->selectedIndexes();
            if (selectedIndexes.isEmpty()) return;
            const QModelIndex& index = selectedIndexes.first();
            if (!index.isValid()) return;
            QString value = model->data(model->index(index.row(), 0), Qt::DisplayRole).toString();
             _settingsAction.getSelectionOptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
        }
    );
}