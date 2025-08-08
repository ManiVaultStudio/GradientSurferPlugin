#pragma once

#include <actions/WidgetAction.h>
#include <actions/IntegralAction.h>
#include <actions/OptionAction.h>
#include <actions/OptionsAction.h>
#include <actions/VariantAction.h>
#include "ClusterData/ClusterData.h"
#include "PointData/PointData.h"
#include <actions/ToggleAction.h>
#include "actions/DatasetPickerAction.h"
#include <PointData/DimensionPickerAction.h>
#include "event/EventListener.h"
#include "actions/Actions.h"
#include "Plugin.h"
#include "DataHierarchyItem.h"
#include "Set.h"
#include <AnalysisPlugin.h>
#include <memory>
#include <algorithm>    
#include <QDebug>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QPushButton>
#include <QGridLayout>
#include <QFormLayout>
#include <QString>
#include <string>
#include<QRadioButton>
#include <event/Event.h>
#include<actions/ColorMap1DAction.h>
#include <set>
#include <actions/HorizontalToolbarAction.h>
#include <actions/VerticalToolbarAction.h>
#include "QStatusBar"
#include "PcaFast.h"
#include "ComputationUtils.h"

using namespace mv::gui;
class QMenu;
class GradientSurferPlugin;
class SettingsAction;


namespace mv
{
    class CoreInterface;
}


class Computation : public WidgetAction
{

public:
    Computation(GradientSurferPlugin& GradientSurferPlugin, SettingsAction& SettingsAction);

private:
    void initializeState();
    void setupDebounceTimers();
    void setupPlotSelectionOptions();
    void setupRangeColoringHandlers();
    void setupEmbeddingDimensionHandlers();
    void setupTableHandlers();
    void setupDatasetFilters();
    void setupClusterUpdateHandlers();
    void setupGeneSelectionHandlers();
    void setupEmbeddingDatasetHandlers();
    void setupComputeHandlers();
    void setupColoringHandlers();
    void setupScatterplotHandlers();
    void setupStatusMessageHandler();
    void setupPointDatasetHandlers();
    
    void setStatusMessage(const QString& status, const QString& message, const QString& color);
    void ManualCompute();
    void AutoCompute();

    // Helper functions for AutoCompute refactor
    bool validateAndRetrieveDatasets(
        mv::Dataset<Clusters>& fullClusterDataset1,
        mv::Dataset<Clusters>& fullClusterDataset2,
        int& xEmbeddingIndex,
        int& yEmbeddingIndex,
        std::vector<int>& selectedIndices,
        QString& scatterplotViewName, int& topNRows,
        int& bottomNRows
    );

    bool createDatasetsIfNeeded();
    void updateClusterOptions();
    void processAfterPCAComplete();
    bool getRotationAngle(const QString& scatterplotViewName, float& rotationAngle);
    bool populateEmbeddingCoordinates(
        int xEmbeddingIndex, int yEmbeddingIndex,
        std::vector<float>& XYCoordinates);
    bool populateAndUpdateDatasets(
        mv::Dataset<Points>& fullPointDataset,
        const std::vector<float>& rotatedXYCordinates,
        /*const std::vector<float>& XYCordinates,*/
        int noOfDimensions,
        const std::vector<int>& columnIndices,
        const std::vector<QString>& dimensionNames);
    bool setupAndTriggerPCA();
    void setSelectionAndAutoCompute();
    void correlationAfterPCA();
    //void populateLineChartColorPointData();
    void colorScatterPlots();
    void colorLinePlots();
    void coloringScatterPlotsOld();
    void coloringLinePlotsOld();
    void clearLinePlots();
    void processAfterPCADatasetsPopulated();
    void clearTables();
    //void  normalizeData();
    void enableOptions();
    void disableOptions();
    void coorelationScoresForPCs();
    //void selectionModeAltToggleLogic(QString scatterplotName);
    void findCommonGenes();
    void datasetNamesCheck();
protected:
    GradientSurferPlugin&          _viewerPlugin;
    SettingsAction&             _settingsAction;
    mv::Dataset<Points>         _selectedCellPointDataset = nullptr;
    mv::Dataset<Points>         _rotatedSelectedEmbeddingDataset = nullptr;
    mv::Dataset<Points>         _pcaSelectedEmbeddingDataset = nullptr;
    mv::Dataset<Points>         _embeddingDataset1 = nullptr;
    mv::Dataset<Points>         _embeddingDataset2 = nullptr;
    //mv::Dataset<Points>         _selectedEmbeddingDataset = nullptr;
    mv::Dataset<Points>         _selectedGenePointDataset1 = nullptr;
    mv::Dataset<Points>         _selectedGenePointDataset2 = nullptr;
    //mv::Dataset<Points>         _selectedEmbeddingPointDataset1 = nullptr;
    //mv::Dataset<Points>         _selectedEmbeddingPointDataset2 = nullptr;
    std::vector<int>            _filteredSelectionIndices;
    mv::Dataset<Points>         _pcaCommonGenesDataset1 = nullptr;
    mv::Dataset<Points>         _pcaCommonGenesDataset2 = nullptr;
    //mv::Dataset<Clusters>       _pcaClusterDatasetColoring1 = nullptr;
    //mv::Dataset<Clusters>       _pcaClusterDatasetColoring2 = nullptr;
    mv::Dataset<Points>         _lineChartPointDataset1 = nullptr;
    mv::Dataset<Clusters>       _lineChartClusterColoringDataset1 = nullptr;
    //mv::Dataset<Points>       _lineChartPointColoringDataset1 = nullptr;
    mv::Dataset<Clusters>       _scatterplotClusterColoringDataset1 = nullptr;
    mv::Dataset<Clusters>       _scatterplotClusterColoringDataset2 = nullptr;
    mv::Dataset<Points>       _scatterplotPointColoringDataset1 = nullptr;
    mv::Dataset<Points>       _scatterplotPointColoringDataset2 = nullptr;
    //mv::Dataset<Points>         _normalizedFullDataset1 = nullptr;
    //mv::Dataset<Points>         _normalizedFullDataset2 = nullptr;
    QString                    _currentNormalizationMethod = "";
    //mv::Dataset<Points>       _lineChartColorPCAPointDataset1 = nullptr;
    mv::plugin::AnalysisPlugin* _pcaAnalysisCommonGenesDataset1Plugin = nullptr;
    mv::plugin::AnalysisPlugin* _pcaAnalysisCommonGenesDataset2Plugin = nullptr;
    SelectedPCData              _selectedPCData1;
    QTimer _clusterSelectionDebounceTimer;
    QTimer _NRangeDebounceTimer;
    QTimer _lineExpressionColoringRangeDebounceTimer;
    QTimer _scatterplot1ExpressionColoringRangeDebounceTimer;
    QTimer _scatterplot2ExpressionColoringRangeDebounceTimer;
    QTimer _colorUpdateDebounceTimer;
    //QTimer _scatterplotColorUpdateDebounceTimer;
    //QTimer _lineplotColorUpdateDebounceTimer;
    bool      _pcaDataset1Complete = false;
    bool      _pcaDataset2Complete = false;
    int             _numOfPointsInLine1=0;
    int             _numOfPointsInLine2 = 0;
    QString    _selectedGene1 = "";
    QString    _selectedGene2 = "";
    bool _isAutoComputeComputing = false;
    bool _isManualComputeComputing = false;
    bool _isPCATriggerComputing = false;
    PcaFast _pca1;
    bool _blockSelectionTrigger=false;
    std::vector<int>   _selectedClusterIndices1;
    std::vector<int>   _selectedClusterIndices2;
    bool _optionsDisabled = false;
    mv::ForegroundTask  _foregroundTask;
    bool _taskDisable = false;
    bool _blockColorAllCellswithExpression=false;
    //bool    _pacColoringPaused = false;
    //bool _geneSelectionPaused = false;
    //bool _blockSelectionMode = false;
    //bool _blockMutualToggle = false;
    bool _blockToggle = false;
    bool _projectOpened = true;
    bool _isColoringScatterplots = false;
    bool _isColoringLineplots = false;
    bool _selectionBlocker = false;
    int  _selectionDataset = -1;
    bool _selectionModeEnable= false;
    //QMetaObject::Connection _mainSelectionChangedConnection;
    //QMetaObject::Connection _pixelSelectionStartedConnection;
    QMetaObject::Connection _embeddingDataset1Connection;
    QMetaObject::Connection _embeddingDataset2Connection;
    friend class ChannelAction;
};