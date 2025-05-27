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

using namespace mv;
using namespace mv::gui;

// Add this helper function at file scope (above Computation constructor or in an anonymous namespace)
static QString getContrastingTextColor(const QString& bgColor) {
    QColor color(bgColor);
    if (!color.isValid()) return "white";
    // Calculate luminance (perceived brightness)
    double luminance = (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) / 255.0;
    return (luminance > 0.5) ? "black" : "white";
}
QVector<QPointF> convertToQPointF(const std::vector<float>& vec, int numOfDims, int numOfPoints) {
    QVector<QPointF> points;

    if (numOfDims < 2) {
        throw std::invalid_argument("At least 2 dimensions are required to create QPointF.");
    }

    if (static_cast<size_t>(numOfDims * numOfPoints) > vec.size()) {
        throw std::invalid_argument("Input vector size is smaller than expected based on dimensions and number of points.");
    }

    for (int i = 0; i < numOfPoints; ++i) {
        float x = vec[i * numOfDims];
        float y = vec[i * numOfDims + 1];
        points.append(QPointF(x, y));
    }

    return points;
}
// Add this helper function at file scope (above Computation constructor or in an anonymous namespace)
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
 
    const auto updateStatus = [this]() -> void {
        auto stringFull = _settingsAction.getSettingsHolder().getStatusMessageAction().getString();

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
    connect(&_settingsAction.getSettingsHolder().getStatusMessageAction(), &StringAction::stringChanged, this, updateStatus);
    const auto scatterplotSelectionOptions = [this]() -> void
        {
            QStringList scatterplotNames = {};
            QString selectedCurrentText = _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText();

            auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
            if (scatterplotViewFactory)
            {

                for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
                {
                    scatterplotNames.append(plugin->getGuiName());
                }
            }

            if (!scatterplotNames.contains(selectedCurrentText))
            {
                selectedCurrentText = "";
                if (scatterplotNames.size() > 0)
                {
                    selectedCurrentText = scatterplotNames[0];
                }
            }

            _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().setOptions(scatterplotNames);
            _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().setCurrentText(selectedCurrentText);


        };

    connect(&mv::plugins(), &AbstractPluginManager::pluginAdded, this, scatterplotSelectionOptions);

    _settingsAction.getSettingsHolder().getClusterDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == ClusterType;
        });
    _settingsAction.getSettingsHolder().getPointDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });
    _settingsAction.getSettingsHolder().getEmbeddingDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });

    const auto updateClusterDataset = [this]() -> void {
        if (_settingsAction.getSettingsHolder().getClusterDatasetAction().getCurrentDataset().isValid())
        {
            auto clusterFullDataset = mv::data().getDataset<Clusters>(_settingsAction.getSettingsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
            auto clusters = clusterFullDataset->getClusters();
            QStringList clusterNames = {};
            if (!clusters.isEmpty())
            {
                for (auto cluster : clusters)
                {
                    clusterNames.append(cluster.getName());
                }
            }
            
            _settingsAction.getSettingsHolder().getClusterSelectionAction().setOptions(clusterNames);
            _settingsAction.getSettingsHolder().getClusterSelectionAction().setSelectedOptions(clusterNames);

        }
        else
        {
            _settingsAction.getSettingsHolder().getClusterSelectionAction().setOptions({});
        }
        setStatusMessage("M", "Cluster dataset updated", "");
        };

    connect(&_settingsAction.getSettingsHolder().getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset);



    const auto updatePointDataset = [this]() -> void {
        if (_settingsAction.getSettingsHolder().getPointDatasetAction().getCurrentDataset().isValid())
        {
            auto dataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
            if (dataset.isValid())
            {
                _settingsAction.getSettingsHolder().getGeneSelectionAction().setPointsDataset(dataset);
                auto dimensions = dataset->getNumDimensions();
                _settingsAction.getSettingsHolder().getTopNAction().setMaximum(dimensions);
            }


        }
        else
        {
            _settingsAction.getSettingsHolder().getGeneSelectionAction().setPointsDataset({});
        }
        setStatusMessage("M", "Point dataset updated", "");
        };

    connect(&_settingsAction.getSettingsHolder().getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset);



    const auto updateGeneSelection= [this]() -> void {
        if (_settingsAction.getSettingsHolder().getGeneSelectionAction().getCurrentDimensionIndex()==-1)
        {
            qDebug() << "No gene selected";
            _settingsAction.getSettingsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }
        if (_settingsAction.getLineChartWidget())
        {
            _settingsAction.getLineChartWidget()->clear();

        }
        auto selectedGene = _settingsAction.getSettingsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene =="")
        {
            qDebug() << "No gene selected";
            _settingsAction.getSettingsHolder().getDeSelectRowAction().setEnabled(false);
            return;
        }
        if (_settingsAction.getSettingsHolder().getPointDatasetAction().getCurrentDataset().isValid() && _settingsAction.getLineChartWidget() && _filteredSelectionIndices.size()>0)
        {
            

            auto pointDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
            if(pointDataset.isValid())
            {
                
                auto dimensionNames = pointDataset->getDimensionNames();
                //get the index of the selected gene QString in the dimension names std::vector<QString>
                auto it= std::find(dimensionNames.begin(), dimensionNames.end(), selectedGene);
                if (it == dimensionNames.end())
                {
                    qDebug() << "Selected gene not found in point dataset dimensions";

                }
                else
                {
                   
                    int geneIndex = std::distance(dimensionNames.begin(), it);
                    LineData selectedGeneLineData;
                    
                    std::vector<int> geneIndices = { geneIndex };
                    std::vector<float> XYCoordinates(_filteredSelectionIndices.size()* geneIndices.size());
                    QVector<QPointF> points;



                    pointDataset->populateDataForDimensions(XYCoordinates, geneIndices, _filteredSelectionIndices);
                    auto pointsData = convertToQPointF(XYCoordinates, _filteredSelectionIndices.size(), geneIndices.size());
                    selectedGeneLineData.points = pointsData;
                    QVector<LineData> lineData;
                    lineData.push_back(selectedGeneLineData);

                    _settingsAction.getLineChartWidget()->setLines(lineData);
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
        _settingsAction.getSettingsHolder().getDeSelectRowAction().setEnabled(true);
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText())
                {
                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
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
                                auto dataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
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

    connect(&_settingsAction.getSettingsHolder().getGeneSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, updateGeneSelection);


    const auto updateEmbeddingDataset = [this]() -> void {
        if (_settingsAction.getSettingsHolder().getEmbeddingDatasetAction().getCurrentDataset().isValid())
        {

            
            _embeddingDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
            
            if (_embeddingDataset.isValid())
            {
                _settingsAction.getSettingsHolder().getEmbeddingDimensionXAction().setPointsDataset(_embeddingDataset);
                _settingsAction.getSettingsHolder().getEmbeddingDimensionYAction().setPointsDataset(_embeddingDataset);
                const auto mainSelectionChanged = [this]() -> void {
                    //qDebug() << "Main selection changed in fullEmbeddingDataset";
                    setStatusMessage("M", "Selected indices changed", "");
                    
                    auto fullclusterDataset = mv::data().getDataset<Clusters>(_settingsAction.getSettingsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
                    if (fullclusterDataset.isValid() && _embeddingDataset.isValid())
                    {
                        auto clusters = fullclusterDataset->getClusters();
                        auto selectedClusters = _settingsAction.getSettingsHolder().getClusterSelectionAction().getSelectedOptions();
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
                        }
                        else
                        {
                            qDebug() << "No clusters selected, clearing selection indices";
                            _embeddingDataset->setSelectionIndices({});
                        }

                    }
                    else
                    {
                        qDebug() << "No cluster dataset selected or invalid cluster dataset";
                        _embeddingDataset->setSelectionIndices({});
                    }
                    setStatusMessage("R", "Computation in progress", "");
                    _settingsAction.getComputeHolder().getComputeAction().setDisabled(true);
                    QApplication::processEvents();

                    AutoCompute();

                    _settingsAction.getComputeHolder().getComputeAction().setDisabled(false);


                    };

                
                bool ok= connect(&_embeddingDataset, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged);
                if (!ok)
                {
                    qDebug() << "Failed to connect to dataSelectionChanged signal of fullEmbeddingDataset";
                }

            }


        }

        setStatusMessage("M", "Embedding dataset updated", "");
        };

    connect(&_settingsAction.getSettingsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset);

    const auto computeInfo = [this]() -> void {

        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getComputeHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        ManualCompute();

        _settingsAction.getComputeHolder().getComputeAction().setDisabled(false);
        };

    connect(&_settingsAction.getComputeHolder().getComputeAction(), &TriggerAction::triggered, this, computeInfo);

    const auto clusterSelectionChanged = [this]() -> void {

        setStatusMessage("M", "Cluster selection updated", "");

        };

    connect(&_settingsAction.getSettingsHolder().getClusterSelectionAction(), &OptionsAction::selectedOptionsChanged, this, clusterSelectionChanged);





    const auto topNChanged = [this]() -> void {

        setStatusMessage("M", "Top N updated", "");

        };

    connect(&_settingsAction.getSettingsHolder().getTopNAction(), &IntegralAction::valueChanged, this, topNChanged);



    const auto showPointSelectionChanged = [this]() -> void {

        QString scatterplotName = _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName)
                {
                    auto pointSelectionScaleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Scale"));
                    auto pointSelectionOpacityAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Opacity"));
                    if (pointSelectionScaleAction)
                    {
                        if (_settingsAction.getSettingsHolder().getHidePointSelectionOutlineAction().isChecked())
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
                        if (_settingsAction.getSettingsHolder().getHidePointSelectionOutlineAction().isChecked())
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

    connect(&_settingsAction.getSettingsHolder().getHidePointSelectionOutlineAction(), &ToggleAction::toggled, this, showPointSelectionChanged);

    const auto freezeSelectionChanged = [this]() -> void {

        QString scatterplotName = _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName)
                {
                    auto freezeAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Selection/Freeze selection"));
                    if (freezeAction)
                    {
                        freezeAction->setChecked(_settingsAction.getSettingsHolder().getFreezeSelectionAction().isChecked());
                    }
                    else
                    {
                        qDebug() << "Freeze selection action not found";
                    }
                }
            }
        }

        };

    connect(&_settingsAction.getSettingsHolder().getFreezeSelectionAction(), &ToggleAction::toggled, this, freezeSelectionChanged);


    const auto focusSelectionChanged = [this]() -> void {

        QString scatterplotName = _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName)
                {
                    auto focusAction = dynamic_cast<ToggleAction*>(plugin->findChildByPath("Settings/Plot/Point/Focus selection"));
                    if (focusAction)
                    {
                        focusAction->setChecked(_settingsAction.getSettingsHolder().getFocusSelectionAction().isChecked());
                    }
                    else
                    {
                        qDebug() << "Focus selection action not found";
                    }
                }
            }
        }

        };

    connect(&_settingsAction.getSettingsHolder().getFocusSelectionAction(), &ToggleAction::toggled, this, focusSelectionChanged);


    const auto embeddingDimensionYActionChanged = [this]() -> void {

        setStatusMessage("M", "Embedding Y dimension changed", "");
        QString scatterplotName = _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText();
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
                            auto embeddingDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
                            if (embeddingDataset.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition = _settingsAction.getSettingsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName();
                            auto yPosition = _settingsAction.getSettingsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName();
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
                                auto dataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
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

    connect(&_settingsAction.getSettingsHolder().getEmbeddingDimensionYAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionYActionChanged);

    const auto embeddingDimensionXActionChanged = [this]() -> void {

        setStatusMessage("M", "Embedding X dimension changed", "");
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText())
                {
                    
                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
                            if (embeddingDataset.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition = _settingsAction.getSettingsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName();
                            auto yPosition = _settingsAction.getSettingsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName();
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
                                auto dataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
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

    connect(&_settingsAction.getSettingsHolder().getEmbeddingDimensionXAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionXActionChanged);

    const auto deselectRow = [this]() -> void {
        auto selectedIndex = _settingsAction.getTableViewAction()->currentIndex();
        if (selectedIndex.isValid())
        {
            _settingsAction.getTableViewAction()->selectionModel()->clearSelection();
            _settingsAction.getTableViewAction()->setCurrentIndex(QModelIndex());
            _settingsAction.getSettingsHolder().getGeneSelectionAction().setCurrentDimensionName("");
        }
        else
        {
            qDebug() << "No row selected";
        }
        _settingsAction.getSettingsHolder().getDeSelectRowAction().setEnabled(false);
        _settingsAction.getSettingsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        mv::gui::DatasetPickerAction* colorDatasetPickerAction;
        mv::gui::DatasetPickerAction* pointDatasetPickerAction;

        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText())
                {
                    
                    pointDatasetPickerAction = dynamic_cast<DatasetPickerAction*>(plugin->findChildByPath("Settings/Datasets/Position"));
                    if (pointDatasetPickerAction)
                    {
                        if (!pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
                            if (embeddingDataset.isValid())
                            {
                                pointDatasetPickerAction->setCurrentDataset(embeddingDataset);
                            }
                        }
                        if (pointDatasetPickerAction->getCurrentDataset().isValid())
                        {
                            auto xPosition = _settingsAction.getSettingsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName();
                            auto yPosition = _settingsAction.getSettingsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName();
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
                                auto dataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
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

        };

    connect(&_settingsAction.getSettingsHolder().getDeSelectRowAction(), &TriggerAction::triggered, this, deselectRow);
    const auto scatterplotviewSelectionChanged = [this]() -> void {

        setStatusMessage("M", "Scatterplotview selection updated", "");

        QString scatterplotName = _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName)
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

    connect(&_settingsAction.getSettingsHolder().getScatterplotViewSelectionAction(), &OptionAction::currentIndexChanged, this, scatterplotviewSelectionChanged);

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
        QApplication::processEvents();

        // Step 1: Validate and retrieve datasets and indices
        mv::Dataset<Points> fullPointDataset;
        mv::Dataset<Clusters> fullClusterDataset;
        int xEmbeddingIndex = -1, yEmbeddingIndex = -1, topN = 0;
        std::vector<int> selectedIndices;
        QString scatterplotViewName;
        if (!validateAndRetrieveDatasets(fullPointDataset, fullClusterDataset, xEmbeddingIndex, yEmbeddingIndex, selectedIndices, scatterplotViewName, topN))
            return;

        // Step 2: Create datasets if needed
        if (!createDatasetsIfNeeded())
            return;

        // Step 3: Get rotation angle from plugin
        float rotationAngle = 0.0f;
        if (!getRotationAngle(scatterplotViewName, rotationAngle))
            return;

        // Step 4: Populate embedding coordinates
        std::vector<float> XYCoordinates;
        if (!populateEmbeddingCoordinates(xEmbeddingIndex, yEmbeddingIndex, XYCoordinates))
            return;

        // Step 5: Rotate and extract coordinates
        std::vector<float> rotatedXYCordinates;
        try {
            rotatedXYCordinates = rotateAxis(XYCoordinates, rotationAngle);
        } catch (const std::exception& ex) {
            qDebug() << "Exception in rotateAxis:" << ex.what();
            setStatusMessage("E", "Error occurred during point rotation", "");
            return;
        }

        std::vector<float> yCoordinates;
        try {
            yCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "Y");
        } catch (const std::exception& ex) {
            qDebug() << "Exception in getCoordinatesOfCells:" << ex.what();
            setStatusMessage("E", "Error occurred during fetching coordinates", "");
            return;
        }

        std::vector<int> sortedCellIndices;
        try {
            sortedCellIndices = getSortedCellIndices(yCoordinates);
        } catch (const std::exception& ex) {
            qDebug() << "Exception in getSortedCellIndices:" << ex.what();
            setStatusMessage("E", "Error occurred during sorting indices", "");
            return;
        }

        int noOfDimensions = fullPointDataset->getNumDimensions();
        const auto& dimensionNames = fullPointDataset->getDimensionNames();
        std::vector<int> columnIndices(noOfDimensions);
        std::iota(columnIndices.begin(), columnIndices.end(), 0);

        // Step 6: Populate and update datasets
        if (!populateAndUpdateDatasets(fullPointDataset, rotatedXYCordinates, noOfDimensions, columnIndices, dimensionNames))
            return;

        // Step 7: Setup and trigger PCA
        if (!setupAndTriggerPCA(noOfDimensions))
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

        QStringList topGenes;
        try {
            topGenes = filterTopNGenesByCorrelation(corrGeneCoords, topN, dimensionNames);
        } catch (const std::exception& ex) {
            qDebug() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            setStatusMessage("E", "Error occurred during filtering top " + QString::number(topN) + " genes", "");
            return;
        }

        // Step 8: Setup table model and connect selection
        setupTableModelAndSelection(topGenes);

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
    qDebug()<< "Number of genes:" << numGenes << "Number of cells:" << numCells << "Number of coordinates:" << numCoords;
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
    qDebug() << "corrVector size" << corrVector.size() << corrVector[0] << corrVector[1] << corrVector[2];
    // output the highest correlation gene symbol
    std::vector<std::pair<float, int>> geneCorrPairs(numGenes);
    for (int i = 0; i < numGenes; ++i) {
        geneCorrPairs[i] = std::make_pair(corrVector[i], i);
    }
    std::sort(geneCorrPairs.begin(), geneCorrPairs.end(), std::greater<std::pair<float, int>>());
    qDebug() << "Top 5 genes with highest correlation:";
    for (int i = 0; i < 5 && i < numGenes; ++i) {
        int geneIdx = geneCorrPairs[i].second;
        QString geneSymbol = dataset->getDimensionNames()[geneIdx];
        qDebug() << "Gene:" << geneSymbol << "Correlation:" << geneCorrPairs[i].first;
    }
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
    _settingsAction.getSettingsHolder().getStatusMessageAction().setString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

// --- Helper function implementations for AutoCompute refactor ---

// 1. Validate and retrieve datasets and indices
bool Computation::validateAndRetrieveDatasets(
    mv::Dataset<Points>& fullPointDataset,
    mv::Dataset<Clusters>& fullClusterDataset,
    int& xEmbeddingIndex,
    int& yEmbeddingIndex,
    std::vector<int>& selectedIndices,
    QString& scatterplotViewName,
    int& topN)
{
    auto& settingsHolder = _settingsAction.getSettingsHolder();

    fullPointDataset = mv::data().getDataset<Points>(settingsHolder.getPointDatasetAction().getCurrentDataset().getDatasetId());
    fullClusterDataset = mv::data().getDataset<Clusters>(settingsHolder.getClusterDatasetAction().getCurrentDataset().getDatasetId());

    if (!_embeddingDataset.isValid() || !fullPointDataset.isValid() || !fullClusterDataset.isValid()) {
        qDebug() << "Invalid datasets";
        setStatusMessage("E", "Error occured during dataset creation", "");
        return false;
    }

    xEmbeddingIndex = settingsHolder.getEmbeddingDimensionXAction().getCurrentDimensionIndex();
    yEmbeddingIndex = settingsHolder.getEmbeddingDimensionYAction().getCurrentDimensionIndex();
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

    auto selectedClusterValues = settingsHolder.getClusterSelectionAction().getSelectedOptions();
    if (selectedClusterValues.isEmpty()) {
        qDebug() << "No selected cluster values";
        setStatusMessage("E", "No clusters selected", "");
        return false;
    }

    scatterplotViewName = settingsHolder.getScatterplotViewSelectionAction().getCurrentText();
    if (scatterplotViewName.isEmpty()) {
        qDebug() << "No scatterplot view selected";
        setStatusMessage("E", "No scatterplot view selected", "");
        return false;
    }

    topN = settingsHolder.getTopNAction().getValue();
    if (topN <= 0) {
        qDebug() << "Top N value is not valid";
        setStatusMessage("E", "Top N value is not valid", "");
        return false;
    }

    return true;
}

// 2. Create datasets if needed
bool Computation::createDatasetsIfNeeded()
{
    if (!_selectedPointDataset.isValid()) {
        _selectedPointDataset = mv::data().createDataset("Points", "Selected Point Data");
        _selectedPointDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_selectedPointDataset);
    }
    if (!_rotatedSelectedEmbeddingDataset.isValid()) {
        _rotatedSelectedEmbeddingDataset = mv::data().createDataset("Points", "Selected Rotated Embedding Data", _selectedPointDataset);
        _rotatedSelectedEmbeddingDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_rotatedSelectedEmbeddingDataset);
    }
    if (!_pcaDataset.isValid()) {
        _pcaDataset = mv::data().createDataset("Points", "PCA Data");
        _pcaDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_pcaDataset);
    }

    if (!_pcaDataset.isValid() || !_rotatedSelectedEmbeddingDataset.isValid() || !_selectedPointDataset.isValid()) {
        qDebug() << "Failed to create datasets";
        setStatusMessage("E", "Error occurred during dataset creation", "");
        return false;
    }

    if (!_pcaAnalysisPlugin) {
        _pcaAnalysisPlugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedPointDataset }, { _pcaDataset });
    }

    if (!_pcaAnalysisPlugin) {
        qDebug() << "PCA Analysis Plugin not found";
        setStatusMessage("E", "PCA plugin not found", "");
        return false;
    }

    return true;
}

// 3. Retrieve rotation angle from plugin
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
                } else {
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
    } else {
        qDebug() << "Scatterplot view factory not found";
        setStatusMessage("E", "Scatterplot view factory not found", "");
        return false;
    }
    return true;
}

// 4. Populate embedding coordinates
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

// 5. Populate and update datasets
bool Computation::populateAndUpdateDatasets(
    mv::Dataset<Points>& fullPointDataset,
    const std::vector<float>& rotatedXYCordinates,
    int noOfDimensions,
    const std::vector<int>& columnIndices,
    const std::vector<QString>& dimensionNames)
{
    std::vector<float> cellbygenematrix(_filteredSelectionIndices.size() * columnIndices.size());
    try {
        fullPointDataset->populateDataForDimensions(cellbygenematrix, columnIndices, _filteredSelectionIndices);
    } catch (const std::exception& ex) {
        qDebug() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }

    _selectedPointDataset->setData(cellbygenematrix.data(), _filteredSelectionIndices.size(), noOfDimensions);
    _selectedPointDataset->setDimensionNames(dimensionNames);
    mv::events().notifyDatasetDataChanged(_selectedPointDataset);

    _rotatedSelectedEmbeddingDataset->setData(rotatedXYCordinates.data(), _filteredSelectionIndices.size(), 2);
    _rotatedSelectedEmbeddingDataset->setDimensionNames({ "X","Y" });
    mv::events().notifyDatasetDataChanged(_rotatedSelectedEmbeddingDataset);

    std::vector<float> emptyData(_filteredSelectionIndices.size() * noOfDimensions, 0.0f);
    _pcaDataset->setData(emptyData.data(), _filteredSelectionIndices.size(), noOfDimensions);
    mv::events().notifyDatasetDataChanged(_pcaDataset);

    return true;
}

// 6. Setup and trigger PCA
bool Computation::setupAndTriggerPCA(int noOfDimensions)
{
    auto componentsAction = dynamic_cast<IntegralAction*>(_pcaDataset->findChildByPath("PCA/Number of PCA components"));
    if (!componentsAction) {
        qDebug() << "Number of PCA components action not found";
        setStatusMessage("E", "PCA components action not found", "");
        return false;
    }
    componentsAction->setValue(noOfDimensions);

    auto startAction = dynamic_cast<TriggerAction*>(_pcaDataset->findChildByPath("PCA/Start analysis"));
    if (startAction) {
        startAction->trigger();
        QEventLoop loop;
        loop.processEvents();
        _pcaDataset->setSelectionIndices({});
    }
    return true;
}

// 7. Setup table model and connect selection
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
            _settingsAction.getSettingsHolder().getGeneSelectionAction().setCurrentDimensionName(value);
        }
    );
}