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
#include <PointData/DimensionPickerAction.h>
#include <QDebug>
#include <QLabel>
#include <string>
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


class Computation : public WidgetAction
{

public:
    Computation(LayerSurferPlugin& LayerSurferPlugin, SettingsAction& SettingsAction);

private:
    std::vector<float> rotateAxis(const std::vector<float>& xyCoordinates, float angle);
    std::vector<float> getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis);
    std::vector<int> getSortedCellIndices(const std::vector<float>& yCoordinates);
    std::vector<int> computeCorrelation(const std::vector<float>& cellbygenematrix, int numOfRows, int noOfCols);

    void computeCorrelationGeneCoords(const std::vector<std::seed_seq::result_type>& cellIndices, const mv::Dataset<Points>, const std::vector<float>& coords, std::vector<float>& corrVector);

    //QStringList filterTopNGenesByCorrelation(const std::vector<int>& correlationMatrix, int N, std::vector<QString> geneNameOrdered);
    QStringList filterTopNGenesByCorrelation(const std::vector<float>& correlationVector, int N, std::vector<QString> geneNameOrdered);
    void Compute();
    void removeDatasets();


    

protected:
    LayerSurferPlugin&          _viewerPlugin;
    SettingsAction&             _settingsAction;
    mv::plugin::AnalysisPlugin* _pcaAnalysisPlugin;
    mv::Dataset<Points>         _pcaDataset;
    mv::Dataset<Points>         _selectedPointDataset;
    mv::Dataset<Points>         _rotatedSelectedEmbeddingDataset;

    friend class ChannelAction;
};