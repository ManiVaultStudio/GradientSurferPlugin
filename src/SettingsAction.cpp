#include "SettingsAction.h"

#include <iostream>
#include <set>
#include "GradientSurferPlugin.h"
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

SettingsAction::SettingsAction(GradientSurferPlugin& GradientSurferPlugin) :
    WidgetAction(&GradientSurferPlugin, "GradientSurferPlugin"),
    _viewerPlugin(GradientSurferPlugin),
    _dataset1OptionsHolder(*this),
    _dataset2OptionsHolder(*this),
    _commonOptionsHolder(*this),
    _customizationOptionsHolder(*this),
    _experimentOptionsHolder(*this)
{
    setSerializationName("GradientSurfer:Settings");
    _dataset1OptionsHolder.getClusterDatasetAction().setSerializationName("GradientSurfer:Clusterdataset");
    _dataset1OptionsHolder.getPointDatasetAction().setSerializationName("GradientSurfer:PointDataset");
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setSerializationName("GradientSurfer:EmbeddingDataset");
    _commonOptionsHolder.getTopNRowsAction().setSerializationName("GradientSurfer:TopNRows");
    _commonOptionsHolder.getBottomNRowsAction().setSerializationName("GradientSurfer:BottomNRows");
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().setSerializationName("GradientSurfer:LowRangeColoringPercentile");
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().setSerializationName("GradientSurfer:HighRangeColoringPercentile");
    _dataset1OptionsHolder.getGeneSelectionAction().setSerializationName("GradientSurfer:GeneSelection");
    _dataset2OptionsHolder.getGeneSelectionAction().setSerializationName("GradientSurfer:GeneSelection2");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setSerializationName("GradientSurfer:ScatterplotViewSelection");
    _dataset1OptionsHolder.getLineplotViewSelectionAction().setSerializationName("GradientSurfer:LineplotViewSelection");
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().setSerializationName("GradientSurfer:EmbeddingDimensionX");
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().setSerializationName("GradientSurfer:EmbeddingDimensionY");
    _dataset2OptionsHolder.getClusterDatasetAction().setSerializationName("GradientSurfer:ClusterDataset2");
    _dataset2OptionsHolder.getPointDatasetAction().setSerializationName("GradientSurfer:PointDataset2");
    _dataset2OptionsHolder.getEmbeddingDatasetAction().setSerializationName("GradientSurfer:EmbeddingDataset2");
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().setSerializationName("GradientSurfer:ScatterplotViewSelection2");
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().setSerializationName("GradientSurfer:EmbeddingDimension2X");
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().setSerializationName("GradientSurfer:EmbeddingDimension2Y");
    //_commonOptionsHolder.getClusterValueSelectionAction().setSerializationName("GradientSurfer:ClusterSelection");
    //_commonOptionsHolder.getClusterDatasetSelectionAction().setSerializationName("GradientSurfer:ClusterDatasetSelection");
   //_commonOptionsHolder.getComparatorDataset1SelectorAction().setSerializationName("GradientSurfer:ComparatorDataset1Selector");
    //_commonOptionsHolder.getComparatorDataset2SelectorAction().setSerializationName("GradientSurfer:ComparatorDataset2Selector");
    _commonOptionsHolder.getDefaultDatasetNamesAction().setSerializationName("GradientSurfer:DefaultDatasetNames");
    _commonOptionsHolder.getDefaultSliceNamesAction().setSerializationName("GradientSurfer:DefaultSliceNames");
    //_commonOptionsHolder.getShowAdvancedOptions().setSerializationName("GradientSurfer:ShowAdvancedOptions");
    //_commonOptionsHolder.getSliceSelectionAction().setSerializationName("GradientSurfer:SliceSelection");


    _dataset1OptionsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _dataset1OptionsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset");
    _commonOptionsHolder.getClusterValueSelectionAction().setToolTip("Cluster Selection");
    _dataset1OptionsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _dataset2OptionsHolder.getGeneSelectionAction().setToolTip("Gene Selection2");
    _commonOptionsHolder.getStatusMessageAction().setToolTip("Status Message");
    _commonOptionsHolder.getComputeAction().setToolTip("Compute Action");
    _commonOptionsHolder.getComputePCAAction().setToolTip("Compute PCA Action");
    _commonOptionsHolder.getStatusBarAction()->setToolTip("Status Bar Action");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection");
    _dataset1OptionsHolder.getLineplotViewSelectionAction().setToolTip("Lineplot View Selection");
    _customizationOptionsHolder.getFocusSelectionAction().setToolTip("Focus Selection Action");
    _commonOptionsHolder.getShowAdvancedOptions().setToolTip("Show Advanced Options Action");
    _customizationOptionsHolder.getHidePointSelectionOutlineAction().setToolTip("Hide Point Selection Outline Action");
    //_commonOptionsHolder.getColorAllCellsWithExpressionAction().setToolTip("Color All Cells With Expression Action");
    _commonOptionsHolder.getTableSearchBoxAction().setToolTip("Table Search Box Action");

    _commonOptionsHolder.getTopNRowsAction().setToolTip("Top N Rows Action");
    _dataset1OptionsHolder.getScatterplotUpperColoringRangeAction().setToolTip("Scatterplot Upper Coloring Range");
    _dataset1OptionsHolder.getScatterplotLowerColoringRangeAction().setToolTip("Scatterplot Lower Coloring Range");
    _dataset2OptionsHolder.getScatterplotUpperColoringRangeAction().setToolTip("Scatterplot Upper Coloring Range2");
    _dataset2OptionsHolder.getScatterplotLowerColoringRangeAction().setToolTip("Scatterplot Lower Coloring Range2");
    _dataset1OptionsHolder.getLineplotUpperColoringRangeAction().setToolTip("Lineplot Upper Coloring Range");
    _dataset1OptionsHolder.getLineplotLowerColoringRangeAction().setToolTip("Lineplot Lower Coloring Range");
    _commonOptionsHolder.getBottomNRowsAction().setToolTip("Bottom N Rows Action");
    _customizationOptionsHolder.getSmoothingWindowPercentageAction().setToolTip("Smoothing Window Percentage Action");
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().setToolTip("Low Range Coloring Percentile Action");
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().setToolTip("High Range Coloring Percentile Action");
    _dataset2OptionsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset 2");
    _dataset2OptionsHolder.getPointDatasetAction().setToolTip("Point Dataset 2");
    _dataset2OptionsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset 2");
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection 2");
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().setToolTip("Embedding Dimension 2 X");
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().setToolTip("Embedding Dimension 2 Y");


    _commonOptionsHolder.getAutoComputeAction().setToolTip("Auto Compute Action");

    _commonOptionsHolder.getPcaColoringAction().setToolTip("PCA Coloring Action");
   // _commonOptionsHolder.getSelectionModeAction().setToolTip("Selection Mode Action");
  //  _commonOptionsHolder.getDualPCAColoringAction().setToolTip("PCA Dual Coloring Action");
    _commonOptionsHolder.getPcaDimensionForColoringAction().setToolTip("PCA Dimension Selection Action");
    _commonOptionsHolder.getCorrelationCalculationTypeAction().setToolTip("Correlation Calculation Type Action");

    _commonOptionsHolder.getAutoComputeAction().setChecked(true);

    _commonOptionsHolder.getPcaColoringAction().setChecked(false);
    //_commonOptionsHolder.getSelectionModeAction().setChecked(true);

    _customizationOptionsHolder.getFocusSelectionAction().setChecked(false);
    _commonOptionsHolder.getShowAdvancedOptions().setChecked(false);
    _customizationOptionsHolder.getHidePointSelectionOutlineAction().setChecked(false);
    //_commonOptionsHolder.getColorAllCellsWithExpressionAction().setChecked(false);





    _commonOptionsHolder.getBottomNRowsAction().setMinimum(0);
    _commonOptionsHolder.getBottomNRowsAction().setMaximum(100);
    _commonOptionsHolder.getBottomNRowsAction().setValue(0);
    _commonOptionsHolder.getTopNRowsAction().setMinimum(0);
    _commonOptionsHolder.getTopNRowsAction().setMaximum(100);
    _commonOptionsHolder.getTopNRowsAction().setValue(5);

    _customizationOptionsHolder.getSmoothingWindowPercentageAction().setMinimum(1);
    _customizationOptionsHolder.getSmoothingWindowPercentageAction().setMaximum(100);
    _customizationOptionsHolder.getSmoothingWindowPercentageAction().setNumberOfDecimals(0);
    _customizationOptionsHolder.getSmoothingWindowPercentageAction().setValue(20);


    _customizationOptionsHolder.getLowRangeColoringPercentileAction().setNumberOfDecimals(2);
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().setMinimum(0.00);
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().setMaximum(1.00);
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().setSingleStep(0.01);
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().setValue(0.00);

    _customizationOptionsHolder.getHighRangeColoringPercentileAction().setNumberOfDecimals(2);
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().setMinimum(0.00);
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().setMaximum(1.00);
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().setSingleStep(0.01);
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().setValue(0.95);



    _dataset1OptionsHolder.getScatterplotUpperColoringRangeAction().setMaximum(1.00);
    _dataset1OptionsHolder.getScatterplotUpperColoringRangeAction().setMinimum(-1.00);
    _dataset1OptionsHolder.getScatterplotLowerColoringRangeAction().setMaximum(1.00);
    _dataset1OptionsHolder.getScatterplotLowerColoringRangeAction().setMinimum(-1.00);
    _dataset1OptionsHolder.getScatterplotUpperColoringRangeAction().setSingleStep(0.01);
    _dataset1OptionsHolder.getScatterplotLowerColoringRangeAction().setSingleStep(0.01);
    _dataset1OptionsHolder.getScatterplotUpperColoringRangeAction().setNumberOfDecimals(6);
    _dataset1OptionsHolder.getScatterplotLowerColoringRangeAction().setNumberOfDecimals(6);

    _dataset1OptionsHolder.getLineplotUpperColoringRangeAction().setMaximum(1.00);
    _dataset1OptionsHolder.getLineplotUpperColoringRangeAction().setMinimum(-1.00);
    _dataset1OptionsHolder.getLineplotLowerColoringRangeAction().setMaximum(1.00);
    _dataset1OptionsHolder.getLineplotLowerColoringRangeAction().setMinimum(-1.00);
    _dataset1OptionsHolder.getLineplotUpperColoringRangeAction().setSingleStep(0.01);
    _dataset1OptionsHolder.getLineplotLowerColoringRangeAction().setSingleStep(0.01);
    _dataset1OptionsHolder.getLineplotUpperColoringRangeAction().setNumberOfDecimals(6);
    _dataset1OptionsHolder.getLineplotLowerColoringRangeAction().setNumberOfDecimals(6);


    _dataset2OptionsHolder.getScatterplotUpperColoringRangeAction().setMaximum(1.00);
    _dataset2OptionsHolder.getScatterplotUpperColoringRangeAction().setMinimum(-1.00);
    _dataset2OptionsHolder.getScatterplotLowerColoringRangeAction().setMaximum(1.00);
    _dataset2OptionsHolder.getScatterplotLowerColoringRangeAction().setMinimum(-1.00);
    _dataset2OptionsHolder.getScatterplotUpperColoringRangeAction().setSingleStep(0.01);
    _dataset2OptionsHolder.getScatterplotLowerColoringRangeAction().setSingleStep(0.01);
    _dataset2OptionsHolder.getScatterplotUpperColoringRangeAction().setNumberOfDecimals(6);
    _dataset2OptionsHolder.getScatterplotLowerColoringRangeAction().setNumberOfDecimals(6);

    _dataset1OptionsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getClusterValueSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox | OptionsAction::Selection);
    _dataset1OptionsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getStatusMessageAction().setDefaultWidgetFlags(StringAction::Label);
    _commonOptionsHolder.getComputeAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _commonOptionsHolder.getComputeAction().setIcon(mv::util::StyledIcon("play"));
    _commonOptionsHolder.getComputeAction().setText("Update tables after PCA");
    _commonOptionsHolder.getComputePCAAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _commonOptionsHolder.getComputePCAAction().setIcon(mv::util::StyledIcon("play"));
    _commonOptionsHolder.getComputePCAAction().setText("Compute PCA across datasets");

    _commonOptionsHolder.getAutoComputeAction().setDefaultWidgetFlags(ToggleAction::CheckBox);

    _commonOptionsHolder.getPcaColoringAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    //_commonOptionsHolder.getSelectionModeAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    //_commonOptionsHolder.getDualPCAColoringAction().setDefaultWidgetFlags(OptionsAction::ComboBox | OptionsAction::Selection);
    //_commonOptionsHolder.getDualPCAColoringAction().initialize({ "Dataset1", "Dataset2" }, { "Dataset1", "Dataset2" });
    _commonOptionsHolder.getPcaDimensionForColoringAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getCorrelationCalculationTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getCorrelationCalculationTypeAction().initialize({ "Absolute","NonAbsolute","Positive","Negative"}, "NonAbsolute");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getLineplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);

    //_commonOptionsHolder.getNormalizationTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    //_commonOptionsHolder.getNormalizationTypeAction().initialize({ "None", "ZScore", "MinMax", "Log2","MaxAbs","L2","L1","Mean","Softmax","Robust","UnitRange","Binarize","CPM","Log1p","CPM_Log1p","CPM_Log1p_ZScore"}, "ZScore");

    //_commonOptionsHolder.getNormalizationTypeAction().setToolTip("Normalization Type Action");

    _commonOptionsHolder.getComputationTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getComputationTypeAction().initialize({ "Full PCA", "Partial PCA" }, "Full PCA");
    _commonOptionsHolder.getComputationTypeAction().setToolTip("Computation Type Action");
    _commonOptionsHolder.getClusterDatasetSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getClusterDatasetSelectionAction().setToolTip("Cluster Dataset Selection Action");
    _commonOptionsHolder.getComparatorDataset1SelectorAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getComparatorDataset1SelectorAction().setToolTip("Comparator Dataset 1 Selector Action");
    _commonOptionsHolder.getSliceSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getSliceSelectionAction().setToolTip("Slice Selection Action");
    _commonOptionsHolder.getSliceSelectionAction().initialize({ "P1", "P2", "P3", "P4","P5", "P6", "P7", "P8","P9", "P10", "P11", "P12" }, "P6");
    //_commonOptionsHolder.getComparatorDataset1SelectorAction().initialize({ "Human", "Marmoset", "Macaque" } , "Human");
    //_commonOptionsHolder.getComparatorDataset2SelectorAction().initialize({ "Human", "Marmoset", "Macaque" }, "Marmoset");
    _commonOptionsHolder.getComparatorDataset2SelectorAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _commonOptionsHolder.getComparatorDataset2SelectorAction().setToolTip("Comparator Dataset 2 Selector Action");
    _commonOptionsHolder.getDefaultDatasetNamesAction().setDefaultWidgetFlags(StringAction::LineEdit);
    _commonOptionsHolder.getDefaultDatasetNamesAction().setText("Default Dataset Names");
    _commonOptionsHolder.getDefaultDatasetNamesAction().setToolTip("Default Dataset Names Action");
    _commonOptionsHolder.getDefaultDatasetNamesAction().setString("Human; Marmoset; Macaque; smFISH; MERFISH; Embryo1; Embryo2; Embryo3");
    _commonOptionsHolder.getDefaultSliceNamesAction().setDefaultWidgetFlags(StringAction::LineEdit);
    _commonOptionsHolder.getDefaultSliceNamesAction().setText("Default Slice Names");
    _commonOptionsHolder.getDefaultSliceNamesAction().setToolTip("Default Slice Names Action");
    _commonOptionsHolder.getDefaultSliceNamesAction().setString("P1; P2; P3; P4; P5; P6; P7; P8; P9; P10; P11; P12");

    _customizationOptionsHolder.getColoringMapSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _customizationOptionsHolder.getColoringMapSelectionAction().setColorMap("Viridis");
    _customizationOptionsHolder.getColoringMapSelectionAction().getSettingsAction().setVisible(false);
    _customizationOptionsHolder.getColoringMapSelectionAction().getSettings1DAction().setVisible(false);
    _customizationOptionsHolder.getColoringMapSelectionAction().getSettings2DAction().setVisible(false);

    //_commonOptionsHolder.getNormalizeCombinedDatasetsAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    //_commonOptionsHolder.getNormalizeCombinedDatasetsAction().setChecked(true);



    _customizationOptionsHolder.getFocusSelectionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _commonOptionsHolder.getShowAdvancedOptions().setDefaultWidgetFlags(ToggleAction::CheckBox);

    _customizationOptionsHolder.getHidePointSelectionOutlineAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    //_commonOptionsHolder.getColorAllCellsWithExpressionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);

    _commonOptionsHolder.getTableSearchBoxAction().setDefaultWidgetFlags(StringAction::LineEdit);
    _commonOptionsHolder.getTableSearchBoxAction().setIconText("Search Tables");
    _commonOptionsHolder.getTableSearchBoxAction().setText("Search Tables");
    //_commonOptionsHolder.getTableSearchBoxAction().setClearable(true);
    //QCompleter* completer = new QCompleter(_tableView0Action->model(), this);
    //completer->setCompletionColumn(0);
    //_commonOptionsHolder.getTableSearchBoxAction().setCompleter(completer);
    _commonOptionsHolder.getTableSearchBoxAction().setIconByName("search");
    _commonOptionsHolder.getTableSearchBoxAction().setSearchMode(true);

    _commonOptionsHolder.getTopNRowsAction().setDefaultWidgetFlags(IntegralAction::Slider | IntegralAction::SpinBox);
    _commonOptionsHolder.getBottomNRowsAction().setDefaultWidgetFlags(IntegralAction::Slider | IntegralAction::SpinBox);
    _customizationOptionsHolder.getSmoothingWindowPercentageAction().setDefaultWidgetFlags(IntegralAction::Slider | IntegralAction::SpinBox);
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);

    _dataset1OptionsHolder.getScatterplotUpperColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);
    _dataset1OptionsHolder.getScatterplotLowerColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);
    _dataset1OptionsHolder.getLineplotUpperColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);
    _dataset1OptionsHolder.getLineplotLowerColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);

    _dataset2OptionsHolder.getScatterplotUpperColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);
    _dataset2OptionsHolder.getScatterplotLowerColoringRangeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);


    _dataset2OptionsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);


    _tableView0Action = new QTableView();
    _tableView0Action->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableView0Action->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableView0Action->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableView0Action->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _tableView0Action->setAlternatingRowColors(false);
    _tableView0Action->setSortingEnabled(true);
    _tableView0Action->setShowGrid(true);
    _tableView0Action->setGridStyle(Qt::SolidLine);
    _tableView0Action->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableView0Action->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableView0Action->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableView0Action->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableView0Action->setCornerButtonEnabled(false);
    _tableView0Action->setWordWrap(false);
    _tableView0Action->setTabKeyNavigation(false);
    _tableView0Action->setAcceptDrops(false);
    _tableView0Action->setDropIndicatorShown(false);
    _tableView0Action->setDragEnabled(false);
    _tableView0Action->setDragDropMode(QAbstractItemView::NoDragDrop);
    _tableView0Action->setDragDropOverwriteMode(false);
    _tableView0Action->setAutoScroll(false);
    _tableView0Action->setAutoScrollMargin(16);
    _tableView0Action->setAutoFillBackground(true);
    _tableView0Action->setFrameShape(QFrame::NoFrame);
    _tableView0Action->setFrameShadow(QFrame::Plain);
    _tableView0Action->setLineWidth(0);
    _tableView0Action->setMidLineWidth(0);
    _tableView0Action->setFocusPolicy(Qt::NoFocus);
    _tableView0Action->setContextMenuPolicy(Qt::NoContextMenu);
    _tableView0Action->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableView0Action->setMinimumSize(QSize(0, 0));
    _tableView0Action->setMaximumSize(QSize(16777215, 16777215));
    _tableView0Action->setBaseSize(QSize(0, 0));
    _tableView0Action->setFocusPolicy(Qt::StrongFocus);
    _tableView0Action->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    _tableView0Action->setSelectionBehavior(QAbstractItemView::SelectRows);

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

}

inline SettingsAction::Dataset1OptionsHolder::Dataset1OptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Dataset1 Advanced Options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _embeddingDatasetAction(this, "Embedding dataset"),
    _scatterplotViewSelectionAction(this, "Scatterplot view selection"),
    _embeddingDimensionXAction(this, "Embedding Dimension X"),
    _embeddingDimensionYAction(this, "Embedding Dimension Y"),
    _geneSelectionAction(this, "Gene Selection"),
    _lineplotViewSelectionAction(this, "Lineplot view selection"),
    _scatterplotUpperColoringRangeAction(this, "Scatterplot UpperColoring Range"),
    _scatterplotLowerColoringRangeAction(this, "Scatterplot Lower Coloring Range"),
    _lineplotUpperColoringRangeAction(this, "Lineplot Upper Coloring Range"),
    _lineplotLowerColoringRangeAction(this, "Lineplot Lower Coloring Range")
{
    setText("Dataset1 Advanced Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_embeddingDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_scatterplotViewSelectionAction);
    addAction(&_lineplotViewSelectionAction);
    addAction(&_embeddingDimensionXAction);
    addAction(&_embeddingDimensionYAction);
    //addAction(&_scatterplotUpperColoringRangeAction);
    //addAction(&_scatterplotLowerColoringRangeAction);
    //addAction(&_lineplotUpperColoringRangeAction);
    //addAction(&_lineplotLowerColoringRangeAction);




}


inline SettingsAction::Dataset2OptionsHolder::Dataset2OptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Dataset2 Advanced Options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _embeddingDatasetAction(this, "Embedding dataset"),
    _scatterplotViewSelectionAction(this, "Scatterplot view selection"),
    _embeddingDimensionXAction(this, "Embedding Dimension X"),
    _embeddingDimensionYAction(this, "Embedding Dimension Y"),
    _geneSelectionAction(this, "Gene Selection"),
    _scatterplotUpperColoringRangeAction(this, "Scatterplot UpperColoring Range"),
    _scatterplotLowerColoringRangeAction(this, "Scatterplot Lower Coloring Range")
{
    setText("Dataset2 Advanced Options");
    setIcon(mv::util::StyledIcon("database"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_embeddingDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_scatterplotViewSelectionAction);
    addAction(&_embeddingDimensionXAction);
    addAction(&_embeddingDimensionYAction);
    //addAction(&_scatterplotUpperColoringRangeAction);
    //addAction(&_scatterplotLowerColoringRangeAction);



}

inline SettingsAction::CustomizationOptionsHolder::CustomizationOptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Customization Options"),
    _settingsOptions(settingsAction),
    _focusSelectionAction(this, "Focus"),
    _hidePointSelectionAction(this, "Hide Outline"),
    _lowRangeColoringPercentileAction(this, "Low Percentile"),
    _highRangeColoringPercentileAction(this, "High Percentile"),
    _coloringMapSelectionAction(this, "Color Map"),
    _smoothingWindowPercentageAction(this, "Line Smoothing")
{
    setText("Customization Options");
    setIcon(mv::util::StyledIcon("wrench"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);

    addAction(&_coloringMapSelectionAction);
    addAction(&_lowRangeColoringPercentileAction);
    addAction(&_highRangeColoringPercentileAction);
    addAction(&_focusSelectionAction);
    addAction(&_hidePointSelectionAction);
    addAction(&_smoothingWindowPercentageAction);

}

inline SettingsAction::CommonOptionsHolder::CommonOptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Settings Options"),
    _settingsOptions(settingsAction),
    // PCA-related actions
    _computeAction(this, "Compute PCs"),
    _computePCAAction(this, "Update Tables"),
    _pcaDimensionForColoringAction(this, "PC Select"),
    _pcaColoringAction(this, "PCA Color"),

    // Clustering and selection actions
    _clusterValueSelectionAction(this, "Cluster"),
    _clusterDatasetSelectionAction(this, "Cluster Set"),

    // Data filtering actions
    _topNRowsAction(this, "Top N Rows"),
    _bottomNRowsAction(this, "Bottom N Rows"),


    // Dataset comparison actions
    _showAdvancedOptions(this, "Advanced Options"),
    _comparatorDataset1SelectorAction(this, "Set 1"),
    _comparatorDataset2SelectorAction(this, "Set 2"),
    _defaultDatasetNamesAction(this, "Default Names"),
    _sliceSelectionAction(this, "Slice"),
    _defaultSliceNamesAction(this, "Default Slice Names"),

    // Computation settings actions
    _autoComputeAction(this, "Auto"),
    _correlationCalculationTypeAction(this, "Corr Type"),
    _computationTypeAction(this, "Comp Type"),

    // Status and search actions
    _statusMessageAction(this, "Status"),
    _statusBarActionWidget(new StatusBarAction(this, "Status Bar")),
    _tableSearchBoxAction(this, "Search")
{
    setText("Settings Options");
    setIcon(mv::util::StyledIcon("gear"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);

    //addAction(&_defaultDatasetNamesAction);
    //addAction(&_defaultSliceNamesAction);
    addAction(&_showAdvancedOptions);
    addAction(&_comparatorDataset1SelectorAction);
    addAction(&_comparatorDataset2SelectorAction);
    addAction(&_sliceSelectionAction);

    addAction(&_clusterDatasetSelectionAction);
    addAction(&_clusterValueSelectionAction);

    addAction(&_topNRowsAction);
    addAction(&_bottomNRowsAction);

    addAction(&_pcaDimensionForColoringAction);
    addAction(&_pcaColoringAction);
    
    addAction(&_autoComputeAction);
    addAction(&_computationTypeAction);
    addAction(&_correlationCalculationTypeAction);

    addAction(&_tableSearchBoxAction);

    addAction(&_computePCAAction);
    addAction(&_computeAction);

    //addAction(_statusBarActionWidget);

}

inline SettingsAction::ExperimentOptionsHolder::ExperimentOptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Experiment Options"),
    _settingsOptions(settingsAction),
    _saveTablesAction(this, "Save Tables"),
    _storeSelectionAction(this, "Store Selection"),
    _loadSelectionAction(this, "Load Selection"),
    _storeCoordinatesAction(this, "Store Coordinates"),
    _loadCoordinatesAction(this, "Load Coordinates"),
    _rotationDatasetAction(this, "Rotation Dataset"),
    _rotationDegreeAction(this, "Rotation Degree"),
    _applyRotationAction(this, "Apply Rotation"),
    _applyFlipAction(this, "Apply Flip"),
    _flipTypeAction(this, "Flip Type")
{
    setText("Experiment Options");
    setIcon(mv::util::StyledIcon("flask"));
    setPopupSizeHint(QSize(300, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_saveTablesAction);
    addAction(&_rotationDatasetAction);
    addAction(&_storeSelectionAction);
    addAction(&_loadSelectionAction);
    addAction(&_rotationDegreeAction);
    addAction(&_applyRotationAction);
    addAction(&_flipTypeAction);
    addAction(&_applyFlipAction);
    addAction(&_storeCoordinatesAction);
    addAction(&_loadCoordinatesAction);
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
    //_commonOptionsHolder.getSliceSelectionAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getClusterValueSelectionAction().fromParentVariantMap(variantMap);
    _commonOptionsHolder.getDefaultDatasetNamesAction().fromParentVariantMap(variantMap);
    _commonOptionsHolder.getDefaultSliceNamesAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getClusterDatasetSelectionAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getComparatorDataset1SelectorAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getComparatorDataset2SelectorAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);
    _commonOptionsHolder.getTopNRowsAction().fromParentVariantMap(variantMap);
    _commonOptionsHolder.getBottomNRowsAction().fromParentVariantMap(variantMap);
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().fromParentVariantMap(variantMap);
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().fromParentVariantMap(variantMap);
   
    _dataset2OptionsHolder.getClusterDatasetAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getPointDatasetAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDatasetAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().fromParentVariantMap(variantMap);
    //_commonOptionsHolder.getShowAdvancedOptions().fromParentVariantMap(variantMap);
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

    //_commonOptionsHolder.getSliceSelectionAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getClusterValueSelectionAction().insertIntoVariantMap(variantMap);
    _commonOptionsHolder.getDefaultDatasetNamesAction().insertIntoVariantMap(variantMap);
    _commonOptionsHolder.getDefaultSliceNamesAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getClusterDatasetSelectionAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getComparatorDataset1SelectorAction().insertIntoVariantMap(variantMap);
   // _commonOptionsHolder.getComparatorDataset2SelectorAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getGeneSelectionAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getGeneSelection2Action().insertIntoVariantMap(variantMap);
    _commonOptionsHolder.getTopNRowsAction().insertIntoVariantMap(variantMap);
    _commonOptionsHolder.getBottomNRowsAction().insertIntoVariantMap(variantMap);
    _customizationOptionsHolder.getLowRangeColoringPercentileAction().insertIntoVariantMap(variantMap);
    _customizationOptionsHolder.getHighRangeColoringPercentileAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDatasetAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getScatterplotViewSelectionAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimensionXAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimensionYAction().insertIntoVariantMap(variantMap);
    //_commonOptionsHolder.getShowAdvancedOptions().insertIntoVariantMap(variantMap);


    return variantMap;
}
