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

    class SettingsHolder : public VerticalGroupAction
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

        const IntegralAction& getTopNAction() const { return _topNAction; }
        IntegralAction& getTopNAction() { return _topNAction; }

        const TriggerAction& getDeSelectRowAction() const { return _deSelectRowAction; }
        TriggerAction& getDeSelectRowAction() { return _deSelectRowAction; }

        const DimensionPickerAction& getEmbeddingDimensionXAction() const { return _embeddingDimensionXAction; }
        DimensionPickerAction& getEmbeddingDimensionXAction() { return _embeddingDimensionXAction; }

        const DimensionPickerAction& getEmbeddingDimensionYAction() const { return _embeddingDimensionYAction; }
        DimensionPickerAction& getEmbeddingDimensionYAction() { return _embeddingDimensionYAction; }

    protected:
        SettingsAction&             _settingsOptions;
        DatasetPickerAction        _clusterDatasetAction;
        DatasetPickerAction        _pointDatasetAction;
        DatasetPickerAction        _embeddingDatasetAction;
        OptionAction               _scatterplotViewSelectionAction;
        OptionsAction              _clusterSelectionAction;
        DimensionPickerAction      _geneSelectionAction;
        StringAction               _statusColorAction;
        IntegralAction              _topNAction;
        TriggerAction                _deSelectRowAction;
        DimensionPickerAction                _embeddingDimensionXAction;
        DimensionPickerAction                _embeddingDimensionYAction;

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
    SettingsAction(LayerSurferPlugin& LayerSurferPlugin);

    ComputeHolder& getComputeHolder() { return _computeHolder; }
    SettingsHolder& getSettingsHolder() { return _settingsHolder; }

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