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
    _settingsHolder.getTopNAction().setSerializationName("LayerSurfer:TopNAction");
    _settingsHolder.getEmbeddingDimensionXAction().setSerializationName("LayerSurfer:EmbeddingDimensionX");
    _settingsHolder.getEmbeddingDimensionYAction().setSerializationName("LayerSurfer:EmbeddingDimensionY");


    _settingsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _settingsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _settingsHolder.getEmbeddingDatasetAction().setToolTip("Embedding Dataset");
    _settingsHolder.getClusterSelectionAction().setToolTip("Cluster Selection");
    _settingsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _settingsHolder.getStatusColorAction().setToolTip("Status Color");
    _computeHolder.getComputeAction().setToolTip("Compute Action");
    _computeHolder.getStatusBarAction()->setToolTip("Status Bar Action");
    _settingsHolder.getScatterplotViewSelectionAction().setToolTip("Scatterplot View Selection");
    _settingsHolder.getTopNAction().setToolTip("Top N Action");
    _settingsHolder.getDeSelectRowAction().setToolTip("DeSelect Row Action");

    _settingsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox);
    _settingsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDimensionXAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getEmbeddingDimensionYAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getStatusColorAction().setDefaultWidgetFlags(StringAction::Label);
    _computeHolder.getComputeAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _computeHolder.getComputeAction().setIcon(mv::util::StyledIcon("play"));
    _computeHolder.getComputeAction().setText("Compute");
    _settingsHolder.getScatterplotViewSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getTopNAction().setDefaultWidgetFlags(IntegralAction::Slider | IntegralAction::SpinBox);
    _settingsHolder.getTopNAction().setValue(10);
    _settingsHolder.getTopNAction().setMinimum(1);
    _settingsHolder.getTopNAction().setMaximum(10000);
    _settingsHolder.getDeSelectRowAction().setDefaultWidgetFlags(ToggleAction::PushButtonIconText);

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

}

inline SettingsAction::SettingsHolder::SettingsHolder(SettingsAction& settingsAction) :
    HorizontalGroupAction(&settingsAction, "Settings options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _embeddingDatasetAction(this, "Embedding dataset"),
    _clusterSelectionAction(this, "Cluster selection"),
    _geneSelectionAction(this, "Gene selection"),
    _statusColorAction(this, "Status color"),
    _scatterplotViewSelectionAction(this, "Scatterplot view selection"),
    _topNAction(this, "Top N"),
    _deSelectRowAction(this, "DeSelect Row"),
    _embeddingDimensionXAction(this, "Embedding Dimension X"),
    _embeddingDimensionYAction(this, "Embedding Dimension Y")
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
    addAction(&_topNAction);
    addAction(&_embeddingDimensionXAction);
    addAction(&_embeddingDimensionYAction);
    //addAction(&_statusColorAction);
    addAction(&_deSelectRowAction);
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
    _settingsHolder.getTopNAction().fromParentVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimensionXAction().fromParentVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimensionYAction().fromParentVariantMap(variantMap);
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
    _settingsHolder.getTopNAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimensionXAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getEmbeddingDimensionYAction().insertIntoVariantMap(variantMap);

    return variantMap;
}