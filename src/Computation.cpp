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
using namespace mv;
using namespace mv::gui;



Computation::Computation(LayerSurferPlugin& LayerSurferPlugin, SettingsAction& SettingsAction)
    : WidgetAction(&LayerSurferPlugin, "Computation"),
    _viewerPlugin(LayerSurferPlugin),
    _settingsAction(SettingsAction)
{
 
    const auto updateStatus = [this]() -> void {

        auto string = _settingsAction.getSettingsHolder().getStatusColorAction().getString();
        QString labelText = "";
        QString backgroundColor = "none";

        if (string == "C") {
            labelText = "Updated";
            backgroundColor = "#28a745"; // Green

        }
        else if (string == "M") {
            labelText = "Outdated";
            backgroundColor = "#ffc107"; // Gold

        }
        else if (string == "E") {
            labelText = "Error";
            backgroundColor = "#dc3545"; // Red
        }
        else if (string == "R")
        {
            labelText = "Processing";
            backgroundColor = "#007bff"; // Blue
        }
        else {
            labelText = "Unknown";
            backgroundColor = "#6c757d"; // Grey
        }

        _settingsAction.getComputeHolder().getStatusBarAction()->showMessage("Status: " + labelText);
        _settingsAction.getComputeHolder().getStatusBarAction()->setStyleSheet("QStatusBar{padding-left:8px;background:" + backgroundColor + ";color:white;}");


        };
    connect(&_settingsAction.getSettingsHolder().getStatusColorAction(), &StringAction::stringChanged, this, updateStatus);
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
        _settingsAction.getSettingsHolder().getStatusColorAction().setString("M");
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
        _settingsAction.getSettingsHolder().getStatusColorAction().setString("M");
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

                const auto mainSelectionChanged = [this]() -> void {
                    _settingsAction.getSettingsHolder().getStatusColorAction().setString("M");
                    qDebug() << "Main selection changed";
                    };
                connect(&fullDataset, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged);

            }


        }
        _settingsAction.getSettingsHolder().getStatusColorAction().setString("M");
        };

    connect(&_settingsAction.getSettingsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset);



    const auto computeInfo = [this]() -> void {

        _settingsAction.getSettingsHolder().getStatusColorAction().setString("R");
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
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        auto selectedIndices = fullEmbeddingDataset->getSelectionIndices();

        if (!(selectedIndices.size() > 0))
        {
            qDebug() << "No selected indices";
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        auto selectedClusterValues = _settingsAction.getSettingsHolder().getClusterSelectionAction().getSelectedOptions();

        if (selectedClusterValues.isEmpty())
        {
            qDebug() << "No selected cluster values";
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        auto scatterplotViewName = _settingsAction.getSettingsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotViewName.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
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
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        std::vector<float> XYCoordinates(filteredSelectionIndices.size() * 2);
        std::vector<int> preComputedEmbeddingColumnIndices({ 0, 1 });

        if (XYCoordinates.size() != filteredSelectionIndices.size() * 2)
        {
            qDebug() << "XYCoordinates size mismatch";
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }
        auto topN = _settingsAction.getSettingsHolder().getTopNAction().getValue();
        if (topN <= 0)
        {
            qDebug() << "Top N value is not valid";
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
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
                        _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
                        return;
                    }
                }
            }
            if (!foundPlugin)
            {
                qDebug() << "Scatterplot plugin not found";
                _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
                return;
            }
        }
        else
        {
            qDebug() << "Scatterplot view factory not found";
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        std::vector<float> rotatedXYCordinates;
        try {
            rotatedXYCordinates = rotateAxis(XYCoordinates, rotationAngle);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in rotateAxis:" << ex.what();
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        std::vector<float> yCoordinates;
        try {
            yCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "Y");
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in getCoordinatesOfCells:" << ex.what();
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        std::vector<int> sortedGeneIndices;
        try {
            sortedGeneIndices = getSortedGeneIndices(yCoordinates);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in getSortedGeneIndices:" << ex.what();
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
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
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        std::vector<int> correlationMatrix;
        try {
            correlationMatrix = computeCorrelation(cellbygenematrix, filteredSelectionIndices.size(), noOfDimensions);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in computeCorrelation:" << ex.what();
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
            return;
        }

        QStringList topGenes;
        try {
            topGenes = filterTopNGenesByCorrelation(correlationMatrix, topN, dimensionNames);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
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

        _settingsAction.getSettingsHolder().getStatusColorAction().setString("C");
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Exception in Computation:" << ex.what();
        _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
    }
    catch (...)
    {
        qDebug() << "Unknown exception in Computation";
        _settingsAction.getSettingsHolder().getStatusColorAction().setString("E");
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
std::vector<int> Computation::getSortedGeneIndices(const std::vector<float>& yCoordinates) {
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

// Filters the top N genes by correlation
QStringList Computation::filterTopNGenesByCorrelation(const std::vector<int>& correlationMatrix, int N, std::vector<QString>  geneNameOrdered) {
    QStringList topGenes;


    // Defensive: correlationMatrix and geneNames should have the same size
    int count = std::min<int>(std::min<int>(N, static_cast<int>(correlationMatrix.size())), geneNameOrdered.size());
    if (count == 0) return topGenes;

    // Create a vector of indices
    std::vector<int> indices(correlationMatrix.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Sort indices by descending correlation value
    std::partial_sort(
        indices.begin(), indices.begin() + count, indices.end(),
        [&](int a, int b) { return correlationMatrix[a] > correlationMatrix[b]; }
    );

    // Map top indices to gene names
    for (int i = 0; i < count; ++i) {
        int idx = indices[i];
        if (idx >= 0 && idx < geneNameOrdered.size())
            topGenes << geneNameOrdered[idx];
    }

    return topGenes;
}

