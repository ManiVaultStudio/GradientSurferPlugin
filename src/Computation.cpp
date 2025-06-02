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

    const auto updateGeneSelection = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionIndex() == -1)
        {
            qDebug() << "No gene selected";
             _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }
        if (_settingsAction.getLineChart1Widget())
        {
            _settingsAction.getLineChart1Widget()->clear();
            _numOfPointsInLine = 0;

        }
        auto selectedGene =  _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene == "")
        {
            qDebug() << "No gene selected";
             _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }
        if ( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() && _settingsAction.getLineChart1Widget() && _filteredSelectionIndices.size() > 0)
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
                    _settingsAction.getLineChart1Widget()->setLines(lineData);
                    /*_settingsAction.getLineChart1Widget()->setBinaryNormalizeXThreshold(0.95f);
                    _settingsAction.getLineChart1Widget()->setBinaryNormalizeX(true);*/
                    //_settingsAction.getLineChart1Widget()->setShowAverageLine(true);
                   // _settingsAction.getLineChart1Widget()->setShowMedianLine(true);
                    //_settingsAction.getLineChart1Widget()->setShowPercentileLines(true);
                    //_settingsAction.getLineChart1Widget()->setShowStddevLines(true);
                    //_settingsAction.getLineChart1Widget()->setShowMaxLine(true);
                    //_settingsAction.getLineChart1Widget()->setShowMinLine(true);
                    _settingsAction.getLineChart1Widget()->setShowPoints(false);
                    _settingsAction.getLineChart1Widget()->setShowLines(false);
                    _settingsAction.getLineChart1Widget()->setMovingAverageWindow(5);
                    _settingsAction.getLineChart1Widget()->setShowMovingAverageLine(true);

                    //_settingsAction.getLineChart1Widget()->setShowKurtosisLine(true);
                    //_settingsAction.getLineChart1Widget()->setShowLowerQuartileLine(true);
                    //_settingsAction.getLineChart1Widget()->setShowMADLine(true);
                    //_settingsAction.getLineChart1Widget()->setShowModeLine(true);
                    //_settingsAction.getLineChart1Widget()->setShowSkewnessLine(true);
                    //_settingsAction.getLineChart1Widget()->setShowUpperQuartileLine(true);
                    //_settingsAction.getLineChart1Widget()->setShowVarianceLine(true);
                    //_settingsAction.getLineChart1Widget()->setGradientLineEnabled(true);
                    
                    _numOfPointsInLine = lineData.first().points.size();
                    _settingsAction.getLineChart1Widget()->setConnectStatsLineN(_numOfPointsInLine* _settingsAction.getLineChartOptionsHolder().getConnectStatsLineAction().getValue());
                    _settingsAction.getLineChart1Widget()->setConnectStatsLineType(HighPerfLineChart::ConnectStatsType::Median);
                    _settingsAction.getLineChart1Widget()->setConnectStatsLineEnabled(true);
                    _settingsAction.getLineChart1Widget()->setShowAvgNumPointsLabel(true);
                    _settingsAction.getLineChart1Widget()->setXAxisName("Rotated Y coordinates");
                    _settingsAction.getLineChart1Widget()->setYAxisName("Gene expression "+ _selectedGene);
   
                    _settingsAction.getLineChart1Widget()->setAxisFont(QFont("Arial", 12));
                    _settingsAction.getLineChart1Widget()->setTickLabelFont(QFont("Arial", 10));
                    _settingsAction.getLineChart1Widget()->setAxisColor(Qt::black);
                    _settingsAction.getLineChart1Widget()->setTickLabelColor(Qt::black);
                    _settingsAction.getLineChart1Widget()->setShowLegend(true); 
                    _settingsAction.getLineChart1Widget()->setShowAvgNumPointsLabel(true);
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

    connect(& _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, updateGeneSelection);

    const auto updateGeneSelection2 = [this]() -> void {

        auto selectedGene2 = _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().getCurrentDimensionName();
        if (selectedGene2 == "")
        {
            qDebug() << "No gene selected";
            _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction().setEnabled(false);
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
                                qDebug() << "Gene selection action not found";
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

        setStatusMessage("M", "Embedding dataset 1 updated", "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset1);

    const auto updateEmbeddingDataset2 = [this]() -> void {
        if (_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().isValid())
        {


            auto embeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId());

            if (embeddingDataset2.isValid())
            {
                _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setPointsDataset(embeddingDataset2);
                _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setPointsDataset(embeddingDataset2);
            }


        }

        setStatusMessage("M", "Embedding dataset 2 updated", "");
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

    const auto updateConnectStatsLine = [this]() -> void {
        if (!_settingsAction.getLineChart1Widget())
        {
            qDebug() << "No line chart widget available";
            return;
        }
        _settingsAction.getLineChart1Widget()->setConnectStatsLineN(_numOfPointsInLine* _settingsAction.getLineChartOptionsHolder().getConnectStatsLineAction().getValue());
        };

    connect(&_settingsAction.getLineChartOptionsHolder().getConnectStatsLineAction(), &DecimalAction::triggered, this, updateConnectStatsLine);


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

        setStatusMessage("M", "Embedding 1 Y dimension changed", "");
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

        setStatusMessage("M", "Embedding 2 Y dimension changed", "");
        QString scatterplotName = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName.isEmpty())
        {
            qDebug() << "No scatterplot view 2 selected";
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
                                        qDebug() << "Embedding 2 X value not found in options";
                                    }

                                    auto yoptions2 = embeddingYVal2->getOptions();
                                    if (yoptions2.contains(yPosition2)) {
                                        embeddingYVal2->setCurrentText(yPosition2);
                                    }
                                    else {
                                        qDebug() << "Embedding 2 Y value not found in options";
                                    }
                                }
                                else {
                                    qDebug() << "Embedding 2 X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
                                if (dataset2.isValid())
                                {
                                    colorDatasetPickerAction->setCurrentDataset(dataset2);
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
                                        qDebug() << "Embedding 2 X value not found in options";
                                    }

                                    auto yoptions2 = embeddingYVal2->getOptions();
                                    if (yoptions2.contains(yPosition2)) {
                                        embeddingYVal2->setCurrentText(yPosition2);
                                    }
                                    else {
                                        qDebug() << "Embedding 2 Y value not found in options";
                                    }
                                }
                                else {
                                    qDebug() << "Embedding 2 X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
                                if (dataset2.isValid())
                                {
                                    colorDatasetPickerAction->setCurrentDataset(dataset2);
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

    const auto deselectRow = [this]() -> void {
        auto selectedIndex = _settingsAction.getTableView1Action()->currentIndex();
        if (selectedIndex.isValid())
        {
            _settingsAction.getTableView1Action()->selectionModel()->clearSelection();
            _settingsAction.getTableView1Action()->setCurrentIndex(QModelIndex());
             _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName("");
             _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionName("");
        }
        else
        {
            qDebug() << "No row selected";
        }
         _settingsAction.getSelectionOptionsHolder().getDeSelectRowAction().setEnabled(false);
         _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);
         _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionIndex(-1);

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
                else if (plugin->getGuiName() == _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText())
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
                                        qDebug() << "Embedding 2 X value not found in options";
                                    }

                                    auto yoptions2 = embeddingYVal2->getOptions();
                                    if (yoptions2.contains(yPosition2)) {
                                        embeddingYVal2->setCurrentText(yPosition2);
                                    }
                                    else {
                                        qDebug() << "Embedding 2 Y value not found in options";
                                    }
                                }
                                else {
                                    qDebug() << "Embedding 2 X or Y value is null";
                                }
                            }

                            colorDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Color"));
                            if (colorDatasetPickerAction)
                            {
                                auto dataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
                                if (dataset2.isValid())
                                {
                                    colorDatasetPickerAction->setCurrentDataset(dataset2);
                                    qDebug() << "Setting dataset to color dataset";
                                }

                            }

                        }



                    }

                }
            }
        }
        _settingsAction.getLineChart1Widget()->clear();
        _numOfPointsInLine = 0;
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


void Computation::AutoCompute()
{
    try
    {
        // Remove all rows in one call if model exists
        if (auto* model = _settingsAction.getTableView1Action()->model()) {
            int rowCount = model->rowCount();
            if (rowCount > 0)
                model->removeRows(0, rowCount);
        }
        _settingsAction.getLineChart1Widget()->clear();
        _numOfPointsInLine = 0;
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
            auto* tableView = _settingsAction.getTableView1Action();
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
        corrVector[geneIdx] = corr;//std::abs(corr); // Use absolute correlation for ranking
    }

    qDebug() << "Correlation computation completed";
}

void Computation::computeCorrelationGenePC(const mv::Dataset<Points>& fullDataset, const std::vector<float>& selectedPC, std::vector<float>& corrVector)
{
    // check
    const int numGenes = fullDataset->getNumDimensions();
    const int numCells = fullDataset->getNumPoints();
    const int numCellsInPC = selectedPC.size();
    qDebug() << "Number of genes:" << numGenes << "Number of cells:" << numCells << "Number of cells in PC:" << numCellsInPC;
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
    if (!_selectedEmbeddingDataset.isValid()) {
        _selectedEmbeddingDataset = mv::data().createDataset("Points", "Selected Embedding Data", _selectedCellPointDataset);
        _selectedEmbeddingDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_selectedEmbeddingDataset);
    }

    if (!_rotatedSelectedEmbeddingDataset.isValid() || !_selectedGenePointDataset1.isValid() || !_selectedGenePointDataset2.isValid() || !_selectedEmbeddingDataset.isValid() || !_selectedCellPointDataset.isValid()) {
        qDebug() << "Failed to create datasets";
        setStatusMessage("E", "Error occurred during dataset creation", "");
        return false;
    }

    return true;
}

void Computation::updateClusterOptions()
{
    QStringList clusterNamesCommon = {};
    QStringList clusterNames1 = {};
    QStringList clusterNames2 = {};
    if (_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().isValid() )
    {
        auto clusterFullDataset1 = mv::data().getDataset<Clusters>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
        auto clusters1 = clusterFullDataset1->getClusters();
        
        if (!clusters1.isEmpty())
        {
            for (auto cluster : clusters1)
            {
                clusterNames1.append(cluster.getName());
            }
        }

    }
    if (_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().isValid())
    {
        auto clusterFullDataset2 = mv::data().getDataset<Clusters>(_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
        auto clusters2 = clusterFullDataset2->getClusters();

        if (!clusters2.isEmpty())
        {
            for (auto cluster : clusters2)
            {
                clusterNames2.append(cluster.getName());
            }
        }

    }
   
    std::sort(clusterNames1.begin(), clusterNames1.end());
    std::sort(clusterNames2.begin(), clusterNames2.end());
    std::set_intersection(clusterNames1.begin(), clusterNames1.end(),
        clusterNames2.begin(), clusterNames2.end(),
        std::back_inserter(clusterNamesCommon));


    _settingsAction.getSelectionOptionsHolder().getClusterSelectionAction().setOptions(clusterNamesCommon);
    _settingsAction.getSelectionOptionsHolder().getClusterSelectionAction().setSelectedOptions(clusterNamesCommon);
}

void Computation::processAfterPCAComplete()
{
    _pcaDataset1Complete = false;
    _pcaDataset2Complete = false;
    //wait for the two pca finished events to complete std::cout << "PCA Plugin: Finished." << std::endl;
    qDebug() << "PCA analysis completed successfully";
    setStatusMessage("C", "PCA analysis completed", "#28a745");
    setStatusMessage("R", "Running correlation between genes and PC", "#28a745");
    Dataset<Points> fullDataset2 = _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset();

    int selectedPCDimension2 = 0;
    int numAllCells2 = fullDataset2->getNumPoints();

    std::vector<float> selectedPC(numAllCells2);
    std::vector<int> allCellIndices2(numAllCells2);
    std::iota(allCellIndices2.begin(), allCellIndices2.end(), 0);
    _pcaCommonGenesDataset2->extractDataForDimension(selectedPC, selectedPCDimension2);

    // compute the correlation between genes and selected PC
    std::vector<float> correlationGenePC;
    computeCorrelationGenePC(fullDataset2, selectedPC, correlationGenePC);
    setStatusMessage("C", "Correlation between genes and PC completed", "#28a745");
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
        !_rotatedSelectedEmbeddingDataset.isValid() || !_selectedGenePointDataset1.isValid() || !_selectedGenePointDataset2.isValid() ||
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


bool Computation::setupAndTriggerPCA()
{
    // noOfDimensions is the number of genes of interest, not the full dataset dimensions
    // compute PCA on all cells w. ith selected genes

    Dataset<Points> fullPointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    Dataset<Points> fullPointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId());

    if (!_selectedGenePointDataset1.isValid() || !fullPointDataset1.isValid()|| !_selectedGenePointDataset2.isValid() || !fullPointDataset2.isValid()) {
        qDebug() << "One or more datasets are invalid!";
        setStatusMessage("E", "Internal error: invalid dataset pointer", "");
        return false;
    }

    auto numAllCells1 = fullPointDataset1->getNumPoints();
    auto numAllCells2 = fullPointDataset2->getNumPoints();
    auto numofIntGenes = _geneOfInterestIndices.size();
    auto dimensionNames1 = fullPointDataset1->getDimensionNames();
    auto dimensionNames2 = fullPointDataset2->getDimensionNames();

    if (numAllCells1 <= 0 || numAllCells2 <= 0 || numofIntGenes <= 0) {
        qDebug() << "Invalid dataset sizes: numAllCells1:" << numAllCells1 << "numAllCells2:" << numAllCells2 << "numofIntGenes:" << numofIntGenes;
        setStatusMessage("E", "Invalid dataset sizes", "");
        return false;
    }
    if (numofIntGenes > dimensionNames1.size() || numofIntGenes > dimensionNames2.size()) {
        qDebug() << "Number of genes of interest exceeds dataset dimensions";
        setStatusMessage("E", "Number of genes of interest exceeds dataset dimensions", "");
        return false;
    }
    std::vector<QString> dimensionNamesOfInterest;
    std::vector<int> geneDataset1;
    std::vector<int> geneDataset2;
    for (int idx : _geneOfInterestIndices) {
        QString geneName = (idx >= 0 && idx < dimensionNames1.size()) ? dimensionNames1[idx] : QString();
        //check if geneName is present in std::vector<QString> dimensionNames2 if yes then get the gene id
        auto it2 = std::find(dimensionNames2.begin(), dimensionNames2.end(), geneName);
        if (it2 != dimensionNames2.end()) {
            int geneId2 = std::distance(dimensionNames2.begin(), it2);
            dimensionNamesOfInterest.push_back(geneName);
            geneDataset2.push_back(geneId2);
            geneDataset1.push_back(idx);

        } 
    }





    qDebug() << "Number of all cells dataset 1:" << numAllCells1 << "Number of all cells dataset 2:" << numAllCells2 << "Number of dimensions of interest:" << dimensionNamesOfInterest.size();

    // create point dataset subset with all cells and genes of interest
    std::vector<int> allCellIndices1(numAllCells1);
    std::iota(allCellIndices1.begin(), allCellIndices1.end(), 0);
    std::vector<float> cellbygenematrix1(numAllCells1 * dimensionNamesOfInterest.size());
    try {
        fullPointDataset1->populateDataForDimensions(cellbygenematrix1, geneDataset1, allCellIndices1);
    }
    catch (const std::exception& ex) {
        qDebug() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }

    std::vector<int> allCellIndices2(numAllCells2);
    std::iota(allCellIndices2.begin(), allCellIndices2.end(), 0);
    std::vector<float> cellbygenematrix2(numAllCells2 * dimensionNamesOfInterest.size());
    try {
        fullPointDataset2->populateDataForDimensions(cellbygenematrix2, geneDataset2, allCellIndices2);
    }
    catch (const std::exception& ex) {
        qDebug() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }

    _selectedGenePointDataset1->setData(cellbygenematrix1.data(), numAllCells1, dimensionNamesOfInterest.size());
    _selectedGenePointDataset2->setData(cellbygenematrix2.data(), numAllCells2, dimensionNamesOfInterest.size());

    _selectedGenePointDataset1->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset1);

    _selectedGenePointDataset2->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset2);

    if (!_pcaCommonGenesDataset1.isValid()) {
        _pcaCommonGenesDataset1 = mv::data().createDataset("Points", "PCA Dataset 1", _selectedGenePointDataset1);
        _pcaCommonGenesDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset1);
        _pcaAnalysisCommonGenesDataset1Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset1 }, { _pcaCommonGenesDataset1 });
    }

    if (!_pcaCommonGenesDataset2.isValid()) {
        _pcaCommonGenesDataset2 = mv::data().createDataset("Points", "PCA Dataset 2", _selectedGenePointDataset2);
        _pcaCommonGenesDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset2);
        _pcaAnalysisCommonGenesDataset2Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset2 }, { _pcaCommonGenesDataset2 });
    }

    std::vector<float> emptyData1(numAllCells1 * dimensionNamesOfInterest.size(), 0.0f);
    _pcaCommonGenesDataset1->setData(emptyData1.data(), numAllCells1, dimensionNamesOfInterest.size());
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset1);

    std::vector<float> emptyData2(numAllCells2 * dimensionNamesOfInterest.size(), 0.0f);
    _pcaCommonGenesDataset2->setData(emptyData2.data(), numAllCells2, dimensionNamesOfInterest.size());
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset2);
    _pcaDataset1Complete = false;
    _pcaDataset2Complete = false;
    disconnect(&_pcaCommonGenesDataset1, &Dataset<Points>::dataChanged, this, nullptr);
    disconnect(&_pcaCommonGenesDataset2, &Dataset<Points>::dataChanged, this, nullptr);
    connect(&_pcaCommonGenesDataset1, &Dataset<Points>::dataChanged, this, [this]() {
        _pcaDataset1Complete = true;
        qDebug() << "PCA Dataset 1 data changed";
        if (_pcaDataset1Complete && _pcaDataset2Complete) {
            processAfterPCAComplete();
        }
        else {
            qDebug() << "PCA Dataset 1 data changed, but PCA Dataset 2 is not complete yet.";
        }
        });
    connect(&_pcaCommonGenesDataset2, &Dataset<Points>::dataChanged, this, [this]() {
        _pcaDataset2Complete = true;
        qDebug() << "PCA Dataset 2 data changed";
        if (_pcaDataset1Complete && _pcaDataset2Complete) {
            processAfterPCAComplete();
        }
        else {
            qDebug() << "PCA Dataset 2 data changed, but PCA Dataset 1 is not complete yet.";
        }
        });

    if (dimensionNamesOfInterest.size() <= 0) {
        qDebug() << "No genes of interest selected for PCA";
        setStatusMessage("E", "No genes of interest selected for PCA", "");
        return false;
    }

    if (!_pcaAnalysisCommonGenesDataset1Plugin) {
        _pcaAnalysisCommonGenesDataset1Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset1 }, { _pcaCommonGenesDataset1 });
    }
    if (!_pcaAnalysisCommonGenesDataset2Plugin) {
        _pcaAnalysisCommonGenesDataset2Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset2 }, { _pcaCommonGenesDataset2 });
    }

    if (!_pcaAnalysisCommonGenesDataset1Plugin) {
        qDebug() << "PCA Analysis Plugin not found";
        setStatusMessage("E", "PCA plugin not found", "");
        return false;
    }
    if (!_pcaAnalysisCommonGenesDataset2Plugin) {
        qDebug() << "PCA Analysis Plugin not found";
        setStatusMessage("E", "PCA plugin not found", "");
        return false;
    }

    auto componentsAction1 = dynamic_cast<IntegralAction*>(_pcaCommonGenesDataset1->findChildByPath("PCA/Number of PCA components"));
    if (!componentsAction1) {
        qDebug() << "Number of PCA components action not found";
        setStatusMessage("E", "PCA components action not found", "");
        return false;
    }
    componentsAction1->setMaximum(numofIntGenes);
    componentsAction1->setValue(numofIntGenes);

    auto componentsAction2 = dynamic_cast<IntegralAction*>(_pcaCommonGenesDataset2->findChildByPath("PCA/Number of PCA components"));
    if (!componentsAction2) {
        qDebug() << "Number of PCA components action not found";
        setStatusMessage("E", "PCA components action not found", "");
        return false;
    }
    componentsAction2->setMaximum(numofIntGenes);
    componentsAction2->setValue(numofIntGenes);

    QApplication::processEvents();

    auto startAction1 = dynamic_cast<TriggerAction*>(_pcaCommonGenesDataset1->findChildByPath("PCA/Start analysis"));
    auto startAction2 = dynamic_cast<TriggerAction*>(_pcaCommonGenesDataset2->findChildByPath("PCA/Start analysis"));
    if (startAction1) {
        startAction1->trigger();
        qDebug() << "PCA analysis 1 started";
        QEventLoop loop;
        loop.processEvents();
        _pcaCommonGenesDataset1->setSelectionIndices({});
        mv::events().notifyDatasetDataSelectionChanged(_pcaCommonGenesDataset1);
        
    }
    else {
        qDebug() << "Start analysis action not found in PCA dataset";
        setStatusMessage("E", "PCA start action not found", "");
        return false;
    }
    if (startAction2) {
        startAction2->trigger();
        qDebug() << "PCA analysis 2 started";
        QEventLoop loop;
        loop.processEvents();
        _pcaCommonGenesDataset2->setSelectionIndices({});
        mv::events().notifyDatasetDataSelectionChanged(_pcaCommonGenesDataset2);

    }
    else {
        qDebug() << "Start analysis action not found in PCA dataset";
        setStatusMessage("E", "PCA start action not found", "");
        return false;
    }
    QApplication::processEvents();

    
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
        auto* tableView = _settingsAction.getTableView1Action();
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

void Computation::setupTableModelAndSelection()
{
    QStandardItemModel* model = new QStandardItemModel(static_cast<int>(_geneCorrValues.size()), 3, _settingsAction.getTableView1Action());
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

    _settingsAction.getTableView1Action()->setModel(model);
    _settingsAction.getTableView1Action()->setItemDelegateForColumn(1, new CorrelationBarDelegate(minCorr, maxCorr, model));
    _settingsAction.getTableView1Action()->resizeColumnsToContents();
    _settingsAction.getTableView1Action()->resizeRowsToContents();
    //_settingsAction.getTableView1Action()->setColumnWidth(0, 100);

    // Enable sorting and set default sort order
    _settingsAction.getTableView1Action()->setSortingEnabled(true);
    _settingsAction.getTableView1Action()->sortByColumn(1, Qt::DescendingOrder);

    if (_settingsAction.getTableView1Action()->selectionModel())
        disconnect(_settingsAction.getTableView1Action()->selectionModel(), nullptr, this, nullptr);

    connect(
        _settingsAction.getTableView1Action()->selectionModel(),
        &QItemSelectionModel::selectionChanged,
        this,
        [this](const QItemSelection& selected, const QItemSelection&) {
            auto model = _settingsAction.getTableView1Action()->model();
            if (!model) return;
            QModelIndexList selectedIndexes = _settingsAction.getTableView1Action()->selectionModel()->selectedIndexes();
            if (selectedIndexes.isEmpty()) return;
            const QModelIndex& index = selectedIndexes.first();
            if (!index.isValid()) return;
            QString value = model->data(model->index(index.row(), 0), Qt::DisplayRole).toString();
             _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
             _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionName(value);
        }
    );
}