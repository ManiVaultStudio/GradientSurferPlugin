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

            // --- Begin: Modified normalization and bar logic for negative/positive values ---
            float minV = minValue, maxV = maxValue;
            if (minV == maxV) {
                minV = -1.0f; maxV = 1.0f;
            }
            value = std::max(std::min(value, maxV), minV);

            QRect barRect = option.rect.adjusted(4, 4, -4, -4);

            float zeroFrac = (0.0f - minV) / (maxV - minV);
            int zeroX = barRect.left() + static_cast<int>(zeroFrac * barRect.width());

            int barStartX, barEndX;
            int valueX = barRect.left() + static_cast<int>(((value - minV) / (maxV - minV)) * barRect.width());
            if (value >= 0.0f) {
                barStartX = zeroX;
                barEndX = valueX;
            }
            else {
                barStartX = valueX;
                barEndX = zeroX;
            }
            if (barStartX > barEndX) std::swap(barStartX, barEndX);

            QRect filledRect = barRect;
            filledRect.setLeft(barStartX);
            filledRect.setRight(barEndX);

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

            // --- Dotted y-axis line: white on black bg, black on white bg, else contrast ---
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
                // Use a high-contrast color (black or white) based on luminance
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
            painter->drawLine(zeroX, axisTop, zeroX, axisBottom);

            // --- Bar color: not blue, contrast to both axis and background ---
            QColor barColor;
            // Pick a color that is not blue, and is contrastive to both bg and axis
            // Use orange for dark backgrounds, purple for light backgrounds, fallback to green
            if (bgIsBlack || axisColor == QColor(Qt::white)) {
                barColor = QColor(255, 140, 0); // Orange
            }
            else if (bgIsWhite || axisColor == QColor(Qt::black)) {
                barColor = QColor(128, 0, 128); // Purple
            }
            else {
                // Fallback: pick a color with high contrast to both
                double axisLuminance = (0.299 * axisColor.red() + 0.587 * axisColor.green() + 0.114 * axisColor.blue()) / 255.0;
                if (axisLuminance > 0.5)
                    barColor = QColor(0, 128, 0); // Green
                else
                    barColor = QColor(255, 165, 0); // Orange
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
            // --- End: Modified normalization and bar logic ---
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

    const auto updateGeneSelection1 = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionIndex() == -1)
        {
            qDebug() << "No gene selected";
             _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }
        if (_settingsAction.getLineChart1Widget())
        {
            _settingsAction.getLineChart1Widget()->clear();
            _numOfPointsInLine1 = 0;

        }
        auto selectedGene =  _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene == "")
        {
            qDebug() << "No gene selected";
             _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().setEnabled(false);
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
                    _settingsAction.getLineChart1Widget()->setShowPoints(false);
                    _settingsAction.getLineChart1Widget()->setShowLines(false);

                    _numOfPointsInLine1 = lineData.first().points.size();
                    _settingsAction.getLineChart1Widget()->setConnectStatsLineN(_numOfPointsInLine1* _settingsAction.getDataset1OptionsHolder().getConnectStatsLineAction().getValue());
                    _settingsAction.getLineChart1Widget()->setConnectStatsLineType(HighPerfLineChart::ConnectStatsType::Mean);
                    _settingsAction.getLineChart1Widget()->setConnectStatsLineEnabled(true);
                    
                    _settingsAction.getLineChart1Widget()->setShowAvgNumPointsLabel(true);
   
   

                    _settingsAction.getLineChart1Widget()->setMovingAverageWindow(std::max(1, int(_numOfPointsInLine1 * _settingsAction.getDataset1OptionsHolder().getConnectStatsLineAction().getValue())));
                    _settingsAction.getLineChart1Widget()->setMovingAverageWindow(10);
                    _settingsAction.getLineChart1Widget()->setShowMovingAverageLine(true);
                    mv::theme().isSystemDarkColorSchemeActive() ?
                        _settingsAction.getLineChart1Widget()->setBackgroundColor(Qt::black) :
                        _settingsAction.getLineChart1Widget()->setBackgroundColor(Qt::white);
                    _settingsAction.getLineChart1Widget()->setAxisFont(QFont("Arial", 12));
                    _settingsAction.getLineChart1Widget()->setTickLabelFont(QFont("Arial", 10));
                    mv::theme().isSystemDarkColorSchemeActive() ?
                        _settingsAction.getLineChart1Widget()->setAxisColor(Qt::white) :
                        _settingsAction.getLineChart1Widget()->setAxisColor(Qt::black);
                    // Fix tick label color for correct contrast
                    mv::theme().isSystemDarkColorSchemeActive() ?
                        _settingsAction.getLineChart1Widget()->setTickLabelColor(Qt::white) :
                        _settingsAction.getLineChart1Widget()->setTickLabelColor(Qt::black);
                    _settingsAction.getLineChart1Widget()->setXAxisName("Rotated Y coordinates");
                    _settingsAction.getLineChart1Widget()->setYAxisName("Gene expression " + _selectedGene1);
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
                                qDebug() << "Gene selection action not found";
                            }
                        }



                    }

                }
            }
        }
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, updateGeneSelection1);



    const auto updateGeneSelection2 = [this]() -> void {

        auto selectedGene2 = _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().getCurrentDimensionName();
        if (selectedGene2 == "")
        {
            qDebug() << "No gene selected";
            _settingsAction.getDataset2OptionsHolder().getDeSelectRow2Action().setEnabled(false);
            return;
        }

        auto selectedGene = _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene == "")
        {
            qDebug() << "No gene selected";
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
                        auto selectedClusters =  _settingsAction.getCommonOptionsHolder().getClusterSelectionAction().getSelectedOptions();
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
                    _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
                    QApplication::processEvents();

                    AutoCompute();

                    _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);


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

        setStatusMessage("R", "Correlation computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        correlationAfterPCA(); 

        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        };

    connect(&_settingsAction.getCommonOptionsHolder().getComputeAction(), &TriggerAction::triggered, this, computeInfo);


    const auto computePCATrigger = [this]() -> void {

        setStatusMessage("R", "PCA Computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputePCAAction().setDisabled(true);
        QApplication::processEvents();
  
        ManualCompute();

        _settingsAction.getCommonOptionsHolder().getComputePCAAction().setDisabled(false);
        };

    connect(&_settingsAction.getCommonOptionsHolder().getComputePCAAction(), &TriggerAction::triggered, this, computePCATrigger);

    const auto pcaDimensionSelectionChange = [this]() -> void {

        correlationAfterPCA();
        };

    connect(&_settingsAction.getCommonOptionsHolder().getDataset1PcSelectionForComputationAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, pcaDimensionSelectionChange);


    const auto pcaColorsToScatterplot = [this]() -> void {
        int selectedDimInd = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();
        QString selectedDimName = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionName();
        if (selectedDimInd < 0 || selectedDimName.isEmpty()) {
            qDebug() << "No PCA dimension selected for coloring";
            return;
        }
        auto options = _settingsAction.getCommonOptionsHolder().getDualPCAColoringAction().getSelectedOptions();
        if (options.isEmpty()) {
            qDebug() << "No PCA coloring options selected";
            return;
        }

        auto setScatterplotColorAndGene = [&](const QString& scatterplotName, mv::Dataset<Points>& dataset) {
            if (!dataset.isValid()) {
                qDebug() << "PCA common genes dataset is not valid";
                return false;
            }
            if (scatterplotName.isEmpty()) {
                qDebug() << "No scatterplot view selected";
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
                            qDebug() << "Gene selection action not found";
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
            if (options.contains("Dataset1") && options.contains("Dataset2")) {
                _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().trigger();
                _settingsAction.getDataset2OptionsHolder().getDeSelectRow2Action().trigger();
            }
            else if (options.contains("Dataset1")) {
                _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().trigger();
            }
            else if (options.contains("Dataset2")) {
                _settingsAction.getDataset2OptionsHolder().getDeSelectRow2Action().trigger();
            }
        }
        };

    connect(&_settingsAction.getCommonOptionsHolder().getPcaColoringAction(), &ToggleAction::toggled, this, pcaColorsToScatterplot);


    connect(&_settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, pcaColorsToScatterplot);

    const auto updateConnectStatsLine1 = [this]() -> void {
        if (!_settingsAction.getLineChart1Widget())
        {
            qDebug() << "No line chart widget available";
            return;
        }
        _settingsAction.getLineChart1Widget()->setConnectStatsLineN(_numOfPointsInLine1* _settingsAction.getDataset1OptionsHolder().getConnectStatsLineAction().getValue());
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getConnectStatsLineAction(), &DecimalAction::triggered, this, updateConnectStatsLine1);


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
            _correlationRangeDebounceTimer1.start(300);
        });
    connect(&_correlationRangeDebounceTimer1, &QTimer::timeout, this, [this]() {
        setStatusMessage("R", "Updating correlation 1 table", "");
        resetupCorrelationTable(1);
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
        setStatusMessage("C", "Correlation table 2 updated", "");
        });

    const auto showPointSelectionChanged1 = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();


        if (scatterplotName1.isEmpty())
        {
            qDebug() << "No scatterplot view 1 selected";

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
                        qDebug() << "Point selection scale action not found";
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
                        qDebug() << "Point selection opacity action not found";
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
            qDebug() << "No scatterplot view 1 selected";

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
                        qDebug() << "Freeze selection action not found";
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
            qDebug() << "No scatterplot view 1 selected";

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
                        qDebug() << "Focus selection action not found";
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
            qDebug() << "No scatterplot view 2 selected";

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
                        qDebug() << "Point selection scale action not found";
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
                        qDebug() << "Point selection opacity action not found";
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
            qDebug() << "No scatterplot view 2 selected";

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
                        qDebug() << "Freeze selection action not found";
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
            qDebug() << "No scatterplot view 2 selected";

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
                        qDebug() << "Focus selection action not found";
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

    const auto deselectRow1 = [this]() -> void {
        auto selectedIndex = _settingsAction.getTableView1Action()->currentIndex();
        if (selectedIndex.isValid())
        {
            _settingsAction.getTableView1Action()->selectionModel()->clearSelection();
            _settingsAction.getTableView1Action()->setCurrentIndex(QModelIndex());
             _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName("");
            
        }
        else
        {
            qDebug() << "No row selected";
        }
         _settingsAction.getDataset1OptionsHolder().getDeSelectRowAction().setEnabled(false);
         _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);
         

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
        _settingsAction.getLineChart1Widget()->clear();
        _numOfPointsInLine1 = 0;
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
            qDebug() << "No row selected";
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
       
        _numOfPointsInLine2 = 0;
        };

    connect(&_settingsAction.getDataset2OptionsHolder().getDeSelectRow2Action(), &TriggerAction::triggered, this, deselectRow2);
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

        
        try {
            populateGenesByCorrelationRange(corrGeneCoords, minCorrelation, maxCorrelation, dimensionNames,1);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            setStatusMessage("E", "Error occurred during filtering genes", "");
            return;
        }

        // Setup table model and connect selection
        setupTableModelAndSelection(1);
        //select the first row of the table


        setStatusMessage("C", "Computation completed", "#28a745");

        if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked())
        {
            setStatusMessage("R", "Computation in progress", "");
            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
            QApplication::processEvents();

            ManualCompute();
            correlationAfterPCA();

            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        }

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
    //mv::Dataset<Points> fullPointDataset = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());

    // Setup and trigger PCA
    if (!setupAndTriggerPCA())
        return;
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

        
        if (_settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction().getCurrentText()=="Absolute")
        {
            corrVector[geneIdx] = std::abs(corr); // Use absolute correlation for ranking
        }
        else 
        {
            corrVector[geneIdx] = corr;
        }
        
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

    auto selectedClusterValues = _settingsAction.getCommonOptionsHolder().getClusterSelectionAction().getSelectedOptions();
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

    minCorrelation = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMinimum();
    maxCorrelation = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMaximum();
    if (minCorrelation < -1.0f || maxCorrelation > 1.0f || minCorrelation >= maxCorrelation) {
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
    auto& geneCorrValues = (datasetNumber == 2) ? _geneCorrValues2 : _geneCorrValues1;
    auto& geneOfInterestIndices = (datasetNumber == 2) ? _geneOfInterestIndices2 : _geneOfInterestIndices1;

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

void Computation::processAfterPCAComplete()
{
    _pcaDataset1Complete = false;
    _pcaDataset2Complete = false;
    
    qDebug() << "PCA analysis completed successfully";


    {

        _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setPointsDataset(_pcaCommonGenesDataset2);
        _settingsAction.getCommonOptionsHolder().getDataset1PcSelectionForComputationAction().setPointsDataset(_pcaCommonGenesDataset2);
        if (_settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getNumberOfDimensions() > 0)
        {
            _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setCurrentDimensionIndex(0);
   
        }
        if (_settingsAction.getCommonOptionsHolder().getDataset1PcSelectionForComputationAction().getNumberOfDimensions() > 0)
        {

            _settingsAction.getCommonOptionsHolder().getDataset1PcSelectionForComputationAction().setCurrentDimensionIndex(0);
        }
    }

    if (_settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getNumberOfDimensions() < 0)
    {
        qDebug() << "No PCA dimensions available for coloring";
        setStatusMessage("E", "No PCA dimensions available for coloring", "");
        return;
    }

    setStatusMessage("C", "PCA analysis completed", "#28a745");
    
}
void Computation::correlationAfterPCA()
{

    setStatusMessage("R", "Running correlation between genes and PC", "#28a745");

    Dataset<Points> fullDataset2 = _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset();
    if (!fullDataset2.isValid()) {
        setStatusMessage("E", "Point dataset 2 is invalid", "");
        return;
    }
   

    int selectedPCDimension2 = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();
    if (selectedPCDimension2 < 0)
    {
                setStatusMessage("E", "No PCA dimension selected for dataset 2", "");
                return;
    }
    const int numAllCells2 = fullDataset2->getNumPoints();
    if (numAllCells2 <= 0) {
        setStatusMessage("E", "No cells in point dataset 2", "");
        return;
    }

    std::vector<float> selectedPC(numAllCells2);
    _pcaCommonGenesDataset2->extractDataForDimension(selectedPC, selectedPCDimension2);

    std::vector<float> correlationGenePC;
    computeCorrelationGenePC(fullDataset2, selectedPC, correlationGenePC);

    const float minCorrelation = _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMinimum();
    const float maxCorrelation = _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMaximum();
    const auto dimensionNames = fullDataset2->getDimensionNames();

    try {
        populateGenesByCorrelationRange(correlationGenePC, minCorrelation, maxCorrelation, dimensionNames, 2);
    }
    catch (const std::exception& ex) {
        qDebug() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
        setStatusMessage("E", "Error occurred during filtering genes", "");
        return;
    }

    setupTableModelAndSelection(2);

    setStatusMessage("C", "Correlation between genes and PC completed", "#28a745");
}
bool Computation::getRotationAngle(const QString& scatterplotViewName, float& rotationAngle)
{
    auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
    if (!scatterplotViewFactory) {
        qDebug() << "Scatterplot view factory not found";
        setStatusMessage("E", "Scatterplot view factory not found", "");
        return false;
    }

    // Use range-based for and early return for clarity and efficiency
    for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
        if (plugin->getGuiName() == scatterplotViewName) {
            auto* angleAction = dynamic_cast<DecimalAction*>(
                plugin->findChildByPath("Settings/Selection/Point Selection/Line angle"));
            if (!angleAction) {
                qDebug() << "Angle action not found";
                setStatusMessage("E", "Angle action not found", "");
                return false;
            }
            rotationAngle = angleAction->getValue();
            return true;
        }
    }

    qDebug() << "Scatterplot plugin not found";
    setStatusMessage("E", "Scatterplot plugin not found", "");
    return false;
}

bool Computation::populateEmbeddingCoordinates(int xEmbeddingIndex, int yEmbeddingIndex, std::vector<float>& XYCoordinates)
{
    if (_filteredSelectionIndices.empty()) {
        qDebug() << "No filtered selection indices";
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
        !_selectedGenePointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() ||
        !_selectedEmbeddingDataset.isValid() ||
        _filteredSelectionIndices.empty() ||
        columnIndices.empty() ||
        dimensionNames.size() != static_cast<size_t>(noOfDimensions))
    {
        qDebug() << "Invalid dataset pointers or input sizes!";
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
        qDebug() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
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

    return true;
}

bool Computation::setupAndTriggerPCA()
{
    // Compute PCA on all cells with selected genes of interest

    auto fullPointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto fullPointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId());

    if (!_selectedGenePointDataset1.isValid() || !fullPointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() || !fullPointDataset2.isValid()) {
        qDebug() << "One or more datasets are invalid!";
        setStatusMessage("E", "Internal error: invalid dataset pointer", "");
        return false;
    }

    const int numAllCells1 = fullPointDataset1->getNumPoints();
    const int numAllCells2 = fullPointDataset2->getNumPoints();
    const int numOfIntGenes = static_cast<int>(_geneOfInterestIndices1.size());
    const auto& dimensionNames1 = fullPointDataset1->getDimensionNames();
    const auto& dimensionNames2 = fullPointDataset2->getDimensionNames();

    if (numAllCells1 <= 0 || numAllCells2 <= 0 || numOfIntGenes <= 0) {
        qDebug() << "Invalid dataset sizes: numAllCells1:" << numAllCells1
            << "numAllCells2:" << numAllCells2
            << "numOfIntGenes:" << numOfIntGenes;
        setStatusMessage("E", "Invalid dataset sizes", "");
        return false;
    }
    if (numOfIntGenes > static_cast<int>(dimensionNames1.size()) ||
        numOfIntGenes > static_cast<int>(dimensionNames2.size())) {
        qDebug() << "Number of genes of interest exceeds dataset dimensions";
        setStatusMessage("E", "Number of genes of interest exceeds dataset dimensions", "");
        return false;
    }

    // Build gene index mapping and names of interest in a single pass
    std::vector<QString> dimensionNamesOfInterest;
    std::vector<int> geneDataset1, geneDataset2;
    dimensionNamesOfInterest.reserve(numOfIntGenes);
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
            geneDataset1.push_back(idx);
            geneDataset2.push_back(geneId2);
        }
    }

    const int numDimsOfInterest = static_cast<int>(dimensionNamesOfInterest.size());
    qDebug() << "Number of all cells dataset 1:" << numAllCells1
        << "Number of all cells dataset 2:" << numAllCells2
        << "Number of dimensions of interest:" << numDimsOfInterest;

    if (numDimsOfInterest <= 0) {
        qDebug() << "No genes of interest selected for PCA";
        setStatusMessage("E", "No genes of interest selected for PCA", "");
        return false;
    }

    // Prepare cell indices
    std::vector<int> allCellIndices1(numAllCells1), allCellIndices2(numAllCells2);
    std::iota(allCellIndices1.begin(), allCellIndices1.end(), 0);
    std::iota(allCellIndices2.begin(), allCellIndices2.end(), 0);

    // Prepare and fill gene expression matrices
    std::vector<float> cellByGeneMatrix1(numAllCells1 * numDimsOfInterest);
    std::vector<float> cellByGeneMatrix2(numAllCells2 * numDimsOfInterest);

    try {
        fullPointDataset1->populateDataForDimensions(cellByGeneMatrix1, geneDataset1, allCellIndices1);
        fullPointDataset2->populateDataForDimensions(cellByGeneMatrix2, geneDataset2, allCellIndices2);
    }
    catch (const std::exception& ex) {
        qDebug() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }

    _selectedGenePointDataset1->setData(cellByGeneMatrix1.data(), numAllCells1, numDimsOfInterest);
    _selectedGenePointDataset2->setData(cellByGeneMatrix2.data(), numAllCells2, numDimsOfInterest);

    _selectedGenePointDataset1->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset1);
    _selectedGenePointDataset2->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset2);

    // Create PCA datasets and plugins if needed
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

    // Always (re)request plugins to ensure valid pointers
    _pcaAnalysisCommonGenesDataset1Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset1 }, { _pcaCommonGenesDataset1 });
    _pcaAnalysisCommonGenesDataset2Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset2 }, { _pcaCommonGenesDataset2 });

    if (!_pcaAnalysisCommonGenesDataset1Plugin || !_pcaAnalysisCommonGenesDataset2Plugin) {
        qDebug() << "PCA Analysis Plugin not found";
        setStatusMessage("E", "PCA plugin not found", "");
        return false;
    }

    // Set empty data before triggering PCA
    std::vector<float> emptyData1(numAllCells1 * numDimsOfInterest, 0.0f);
    std::vector<float> emptyData2(numAllCells2 * numDimsOfInterest, 0.0f);
    _pcaCommonGenesDataset1->setData(emptyData1.data(), numAllCells1, numDimsOfInterest);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset1);
    _pcaCommonGenesDataset2->setData(emptyData2.data(), numAllCells2, numDimsOfInterest);
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

    // Set PCA component actions
    auto componentsAction1 = dynamic_cast<IntegralAction*>(_pcaCommonGenesDataset1->findChildByPath("PCA/Number of PCA components"));
    auto componentsAction2 = dynamic_cast<IntegralAction*>(_pcaCommonGenesDataset2->findChildByPath("PCA/Number of PCA components"));
    if (!componentsAction1 || !componentsAction2) {
        qDebug() << "Number of PCA components action not found";
        setStatusMessage("E", "PCA components action not found", "");
        return false;
    }
    componentsAction1->setMaximum(numOfIntGenes);
    componentsAction1->setValue(numOfIntGenes);
    componentsAction2->setMaximum(numOfIntGenes);
    componentsAction2->setValue(numOfIntGenes);

    QApplication::processEvents();

    // Start PCA analysis
    auto startAction1 = dynamic_cast<TriggerAction*>(_pcaCommonGenesDataset1->findChildByPath("PCA/Start analysis"));
    auto startAction2 = dynamic_cast<TriggerAction*>(_pcaCommonGenesDataset2->findChildByPath("PCA/Start analysis"));
    if (!startAction1 || !startAction2) {
        qDebug() << "Start analysis action not found in PCA dataset";
        setStatusMessage("E", "PCA start action not found", "");
        return false;
    }
    startAction1->trigger();
    qDebug() << "PCA analysis 1 started";
    QEventLoop().processEvents();
    _pcaCommonGenesDataset1->setSelectionIndices({});
    mv::events().notifyDatasetDataSelectionChanged(_pcaCommonGenesDataset1);

    startAction2->trigger();
    qDebug() << "PCA analysis 2 started";
    QEventLoop().processEvents();
    _pcaCommonGenesDataset2->setSelectionIndices({});
    mv::events().notifyDatasetDataSelectionChanged(_pcaCommonGenesDataset2);

    QApplication::processEvents();

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
    auto& geneCorrValues = (datasetNumber == 2) ? _geneCorrValues2 : _geneCorrValues1;
    auto& geneOfInterestIndices = (datasetNumber == 2) ? _geneOfInterestIndices2 : _geneOfInterestIndices1;

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
        : _settingsAction.getTableView1Action();

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
    // Select appropriate data and view pointers
    const auto& geneCorrValues = (datasetNumber == 2) ? _geneCorrValues2 : _geneCorrValues1;
    QTableView* tableView = (datasetNumber == 2)
        ? _settingsAction.getTableView2Action()
        : _settingsAction.getTableView1Action();

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
            geneItem->setBackground(QColor("#FFFACD"));
            geneItem->setForeground(QColor("#000000"));
            QFont fontR = rankItem->font();
            fontR.setBold(true);
            rankItem->setFont(fontR);
            rankItem->setBackground(QColor("#FFFACD"));
            rankItem->setForeground(QColor("#000000"));
        }
        corrItem->setForeground(QColor("#000000"));

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
                ? _settingsAction.getTableView2Action()
                : _settingsAction.getTableView1Action();
            auto* mdl = tv->model();
            if (!mdl) return;
            QModelIndexList selectedIndexes = tv->selectionModel()->selectedIndexes();
            if (selectedIndexes.isEmpty()) return;
            const QModelIndex& index = selectedIndexes.first();
            if (!index.isValid()) return;
            QString value = mdl->data(mdl->index(index.row(), 0), Qt::DisplayRole).toString();
            if (datasetNumber == 2)
                _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionName(value);
            else
                _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
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