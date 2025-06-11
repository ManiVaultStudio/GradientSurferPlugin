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

using namespace mv::gui;
class QMenu;
class LayerSurferPlugin;
class SettingsAction;


namespace mv
{
    class CoreInterface;
}

struct CorrelationGeneCoords
{
    int geneIndex;
    QString geneName;
    float correlationValue;
    int   rank;
    bool  isWithinRange;
};

class Computation : public WidgetAction
{

public:
    Computation(LayerSurferPlugin& LayerSurferPlugin, SettingsAction& SettingsAction);

private:
    std::vector<float> rotateAxis(const std::vector<float>& xyCoordinates, float angle);
    std::vector<float> getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis);
    std::vector<int> getSortedCellIndices(const std::vector<float>& yCoordinates);
    void setStatusMessage(const QString& status, const QString& message, const QString& color);
    void computeCorrelationGeneCoords(const std::vector<int>& cellIndices, const mv::Dataset<Points>, const std::vector<float>& coords, std::vector<float>& corrVector);
    void computeCorrelationGeneCoordsFast(const std::vector<int>& cellIndices, const mv::Dataset<Points>, const std::vector<float>& coords, std::vector<float>& corrVector);
    void computeCorrelationGenePC(const mv::Dataset<Points>& fullDataset, const std::vector<float>& selectedPC, std::vector<float>& corrVector);
    void computeCorrelationGenePCFast(const mv::Dataset<Points>& fullDataset, const std::vector<float>& selectedPC, std::vector<float>& corrVector);
    void setupTableModelAndSelection(int datasetNumber);

    void populateGenesByCorrelationRange(const std::vector<float>& correlationVector,float min,float max,const std::vector<QString>& geneNameOrdered, int datasetNumber);
    void ManualCompute();
    void AutoCompute();

    // Helper functions for AutoCompute refactor
    bool validateAndRetrieveDatasets(
        mv::Dataset<Points>& fullPointDataset,
        mv::Dataset<Clusters>& fullClusterDataset,
        int& xEmbeddingIndex,
        int& yEmbeddingIndex,
        std::vector<int>& selectedIndices,
        QString& scatterplotViewName,
        float& minCorrelation,
        float& maxCorrelation
    );

    bool createDatasetsIfNeeded();
    void updateClusterOptions();
    void processAfterPCAComplete();
    bool getRotationAngle(const QString& scatterplotViewName, float& rotationAngle);
    void drawCombinedLines();
    bool populateEmbeddingCoordinates(
        int xEmbeddingIndex, int yEmbeddingIndex,
        std::vector<float>& XYCoordinates);

    bool populateAndUpdateDatasets(
        mv::Dataset<Points>& fullPointDataset,
        const std::vector<float>& rotatedXYCordinates,
        const std::vector<float>& XYCordinates,
        int noOfDimensions,
        const std::vector<int>& columnIndices,
        const std::vector<QString>& dimensionNames);

    bool setupAndTriggerPCA();
    void resetupCorrelationTable(int datasetNumber);
    void setSelectionAndAutoCompute();
    void correlationAfterPCA();

protected:
    LayerSurferPlugin&          _viewerPlugin;
    SettingsAction&             _settingsAction;
    mv::Dataset<Points>         _selectedCellPointDataset = nullptr;
    mv::Dataset<Points>         _rotatedSelectedEmbeddingDataset = nullptr;
    mv::Dataset<Points>         _embeddingDataset = nullptr;
    mv::Dataset<Points>         _selectedEmbeddingDataset = nullptr;
    mv::Dataset<Points>         _selectedGenePointDataset1 = nullptr;
    mv::Dataset<Points>         _selectedGenePointDataset2 = nullptr;
    std::vector<int>            _filteredSelectionIndices;
    std::vector<CorrelationGeneCoords> _geneCorrValues1;
    std::vector<CorrelationGeneCoords> _geneCorrValues2;
    std::vector<CorrelationGeneCoords> _geneCorrValues3;
    std::vector<int>            _geneOfInterestIndices1;
    std::vector<int>            _geneOfInterestIndices2;
    std::vector<int>            _geneOfInterestIndices3;
    QStringList                 _commonGeneNames;
    mv::Dataset<Points>         _pcaCommonGenesDataset1 = nullptr;
    mv::Dataset<Points>         _pcaCommonGenesDataset2 = nullptr;
    mv::plugin::AnalysisPlugin* _pcaAnalysisCommonGenesDataset1Plugin = nullptr;
    mv::plugin::AnalysisPlugin* _pcaAnalysisCommonGenesDataset2Plugin = nullptr;

    QTimer _correlationRangeDebounceTimer1;
    QTimer _clusterSelectionDebounceTimer;
    //QTimer _connectedStatsLineDebounceTimer;
    //QTimer _lineSmoothingWindowDebounceTimer;
    QTimer _correlationRangeDebounceTimer2;

    bool      _pcaDataset1Complete = false;
    bool      _pcaDataset2Complete = false;
    int             _numOfPointsInLine1=0;
    int             _numOfPointsInLine2 = 0;
    QString    _selectedGene1 = "";
    QString    _selectedGene2 = "";

    friend class ChannelAction;
};