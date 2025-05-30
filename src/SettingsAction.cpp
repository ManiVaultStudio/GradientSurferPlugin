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
    _selectionOptionsHolder(*this),
    _scatterplotOptionsHolder(*this),
    _computeHolder(*this)
{
    setSerializationName("LayerSurfer:Settings");
    _dataset1OptionsHolder.getClusterDatasetAction().setSerializationName("LayerSurfer:Clusterdataset");
    _dataset1OptionsHolder.getPointDatasetAction().setSerializationName("LayerSurfer:PointDataset");
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setSerializationName("LayerSurfer:EmbeddingDataset");
    _selectionOptionsHolder.getClusterSelectionAction().setSerializationName("LayerSurfer:ClusterSelection");
    _dataset1OptionsHolder.getGeneSelectionAction().setSerializationName("LayerSurfer:GeneSelection");
    _dataset2OptionsHolder.getGeneSelection2Action().setSerializationName("LayerSurfer:GeneSelection2");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setSerializationName("LayerSurfer:ScatterplotViewSelection");
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().setSerializationName("LayerSurfer:EmbeddingDimensionX");
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().setSerializationName("LayerSurfer:EmbeddingDimensionY");
    _selectionOptionsHolder.getCorrelationRangeAction().setSerializationName("LayerSurfer:CorrelationRange");

    _dataset2OptionsHolder.getClusterDataset2Action().setSerializationName("LayerSurfer:ClusterDataset2");
    _dataset2OptionsHolder.getPointDataset2Action().setSerializationName("LayerSurfer:PointDataset2");
    _dataset2OptionsHolder.getEmbeddingDataset2Action().setSerializationName("LayerSurfer:EmbeddingDataset2");
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().setSerializationName("LayerSurfer:ScatterplotViewSelection2");
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().setSerializationName("LayerSurfer:EmbeddingDimension2X");
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().setSerializationName("LayerSurfer:EmbeddingDimension2Y");


    _dataset1OptionsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _dataset1OptionsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset");
    _selectionOptionsHolder.getClusterSelectionAction().setToolTip("Cluster Selection");
    _dataset1OptionsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _dataset2OptionsHolder.getGeneSelection2Action().setToolTip("Gene Selection2");
    _selectionOptionsHolder.getStatusMessageAction().setToolTip("Status Message");
    _computeHolder.getComputeAction().setToolTip("Compute Action");
    _computeHolder.getStatusBarAction()->setToolTip("Status Bar Action");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection");
    _selectionOptionsHolder.getDeSelectRowAction().setToolTip("DeSelect Row Action");
    _scatterplotOptionsHolder.getFocusSelectionAction().setToolTip("Focus Selection Action");
    _scatterplotOptionsHolder.getHidePointSelectionOutlineAction().setToolTip("Hide Point Selection Outline Action");
    _scatterplotOptionsHolder.getFreezeSelectionAction().setToolTip("Freeze Selection Action");
    _selectionOptionsHolder.getCorrelationRangeAction().setToolTip("Correlation Range Action");

    _dataset2OptionsHolder.getClusterDataset2Action().setToolTip("Cluster Dataset 2");
    _dataset2OptionsHolder.getPointDataset2Action().setToolTip("Point Dataset 2");
    _dataset2OptionsHolder.getEmbeddingDataset2Action().setToolTip("Embedding Dataset 2");
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().setToolTip("Scatterplot View Selection 2");
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().setToolTip("Embedding Dimension 2 X");
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().setToolTip("Embedding Dimension 2 Y");


    _scatterplotOptionsHolder.getFocusSelectionAction().setChecked(false);
    _scatterplotOptionsHolder.getHidePointSelectionOutlineAction().setChecked(false);
    _scatterplotOptionsHolder.getFreezeSelectionAction().setChecked(false);
    _selectionOptionsHolder.getCorrelationRangeAction().setLimitsMaximum(1.0);
    _selectionOptionsHolder.getCorrelationRangeAction().setLimitsMinimum(0.0);
    _selectionOptionsHolder.getCorrelationRangeAction().setMaximum(1.0);
    _selectionOptionsHolder.getCorrelationRangeAction().setMinimum(0.5);

    _dataset1OptionsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _selectionOptionsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox | OptionsAction::Selection);
    _dataset1OptionsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getGeneSelection2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDimensionXAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset1OptionsHolder.getEmbeddingDimensionYAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _selectionOptionsHolder.getStatusMessageAction().setDefaultWidgetFlags(StringAction::Label);
    _computeHolder.getComputeAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _computeHolder.getComputeAction().setIcon(mv::util::StyledIcon("play"));
    _computeHolder.getComputeAction().setText("Compute");
    _dataset1OptionsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _selectionOptionsHolder.getDeSelectRowAction().setDefaultWidgetFlags(ToggleAction::PushButtonIconText);
    _scatterplotOptionsHolder.getFocusSelectionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _scatterplotOptionsHolder.getHidePointSelectionOutlineAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _scatterplotOptionsHolder.getFreezeSelectionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _selectionOptionsHolder.getCorrelationRangeAction().setDefaultWidgetFlags(DecimalRangeAction::Slider  | DecimalRangeAction::MaximumSpinBox | DecimalRangeAction::MinimumSpinBox);

    _dataset2OptionsHolder.getClusterDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getPointDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().setDefaultWidgetFlags(OptionAction::ComboBox);


    _tableViewAction = new QTableView();
    _tableViewAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableViewAction->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableViewAction->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableViewAction->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _tableViewAction->setAlternatingRowColors(false);
    _tableViewAction->setSortingEnabled(true);
    _tableViewAction->setShowGrid(true);
    _tableViewAction->setGridStyle(Qt::SolidLine);
    _tableViewAction->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableViewAction->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    _tableViewAction->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableViewAction->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _tableViewAction->setCornerButtonEnabled(false);
    _tableViewAction->setWordWrap(false);
    _tableViewAction->setTabKeyNavigation(false);
    _tableViewAction->setAcceptDrops(false);
    _tableViewAction->setDropIndicatorShown(false);
    _tableViewAction->setDragEnabled(false);
    _tableViewAction->setDragDropMode(QAbstractItemView::NoDragDrop);
    _tableViewAction->setDragDropOverwriteMode(false);
    _tableViewAction->setAutoScroll(false);
    _tableViewAction->setAutoScrollMargin(16);
    _tableViewAction->setAutoFillBackground(true);
    _tableViewAction->setFrameShape(QFrame::NoFrame);
    _tableViewAction->setFrameShadow(QFrame::Plain);
    _tableViewAction->setLineWidth(0);
    _tableViewAction->setMidLineWidth(0);
    _tableViewAction->setFocusPolicy(Qt::NoFocus);
    _tableViewAction->setContextMenuPolicy(Qt::NoContextMenu);
    _tableViewAction->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    _tableViewAction->setMinimumSize(QSize(0, 0));
    _tableViewAction->setMaximumSize(QSize(16777215, 16777215));
    _tableViewAction->setBaseSize(QSize(0, 0));
    _tableViewAction->setFocusPolicy(Qt::StrongFocus);
    _tableViewAction->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    //only highlight multiple rows if shiuft is pressed
    _tableViewAction->setSelectionBehavior(QAbstractItemView::SelectRows);

    _lineChartWidget = new HighPerfLineChart();

}

inline SettingsAction::Dataset1OptionsHolder::Dataset1OptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Settings options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _embeddingDatasetAction(this, "Embedding dataset"),
    _scatterplotViewSelectionAction(this, "Scatterplot view selection"),
    _embeddingDimensionXAction(this, "Embedding Dimension X"),
    _embeddingDimensionYAction(this, "Embedding Dimension Y"),
    _geneSelectionAction(this, "Gene Selection")
{
    setText("Settings Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDatasetAction);
    addAction(&_embeddingDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_scatterplotViewSelectionAction);
    addAction(&_embeddingDimensionXAction);
    addAction(&_embeddingDimensionYAction);
}


inline SettingsAction::Dataset2OptionsHolder::Dataset2OptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Settings options"),
    _settingsOptions(settingsAction),
    _clusterDataset2Action(this, "Cluster dataset 2"),
    _pointDataset2Action(this, "Point dataset 2"),
    _embeddingDataset2Action(this, "Embedding dataset 2"),
    _scatterplotViewSelection2Action(this, "Scatterplot view selection 2"),
    _embeddingDimension2XAction(this, "Embedding Dimension 2 X"),
    _embeddingDimension2YAction(this, "Embedding Dimension 2 Y"),
    _geneSelection2Action(this, "Gene Selection 2")
{
    setText("Settings Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDataset2Action);
    addAction(&_embeddingDataset2Action);
    addAction(&_clusterDataset2Action);
    addAction(&_scatterplotViewSelection2Action);
    addAction(&_embeddingDimension2XAction);
    addAction(&_embeddingDimension2YAction);

}

inline SettingsAction::SelectionOptionsHolder::SelectionOptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Settings options"),
    _settingsOptions(settingsAction),
    _clusterSelectionAction(this, "Cluster selection"),
    _statusMessageAction(this, "Status message"),
    _deSelectRowAction(this, "DeSelect Row"),
    _correlationRangeAction(this, "Correlation Range")
{
    setText("Settings Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_clusterSelectionAction);
    addAction(&_correlationRangeAction);
    addAction(&_deSelectRowAction);

}

inline SettingsAction::ScatterplotOptionsHolder::ScatterplotOptionsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Settings options"),
    _settingsOptions(settingsAction),
    _focusSelectionAction(this, "Focus Selection"),
    _hidePointSelectionAction(this, "Hide Point Selection Outline"),
    _freezeSelectionAction(this, "Freeze Selection")
{
    setText("Settings Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_focusSelectionAction);
    addAction(&_hidePointSelectionAction);
    addAction(&_freezeSelectionAction);

}






inline SettingsAction::ComputeHolder::ComputeHolder(SettingsAction& settingsAction) :
    HorizontalGroupAction(&settingsAction, "Compute options"),
    _settingsOptions(settingsAction),
    _computeAction(this, "Compute")
{
    setText("Color Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_computeAction);
    _statusBarAction = new QStatusBar();
    _statusBarAction->setStatusTip("Status");
    _statusBarAction->setMaximumHeight(22);
    //_statusBarAction->setFixedWidth(120);
    //_statusBarAction->setMinimumWidth(100);
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

    _selectionOptionsHolder.getClusterSelectionAction().fromParentVariantMap(variantMap);
    _dataset1OptionsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getGeneSelection2Action().fromParentVariantMap(variantMap);
    _selectionOptionsHolder.getCorrelationRangeAction().fromParentVariantMap(variantMap);


    _dataset2OptionsHolder.getClusterDataset2Action().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getPointDataset2Action().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDataset2Action().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().fromParentVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().fromParentVariantMap(variantMap);


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


    _selectionOptionsHolder.getClusterSelectionAction().insertIntoVariantMap(variantMap);
    _dataset1OptionsHolder.getGeneSelectionAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getGeneSelection2Action().insertIntoVariantMap(variantMap);
    _selectionOptionsHolder.getCorrelationRangeAction().insertIntoVariantMap(variantMap);

    _dataset2OptionsHolder.getClusterDataset2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getPointDataset2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDataset2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getScatterplotViewSelection2Action().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimension2XAction().insertIntoVariantMap(variantMap);
    _dataset2OptionsHolder.getEmbeddingDimension2YAction().insertIntoVariantMap(variantMap);

    return variantMap;
}