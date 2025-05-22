#include "SettingsAction.h"

#include <iostream>
#include <set>
#include "LayerSurferPlugin.h"
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

SettingsAction::SettingsAction(LayerSurferPlugin& LayerSurferPlugin, mv::CoreInterface* core) :
    WidgetAction(&LayerSurferPlugin, "LayerSurferPlugin"),
    _viewerPlugin(LayerSurferPlugin),
    _core(core),
    _settingsHolder(*this),
    _computeHolder(*this)
{
    setSerializationName("LayerSurfer:Settings");
    _settingsHolder.getClusterDatasetAction().setSerializationName("LayerSurfer:Clusterdataset");
    _settingsHolder.getPointDatasetAction().setSerializationName("LayerSurfer:PointDataset");
    _settingsHolder.getEmbeddingDatasetAction().setSerializationName("LayerSurfer:EmbeddingDataset");
    _settingsHolder.getClusterSelectionAction().setSerializationName("LayerSurfer:ClusterSelection");
    _settingsHolder.getGeneSelectionAction().setSerializationName("LayerSurfer:GeneSelection");
    _settingsHolder.getScatterplotViewSelectionAction().setSerializationName("LayerSurfer:ScatterplotViewSelection");


    _settingsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _settingsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _settingsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset");
    _settingsHolder.getClusterSelectionAction().setToolTip("Cluster Selection");
    _settingsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _settingsHolder.getStatusColorAction().setToolTip("Status Color");
    _computeHolder.getComputeAction().setToolTip("Compute Action");
    _computeHolder.getStatusBarAction()->setToolTip("Status Bar Action");
    _settingsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection");

    _settingsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox);
    _settingsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getStatusColorAction().setDefaultWidgetFlags(StringAction::Label);
    _computeHolder.getComputeAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _computeHolder.getComputeAction().setIcon(mv::util::StyledIcon("play"));
    _computeHolder.getComputeAction().setText("Compute");
    _settingsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);

    _tableViewAction = new QTableView();
    _tableViewAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableViewAction->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableViewAction->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableViewAction->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _tableViewAction->setAlternatingRowColors(false);
    _tableViewAction->setSortingEnabled(true);
    _tableViewAction->setShowGrid(true);
    _tableViewAction->setGridStyle(Qt::SolidLine);
    _tableViewAction->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableViewAction->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableViewAction->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableViewAction->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableViewAction->setCornerButtonEnabled(false);
    _tableViewAction->setWordWrap(false);
    _tableViewAction->setTabKeyNavigation(false);
    _tableViewAction->setAcceptDrops(false);
    _tableViewAction->setDropIndicatorShown(false);
    _tableViewAction->setDragEnabled(false);
    _tableViewAction->setDragDropMode(QAbstractItemView::NoDragDrop);
    _tableViewAction->setDragDropOverwriteMode(false);
    _tableViewAction->setAutoScroll(false);
    _tableViewAction->setAutoScrollMargin(16);
    _tableViewAction->setAutoFillBackground(true);
    _tableViewAction->setFrameShape(QFrame::NoFrame);
    _tableViewAction->setFrameShadow(QFrame::Plain);
    _tableViewAction->setLineWidth(0);
    _tableViewAction->setMidLineWidth(0);
    _tableViewAction->setFocusPolicy(Qt::NoFocus);
    _tableViewAction->setContextMenuPolicy(Qt::NoContextMenu);
    _tableViewAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableViewAction->setMinimumSize(QSize(0, 0));
    _tableViewAction->setMaximumSize(QSize(16777215, 16777215));
    _tableViewAction->setBaseSize(QSize(0, 0));
    _tableViewAction->setFocusPolicy(Qt::StrongFocus);
    _tableViewAction->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    //only highlight multiple rows if shiuft is pressed
    _tableViewAction->setSelectionBehavior(QAbstractItemView::SelectRows);

    const auto updateStatus = [this]() -> void {

        auto string = _settingsHolder.getStatusColorAction().getString();
        QString labelText = "";
        QString backgroundColor = "none";
        /*
        if (string == "C")
        {
            _computeHolder.getComputeAction().setDisabled(true);
            //if (_popupMessageInit->isVisible())
            //{
                //_popupMessageInit->hide();
            //}
        }
        else
        {
            _computeHolder.getComputeAction().setDisabled(false);
        }
        */

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




        // Update the _statusBarActionWidget with the new label text and background color
        _computeHolder.getStatusBarAction()->showMessage("Status: " + labelText);
        _computeHolder.getStatusBarAction()->setStyleSheet("QStatusBar{padding-left:8px;background:" + backgroundColor + ";color:white;}");


        };
    connect(&_settingsHolder.getStatusColorAction(), &StringAction::stringChanged, this, updateStatus);
    const auto scatterplotSelection = [this]() -> void
        {
            QStringList scatterplotNames = {};
            QString selectedCurrentText = _settingsHolder.getScatterplotViewSelectionAction().getCurrentText();

            auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
            if (scatterplotViewFactory)
            {

                for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
                {
                    scatterplotNames.append(plugin->getGuiName());
                }
            }
            //check if the current text is in the scatterplotNames
            if (!scatterplotNames.contains(selectedCurrentText))
            {
                selectedCurrentText = "";
                if (scatterplotNames.size() > 0)
                {
                    selectedCurrentText = scatterplotNames[0];
                }
            }

            _settingsHolder.getScatterplotViewSelectionAction().setOptions(scatterplotNames);
            _settingsHolder.getScatterplotViewSelectionAction().setCurrentText(selectedCurrentText);


        };

    connect(&mv::plugins(), &AbstractPluginManager::pluginAdded, this, scatterplotSelection);

    _settingsHolder.getClusterDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == ClusterType;
        });
    _settingsHolder.getPointDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });
    _settingsHolder.getEmbeddingDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });

    const auto updateClusterDataset = [this]() -> void {
        if (_settingsHolder.getClusterDatasetAction().getCurrentDataset().isValid())
        {
            auto clusterFullDataset = mv::data().getDataset<Clusters>(_settingsHolder.getClusterDatasetAction().getCurrentDataset().getDatasetId());
            auto clusters = clusterFullDataset->getClusters();
            QStringList clusterNames = {};
            if (!clusters.isEmpty())
            {
                for (auto cluster : clusters)
                {
                    clusterNames.append(cluster.getName());
                }
            }
            
            _settingsHolder.getClusterSelectionAction().setOptions(clusterNames);

        }
        else
        {
            _settingsHolder.getClusterSelectionAction().setOptions({});
        }
        _settingsHolder.getStatusColorAction().setString("M");
        };

    connect(&_settingsHolder.getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset);

    const auto updatePointDataset = [this]() -> void {
        if (_settingsHolder.getPointDatasetAction().getCurrentDataset().isValid())
        {
            auto dataset = mv::data().getDataset<Points>(_settingsHolder.getPointDatasetAction().getCurrentDataset().getDatasetId());
            if (dataset.isValid())
            {
                _settingsHolder.getGeneSelectionAction().setPointsDataset(dataset);

            }


        }
        else
        {
            _settingsHolder.getGeneSelectionAction().setPointsDataset({});
        }
        _settingsHolder.getStatusColorAction().setString("M");
        };

    connect(&_settingsHolder.getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset);

    const auto updateEmbeddingDataset = [this]() -> void {
        if (_settingsHolder.getEmbeddingDatasetAction().getCurrentDataset().isValid())
        {

            auto fullDataset = mv::data().getDataset<Points>(_settingsHolder.getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
            if (fullDataset.isValid())
            {

                const auto mainSelectionChanged = [this]() -> void {
                    _settingsHolder.getStatusColorAction().setString("M");
                    qDebug() << "Main selection changed";
                    };
                connect(&fullDataset, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged);

            }


        }
        _settingsHolder.getStatusColorAction().setString("M");
        };

    connect(&_settingsHolder.getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset);



    const auto computeInfo = [this]() -> void {

        _settingsHolder.getStatusColorAction().setString("R");
        _computeHolder.getComputeAction().setDisabled(true);
        QApplication::processEvents();

        //std::this_thread::sleep_for(std::chrono::seconds(1));

        
        
        Computation();

        _computeHolder.getComputeAction().setDisabled(false);
        };

    connect(&_computeHolder.getComputeAction(), &TriggerAction::triggered, this, computeInfo);





}

inline SettingsAction::SettingsHolder::SettingsHolder(SettingsAction& settingsAction) :
    HorizontalGroupAction(&settingsAction, "Settings options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _embeddingDatasetAction(this, "Embedding dataset"),
    _clusterSelectionAction(this, "Cluster selection"),
    _geneSelectionAction(this, "Gene selection"),
    _statusColorAction(this, "Status color"),
    _scatterplotViewSelectionAction(this, "Scatterplot view selection")
{
    setText("Settings Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDatasetAction);
    addAction(&_embeddingDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_clusterSelectionAction);
    addAction(&_geneSelectionAction);
    addAction(&_scatterplotViewSelectionAction);
}
inline SettingsAction::ComputeHolder::ComputeHolder(SettingsAction& settingsAction) :
    HorizontalGroupAction(&settingsAction, "Compute options"),
    _settingsOptions(settingsAction),
    _computeAction(this, "Compute")
{
    setText("Color Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_computeAction);
    _statusBarAction = new QStatusBar();
    _statusBarAction->setStatusTip("Status");
    _statusBarAction->setMaximumHeight(22);
    //_statusBarAction->setFixedWidth(120);
    //_statusBarAction->setMinimumWidth(100);
    _statusBarAction->setMaximumWidth(800);
    _statusBarAction->setAutoFillBackground(true);
    _statusBarAction->setSizeGripEnabled(false);
    
}
void SettingsAction::Computation()
{
    try
    {
        if (_tableViewAction->model()) {
            _tableViewAction->model()->removeRows(0, _tableViewAction->model()->rowCount());
        }
        QApplication::processEvents();

        auto fullEmbeddingDataset = mv::data().getDataset<Points>(_settingsHolder.getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
        auto fullPointDataset = mv::data().getDataset<Points>(_settingsHolder.getPointDatasetAction().getCurrentDataset().getDatasetId());
        auto fullClusterDataset = mv::data().getDataset<Clusters>(_settingsHolder.getClusterDatasetAction().getCurrentDataset().getDatasetId());

        if (!fullEmbeddingDataset.isValid() || !fullPointDataset.isValid() || !fullClusterDataset.isValid())
        {
            qDebug() << "Invalid datasets";
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        auto selectedIndices = fullEmbeddingDataset->getSelectionIndices();

        if (!(selectedIndices.size() > 0))
        {
            qDebug() << "No selected indices";
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        auto selectedClusterValues = _settingsHolder.getClusterSelectionAction().getSelectedOptions();

        if (selectedClusterValues.isEmpty())
        {
            qDebug() << "No selected cluster values";
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        auto scatterplotViewName = _settingsHolder.getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotViewName.isEmpty())
        {
            qDebug() << "No scatterplot view selected";
            _settingsHolder.getStatusColorAction().setString("E");
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
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        std::vector<float> XYCoordinates(filteredSelectionIndices.size() * 2);
        std::vector<int> preComputedEmbeddingColumnIndices({ 0, 1 });

        if (XYCoordinates.size() != filteredSelectionIndices.size() * 2)
        {
            qDebug() << "XYCoordinates size mismatch";
            _settingsHolder.getStatusColorAction().setString("E");
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
                        _settingsHolder.getStatusColorAction().setString("E");
                        return;
                    }
                }
            }
            if (!foundPlugin)
            {
                qDebug() << "Scatterplot plugin not found";
                _settingsHolder.getStatusColorAction().setString("E");
                return;
            }
        }
        else
        {
            qDebug() << "Scatterplot view factory not found";
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        std::vector<float> rotatedXYCordinates;
        try {
            rotatedXYCordinates = rotateAxis(XYCoordinates, rotationAngle);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in rotateAxis:" << ex.what();
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        std::vector<float> yCoordinates;
        try {
            yCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "Y");
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in getCoordinatesOfCells:" << ex.what();
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        std::vector<int> sortedGeneIndices;
        try {
            sortedGeneIndices = getSortedGeneIndices(yCoordinates);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in getSortedGeneIndices:" << ex.what();
            _settingsHolder.getStatusColorAction().setString("E");
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
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        std::vector<int> correlationMatrix;
        try {
            correlationMatrix = computeCorrelation(cellbygenematrix, filteredSelectionIndices.size(), noOfDimensions);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in computeCorrelation:" << ex.what();
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        QStringList topGenes;
        try {
            topGenes = filterTopNGenesByCorrelation(correlationMatrix, 10, dimensionNames);
        }
        catch (const std::exception& ex) {
            qDebug() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            _settingsHolder.getStatusColorAction().setString("E");
            return;
        }

        QStandardItemModel* model = new QStandardItemModel(0, 1, _tableViewAction);
        model->setHorizontalHeaderLabels(QStringList() << "Gene");
        for (int i = 0; i < topGenes.size(); i++)
        {
            QStandardItem* geneItem = new QStandardItem(topGenes.at(i));
            model->setItem(i, 0, geneItem);
        }
        _tableViewAction->setModel(model);
        _tableViewAction->resizeColumnsToContents();
        _tableViewAction->resizeRowsToContents();
        _tableViewAction->setColumnWidth(0, 200);

        connect(_tableViewAction, &QTableView::clicked, this, [this](const QModelIndex& index) {
            if (!index.isValid()) return;
            auto model = _tableViewAction->model();
            if (!model) return;
            QString value = model->data(index, Qt::DisplayRole).toString();
            _settingsHolder.getGeneSelectionAction().setCurrentDimensionName(value);
            });

        _settingsHolder.getStatusColorAction().setString("C");
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Exception in Computation:" << ex.what();
        _settingsHolder.getStatusColorAction().setString("E");
    }
    catch (...)
    {
        qDebug() << "Unknown exception in Computation";
        _settingsHolder.getStatusColorAction().setString("E");
    }
}

// Rotates the XY coordinates by a given angle
std::vector<float> SettingsAction::rotateAxis(const std::vector<float>& xyCoordinates, float degreeValue) {
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
std::vector<float> SettingsAction::getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis) {
    std::vector<float> coords;
    int offset = (axis == "Y") ? 1 : 0;
    for (size_t i = offset; i < rotatedXYCoordinates.size(); i += 2) {
        coords.push_back(rotatedXYCoordinates[i]);
    }
    return coords;
}

// Sorts gene indices based on the provided Y coordinates
std::vector<int> SettingsAction::getSortedGeneIndices(const std::vector<float>& yCoordinates) {
    std::vector<int> indices(yCoordinates.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return yCoordinates[a] < yCoordinates[b];
        });
    return indices;
}

// Computes a correlation matrix from a cell-by-gene matrix
std::vector<int> SettingsAction::computeCorrelation(const std::vector<float>& cellbygenematrix, int numOfRows, int noOfCols) {
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
QStringList SettingsAction::filterTopNGenesByCorrelation(const std::vector<int>& correlationMatrix, int N, std::vector<QString>  geneNameOrdered) {
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

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    _settingsHolder.getClusterDatasetAction().fromParentVariantMap(variantMap);
    _settingsHolder.getPointDatasetAction().fromParentVariantMap(variantMap);
    _settingsHolder.getEmbeddingDatasetAction().fromParentVariantMap(variantMap);
    _settingsHolder.getClusterSelectionAction().fromParentVariantMap(variantMap);
    _settingsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);
    _settingsHolder.getScatterplotViewSelectionAction().fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _settingsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getClusterSelectionAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getGeneSelectionAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getScatterplotViewSelectionAction().insertIntoVariantMap(variantMap);

    return variantMap;
}