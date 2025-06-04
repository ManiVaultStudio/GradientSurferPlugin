#include "SettingsAction.h"

#include <iostream>
#include <set>
#include "LayerSurferPlugin.h"
#include<string>  
#include <QFileDialog>
#include <QPageLayout>
#include <QWebEngineView>
#include <chrono>
#include <typeinfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QSet>
#include <QJsonValue>
using namespace mv;
using namespace mv::gui;

SettingsAction::SettingsAction(LayerSurferPlugin& LayerSurferPlugin) :
    WidgetAction(&LayerSurferPlugin, "LayerSurferPlugin"),
    _viewerPlugin(LayerSurferPlugin),
    _dataset1OptionsHolder(*this),
    _dataset2OptionsHolder(*this),
    _commonOptionsHolder(*this),
    _statusHolder(*this)
{
    setSerializationName("LayerSurfer:Settings");
    _dataset1OptionsHolder.getClusterDatasetAction().setSerializationName("LayerSurfer:Clusterdataset");
    _dataset1OptionsHolder.getPointDatasetAction().setSerializationName("LayerSurfer:PointDataset");
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setSerializationName("LayerSurfer:EmbeddingDataset");
    _commonOptionsHolder.getClusterSelectionAction().setSerializationName("LayerSurfer:ClusterSelection");
    _dataset1OptionsHolder.getGeneSelectionAction().setSerializationName("LayerSurfer:GeneSelection");
    _dataset2OptionsHolder.getGeneSelection2Action().setSerializationName("LayerSurfer:GeneSelection2");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setSerializationName("LayerSurfer:ScatterplotViewSelection");
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().setSerializationName("LayerSurfer:EmbeddingDimensionX");
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().setSerializationName("LayerSurfer:EmbeddingDimensionY");
    _dataset1OptionsHolder.getCorrelationRangeAction().setSerializationName("LayerSurfer:CorrelationRange");
    
    _dataset2OptionsHolder.getCorrelationRange2Action().setSerializationName("LayerSurfer:CorrelationRange2");
    _dataset2OptionsHolder.getClusterDataset2Action().setSerializationName("LayerSurfer:ClusterDataset2");
    _dataset2OptionsHolder.getPointDataset2Action().setSerializationName("LayerSurfer:PointDataset2");
    _dataset2OptionsHolder.getEmbeddingDataset2Action().setSerializationName("LayerSurfer:EmbeddingDataset2");
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().setSerializationName("LayerSurfer:ScatterplotViewSelection2");
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().setSerializationName("LayerSurfer:EmbeddingDimension2X");
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().setSerializationName("LayerSurfer:EmbeddingDimension2Y");



    //_commonOptionsHolder.getAutoComputeAction().setSerializationName("LayerSurfer:AutoCompute");
    //_commonOptionsHolder.getPcaColoringAction().setSerializationName("LayerSurfer:PcaColoring");
    //_commonOptionsHolder.getDualPCAColoringAction().setSerializationName("LayerSurfer:DualPcaColoring");
    //_commonOptionsHolder.getPcaDimensionForColoringAction().setSerializationName("LayerSurfer:PcaDimensionForColoring");
    //_commonOptionsHolder.getCorrelationCalculationTypeAction().setSerializationName("LayerSurfer:CorrelationCalculationType");
    //_commonOptionsHolder.getDataset1PcSelectionForComputationAction().setSerializationName("LayerSurfer:Dataset1PcSelectionForComputation");


    _dataset1OptionsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _dataset1OptionsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset");
    _commonOptionsHolder.getClusterSelectionAction().setToolTip("Cluster Selection");
    _dataset1OptionsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _dataset2OptionsHolder.getGeneSelection2Action().setToolTip("Gene Selection2");
    _statusHolder.getStatusMessageAction().setToolTip("Status Message");
    _commonOptionsHolder.getComputeAction().setToolTip("Compute Action");
    _commonOptionsHolder.getComputePCAAction().setToolTip("Compute PCA Action");
    _statusHolder.getStatusBarAction()->setToolTip("Status Bar Action");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection");
    _dataset1OptionsHolder.getDeSelectRowAction().setToolTip("DeSelect Row Action");
    _dataset2OptionsHolder.getDeSelectRow2Action().setToolTip("DeSelect Row Action 2");
    _dataset1OptionsHolder.getFocusSelectionAction().setToolTip("Focus Selection Action");
    _dataset2OptionsHolder.getFocusSelection2Action().setToolTip("Focus Selection Action 2");
    _dataset1OptionsHolder.getHidePointSelectionOutlineAction().setToolTip("Hide Point Selection Outline Action");
    _dataset2OptionsHolder.getHidePointSelectionOutline2Action().setToolTip("Hide Point Selection Outline Action 2");
    _dataset1OptionsHolder.getFreezeSelectionAction().setToolTip("Freeze Selection Action");
    _dataset2OptionsHolder.getFreezeSelection2Action().setToolTip("Freeze Selection Action 2");
    _dataset1OptionsHolder.getCorrelationRangeAction().setToolTip("Correlation Range Action");
    _dataset1OptionsHolder.getLineSmoothingWindowAction().setToolTip("Line Smoothing Window Action");
    _dataset2OptionsHolder.getCorrelationRange2Action().setToolTip("Correlation Range Action 2");
    _dataset2OptionsHolder.getClusterDataset2Action().setToolTip("Cluster Dataset 2");
    _dataset2OptionsHolder.getPointDataset2Action().setToolTip("Point Dataset 2");
    _dataset2OptionsHolder.getEmbeddingDataset2Action().setToolTip("Embedding Dataset 2");
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().setToolTip("Scatterplot View Selection 2");
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().setToolTip("Embedding Dimension 2 X");
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().setToolTip("Embedding Dimension 2 Y");

    _dataset1OptionsHolder.getConnectStatsLineAction().setToolTip("Connect Stats Line Action");


    _commonOptionsHolder.getAutoComputeAction().setToolTip("Auto Compute Action");
    _commonOptionsHolder.getPcaColoringAction().setToolTip("PCA Coloring Action");
    _commonOptionsHolder.getDualPCAColoringAction().setToolTip("PCA Dual Coloring Action");
    _commonOptionsHolder.getPcaDimensionForColoringAction().setToolTip("PCA Dimension for Coloring Action");
    _commonOptionsHolder.getCorrelationCalculationTypeAction().setToolTip("Correlation Calculation Type Action");
    _commonOptionsHolder.getDataset1PcSelectionForComputationAction().setToolTip("Dataset1 PCA Selection for Computation Action");

    _commonOptionsHolder.getAutoComputeAction().setChecked(true);
    _commonOptionsHolder.getPcaColoringAction().setChecked(false);




    _dataset1OptionsHolder.getConnectStatsLineAction().setMaximum(1.0);
    _dataset1OptionsHolder.getConnectStatsLineAction().setMinimum(0.0);
    _dataset1OptionsHolder.getConnectStatsLineAction().setValue(0.5);
    _dataset1OptionsHolder.getConnectStatsLineAction().setSingleStep(0.01);

    _dataset1OptionsHolder.getFocusSelectionAction().setChecked(false);
    _dataset1OptionsHolder.getHidePointSelectionOutlineAction().setChecked(false);
    _dataset1OptionsHolder.getFreezeSelectionAction().setChecked(false);

    _dataset2OptionsHolder.getFocusSelection2Action().setChecked(false);
    _dataset2OptionsHolder.getHidePointSelectionOutline2Action().setChecked(false);
    _dataset2OptionsHolder.getFreezeSelection2Action().setChecked(false);


    _dataset1OptionsHolder.getCorrelationRangeAction().setLimitsMaximum(1.00);
    _dataset1OptionsHolder.getCorrelationRangeAction().setLimitsMinimum(-1.00);
    _dataset1OptionsHolder.getCorrelationRangeAction().setMaximum(0.50);
    _dataset1OptionsHolder.getCorrelationRangeAction().setMinimum(-1.00);

    _dataset1OptionsHolder.getLineSmoothingWindowAction().setMaximum(20);
    _dataset1OptionsHolder.getLineSmoothingWindowAction().setMinimum(2);
    _dataset1OptionsHolder.getLineSmoothingWindowAction().setValue(10);

    _dataset2OptionsHolder.getCorrelationRange2Action().setLimitsMaximum(1.00);
    _dataset2OptionsHolder.getCorrelationRange2Action().setLimitsMinimum(-1.00);
    _dataset2OptionsHolder.getCorrelationRange2Action().setMaximum(0.50);
    _dataset2OptionsHolder.getCorrelationRange2Action().setMinimum(-1.00);

    _dataset1OptionsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox | OptionsAction::Selection);
    _dataset1OptionsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getGeneSelection2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _statusHolder.getStatusMessageAction().setDefaultWidgetFlags(StringAction::Label);
    _commonOptionsHolder.getComputeAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _commonOptionsHolder.getComputeAction().setIcon(mv::util::StyledIcon("play"));
    _commonOptionsHolder.getComputeAction().setText("Compute");
    _commonOptionsHolder.getComputePCAAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _commonOptionsHolder.getComputePCAAction().setIcon(mv::util::StyledIcon("play"));
    _commonOptionsHolder.getComputePCAAction().setText("Compute PCA");

    _commonOptionsHolder.getAutoComputeAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _commonOptionsHolder.getPcaColoringAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _commonOptionsHolder.getDualPCAColoringAction().setDefaultWidgetFlags(OptionsAction::ComboBox | OptionsAction::Selection);
    _commonOptionsHolder.getDualPCAColoringAction().setOptions({ "Dataset1", "Dataset2" });
    _commonOptionsHolder.getDualPCAColoringAction().setSelectedOptions({ "Dataset1", "Dataset2" });
    _commonOptionsHolder.getPcaDimensionForColoringAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getCorrelationCalculationTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getDataset1PcSelectionForComputationAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getCorrelationCalculationTypeAction().setOptions({ "Absolute","NonAbsolute" });
    _commonOptionsHolder.getCorrelationCalculationTypeAction().setCurrentText("NonAbsolute");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);

    _dataset1OptionsHolder.getDeSelectRowAction().setDefaultWidgetFlags(ToggleAction::PushButtonIconText);
    _dataset2OptionsHolder.getDeSelectRow2Action().setDefaultWidgetFlags(ToggleAction::PushButtonIconText);

    _dataset1OptionsHolder.getFocusSelectionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _dataset2OptionsHolder.getFocusSelection2Action().setDefaultWidgetFlags(ToggleAction::CheckBox);

    _dataset1OptionsHolder.getHidePointSelectionOutlineAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _dataset2OptionsHolder.getHidePointSelectionOutline2Action().setDefaultWidgetFlags(ToggleAction::CheckBox);

    _dataset1OptionsHolder.getFreezeSelectionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _dataset2OptionsHolder.getFreezeSelection2Action().setDefaultWidgetFlags(ToggleAction::CheckBox);

    _dataset1OptionsHolder.getCorrelationRangeAction().setDefaultWidgetFlags(IntegralRangeAction::Slider  | IntegralRangeAction::MaximumSpinBox | IntegralRangeAction::MinimumSpinBox);
    _dataset2OptionsHolder.getCorrelationRange2Action().setDefaultWidgetFlags(IntegralRangeAction::Slider | IntegralRangeAction::MaximumSpinBox | IntegralRangeAction::MinimumSpinBox );

    _dataset1OptionsHolder.getLineSmoothingWindowAction().setDefaultWidgetFlags(IntegralAction::Slider | IntegralAction::SpinBox );

    _dataset2OptionsHolder.getClusterDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getPointDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().setDefaultWidgetFlags(OptionAction::ComboBox);


    _tableView1Action = new QTableView();
    _tableView1Action->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableView1Action->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableView1Action->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableView1Action->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _tableView1Action->setAlternatingRowColors(false);
    _tableView1Action->setSortingEnabled(true);
    _tableView1Action->setShowGrid(true);
    _tableView1Action->setGridStyle(Qt::SolidLine);
    _tableView1Action->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableView1Action->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableView1Action->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableView1Action->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableView1Action->setCornerButtonEnabled(false);
    _tableView1Action->setWordWrap(false);
    _tableView1Action->setTabKeyNavigation(false);
    _tableView1Action->setAcceptDrops(false);
    _tableView1Action->setDropIndicatorShown(false);
    _tableView1Action->setDragEnabled(false);
    _tableView1Action->setDragDropMode(QAbstractItemView::NoDragDrop);
    _tableView1Action->setDragDropOverwriteMode(false);
    _tableView1Action->setAutoScroll(false);
    _tableView1Action->setAutoScrollMargin(16);
    _tableView1Action->setAutoFillBackground(true);
    _tableView1Action->setFrameShape(QFrame::NoFrame);
    _tableView1Action->setFrameShadow(QFrame::Plain);
    _tableView1Action->setLineWidth(0);
    _tableView1Action->setMidLineWidth(0);
    _tableView1Action->setFocusPolicy(Qt::NoFocus);
    _tableView1Action->setContextMenuPolicy(Qt::NoContextMenu);
    _tableView1Action->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableView1Action->setMinimumSize(QSize(0, 0));
    _tableView1Action->setMaximumSize(QSize(16777215, 16777215));
    _tableView1Action->setBaseSize(QSize(0, 0));
    _tableView1Action->setFocusPolicy(Qt::StrongFocus);
    _tableView1Action->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    //only highlight multiple rows if shiuft is pressed
    _tableView1Action->setSelectionBehavior(QAbstractItemView::SelectRows);

    _tableView2Action = new QTableView();
    _tableView2Action->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableView2Action->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableView2Action->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableView2Action->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _tableView2Action->setAlternatingRowColors(false);
    _tableView2Action->setSortingEnabled(true);
    _tableView2Action->setShowGrid(true);
    _tableView2Action->setGridStyle(Qt::SolidLine);
    _tableView2Action->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableView2Action->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableView2Action->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableView2Action->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableView2Action->setCornerButtonEnabled(false);
    _tableView2Action->setWordWrap(false);
    _tableView2Action->setTabKeyNavigation(false);
    _tableView2Action->setAcceptDrops(false);
    _tableView2Action->setDropIndicatorShown(false);
    _tableView2Action->setDragEnabled(false);
    _tableView2Action->setDragDropMode(QAbstractItemView::NoDragDrop);
    _tableView2Action->setDragDropOverwriteMode(false);
    _tableView2Action->setAutoScroll(false);
    _tableView2Action->setAutoScrollMargin(16);
    _tableView2Action->setAutoFillBackground(true);
    _tableView2Action->setFrameShape(QFrame::NoFrame);
    _tableView2Action->setFrameShadow(QFrame::Plain);
    _tableView2Action->setLineWidth(0);
    _tableView2Action->setMidLineWidth(0);
    _tableView2Action->setFocusPolicy(Qt::NoFocus);
    _tableView2Action->setContextMenuPolicy(Qt::NoContextMenu);
    _tableView2Action->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableView2Action->setMinimumSize(QSize(0, 0));
    _tableView2Action->setMaximumSize(QSize(16777215, 16777215));
    _tableView2Action->setBaseSize(QSize(0, 0));
    _tableView2Action->setFocusPolicy(Qt::StrongFocus);
    _tableView2Action->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    //only highlight multiple rows if shiuft is pressed
    _tableView2Action->setSelectionBehavior(QAbstractItemView::SelectRows);

    _lineChart1Widget = new HighPerfLineChart();


}

inline SettingsAction::Dataset1OptionsHolder::Dataset1OptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Dataset1 Options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _embeddingDatasetAction(this, "Embedding dataset"),
    _scatterplotViewSelectionAction(this, "Scatterplot view selection"),
    _embeddingDimensionXAction(this, "Embedding Dimension X"),
    _embeddingDimensionYAction(this, "Embedding Dimension Y"),
    _geneSelectionAction(this, "Gene Selection"),
    _correlationRangeAction(this, "Correlation Range"),
    _deSelectRowAction(this, "DeSelect Row"),
    _focusSelectionAction(this, "Focus Selection"),
    _hidePointSelectionAction(this, "Hide Point Selection Outline"),
    _freezeSelectionAction(this, "Freeze Selection"),
    _connectStatsLineAction(this, "Stats Line Top Bottom N"),
    _lineSmoothingWindow(this, "Line Smoothing Window")
{
    setText("Dataset1 Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDatasetAction);
    addAction(&_embeddingDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_scatterplotViewSelectionAction);
    addAction(&_embeddingDimensionXAction);
    addAction(&_embeddingDimensionYAction);
    addAction(&_correlationRangeAction);
    addAction(&_deSelectRowAction);
    addAction(&_focusSelectionAction);
    addAction(&_hidePointSelectionAction);
    addAction(&_freezeSelectionAction);
    addAction(&_connectStatsLineAction);
    addAction(&_lineSmoothingWindow);
}


inline SettingsAction::Dataset2OptionsHolder::Dataset2OptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Dataset2 Options"),
    _settingsOptions(settingsAction),
    _clusterDataset2Action(this, "Cluster dataset 2"),
    _pointDataset2Action(this, "Point dataset 2"),
    _embeddingDataset2Action(this, "Embedding dataset 2"),
    _scatterplotViewSelection2Action(this, "Scatterplot view selection 2"),
    _embeddingDimension2XAction(this, "Embedding Dimension 2 X"),
    _embeddingDimension2YAction(this, "Embedding Dimension 2 Y"),
    _geneSelection2Action(this, "Gene Selection 2"),
    _correlationRange2Action(this, "Correlation Range 2"),
    _deSelectRow2Action(this, "DeSelect Row 2"),
    _focusSelection2Action(this, "Focus Selection 2"),
    _hidePointSelection2Action(this, "Hide Point Selection Outline 2"),
    _freezeSelection2Action(this, "Freeze Selection 2")
{
    setText("Dataset2 Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDataset2Action);
    addAction(&_embeddingDataset2Action);
    addAction(&_clusterDataset2Action);
    addAction(&_scatterplotViewSelection2Action);
    addAction(&_embeddingDimension2XAction);
    addAction(&_embeddingDimension2YAction);
    addAction(&_correlationRange2Action);
    addAction(&_deSelectRow2Action);
    addAction(&_focusSelection2Action);
    addAction(&_hidePointSelection2Action);
    addAction(&_freezeSelection2Action);

}

inline SettingsAction::CommonOptionsHolder::CommonOptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Shared Options"),
    _settingsOptions(settingsAction),
    _computeAction(this, "Compute"),
    _clusterSelectionAction(this, "Cluster selection"),
    _pcaDimensionForColoringAction(this, "PCA Dimension for Coloring"),
    _dualPCAColoringAction(this, "PCA Coloring"),
    _pcaColoringAction(this, "PCA Coloring"),
    _autoComputeAction(this, "Auto Compute"),
    _correlationCalculationTypeAction(this, "Correlation Calculation Type"),
    _dataset1PcSelectionForComputationAction(this, "Dataset1 PCA Selection for Computation"),
    _computePCAAction(this, "Compute PCA")
{
    setText("Shared Options");
    setIcon(mv::util::StyledIcon("gear"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_clusterSelectionAction);
    addAction(&_computePCAAction);
    addAction(&_dualPCAColoringAction);
    addAction(&_pcaDimensionForColoringAction);
    addAction(&_pcaColoringAction);
    addAction(&_autoComputeAction);
    addAction(&_correlationCalculationTypeAction);
    addAction(&_dataset1PcSelectionForComputationAction);
    addAction(&_computeAction);

}




inline SettingsAction::StatusHolder::StatusHolder(SettingsAction& settingsAction) :
    HorizontalGroupAction(&settingsAction, "Compute Options"),
    _settingsOptions(settingsAction),

    _statusMessageAction(this, "Status message")
{
    setText("Compute Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    _statusBarAction = new QStatusBar();
    _statusBarAction->setStatusTip("Status");
    _statusBarAction->setMaximumHeight(22);
    _statusBarAction->setMaximumWidth(800);
    _statusBarAction->setAutoFillBackground(true);
    _statusBarAction->setSizeGripEnabled(false);
    
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    _dataset1OptionsHolder.getClusterDatasetAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getPointDatasetAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDatasetAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().fromParentVariantMap(variantMap);

    _commonOptionsHolder.getClusterSelectionAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getGeneSelection2Action().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getCorrelationRangeAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getCorrelationRange2Action().fromParentVariantMap(variantMap);

    _dataset2OptionsHolder.getClusterDataset2Action().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getPointDataset2Action().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDataset2Action().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().fromParentVariantMap(variantMap);

    //_commonOptionsHolder.getAutoComputeAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getPcaColoringAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getDualPCAColoringAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getPcaDimensionForColoringAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getCorrelationCalculationTypeAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getDataset1PcSelectionForComputationAction().fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _dataset1OptionsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDatasetAction().insertIntoVariantMap(variantMap);

    _dataset1OptionsHolder.getScatterplotViewSelectionAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().insertIntoVariantMap(variantMap);


    _commonOptionsHolder.getClusterSelectionAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getGeneSelectionAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getGeneSelection2Action().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getCorrelationRangeAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getCorrelationRange2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getClusterDataset2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getPointDataset2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDataset2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().insertIntoVariantMap(variantMap);

    //_commonOptionsHolder.getAutoComputeAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getPcaColoringAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getDualPCAColoringAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getPcaDimensionForColoringAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getCorrelationCalculationTypeAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getDataset1PcSelectionForComputationAction().insertIntoVariantMap(variantMap);

    return variantMap;
}