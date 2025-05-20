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

        const OptionsAction& getClusterSelectionAction() const { return _clusterSelectionAction; }
        OptionsAction& getClusterSelectionAction() { return _clusterSelectionAction; }

        const DimensionPickerAction& getGeneSelectionAction() const { return _geneSelectionAction; }
        DimensionPickerAction& getGeneSelectionAction() { return _geneSelectionAction; }

    protected:
        SettingsAction& _settingsOptions;
        DatasetPickerAction        _clusterDatasetAction;
        DatasetPickerAction        _pointDatasetAction;
        OptionsAction              _clusterSelectionAction;
        DimensionPickerAction      _geneSelectionAction;
    };


public:
    SettingsAction(LayerSurferPlugin& LayerSurferPlugin, mv::CoreInterface* core);
    SettingsHolder& getSettingsHolder() { return _settingsHolder; }

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

protected:
    LayerSurferPlugin& _viewerPlugin;
    mv::CoreInterface* _core;
    SettingsHolder              _settingsHolder;
    friend class ChannelAction;
};