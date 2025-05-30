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
    _settingsHolder(*this),
    _computeHolder(*this)
{
    setSerializationName("LayerSurfer:Settings");
    _settingsHolder.getClusterDatasetAction().setSerializationName("LayerSurfer:Clusterdataset");
    _settingsHolder.getPointDatasetAction().setSerializationName("LayerSurfer:PointDataset");
    _settingsHolder.getEmbeddingDatasetAction().setSerializationName("LayerSurfer:EmbeddingDataset");
    _settingsHolder.getClusterSelectionAction().setSerializationName("LayerSurfer:ClusterSelection");
    _settingsHolder.getGeneSelectionAction().setSerializationName("LayerSurfer:GeneSelection");
    _settingsHolder.getScatterplotViewSelectionAction().setSerializationName("LayerSurfer:ScatterplotViewSelection");
    _settingsHolder.getEmbeddingDimensionXAction().setSerializationName("LayerSurfer:EmbeddingDimensionX");
    _settingsHolder.getEmbeddingDimensionYAction().setSerializationName("LayerSurfer:EmbeddingDimensionY");
    _settingsHolder.getCorrelationRangeAction().setSerializationName("LayerSurfer:CorrelationRange");

    _settingsHolder.getClusterDataset2Action().setSerializationName("LayerSurfer:ClusterDataset2");
    _settingsHolder.getPointDataset2Action().setSerializationName("LayerSurfer:PointDataset2");
    _settingsHolder.getEmbeddingDataset2Action().setSerializationName("LayerSurfer:EmbeddingDataset2");
    _settingsHolder.getScatterplotViewSelection2Action().setSerializationName("LayerSurfer:ScatterplotViewSelection2");
    _settingsHolder.getEmbeddingDimension2XAction().setSerializationName("LayerSurfer:EmbeddingDimension2X");
    _settingsHolder.getEmbeddingDimension2YAction().setSerializationName("LayerSurfer:EmbeddingDimension2Y");


    _settingsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _settingsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _settingsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset");
    _settingsHolder.getClusterSelectionAction().setToolTip("Cluster Selection");
    _settingsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _settingsHolder.getStatusMessageAction().setToolTip("Status Message");
    _computeHolder.getComputeAction().setToolTip("Compute Action");
    _computeHolder.getStatusBarAction()->setToolTip("Status Bar Action");
    _settingsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection");
    _settingsHolder.getDeSelectRowAction().setToolTip("DeSelect Row Action");
    _settingsHolder.getFocusSelectionAction().setToolTip("Focus Selection Action");
    _settingsHolder.getHidePointSelectionOutlineAction().setToolTip("Hide Point Selection Outline Action");
    _settingsHolder.getFreezeSelectionAction().setToolTip("Freeze Selection Action");
    _settingsHolder.getCorrelationRangeAction().setToolTip("Correlation Range Action");

    _settingsHolder.getClusterDataset2Action().setToolTip("Cluster Dataset 2");
    _settingsHolder.getPointDataset2Action().setToolTip("Point Dataset 2");
    _settingsHolder.getEmbeddingDataset2Action().setToolTip("Embedding Dataset 2");
    _settingsHolder.getScatterplotViewSelection2Action().setToolTip("Scatterplot View Selection 2");
    _settingsHolder.getEmbeddingDimension2XAction().setToolTip("Embedding Dimension 2 X");
    _settingsHolder.getEmbeddingDimension2YAction().setToolTip("Embedding Dimension 2 Y");


    _settingsHolder.getFocusSelectionAction().setChecked(false);
    _settingsHolder.getHidePointSelectionOutlineAction().setChecked(false);
    _settingsHolder.getFreezeSelectionAction().setChecked(false);
    _settingsHolder.getCorrelationRangeAction().setLimitsMaximum(1.0);
    _settingsHolder.getCorrelationRangeAction().setLimitsMinimum(0.0);
    _settingsHolder.getCorrelationRangeAction().setMaximum(1.0);
    _settingsHolder.getCorrelationRangeAction().setMinimum(0.5);

    _settingsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox | OptionsAction::Selection);
    _settingsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDimensionXAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDimensionYAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getStatusMessageAction().setDefaultWidgetFlags(StringAction::Label);
    _computeHolder.getComputeAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _computeHolder.getComputeAction().setIcon(mv::util::StyledIcon("play"));
    _computeHolder.getComputeAction().setText("Compute");
    _settingsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getDeSelectRowAction().setDefaultWidgetFlags(ToggleAction::PushButtonIconText);
    _settingsHolder.getFocusSelectionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _settingsHolder.getHidePointSelectionOutlineAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _settingsHolder.getFreezeSelectionAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    _settingsHolder.getCorrelationRangeAction().setDefaultWidgetFlags(DecimalRangeAction::Slider  | DecimalRangeAction::MaximumSpinBox | DecimalRangeAction::MinimumSpinBox);

    _settingsHolder.getClusterDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getPointDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDataset2Action().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDimension2XAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDimension2YAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getScatterplotViewSelection2Action().setDefaultWidgetFlags(OptionAction::ComboBox);


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

inline SettingsAction::SettingsHolder::SettingsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Settings options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _embeddingDatasetAction(this, "Embedding dataset"),
    _clusterSelectionAction(this, "Cluster selection"),
    _geneSelectionAction(this, "Gene selection"),
    _statusMessageAction(this, "Status message"),
    _scatterplotViewSelectionAction(this, "Scatterplot view selection"),
    _deSelectRowAction(this, "DeSelect Row"),
    _embeddingDimensionXAction(this, "Embedding Dimension X"),
    _embeddingDimensionYAction(this, "Embedding Dimension Y"),
    _focusSelectionAction(this, "Focus Selection"),
    _hidePointSelectionAction(this, "Hide Point Selection Outline"),
    _freezeSelectionAction(this, "Freeze Selection"),
    _correlationRangeAction(this, "Correlation Range"),
    _clusterDataset2Action(this, "Cluster dataset 2"),
    _pointDataset2Action(this, "Point dataset 2"),
    _embeddingDataset2Action(this, "Embedding dataset 2"),
    _scatterplotViewSelection2Action(this, "Scatterplot view selection 2"),
    _embeddingDimension2XAction(this, "Embedding Dimension 2 X"),
    _embeddingDimension2YAction(this, "Embedding Dimension 2 Y")
{
    setText("Settings Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDatasetAction);
    addAction(&_embeddingDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_clusterSelectionAction);
    //addAction(&_geneSelectionAction);
    addAction(&_scatterplotViewSelectionAction);
    addAction(&_embeddingDimensionXAction);
    addAction(&_embeddingDimensionYAction);
    addAction(&_focusSelectionAction);
    addAction(&_hidePointSelectionAction);
    addAction(&_freezeSelectionAction);
    addAction(&_correlationRangeAction);
    //addAction(&_statusColorAction);
    addAction(&_deSelectRowAction);
    addAction(&_clusterDataset2Action);
    addAction(&_pointDataset2Action);
    addAction(&_embeddingDataset2Action);
    addAction(&_scatterplotViewSelection2Action);
    addAction(&_embeddingDimension2XAction);
    addAction(&_embeddingDimension2YAction);

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

    _settingsHolder.getClusterDatasetAction().fromParentVariantMap(variantMap);
    _settingsHolder.getPointDatasetAction().fromParentVariantMap(variantMap);
    _settingsHolder.getEmbeddingDatasetAction().fromParentVariantMap(variantMap);
    _settingsHolder.getClusterSelectionAction().fromParentVariantMap(variantMap);
    _settingsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);
    _settingsHolder.getScatterplotViewSelectionAction().fromParentVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimensionXAction().fromParentVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimensionYAction().fromParentVariantMap(variantMap);
    _settingsHolder.getCorrelationRangeAction().fromParentVariantMap(variantMap);


    _settingsHolder.getClusterDataset2Action().fromParentVariantMap(variantMap);
    _settingsHolder.getPointDataset2Action().fromParentVariantMap(variantMap);

        _settingsHolder.getEmbeddingDataset2Action().fromParentVariantMap(variantMap);
  
        _settingsHolder.getScatterplotViewSelection2Action().fromParentVariantMap(variantMap);

        _settingsHolder.getEmbeddingDimension2XAction().fromParentVariantMap(variantMap);

        _settingsHolder.getEmbeddingDimension2YAction().fromParentVariantMap(variantMap);


}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _settingsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getClusterSelectionAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getGeneSelectionAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getScatterplotViewSelectionAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimensionXAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimensionYAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getCorrelationRangeAction().insertIntoVariantMap(variantMap);

   _settingsHolder.getClusterDataset2Action().insertIntoVariantMap(variantMap);
  _settingsHolder.getPointDataset2Action().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDataset2Action().insertIntoVariantMap(variantMap);
    _settingsHolder.getScatterplotViewSelection2Action().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimension2XAction().insertIntoVariantMap(variantMap);
   _settingsHolder.getEmbeddingDimension2YAction().insertIntoVariantMap(variantMap);

    return variantMap;
}