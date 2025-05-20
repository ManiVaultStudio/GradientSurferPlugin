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

    class ColorSettingsHolder : public VerticalGroupAction
    {
    public:
        ColorSettingsHolder(SettingsAction& settingsAction);


        const DatasetPickerAction& getColorDatasetAction() const { return _colorDatasetAction; }
        DatasetPickerAction& getColorDatasetAction() { return _colorDatasetAction; }

        const OptionsAction& getSelectionStringList() const { return _selectionStringList; }
        OptionsAction& getSelectionStringList() { return _selectionStringList; }

        const ColorAction& getSelectionColorAction() const { return _selectionColorAction; }
        ColorAction& getSelectionColorAction() { return _selectionColorAction; }

        const StringAction& getClusterSelectionString() const { return _clusterSelectionString; }
        StringAction& getClusterSelectionString() { return _clusterSelectionString; }

        const StringAction& getDelimiterString() const { return _delimiterString; }
        StringAction& getDelimiterString() { return _delimiterString; }

        const DimensionPickerAction& getDimensionSelectionAction() const { return _dimensionSelectionAction; }
        DimensionPickerAction& getDimensionSelectionAction() { return _dimensionSelectionAction; }

        const ColorMap1DAction& getColorMapAction() const { return _colorMapAction; }
        ColorMap1DAction& getColorMapAction() { return _colorMapAction; }

        const StringAction& getDimSelectionString() const { return _dimSelectionString; }
        StringAction& getDimSelectionString() { return _dimSelectionString; }

        const ColorAction& getConstantColorAction() const { return _constantColorAction; }
        ColorAction& getConstantColorAction() { return _constantColorAction; }

        const StringAction& getChartTitleAction() const { return _chartTitleAction; }
        StringAction& getChartTitleAction() { return _chartTitleAction; }

    protected:
        SettingsAction& _settingsOptions;
        DatasetPickerAction        _colorDatasetAction;
        OptionsAction              _selectionStringList;
        StringAction               _clusterSelectionString;
        StringAction               _delimiterString;
        ColorAction                _selectionColorAction;
        DimensionPickerAction      _dimensionSelectionAction;
        ColorMap1DAction           _colorMapAction;
        ColorAction                _constantColorAction;   /** Action for picking the constant color */
        StringAction               _dimSelectionString;
        StringAction               _chartTitleAction;
    };


public:
    SettingsAction(LayerSurferPlugin& LayerSurferPlugin, mv::CoreInterface* core);
    ColorSettingsHolder& getColorSettingsHolder() { return _colorSettingsHolder; }

public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

protected:
    LayerSurferPlugin& _viewerPlugin;
    mv::CoreInterface* _core;
    ColorSettingsHolder              _colorSettingsHolder;
    friend class ChannelAction;
};