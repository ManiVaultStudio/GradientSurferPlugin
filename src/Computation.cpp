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

        _settingsAction.getComputeHolder().getStatusBarAction()->showMessage("Status: " + labelText);
        _settingsAction.getComputeHolder().getStatusBarAction()->setStyleSheet(
            "QStatusBar{padding-left:8px;background:" + backgroundColor + ";color:" + textColor + ";}"
        );
    };
    connect(&_settingsAction.getSettingsHolder().getStatusMessageAction(), &StringAction::stringChanged, this, updateStatus);
    const auto scatterplotSelection = [this]() -> void
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

    connect(&mv::plugins(), &AbstractPluginManager::pluginAdded, this, scatterplotSelection);

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

        auto selectedGene = _settingsAction.getSettingsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene =="")
        {
            qDebug() << "No gene selected";
            _settingsAction.getSettingsHolder().getDeSelectRowAction().setEnabled(false);
            return;
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

            auto fullDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
            if (fullDataset.isValid())
            {
                _settingsAction.getSettingsHolder().getEmbeddingDimensionXAction().setPointsDataset(fullDataset);
                _settingsAction.getSettingsHolder().getEmbeddingDimensionYAction().setPointsDataset(fullDataset);
                const auto mainSelectionChanged = [this]() -> void {
                    setStatusMessage("M", "Selected indices changed", "");
                    };
                connect(&fullDataset, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged);

            }


        }

        setStatusMessage("M", "Embedding dataset updated", "");
        };

    connect(&_settingsAction.getSettingsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset);



    const auto computeInfo = [this]() -> void {

        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getComputeHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();
        
        Compute();

        _settingsAction.getComputeHolder().getComputeAction().setDisabled(false);
        };

    connect(&_settingsAction.getComputeHolder().getComputeAction(), &TriggerAction::triggered, this, computeInfo);


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
                    plugin->printChildren();
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


void Computation::Compute()
{
    try
    {
        if (_settingsAction.getTableViewAction()->model()) {
            _settingsAction.getTableViewAction()->model()->removeRows(0, _settingsAction.getTableViewAction()->model()->rowCount());
        }
        QApplication::processEvents();

        auto fullEmbeddingDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
        auto fullPointDataset = mv::data().getDataset<Points>(_settingsAction.getSettingsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
        auto fullClusterDataset = mv::data().getDataset<Clusters>(_settingsAction.getSettingsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());

        if (!fullEmbeddingDataset.isValid() || !fullPointDataset.isValid() || !fullClusterDataset.isValid())
        {
            qDebug() << "Invalid datasets";
            setStatusMessage("E", "Error occured during dataset creation", "");
            return;
        }

        if(_settingsAction.getSettingsHolder().getEmbeddingDimensionXAction().getCurrentDimensionIndex() == -1 || _settingsAction.getSettingsHolder().getEmbeddingDimensionYAction().getCurrentDimensionIndex() == -1)
        {
            qDebug() << "No embedding dimensions selected";
            setStatusMessage("E", "No embedding dataset dimensions selected", "");
            return;
        }


        auto selectedIndices = fullEmbeddingDataset->getSelectionIndices();



        if (!(selectedIndices.size() > 0))
        {
            qDebug() << "No selected indices";
            setStatusMessage("E", "No indices selected", "");
            return;
        }

        auto selectedClusterValues = _settingsAction.getSettingsHolder().getClusterSelectionAction().getSelectedOptions();

        if (selectedClusterValues.isEmpty())
        {
            qDebug() << "No selected cluster values";
            setStatusMessage("E", "No clusters selected", "");
            return;
        }

        auto scatterplotViewName = _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotViewName.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            setStatusMessage("E", "No scatterplot view selected", "");
            return;
        }

        //removeDatasets();
        if (!_selectedPointDataset.isValid())
        {
            _selectedPointDataset = mv::data().createDataset("Points", "Selected Point Data");
            _selectedPointDataset->setGroupIndex(666);
            mv::events().notifyDatasetAdded(_selectedPointDataset);
        }
        if (!_rotatedSelectedEmbeddingDataset.isValid())
        {
            _rotatedSelectedEmbeddingDataset = mv::data().createDataset("Points", "Selected Rotated Embedding Data", _selectedPointDataset);
            _rotatedSelectedEmbeddingDataset->setGroupIndex(666);
            mv::events().notifyDatasetAdded(_rotatedSelectedEmbeddingDataset);
        }
        if (!_pcaDataset.isValid())
        {
            _pcaDataset = mv::data().createDataset("Points", "PCA Data");
            _pcaDataset->setGroupIndex(666);
            mv::events().notifyDatasetAdded(_pcaDataset);
        }

        if (!_pcaDataset.isValid() || !_rotatedSelectedEmbeddingDataset.isValid() || !_selectedPointDataset.isValid())
        {
            qDebug() << "Failed to create datasets";
            setStatusMessage("E", "Error occurred during dataset creation", "");
            return;
        }

        if (!_pcaAnalysisPlugin)
        {
            _pcaAnalysisPlugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedPointDataset }, { _pcaDataset });

        }

        if (!_pcaAnalysisPlugin)
        {

            qDebug() << "PCA Analysis Plugin not found";
            setStatusMessage("E", "PCA plugin not found", "");
            return;
        }

        std::sort(selectedIndices.begin(), selectedIndices.end());
        auto clusterValues = fullClusterDataset->getClusters();

        std::vector<std::seed_seq::result_type> filteredSelectionIndices;
        for (const auto& cluster : clusterValues)
        {
            auto clusterName = cluster.getName();
            auto clusterIndices = cluster.getIndices();
            auto sortedIndices = clusterIndices;
            std::sort(sortedIndices.begin(), sortedIndices.end());
            std::vector<int> intersection;
            std::set_intersection(
                selectedIndices.begin(), selectedIndices.end(),
                sortedIndices.begin(), sortedIndices.end(),
                std::back_inserter(intersection)
            );
            filteredSelectionIndices.insert(filteredSelectionIndices.end(), intersection.begin(), intersection.end());
        }

        if (filteredSelectionIndices.empty())
        {
            qDebug() << "No filtered selection indices";
            setStatusMessage("E", "No indices selected", "");
            return;
        }

        std::vector<float> XYCoordinates(filteredSelectionIndices.size() * 2);

        int xEmbeddingIndex = _settingsAction.getSettingsHolder().getEmbeddingDimensionXAction().getCurrentDimensionIndex();
        int yEmbeddingIndex = _settingsAction.getSettingsHolder().getEmbeddingDimensionYAction().getCurrentDimensionIndex();


        std::vector<int> preComputedEmbeddingColumnIndices({ xEmbeddingIndex, yEmbeddingIndex });

        if (XYCoordinates.size() != filteredSelectionIndices.size() * 2)
        {
            qDebug() << "XYCoordinates size mismatch";
            setStatusMessage("E", "XYCoordinates size mismatch", "");
            return;
        }
        auto topN = _settingsAction.getSettingsHolder().getTopNAction().getValue();
        if (topN <= 0)
        {
            qDebug() << "Top N value is not valid";
            setStatusMessage("E", "Top N value is not valid", "");
            return;
        }
        fullEmbeddingDataset->populateDataForDimensions(XYCoordinates, preComputedEmbeddingColumnIndices, filteredSelectionIndices);

        float rotationAngle = 0.0;
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            bool foundPlugin = false;
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotViewName)
                {
                    foundPlugin = true;
                    mv::gui::DecimalAction* angleAction = dynamic_cast<DecimalAction*>(plugin->findChildByPath("Settings/Selection/Point Selection/Line angle"));
                    if (angleAction)
                    {
                        rotationAngle = angleAction->getValue();
                    }
                    else
                    {
                        qDebug() << "Angle action not found";
                        setStatusMessage("E", "Angle action not found", "");
                        return;
                    }
                }
            }
            if (!foundPlugin)
            {
                qDebug() << "Scatterplot plugin not found";
                setStatusMessage("E", "Scatterplot plugin not found", "");
                return;
            }
        }
        else
        {
            qDebug() << "Scatterplot view factory not found";
            setStatusMessage("E", "Scatterplot view factory not found", "");
            return;
        }

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

        auto noOfDimensions = fullPointDataset->getNumDimensions();
        auto dimensionNames = fullPointDataset->getDimensionNames();
        std::vector<int> columnIndices;
        for (int i = 0; i < noOfDimensions; i++)
        {
            columnIndices.push_back(i);
        }

        std::vector<float> cellbygenematrix(filteredSelectionIndices.size() * columnIndices.size());

        try {
            fullPointDataset->populateDataForDimensions(cellbygenematrix, columnIndices, filteredSelectionIndices);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in populateDataForDimensions:" << ex.what();
            setStatusMessage("E", "Error occurred during populating filtered data", "");
            return;
        }


        _selectedPointDataset->setData(cellbygenematrix.data(), filteredSelectionIndices.size(), noOfDimensions);
        _selectedPointDataset->setDimensionNames(dimensionNames);
        mv::events().notifyDatasetDataChanged(_selectedPointDataset);

        _rotatedSelectedEmbeddingDataset->setData(rotatedXYCordinates.data(), filteredSelectionIndices.size(), 2);
        _rotatedSelectedEmbeddingDataset->setDimensionNames({ "X","Y" });
        mv::events().notifyDatasetDataChanged(_rotatedSelectedEmbeddingDataset);

        std::vector<float> emptyData(filteredSelectionIndices.size()* noOfDimensions, 0.0f);
        _pcaDataset->setData(emptyData.data(), filteredSelectionIndices.size(), noOfDimensions);
        mv::events().notifyDatasetDataChanged(_pcaDataset);

        /*if (_pcaAnalysisPlugin)
        {
            mv::plugins().destroyPlugin(_pcaAnalysisPlugin);
        }*/
        



        auto componentsAction = dynamic_cast<IntegralAction*>(_pcaDataset->findChildByPath("PCA/Number of PCA components"));
        if (!componentsAction)
        {
            qDebug() << "Number of PCA components action not found";
            setStatusMessage("E", "PCA components action not found", "");
            return;
        }

        componentsAction->setValue(noOfDimensions); 

        auto startAction = dynamic_cast<TriggerAction*>(_pcaDataset->findChildByPath("PCA/Start analysis"));
        if (startAction)
        {
            startAction->trigger();
            QEventLoop loop;
            loop.processEvents();
            _pcaDataset->setSelectionIndices({});
        }


        // output min and max of rotated xy coordinates
        auto xCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "X");
        float minX = *std::min_element(xCoordinates.begin(), xCoordinates.end());
        float maxX = *std::max_element(xCoordinates.begin(), xCoordinates.end());
        float minY = *std::min_element(yCoordinates.begin(), yCoordinates.end());
        float maxY = *std::max_element(yCoordinates.begin(), yCoordinates.end());
        qDebug() << "Rotated XY Coordinates minX:" << minX << "maxX:" << maxX << "minY:" << minY << "maxY:" << maxY;
        qDebug() << "Rotated XY Coordinates size:" << rotatedXYCordinates.size() << "filteredSelectionIndices size:" << filteredSelectionIndices.size() << "yCoordinates size:" << yCoordinates.size();
        // compute correlation between gene and selected y coordinates
        std::vector<float> corrGeneCoords;
        computeCorrelationGeneCoords(filteredSelectionIndices, fullPointDataset, yCoordinates, corrGeneCoords);


        QStringList topGenes;
        try {
            topGenes = filterTopNGenesByCorrelation(corrGeneCoords, topN, dimensionNames);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            setStatusMessage("E", "Error occurred during filtering top "+ QString::number(topN) +" genes", "");
            return;
        }

        QStandardItemModel* model = new QStandardItemModel(0, 1, _settingsAction.getTableViewAction());
        model->setHorizontalHeaderLabels(QStringList() << "Gene");
        for (int i = 0; i < topGenes.size(); i++)
        {
            QStandardItem* geneItem = new QStandardItem(topGenes.at(i));
            model->setItem(i, 0, geneItem);
        }
        _settingsAction.getTableViewAction()->setModel(model);
        _settingsAction.getTableViewAction()->resizeColumnsToContents();
        _settingsAction.getTableViewAction()->resizeRowsToContents();
        _settingsAction.getTableViewAction()->setColumnWidth(0, 200);


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

 
        setStatusMessage("C", "Computation completed", "#28a745"); 
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Exception in Computation:" << ex.what();
        setStatusMessage("E", "Error occurred during computation: " + QString::fromStdString(ex.what()),"");

    }
    catch (...)
    {
        qDebug() << "Unknown exception in Computation";
        setStatusMessage("E", "Unknown error occurred during computation", "");
    }
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

void Computation::computeCorrelationGeneCoords(const std::vector<std::seed_seq::result_type>& cellIndices, const mv::Dataset<Points> dataset, const std::vector<float>& coords, std::vector<float>& corrVector)
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