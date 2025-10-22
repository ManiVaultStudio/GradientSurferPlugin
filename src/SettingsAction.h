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
#include <actions/ColorMap1DAction.h>

using namespace mv::gui;
class QMenu;
class GradientSurferPlugin;

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
class StatusBarAction : public mv::gui::WidgetAction {
public:
    StatusBarAction(QObject* parent, const QString& title)
        : WidgetAction(parent, title), _statusBar(nullptr) {
    }

    QStatusBar* statusBar() {
        if (!_statusBar)
            getWidget(nullptr, 0);
        return _statusBar;
    }

protected:
    QWidget* getWidget(QWidget* parent, const std::int32_t&) override {
        if (!_statusBar) {
            _statusBar = new QStatusBar(parent);
            _statusBar->setStatusTip("Status");
            _statusBar->setMaximumHeight(22);
            _statusBar->setMaximumWidth(800);
            _statusBar->setAutoFillBackground(true);
            _statusBar->setSizeGripEnabled(false);
            _statusBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        }
        return _statusBar;
    }
private:
    QStatusBar* _statusBar;
};

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


        const OptionsAction& getClusterValueSelectionAction() const { return _clusterValueSelectionAction; }
        OptionsAction& getClusterValueSelectionAction() { return _clusterValueSelectionAction; }

        const TriggerAction& getComputeAction() const { return _computeAction; }
        TriggerAction& getComputeAction() { return _computeAction; }




        const ToggleAction& getAutoComputeAction() const { return _autoComputeAction; }
        ToggleAction& getAutoComputeAction() { return _autoComputeAction; }

        const ToggleAction& getPcaColoringAction() const { return _pcaColoringAction; }
        ToggleAction& getPcaColoringAction() { return _pcaColoringAction; }

        //const OptionsAction& getDualPCAColoringAction() const { return _dualPCAColoringAction; }
        //OptionsAction& getDualPCAColoringAction() { return _dualPCAColoringAction; }

        const DimensionPickerAction& getPcaDimensionForColoringAction() const { return _pcaDimensionForColoringAction; }
        DimensionPickerAction& getPcaDimensionForColoringAction() { return _pcaDimensionForColoringAction; }

        const OptionAction& getCorrelationCalculationTypeAction() const { return _correlationCalculationTypeAction; }
        OptionAction& getCorrelationCalculationTypeAction() { return _correlationCalculationTypeAction; }


        const TriggerAction& getComputePCAAction() const { return _computePCAAction; }
        TriggerAction& getComputePCAAction() { return _computePCAAction; }

        /*
        const OptionAction& getNormalizationTypeAction() const { return _normalizationTypeAction; }
        OptionAction& getNormalizationTypeAction() { return _normalizationTypeAction; }

        const ToggleAction& getNormalizeCombinedDatasetsAction() const { return _normalizeCombinedDatasetsAction; }
        ToggleAction& getNormalizeCombinedDatasetsAction() { return _normalizeCombinedDatasetsAction; }
        */

        //const ToggleAction& getSelectionModeAction() const { return _selectionModeAction; }
        //ToggleAction& getSelectionModeAction() { return _selectionModeAction; }


        const IntegralAction& getBottomNRowsAction() const { return _bottomNRowsAction; }
        IntegralAction& getBottomNRowsAction() { return _bottomNRowsAction; }

        const IntegralAction& getTopNRowsAction() const { return _topNRowsAction; }
        IntegralAction& getTopNRowsAction() { return _topNRowsAction; }

        const OptionAction& getComputationTypeAction() const { return _computationTypeAction; }
        OptionAction& getComputationTypeAction() { return _computationTypeAction; }

        const StatusBarAction& getStatusBarAction() const { return *_statusBarActionWidget; }
        StatusBarAction* getStatusBarAction() { return _statusBarActionWidget; }
        const StringAction& getStatusMessageAction() const { return _statusMessageAction; }
        StringAction& getStatusMessageAction() { return _statusMessageAction; }

        //const ToggleAction& getColorAllCellsWithExpressionAction() const { return _colorAllCellsWithExpressionAction; }
        //ToggleAction& getColorAllCellsWithExpressionAction() { return _colorAllCellsWithExpressionAction; }

        const StringAction& getTableSearchBoxAction() const { return _tableSearchBoxAction; }
        StringAction& getTableSearchBoxAction() { return _tableSearchBoxAction; }



        const OptionAction& getClusterDatasetSelectionAction() const { return _clusterDatasetSelectionAction; }
        OptionAction& getClusterDatasetSelectionAction() { return _clusterDatasetSelectionAction; }

        const OptionAction& getComparatorDataset1SelectorAction() const { return _comparatorDataset1SelectorAction; }
        OptionAction& getComparatorDataset1SelectorAction() { return _comparatorDataset1SelectorAction; }

        const OptionAction& getComparatorDataset2SelectorAction() const { return _comparatorDataset2SelectorAction; }
        OptionAction& getComparatorDataset2SelectorAction() { return _comparatorDataset2SelectorAction; }

        const StringAction& getDefaultDatasetNamesAction() const { return _defaultDatasetNamesAction; }
        StringAction& getDefaultDatasetNamesAction() { return _defaultDatasetNamesAction; }

        const ToggleAction& getShowAdvancedOptions() const { return _showAdvancedOptions; }
        ToggleAction& getShowAdvancedOptions() { return _showAdvancedOptions; }

        const OptionAction& getSliceSelectionAction() const { return _sliceSelectionAction; }
        OptionAction& getSliceSelectionAction() { return _sliceSelectionAction; }

        const StringAction& getDefaultSliceNamesAction() const { return _defaultSliceNamesAction; }
        StringAction& getDefaultSliceNamesAction() { return _defaultSliceNamesAction; }



    protected:
        SettingsAction& _settingsOptions;

        
        OptionsAction                       _clusterValueSelectionAction;
        TriggerAction              _computeAction;
        TriggerAction              _computePCAAction;

        ToggleAction                _autoComputeAction;
        ToggleAction                 _pcaColoringAction;
        //OptionsAction                 _dualPCAColoringAction;
        DimensionPickerAction        _pcaDimensionForColoringAction;
        OptionAction                 _correlationCalculationTypeAction;
        //ToggleAction                 _normalizeCombinedDatasetsAction;
        //OptionAction                _normalizationTypeAction;
        //ToggleAction                _selectionModeAction;



        IntegralAction              _bottomNRowsAction;
        IntegralAction              _topNRowsAction;
        OptionAction               _computationTypeAction;
        ToggleAction               _showAdvancedOptions;
        StatusBarAction* _statusBarActionWidget;
        StringAction                        _statusMessageAction;
        //ToggleAction         _colorAllCellsWithExpressionAction;
        StringAction         _tableSearchBoxAction;
        OptionAction       _clusterDatasetSelectionAction;
        OptionAction       _comparatorDataset1SelectorAction;
        OptionAction       _comparatorDataset2SelectorAction;
        StringAction       _defaultDatasetNamesAction;
        OptionAction       _sliceSelectionAction;
        StringAction        _defaultSliceNamesAction;

    };


    class CustomizationOptionsHolder : public VerticalGroupAction
    {
    public:
        CustomizationOptionsHolder(SettingsAction& settingsAction);

        const ColorMap1DAction& getColoringMapSelectionAction() const { return _coloringMapSelectionAction; }
        ColorMap1DAction& getColoringMapSelectionAction() { return _coloringMapSelectionAction; }




        const ToggleAction& getFocusSelectionAction() const { return _focusSelectionAction; }
        ToggleAction& getFocusSelectionAction() { return _focusSelectionAction; }

        const ToggleAction& getHidePointSelectionOutlineAction() const { return _hidePointSelectionAction; }
        ToggleAction& getHidePointSelectionOutlineAction() { return _hidePointSelectionAction; }


        const DecimalAction& getLowRangeColoringPercentileAction() const { return _lowRangeColoringPercentileAction; }
        DecimalAction& getLowRangeColoringPercentileAction() { return _lowRangeColoringPercentileAction; }

        const DecimalAction& getHighRangeColoringPercentileAction() const { return _highRangeColoringPercentileAction; }
        DecimalAction& getHighRangeColoringPercentileAction() { return _highRangeColoringPercentileAction; }


        const IntegralAction& getSmoothingWindowPercentageAction() const { return _smoothingWindowPercentageAction; }
        IntegralAction& getSmoothingWindowPercentageAction() { return _smoothingWindowPercentageAction; }

    protected:
        SettingsAction& _settingsOptions;

        ColorMap1DAction            _coloringMapSelectionAction;
        DecimalAction       _lowRangeColoringPercentileAction;
        DecimalAction       _highRangeColoringPercentileAction;
        ToggleAction                        _focusSelectionAction;
        ToggleAction                        _hidePointSelectionAction;
        IntegralAction      _smoothingWindowPercentageAction;
    };

    class ExperimentOptionsHolder : public VerticalGroupAction
    {
    public:
        ExperimentOptionsHolder(SettingsAction& settingsAction);

        const TriggerAction& getSaveTablesAction() const { return _saveTablesAction; }
        TriggerAction& getSaveTablesAction() { return _saveTablesAction; }

        const TriggerAction& getStoreSelectionAction() const { return _storeSelectionAction; }
        TriggerAction& getStoreSelectionAction() { return _storeSelectionAction; }

        const TriggerAction& getLoadSelectionAction() const { return _loadSelectionAction; }
        TriggerAction& getLoadSelectionAction() { return _loadSelectionAction; }

        const DatasetPickerAction& getRotationDatasetAction() const { return _rotationDatasetAction; }
        DatasetPickerAction& getRotationDatasetAction() { return _rotationDatasetAction; }

        const DatasetPickerAction& getSelectionDatasetAction() const { return _selectionDatasetAction; }
        DatasetPickerAction& getSelectionDatasetAction() { return _selectionDatasetAction; }

        const TriggerAction& getStoreCoordinatesAction() const { return _storeCoordinatesAction; }
        TriggerAction& getStoreCoordinatesAction() { return _storeCoordinatesAction; }

        const TriggerAction& getLoadCoordinatesAction() const { return _loadCoordinatesAction; }
        TriggerAction& getLoadCoordinatesAction() { return _loadCoordinatesAction; }

        const DecimalAction& getRotationDegreeAction() const { return _rotationDegreeAction; }
        DecimalAction& getRotationDegreeAction() { return _rotationDegreeAction; }

        const TriggerAction& getApplyRotationAction() const { return _applyRotationAction; }
        TriggerAction& getApplyRotationAction() { return _applyRotationAction; }

        const TriggerAction& getApplyFlipAction() const { return _applyFlipAction; }
        TriggerAction& getApplyFlipAction() { return _applyFlipAction; }

        const OptionAction& getFlipTypeAction() const { return _flipTypeAction; }
        OptionAction& getFlipTypeAction() { return _flipTypeAction; }

        ToggleAction& getAutoComputationAction() { return _autoComputationAction; }
        const ToggleAction& getAutoComputationAction() const { return _autoComputationAction; }

        TriggerAction& getAutoComputationTriggerAction() { return _autoComputationTriggerAction; }
        const TriggerAction& getAutoComputationTriggerAction() const { return _autoComputationTriggerAction; }

        TriggerAction& getSaveSelectedGeneListAction() { return _saveSelectedGeneListAction; }
        const TriggerAction& getSaveSelectedGeneListAction() const { return _saveSelectedGeneListAction; }

    protected:
        SettingsAction& _settingsOptions;
        TriggerAction               _saveTablesAction;
        TriggerAction               _storeSelectionAction;
        TriggerAction               _loadSelectionAction;
        TriggerAction               _storeCoordinatesAction;
        TriggerAction               _loadCoordinatesAction;
        DatasetPickerAction         _rotationDatasetAction;
        DatasetPickerAction         _selectionDatasetAction;
        DecimalAction              _rotationDegreeAction;
        TriggerAction              _applyRotationAction;
        TriggerAction              _applyFlipAction;
        OptionAction               _flipTypeAction;
        ToggleAction               _autoComputationAction;
        TriggerAction              _autoComputationTriggerAction;
        TriggerAction              _saveSelectedGeneListAction;
    };


public:
    SettingsAction(GradientSurferPlugin& GradientSurferPlugin);

    Dataset1OptionsHolder& getDataset1OptionsHolder()   { return _dataset1OptionsHolder; }
    Dataset2OptionsHolder& getDataset2OptionsHolder()   { return _dataset2OptionsHolder; }
    CommonOptionsHolder& getCommonOptionsHolder()       { return _commonOptionsHolder; }
    CustomizationOptionsHolder& getCustomizationOptionsHolder() { return _customizationOptionsHolder; }
    ExperimentOptionsHolder& getExperimentOptionsHolder() { return _experimentOptionsHolder; }


public: 
    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

    QTableView* getTableView0Action()       { return _tableView0Action; }
    QTableView* getTableView2Action()       { return _tableView2Action; }
    QTableView* getTableView1Action()       { return _tableView1Action; }
    QLabel*& getSelectionTableHeaderLabel() { return _selectionTableHeaderLabel; }
    QLabel*& getTable1HeaderLabel()         { return _table1HeaderLabel; }
    QLabel*& getTable2HeaderLabel()         { return _table2HeaderLabel; }

protected:
    GradientSurferPlugin& _viewerPlugin;
    mv::CoreInterface* _core;
    Dataset1OptionsHolder           _dataset1OptionsHolder;
    Dataset2OptionsHolder           _dataset2OptionsHolder;
    CommonOptionsHolder             _commonOptionsHolder;
    ExperimentOptionsHolder        _experimentOptionsHolder;
    CustomizationOptionsHolder      _customizationOptionsHolder;
    QTableView*                     _tableView0Action = nullptr;
    QTableView*                     _tableView2Action = nullptr;
    QTableView*                     _tableView1Action = nullptr;
    QLabel*                         _selectionTableHeaderLabel = nullptr;
    QLabel*                         _table1HeaderLabel = nullptr;
    QLabel*                         _table2HeaderLabel = nullptr;
    friend class ChannelAction;
};