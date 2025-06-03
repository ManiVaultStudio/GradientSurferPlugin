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
#include "HighPerfLineChart.h"

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

    class Dataset1OptionsHolder : public VerticalGroupAction
    {
    public:
        Dataset1OptionsHolder(SettingsAction& settingsAction);


        const DatasetPickerAction& getClusterDatasetAction() const { return _clusterDatasetAction; }
        DatasetPickerAction& getClusterDatasetAction() { return _clusterDatasetAction; }

        const DatasetPickerAction& getPointDatasetAction() const { return _pointDatasetAction; }
        DatasetPickerAction& getPointDatasetAction() { return _pointDatasetAction; }

        const DatasetPickerAction& getEmbeddingDatasetAction() const { return _embeddingDatasetAction; }
        DatasetPickerAction& getEmbeddingDatasetAction() { return _embeddingDatasetAction; }


        const OptionAction& getScatterplotViewSelectionAction() const { return _scatterplotViewSelectionAction; }
        OptionAction& getScatterplotViewSelectionAction() { return _scatterplotViewSelectionAction; }



        const DimensionPickerAction& getEmbeddingDimensionXAction() const { return _embeddingDimensionXAction; }
        DimensionPickerAction& getEmbeddingDimensionXAction() { return _embeddingDimensionXAction; }

        const DimensionPickerAction& getEmbeddingDimensionYAction() const { return _embeddingDimensionYAction; }
        DimensionPickerAction& getEmbeddingDimensionYAction() { return _embeddingDimensionYAction; }

        const DimensionPickerAction& getGeneSelectionAction() const { return _geneSelectionAction; }
        DimensionPickerAction& getGeneSelectionAction() { return _geneSelectionAction; }

        //new options
        const DecimalRangeAction& getCorrelationRangeAction() const { return _correlationRangeAction; }
        DecimalRangeAction& getCorrelationRangeAction() { return _correlationRangeAction; }

        const ToggleAction& getFocusSelectionAction() const { return _focusSelectionAction; }
        ToggleAction& getFocusSelectionAction() { return _focusSelectionAction; }

        const ToggleAction& getHidePointSelectionOutlineAction() const { return _hidePointSelectionAction; }
        ToggleAction& getHidePointSelectionOutlineAction() { return _hidePointSelectionAction; }

        const ToggleAction& getFreezeSelectionAction() const { return _freezeSelectionAction; }
        ToggleAction& getFreezeSelectionAction() { return _freezeSelectionAction; }

        const DecimalAction& getConnectStatsLineAction() const { return _connectStatsLineAction; }
        DecimalAction& getConnectStatsLineAction() { return _connectStatsLineAction; }
        const TriggerAction& getDeSelectRowAction() const { return _deSelectRowAction; }
        TriggerAction& getDeSelectRowAction() { return _deSelectRowAction; }

    protected:
        SettingsAction&                     _settingsOptions;
        
        
        //Dataset1 Options
        DatasetPickerAction                 _clusterDatasetAction;
        DatasetPickerAction                 _pointDatasetAction;
        DatasetPickerAction                 _embeddingDatasetAction;
        OptionAction                        _scatterplotViewSelectionAction;
        DimensionPickerAction               _embeddingDimensionXAction;
        DimensionPickerAction               _embeddingDimensionYAction;
        DimensionPickerAction               _geneSelectionAction;

        //new options
        DecimalRangeAction                  _correlationRangeAction;
        ToggleAction                        _focusSelectionAction;
        ToggleAction                        _hidePointSelectionAction;
        ToggleAction                        _freezeSelectionAction;
        DecimalAction                       _connectStatsLineAction;
        TriggerAction                       _deSelectRowAction;

    };

    class Dataset2OptionsHolder : public VerticalGroupAction
    {
    public:
        Dataset2OptionsHolder(SettingsAction& settingsAction);


        // Dataset2 getters
        const DatasetPickerAction& getClusterDataset2Action() const { return _clusterDataset2Action; }
        DatasetPickerAction& getClusterDataset2Action() { return _clusterDataset2Action; }

        const DatasetPickerAction& getPointDataset2Action() const { return _pointDataset2Action; }
        DatasetPickerAction& getPointDataset2Action() { return _pointDataset2Action; }

        const DatasetPickerAction& getEmbeddingDataset2Action() const { return _embeddingDataset2Action; }
        DatasetPickerAction& getEmbeddingDataset2Action() { return _embeddingDataset2Action; }

        const OptionAction& getScatterplotViewSelection2Action() const { return _scatterplotViewSelection2Action; }
        OptionAction& getScatterplotViewSelection2Action() { return _scatterplotViewSelection2Action; }

        const DimensionPickerAction& getEmbeddingDimension2XAction() const { return _embeddingDimension2XAction; }
        DimensionPickerAction& getEmbeddingDimension2XAction() { return _embeddingDimension2XAction; }

        const DimensionPickerAction& getEmbeddingDimension2YAction() const { return _embeddingDimension2YAction; }
        DimensionPickerAction& getEmbeddingDimension2YAction() { return _embeddingDimension2YAction; }

        const DimensionPickerAction& getGeneSelection2Action() const { return _geneSelection2Action; }
        DimensionPickerAction& getGeneSelection2Action() { return _geneSelection2Action; }

        const DecimalRangeAction& getCorrelationRange2Action() const { return _correlationRange2Action; }
        DecimalRangeAction& getCorrelationRange2Action() { return _correlationRange2Action; }

        const ToggleAction& getFocusSelection2Action() const { return _focusSelection2Action; }
        ToggleAction& getFocusSelection2Action() { return _focusSelection2Action; }

        const ToggleAction& getHidePointSelectionOutline2Action() const { return _hidePointSelection2Action; }
        ToggleAction& getHidePointSelectionOutline2Action() { return _hidePointSelection2Action; }

        const ToggleAction& getFreezeSelection2Action() const { return _freezeSelection2Action; }
        ToggleAction& getFreezeSelection2Action() { return _freezeSelection2Action; }

        const TriggerAction& getDeSelectRow2Action() const { return _deSelectRow2Action; }
        TriggerAction& getDeSelectRow2Action() { return _deSelectRow2Action; }

    protected:
        SettingsAction& _settingsOptions;


        //Dataset2 Options
        DatasetPickerAction                 _clusterDataset2Action;
        DatasetPickerAction                 _pointDataset2Action;
        DatasetPickerAction                 _embeddingDataset2Action;
        OptionAction                        _scatterplotViewSelection2Action;
        DimensionPickerAction               _embeddingDimension2XAction;
        DimensionPickerAction               _embeddingDimension2YAction;
        DimensionPickerAction               _geneSelection2Action;

        DecimalRangeAction                  _correlationRange2Action;
        ToggleAction                        _focusSelection2Action;
        ToggleAction                        _hidePointSelection2Action;
        ToggleAction                        _freezeSelection2Action;
        TriggerAction                       _deSelectRow2Action;

    };

    class CommonOptionsHolder : public VerticalGroupAction
    {
    public:
        CommonOptionsHolder(SettingsAction& settingsAction);


        const OptionsAction& getClusterSelectionAction() const { return _clusterSelectionAction; }
        OptionsAction& getClusterSelectionAction() { return _clusterSelectionAction; }

        const TriggerAction& getComputeAction() const { return _computeAction; }
        TriggerAction& getComputeAction() { return _computeAction; }




        const ToggleAction& getAutoComputeAction() const { return _autoComputeAction; }
        ToggleAction& getAutoComputeAction() { return _autoComputeAction; }

        const ToggleAction& getPcaColoringAction() const { return _pcaColoringAction; }
        ToggleAction& getPcaColoringAction() { return _pcaColoringAction; }

        const OptionsAction& getDualPCAColoringAction() const { return _dualPCAColoringAction; }
        OptionsAction& getDualPCAColoringAction() { return _dualPCAColoringAction; }

        const DimensionPickerAction& getPcaDimensionForColoringAction() const { return _pcaDimensionForColoringAction; }
        DimensionPickerAction& getPcaDimensionForColoringAction() { return _pcaDimensionForColoringAction; }

        const OptionAction& getCorrelationCalculationTypeAction() const { return _correlationCalculationTypeAction; }
        OptionAction& getCorrelationCalculationTypeAction() { return _correlationCalculationTypeAction; }

        const OptionAction& getDataset1PcSelectionForComputationAction() const { return _dataset1PcSelectionForComputationAction; }
        OptionAction& getDataset1PcSelectionForComputationAction() { return _dataset1PcSelectionForComputationAction; }



    protected:
        SettingsAction& _settingsOptions;

        //Selection Options

        
        OptionsAction                       _clusterSelectionAction;
        TriggerAction              _computeAction;


        ToggleAction                _autoComputeAction;
        ToggleAction                 _pcaColoringAction;
        OptionsAction                 _dualPCAColoringAction;
        DimensionPickerAction        _pcaDimensionForColoringAction;
        OptionAction                 _correlationCalculationTypeAction;
        OptionAction                 _dataset1PcSelectionForComputationAction;

    };




    class StatusHolder : public HorizontalGroupAction
    {
    public:
        StatusHolder(SettingsAction& settingsAction);

        const QStatusBar& getStatusBarAction() const { return *_statusBarAction; }
        QStatusBar* getStatusBarAction() { return _statusBarAction; }
        const StringAction& getStatusMessageAction() const { return _statusMessageAction; }
        StringAction& getStatusMessageAction() { return _statusMessageAction; }


    protected:
        SettingsAction& _settingsOptions;
        
        //Computation Options
        QStatusBar*                 _statusBarAction;
        StringAction                        _statusMessageAction;
    };

public:
    SettingsAction(LayerSurferPlugin& LayerSurferPlugin);

    StatusHolder& getStatusHolder() { return _statusHolder; }
    Dataset1OptionsHolder& getDataset1OptionsHolder() { return _dataset1OptionsHolder; }
    Dataset2OptionsHolder& getDataset2OptionsHolder() { return _dataset2OptionsHolder; }
    CommonOptionsHolder& getCommonOptionsHolder() { return _commonOptionsHolder; }


public: // Serialization
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

    QTableView* getTableView1Action() { return _tableView1Action; }
    HighPerfLineChart* getLineChart1Widget() { return _lineChart1Widget; }
    QTableView* getTableView2Action() { return _tableView2Action; }

protected:
    LayerSurferPlugin& _viewerPlugin;
    mv::CoreInterface* _core;
    Dataset1OptionsHolder              _dataset1OptionsHolder;
    Dataset2OptionsHolder              _dataset2OptionsHolder;
    CommonOptionsHolder              _commonOptionsHolder;
    StatusHolder                   _statusHolder;
    QTableView*                 _tableView1Action = nullptr;
    HighPerfLineChart*          _lineChart1Widget = nullptr;
    QTableView*                 _tableView2Action = nullptr;
    friend class ChannelAction;
};