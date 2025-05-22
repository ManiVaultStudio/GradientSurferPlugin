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


    _settingsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _settingsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _settingsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset");
    _settingsHolder.getClusterSelectionAction().setToolTip("Cluster Selection");
    _settingsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _settingsHolder.getStatusColorAction().setToolTip("Status Color");
    _computeHolder.getComputeAction().setToolTip("Compute Action");

    _settingsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox);
    _settingsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getStatusColorAction().setDefaultWidgetFlags(StringAction::Label);
    _computeHolder.getComputeAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _computeHolder.getComputeAction().setIcon(mv::util::StyledIcon("play"));
    _computeHolder.getComputeAction().setText("Compute");

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
        _settingsHolder.getStatusColorAction().setString("C");
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
    _statusColorAction(this, "Status color")
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


    if (_tableViewAction->model()) {
        _tableViewAction->model()->removeRows(0, _tableViewAction->model()->rowCount());
    }
    QApplication::processEvents();

    auto fullEmbeddingDataset = mv::data().getDataset<Points>(_settingsHolder.getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
    auto fullPointDataset = mv::data().getDataset<Points>(_settingsHolder.getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto fullClusterDataset = mv::data().getDataset<Clusters>(_settingsHolder.getClusterDatasetAction().getCurrentDataset().getDatasetId());

    if (!fullEmbeddingDataset.isValid() || !fullPointDataset.isValid() || !fullClusterDataset.isValid() )
    {
        qDebug() << "Invalid datasets";
        _settingsHolder.getStatusColorAction().setString("E");
        return;
    }

    auto selectedIndices = fullEmbeddingDataset->getSelectionIndices();
    
    if (!selectedIndices.size()>0)
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




    std::sort(selectedIndices.begin(), selectedIndices.end());
    auto clusterValues = fullClusterDataset->getClusters();



    // Filters cells by cell type and returns their indices

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
    



    // Gets XY coordinates for the given cell indices
    //std::vector<int> XYCoordinates(2 * filteredSelectionIndices.size(), 0);
    //for (const auto& index : filteredSelectionIndices)
    //{
    //    auto xCoordinate = fullEmbeddingDataset->getValueAt(2 * index);
    //    auto yCoordinate = fullEmbeddingDataset->getValueAt(2 * index + 1);
    //}
    std::vector<float> XYCoordinates(filteredSelectionIndices.size() * 2);

    std::vector<int> preComputedEmbeddingColumnIndices(2);

    std::vector<std::seed_seq::result_type>_selectedIndicesFromStorage;

    fullEmbeddingDataset->populateDataForDimensions(XYCoordinates, preComputedEmbeddingColumnIndices, filteredSelectionIndices);




    float angle = 0.0;
    std::vector<int> rotatedXYCordinates= rotateAxis(XYCoordinates,angle);


    std::vector<int> yCoordinates = getCoordinatesOfCells(rotatedXYCordinates,"Y");


    std::vector<int> sortedGeneIndices = getSortedGeneIndices(yCoordinates);

    std::vector<float> cellbygenematrix;

    for (int i = 0; i < 100; i++)
    {
        cellbygenematrix.push_back(0.0);
    }
    std::vector<int> correlationMatrix = computeCorrelation(cellbygenematrix);


    QStringList topGenes = filterTopNGenesByCorrelation(correlationMatrix, 10); 

    //populate table
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

}



// Rotates the XY coordinates by a given angle
std::vector<int> SettingsAction::rotateAxis(const std::vector<float>& xyCoordinates, float angle) {
    std::vector<int> rotated;

    return rotated;
}

// Extracts either X or Y coordinates from the rotated coordinates
std::vector<int> SettingsAction::getCoordinatesOfCells(const std::vector<int>& rotatedXYCoordinates, const std::string& axis) {
    std::vector<int> coords;
    int offset = (axis == "Y") ? 1 : 0;
    for (size_t i = offset; i < rotatedXYCoordinates.size(); i += 2) {
        coords.push_back(rotatedXYCoordinates[i]);
    }
    return coords;
}

// Sorts gene indices based on the provided Y coordinates
std::vector<int> SettingsAction::getSortedGeneIndices(const std::vector<int>& yCoordinates) {
    std::vector<int> indices(yCoordinates.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return yCoordinates[a] < yCoordinates[b];
        });
    return indices;
}

// Computes a correlation matrix from a cell-by-gene matrix
std::vector<int> SettingsAction::computeCorrelation(const std::vector<float>& cellbygenematrix) {
    std::vector<int> correlation;

    return correlation;
}

// Filters the top N genes by correlation
QStringList SettingsAction::filterTopNGenesByCorrelation(const std::vector<int>& correlationMatrix, int N) {
    QStringList topGenes;

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

}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _settingsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getClusterSelectionAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getGeneSelectionAction().insertIntoVariantMap(variantMap);

    return variantMap;
}