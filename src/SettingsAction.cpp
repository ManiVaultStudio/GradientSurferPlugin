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
    _commonOptionsHolder.getTopNRowsAction().setSerializationName("LayerSurfer:TopNRows");
    _commonOptionsHolder.getBottomNRowsAction().setSerializationName("LayerSurfer:BottomNRows");
    _dataset1OptionsHolder.getGeneSelectionAction().setSerializationName("LayerSurfer:GeneSelection");
    _dataset2OptionsHolder.getGeneSelectionAction().setSerializationName("LayerSurfer:GeneSelection2");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setSerializationName("LayerSurfer:ScatterplotViewSelection");
    _dataset1OptionsHolder.getLineplotViewSelectionAction().setSerializationName("LayerSurfer:LineplotViewSelection");
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().setSerializationName("LayerSurfer:EmbeddingDimensionX");
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().setSerializationName("LayerSurfer:EmbeddingDimensionY");
    _dataset2OptionsHolder.getClusterDatasetAction().setSerializationName("LayerSurfer:ClusterDataset2");
    _dataset2OptionsHolder.getPointDatasetAction().setSerializationName("LayerSurfer:PointDataset2");
    _dataset2OptionsHolder.getEmbeddingDatasetAction().setSerializationName("LayerSurfer:EmbeddingDataset2");
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().setSerializationName("LayerSurfer:ScatterplotViewSelection2");
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().setSerializationName("LayerSurfer:EmbeddingDimension2X");
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().setSerializationName("LayerSurfer:EmbeddingDimension2Y");


    _dataset1OptionsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _dataset1OptionsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset");
    _commonOptionsHolder.getClusterSelectionAction().setToolTip("Cluster Selection");
    _dataset1OptionsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _dataset2OptionsHolder.getGeneSelectionAction().setToolTip("Gene Selection2");
    _statusHolder.getStatusMessageAction().setToolTip("Status Message");
    _commonOptionsHolder.getComputeAction().setToolTip("Compute Action");
    _commonOptionsHolder.getComputePCAAction().setToolTip("Compute PCA Action");
    _statusHolder.getStatusBarAction()->setToolTip("Status Bar Action");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection");
    _dataset1OptionsHolder.getLineplotViewSelectionAction().setToolTip("Lineplot View Selection");
    _commonOptionsHolder.getFocusSelectionAction().setToolTip("Focus Selection Action");
    _commonOptionsHolder.getHidePointSelectionOutlineAction().setToolTip("Hide Point Selection Outline Action");

    _commonOptionsHolder.getTopNRowsAction().setToolTip("Top N Rows Action");
    _dataset1OptionsHolder.getExpressionUpperColoringRangeAction().setToolTip("Expression Upper Coloring Range");
    _dataset1OptionsHolder.getExpressionLowerColoringRangeAction().setToolTip("Expression Lower Coloring Range");
    _dataset2OptionsHolder.getExpressionUpperColoringRangeAction().setToolTip("Expression Upper Coloring Range2");
    _dataset2OptionsHolder.getExpressionLowerColoringRangeAction().setToolTip("Expression Lower Coloring Range2");
    _commonOptionsHolder.getBottomNRowsAction().setToolTip("Bottom N Rows Action");
    _dataset2OptionsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset 2");
    _dataset2OptionsHolder.getPointDatasetAction().setToolTip("Point Dataset 2");
    _dataset2OptionsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset 2");
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection 2");
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().setToolTip("Embedding Dimension 2 X");
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().setToolTip("Embedding Dimension 2 Y");


    _commonOptionsHolder.getAutoComputeAction().setToolTip("Auto Compute Action");

    _commonOptionsHolder.getPcaColoringAction().setToolTip("PCA Coloring Action");
    _commonOptionsHolder.getSelectionModeAction().setToolTip("Selection Mode Action");
    _commonOptionsHolder.getDualPCAColoringAction().setToolTip("PCA Dual Coloring Action");
    _commonOptionsHolder.getPcaDimensionForColoringAction().setToolTip("PCA Dimension Selection Action");
    _commonOptionsHolder.getCorrelationCalculationTypeAction().setToolTip("Correlation Calculation Type Action");

    _commonOptionsHolder.getAutoComputeAction().setChecked(true);

    _commonOptionsHolder.getPcaColoringAction().setChecked(false);
    _commonOptionsHolder.getSelectionModeAction().setChecked(true);

    _commonOptionsHolder.getFocusSelectionAction().setChecked(false);
    _commonOptionsHolder.getHidePointSelectionOutlineAction().setChecked(false);





    _commonOptionsHolder.getBottomNRowsAction().setMinimum(0);
    _commonOptionsHolder.getBottomNRowsAction().setMaximum(100);
    _commonOptionsHolder.getBottomNRowsAction().setValue(0);
    _commonOptionsHolder.getTopNRowsAction().setMinimum(0);
    _commonOptionsHolder.getTopNRowsAction().setMaximum(100);
    _commonOptionsHolder.getTopNRowsAction().setValue(20);


    _dataset1OptionsHolder.getExpressionUpperColoringRangeAction().setMaximum(1.00);
    _dataset1OptionsHolder.getExpressionUpperColoringRangeAction().setMinimum(-1.00);
    _dataset1OptionsHolder.getExpressionLowerColoringRangeAction().setMaximum(1.00);
    _dataset1OptionsHolder.getExpressionLowerColoringRangeAction().setMinimum(-1.00);
    _dataset1OptionsHolder.getExpressionUpperColoringRangeAction().setSingleStep(0.01);
    _dataset1OptionsHolder.getExpressionLowerColoringRangeAction().setSingleStep(0.01);
    _dataset1OptionsHolder.getExpressionUpperColoringRangeAction().setNumberOfDecimals(6);
    _dataset1OptionsHolder.getExpressionLowerColoringRangeAction().setNumberOfDecimals(6);


    _dataset2OptionsHolder.getExpressionUpperColoringRangeAction().setMaximum(1.00);
    _dataset2OptionsHolder.getExpressionUpperColoringRangeAction().setMinimum(-1.00);
    _dataset2OptionsHolder.getExpressionLowerColoringRangeAction().setMaximum(1.00);
    _dataset2OptionsHolder.getExpressionLowerColoringRangeAction().setMinimum(-1.00);
    _dataset2OptionsHolder.getExpressionUpperColoringRangeAction().setSingleStep(0.01);
    _dataset2OptionsHolder.getExpressionLowerColoringRangeAction().setSingleStep(0.01);
    _dataset2OptionsHolder.getExpressionUpperColoringRangeAction().setNumberOfDecimals(6);
    _dataset2OptionsHolder.getExpressionLowerColoringRangeAction().setNumberOfDecimals(6);

    _dataset1OptionsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox | OptionsAction::Selection);
    _dataset1OptionsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
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
    _commonOptionsHolder.getSelectionModeAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _commonOptionsHolder.getDualPCAColoringAction().setDefaultWidgetFlags(OptionsAction::ComboBox | OptionsAction::Selection);
    _commonOptionsHolder.getDualPCAColoringAction().initialize({ "Dataset1", "Dataset2" }, { "Dataset1", "Dataset2" });
    _commonOptionsHolder.getPcaDimensionForColoringAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getCorrelationCalculationTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getCorrelationCalculationTypeAction().initialize({ "Absolute","NonAbsolute","Positive","Negative"}, "NonAbsolute");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getLineplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);

    _commonOptionsHolder.getNormalizationTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getNormalizationTypeAction().initialize({ "None", "ZScore", "MinMax", "Log2","MaxAbs","L2","L1","Mean","Softmax","Robust","UnitRange","Binarize","CPM","Log1p","CPM_Log1p","CPM_Log1p_ZScore"}, "ZScore");

    _commonOptionsHolder.getColoringMapSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    //_commonOptionsHolder.getColoringMapSelectionAction().setDisabled(true);
    _commonOptionsHolder.getColoringMapSelectionAction().setColorMap("Viridis");
    _commonOptionsHolder.getColoringMapSelectionAction().getSettingsAction().setVisible(false);
    _commonOptionsHolder.getColoringMapSelectionAction().getSettings1DAction().setVisible(false);
    _commonOptionsHolder.getColoringMapSelectionAction().getSettings2DAction().setVisible(false);

    _commonOptionsHolder.getNormalizeCombinedDatasetsAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _commonOptionsHolder.getNormalizeCombinedDatasetsAction().setChecked(true);



    _commonOptionsHolder.getFocusSelectionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);


    _commonOptionsHolder.getHidePointSelectionOutlineAction().setDefaultWidgetFlags(ToggleAction::CheckBox);


    _commonOptionsHolder.getTopNRowsAction().setDefaultWidgetFlags(IntegralAction::Slider | IntegralAction::SpinBox);
    _commonOptionsHolder.getBottomNRowsAction().setDefaultWidgetFlags(IntegralAction::Slider | IntegralAction::SpinBox);

    _dataset1OptionsHolder.getExpressionUpperColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);
    _dataset1OptionsHolder.getExpressionLowerColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);

    _dataset2OptionsHolder.getExpressionUpperColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);
    _dataset2OptionsHolder.getExpressionLowerColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);


    _dataset2OptionsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);


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

    _tableView2Action->setSelectionBehavior(QAbstractItemView::SelectRows);


    _tableView3Action = new QTableView();
    _tableView3Action->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableView3Action->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableView3Action->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableView3Action->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _tableView3Action->setAlternatingRowColors(false);
    _tableView3Action->setSortingEnabled(true);
    _tableView3Action->setShowGrid(true);
    _tableView3Action->setGridStyle(Qt::SolidLine);
    _tableView3Action->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableView3Action->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableView3Action->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableView3Action->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableView3Action->setCornerButtonEnabled(false);
    _tableView3Action->setWordWrap(false);
    _tableView3Action->setTabKeyNavigation(false);
    _tableView3Action->setAcceptDrops(false);
    _tableView3Action->setDropIndicatorShown(false);
    _tableView3Action->setDragEnabled(false);
    _tableView3Action->setDragDropMode(QAbstractItemView::NoDragDrop);
    _tableView3Action->setDragDropOverwriteMode(false);
    _tableView3Action->setAutoScroll(false);
    _tableView3Action->setAutoScrollMargin(16);
    _tableView3Action->setAutoFillBackground(true);
    _tableView3Action->setFrameShape(QFrame::NoFrame);
    _tableView3Action->setFrameShadow(QFrame::Plain);
    _tableView3Action->setLineWidth(0);
    _tableView3Action->setMidLineWidth(0);
    _tableView3Action->setFocusPolicy(Qt::NoFocus);
    _tableView3Action->setContextMenuPolicy(Qt::NoContextMenu);
    _tableView3Action->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableView3Action->setMinimumSize(QSize(0, 0));
    _tableView3Action->setMaximumSize(QSize(16777215, 16777215));
    _tableView3Action->setBaseSize(QSize(0, 0));
    _tableView3Action->setFocusPolicy(Qt::StrongFocus);
    _tableView3Action->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    _tableView3Action->setSelectionBehavior(QAbstractItemView::SelectRows);

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
    _lineplotViewSelectionAction(this, "Lineplot view selection"),
    _expressionUpperColoringRangeAction(this, "Expression UpperColoring Range"),
    _expressionLowerColoringRangeAction(this, "Expression Lower Coloring Range")
{
    setText("Dataset1 Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_embeddingDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_scatterplotViewSelectionAction);
    addAction(&_lineplotViewSelectionAction);
    addAction(&_embeddingDimensionXAction);
    addAction(&_embeddingDimensionYAction);
    addAction(&_expressionUpperColoringRangeAction);
    addAction(&_expressionLowerColoringRangeAction);




}


inline SettingsAction::Dataset2OptionsHolder::Dataset2OptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Dataset2 Options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _embeddingDatasetAction(this, "Embedding dataset"),
    _scatterplotViewSelectionAction(this, "Scatterplot view selection"),
    _embeddingDimensionXAction(this, "Embedding Dimension X"),
    _embeddingDimensionYAction(this, "Embedding Dimension Y"),
    _geneSelectionAction(this, "Gene Selection"),
    _expressionUpperColoringRangeAction(this, "Expression UpperColoring Range"),
    _expressionLowerColoringRangeAction(this, "Expression Lower Coloring Range")
{
    setText("Dataset2 Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_embeddingDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_scatterplotViewSelectionAction);
    addAction(&_embeddingDimensionXAction);
    addAction(&_embeddingDimensionYAction);
    addAction(&_expressionUpperColoringRangeAction);
    addAction(&_expressionLowerColoringRangeAction);



}

inline SettingsAction::CommonOptionsHolder::CommonOptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Shared Options"),
    _settingsOptions(settingsAction),
    _computeAction(this, "Compute: Expand PC's across datasets"),
    _clusterSelectionAction(this, "Cluster selection"),
    _pcaDimensionForColoringAction(this, "PC Selection"),
    _dualPCAColoringAction(this, "PCA Coloring"),
    _pcaColoringAction(this, "PCA Coloring"),
    _autoComputeAction(this, "Auto Compute"),
    _correlationCalculationTypeAction(this, "Correlation Calculation Type"),
    _computePCAAction(this, "Compute: Selection PCs"),
    _normalizeCombinedDatasetsAction(this, "Normalize Combined Datasets"),
    _normalizationTypeAction(this, "Normalization Type"),
    _selectionModeAction(this, "Selection Mode"),
    _focusSelectionAction(this, "Focus Selection"),
    _hidePointSelectionAction(this, "Hide Point Selection Outline"),
    _coloringMapSelectionAction(this, "Coloring Map Selection"),
    _topNRowsAction(this, "Top N Rows Selection"),
    _bottomNRowsAction(this, "Bottom N Rows Selection")

{
    setText("Shared Options");
    setIcon(mv::util::StyledIcon("gear"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_clusterSelectionAction);
    addAction(&_dualPCAColoringAction);
    addAction(&_focusSelectionAction);
    addAction(&_hidePointSelectionAction);
    addAction(&_topNRowsAction);
    addAction(&_bottomNRowsAction);
    addAction(&_pcaDimensionForColoringAction);
    addAction(&_pcaColoringAction);
    addAction(&_coloringMapSelectionAction);
    addAction(&_autoComputeAction);
    addAction(&_correlationCalculationTypeAction);
    addAction(&_normalizationTypeAction);
    addAction(&_normalizeCombinedDatasetsAction);
    addAction(&_selectionModeAction);
    addAction(&_computePCAAction);
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
    _dataset1OptionsHolder.getLineplotViewSelectionAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().fromParentVariantMap(variantMap);

    _commonOptionsHolder.getClusterSelectionAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);
    _commonOptionsHolder.getTopNRowsAction().fromParentVariantMap(variantMap);
    _commonOptionsHolder.getBottomNRowsAction().fromParentVariantMap(variantMap);

    _dataset2OptionsHolder.getClusterDatasetAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getPointDatasetAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDatasetAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().fromParentVariantMap(variantMap);

}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _dataset1OptionsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDatasetAction().insertIntoVariantMap(variantMap);

    _dataset1OptionsHolder.getScatterplotViewSelectionAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getLineplotViewSelectionAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().insertIntoVariantMap(variantMap);


    _commonOptionsHolder.getClusterSelectionAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getGeneSelectionAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getGeneSelection2Action().insertIntoVariantMap(variantMap);
    _commonOptionsHolder.getTopNRowsAction().insertIntoVariantMap(variantMap);
    _commonOptionsHolder.getBottomNRowsAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDatasetAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().insertIntoVariantMap(variantMap);



    return variantMap;
}
