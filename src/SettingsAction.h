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
#include <actions/ColorMap1DAction.h>

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



        const OptionAction& getLineplotViewSelectionAction() const { return _lineplotViewSelectionAction; }
        OptionAction& getLineplotViewSelectionAction() { return _lineplotViewSelectionAction; }




        const DimensionPickerAction& getEmbeddingDimensionXAction() const { return _embeddingDimensionXAction; }
        DimensionPickerAction& getEmbeddingDimensionXAction() { return _embeddingDimensionXAction; }

        const DimensionPickerAction& getEmbeddingDimensionYAction() const { return _embeddingDimensionYAction; }
        DimensionPickerAction& getEmbeddingDimensionYAction() { return _embeddingDimensionYAction; }

        const DimensionPickerAction& getGeneSelectionAction() const { return _geneSelectionAction; }
        DimensionPickerAction& getGeneSelectionAction() { return _geneSelectionAction; }


        const DecimalAction& getScatterplotUpperColoringRangeAction() const { return _scatterplotUpperColoringRangeAction; }
        DecimalAction& getScatterplotUpperColoringRangeAction() { return _scatterplotUpperColoringRangeAction; }
        const DecimalAction& getScatterplotLowerColoringRangeAction() const { return _scatterplotLowerColoringRangeAction; }
        DecimalAction& getScatterplotLowerColoringRangeAction() { return _scatterplotLowerColoringRangeAction; }

        const DecimalAction& getLineplotUpperColoringRangeAction() const { return _lineplotUpperColoringRangeAction; }
        DecimalAction& getLineplotUpperColoringRangeAction() { return _lineplotUpperColoringRangeAction; }
        const DecimalAction& getLineplotLowerColoringRangeAction() const { return _lineplotLowerColoringRangeAction; }
        DecimalAction& getLineplotLowerColoringRangeAction() { return _lineplotLowerColoringRangeAction; }

 


    protected:
        SettingsAction&                     _settingsOptions;
        
        

        DatasetPickerAction                 _clusterDatasetAction;
        DatasetPickerAction                 _pointDatasetAction;
        DatasetPickerAction                 _embeddingDatasetAction;
        OptionAction                        _scatterplotViewSelectionAction;
        OptionAction                        _lineplotViewSelectionAction;
        DimensionPickerAction               _embeddingDimensionXAction;
        DimensionPickerAction               _embeddingDimensionYAction;
        DimensionPickerAction               _geneSelectionAction;


        DecimalAction          _scatterplotUpperColoringRangeAction;
        DecimalAction          _scatterplotLowerColoringRangeAction;
        DecimalAction          _lineplotUpperColoringRangeAction;
        DecimalAction          _lineplotLowerColoringRangeAction;
  
    };

    class Dataset2OptionsHolder : public VerticalGroupAction
    {
    public:
        Dataset2OptionsHolder(SettingsAction& settingsAction);


    
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

        const DimensionPickerAction& getGeneSelection2Action() const { return _geneSelectionAction; }
        DimensionPickerAction& getGeneSelectionAction() { return _geneSelectionAction; }

 
        const DecimalAction& getScatterplotUpperColoringRangeAction() const { return _scatterplotUpperColoringRangeAction; }
        DecimalAction& getScatterplotUpperColoringRangeAction() { return _scatterplotUpperColoringRangeAction; }
        const DecimalAction& getScatterplotLowerColoringRangeAction() const { return _scatterplotLowerColoringRangeAction; }
        DecimalAction& getScatterplotLowerColoringRangeAction() { return _scatterplotLowerColoringRangeAction; }



    protected:
        SettingsAction& _settingsOptions;



        DatasetPickerAction                 _clusterDatasetAction;
        DatasetPickerAction                 _pointDatasetAction;
        DatasetPickerAction                 _embeddingDatasetAction;
        OptionAction                        _scatterplotViewSelectionAction;
        DimensionPickerAction               _embeddingDimensionXAction;
        DimensionPickerAction               _embeddingDimensionYAction;
        DimensionPickerAction               _geneSelectionAction;
        DecimalAction                       _scatterplotUpperColoringRangeAction;
        DecimalAction                       _scatterplotLowerColoringRangeAction;
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


        const TriggerAction& getComputePCAAction() const { return _computePCAAction; }
        TriggerAction& getComputePCAAction() { return _computePCAAction; }

        const OptionAction& getNormalizationTypeAction() const { return _normalizationTypeAction; }
        OptionAction& getNormalizationTypeAction() { return _normalizationTypeAction; }

        const ToggleAction& getNormalizeCombinedDatasetsAction() const { return _normalizeCombinedDatasetsAction; }
        ToggleAction& getNormalizeCombinedDatasetsAction() { return _normalizeCombinedDatasetsAction; }

        const ToggleAction& getSelectionModeAction() const { return _selectionModeAction; }
        ToggleAction& getSelectionModeAction() { return _selectionModeAction; }

        const ColorMap1DAction& getColoringMapSelectionAction() const { return _coloringMapSelectionAction; }
        ColorMap1DAction& getColoringMapSelectionAction() { return _coloringMapSelectionAction; }




        const ToggleAction& getFocusSelectionAction() const { return _focusSelectionAction; }
        ToggleAction& getFocusSelectionAction() { return _focusSelectionAction; }

        const ToggleAction& getHidePointSelectionOutlineAction() const { return _hidePointSelectionAction; }
        ToggleAction& getHidePointSelectionOutlineAction() { return _hidePointSelectionAction; }

        const IntegralAction& getBottomNRowsAction() const { return _bottomNRowsAction; }
        IntegralAction& getBottomNRowsAction() { return _bottomNRowsAction; }

        const IntegralAction& getTopNRowsAction() const { return _topNRowsAction; }
        IntegralAction& getTopNRowsAction() { return _topNRowsAction; }

        const OptionAction& getComputationTypeAction() const { return _computationTypeAction; }
        OptionAction& getComputationTypeAction() { return _computationTypeAction; }



    protected:
        SettingsAction& _settingsOptions;

        
        OptionsAction                       _clusterSelectionAction;
        TriggerAction              _computeAction;
        TriggerAction              _computePCAAction;

        ToggleAction                _autoComputeAction;
        ToggleAction                 _pcaColoringAction;
        OptionsAction                 _dualPCAColoringAction;
        DimensionPickerAction        _pcaDimensionForColoringAction;
        OptionAction                 _correlationCalculationTypeAction;
        ToggleAction                 _normalizeCombinedDatasetsAction;
        OptionAction                _normalizationTypeAction;
        ToggleAction                _selectionModeAction;
        ColorMap1DAction            _coloringMapSelectionAction;

        ToggleAction                        _focusSelectionAction;
        ToggleAction                        _hidePointSelectionAction;

        IntegralAction              _bottomNRowsAction;
        IntegralAction              _topNRowsAction;
        OptionAction               _computationTypeAction;

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
        
 
        QStatusBar*                 _statusBarAction;
        StringAction                        _statusMessageAction;
    };

public:
    SettingsAction(LayerSurferPlugin& LayerSurferPlugin);

    StatusHolder& getStatusHolder() { return _statusHolder; }
    Dataset1OptionsHolder& getDataset1OptionsHolder() { return _dataset1OptionsHolder; }
    Dataset2OptionsHolder& getDataset2OptionsHolder() { return _dataset2OptionsHolder; }
    CommonOptionsHolder& getCommonOptionsHolder() { return _commonOptionsHolder; }


public: 
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

    QTableView* getTableView1Action() { return _tableView1Action; }
    QTableView* getTableView2Action() { return _tableView2Action; }
    QTableView* getTableView3Action() { return _tableView3Action; }

protected:
    LayerSurferPlugin& _viewerPlugin;
    mv::CoreInterface* _core;
    Dataset1OptionsHolder              _dataset1OptionsHolder;
    Dataset2OptionsHolder              _dataset2OptionsHolder;
    CommonOptionsHolder              _commonOptionsHolder;
    StatusHolder                   _statusHolder;
    QTableView*                 _tableView1Action = nullptr;
    QTableView*                 _tableView2Action = nullptr;
    QTableView*                 _tableView3Action = nullptr;
    friend class ChannelAction;
};