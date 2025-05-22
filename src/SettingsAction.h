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

struct ClusterDetails
{
    QString clusterName;
    QColor color;
    int count;
    int order;
};

namespace mv
{
    class CoreInterface;
}


class SettingsAction : public WidgetAction
{
public:

    class SettingsHolder : public HorizontalGroupAction
    {
    public:
        SettingsHolder(SettingsAction& settingsAction);


        const DatasetPickerAction& getClusterDatasetAction() const { return _clusterDatasetAction; }
        DatasetPickerAction& getClusterDatasetAction() { return _clusterDatasetAction; }

        const DatasetPickerAction& getPointDatasetAction() const { return _pointDatasetAction; }
        DatasetPickerAction& getPointDatasetAction() { return _pointDatasetAction; }

        const DatasetPickerAction& getEmbeddingDatasetAction() const { return _embeddingDatasetAction; }
        DatasetPickerAction& getEmbeddingDatasetAction() { return _embeddingDatasetAction; }

        const OptionsAction& getClusterSelectionAction() const { return _clusterSelectionAction; }
        OptionsAction& getClusterSelectionAction() { return _clusterSelectionAction; }

        const DimensionPickerAction& getGeneSelectionAction() const { return _geneSelectionAction; }
        DimensionPickerAction& getGeneSelectionAction() { return _geneSelectionAction; }

        const StringAction& getStatusColorAction() const { return _statusColorAction; }
        StringAction& getStatusColorAction() { return _statusColorAction; }

        const OptionAction& getScatterplotViewSelectionAction() const { return _scatterplotViewSelectionAction; }
        OptionAction& getScatterplotViewSelectionAction() { return _scatterplotViewSelectionAction; }

    protected:
        SettingsAction& _settingsOptions;
        DatasetPickerAction        _clusterDatasetAction;
        DatasetPickerAction        _pointDatasetAction;
        DatasetPickerAction        _embeddingDatasetAction;
        OptionAction               _scatterplotViewSelectionAction;
        OptionsAction              _clusterSelectionAction;
        DimensionPickerAction      _geneSelectionAction;
        StringAction               _statusColorAction;
    };

    class ComputeHolder : public HorizontalGroupAction
    {
    public:
        ComputeHolder(SettingsAction& settingsAction);

        const QStatusBar& getStatusBarAction() const { return *_statusBarAction; }
        QStatusBar* getStatusBarAction() { return _statusBarAction; }
        
        const TriggerAction& getComputeAction() const { return _computeAction; }
        TriggerAction& getComputeAction() { return _computeAction; }

    protected:
        SettingsAction& _settingsOptions;
        QStatusBar*                 _statusBarAction;
        TriggerAction              _computeAction;
    };

public:
    SettingsAction(LayerSurferPlugin& LayerSurferPlugin, mv::CoreInterface* core);
    ComputeHolder& getComputeHolder() { return _computeHolder; }
    SettingsHolder& getSettingsHolder() { return _settingsHolder; }
private:
    std::vector<float> rotateAxis(const std::vector<float>& xyCoordinates, float angle);
    std::vector<float> getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis);
    std::vector<int> getSortedGeneIndices(const std::vector<float>& yCoordinates);
    std::vector<int> computeCorrelation(const std::vector<float>& cellbygenematrix, int numOfRows, int noOfCols);
    QStringList filterTopNGenesByCorrelation(const std::vector<int>& correlationMatrix, int N, std::vector<QString> geneNameOrdered);
    void Computation();

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

    QTableView* getTableViewAction() { return _tableViewAction; }


protected:
    LayerSurferPlugin& _viewerPlugin;
    mv::CoreInterface* _core;
    SettingsHolder              _settingsHolder;
    ComputeHolder               _computeHolder;
    QTableView*                 _tableViewAction;
    friend class ChannelAction;
};