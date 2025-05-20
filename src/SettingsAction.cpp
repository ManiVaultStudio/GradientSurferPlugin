#include "SettingsAction.h"

#include <iostream>
#include <set>
#include "LayerSurferPlugin.h"
#include<string>  
#include <QFileDialog>
#include <QPageLayout>
#include <QWebEngineView>
#include <string>
#include <typeinfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QSet>
#include <QJsonValue>
using namespace mv;
using namespace mv::gui;

SettingsAction::SettingsAction(LayerSurferPlugin& LayerSurferPlugin, mv::CoreInterface* core) :
    WidgetAction(&LayerSurferPlugin, "LayerSurferPlugin"),
    _viewerPlugin(LayerSurferPlugin),
    _core(core),
    _settingsHolder(*this),
    _computeHolder(*this)
{
    setSerializationName("LayerSurfer:Settings");
    _settingsHolder.getClusterDatasetAction().setSerializationName("LayerSurfer:Clusterdataset");
    _settingsHolder.getPointDatasetAction().setSerializationName("LayerSurfer:PointDataset");
    _settingsHolder.getClusterSelectionAction().setSerializationName("LayerSurfer:ClusterSelection");
    _settingsHolder.getGeneSelectionAction().setSerializationName("LayerSurfer:GeneSelection");


    _settingsHolder.getClusterDatasetAction().setToolTip("Cluster Dataset");
    _settingsHolder.getPointDatasetAction().setToolTip("Point Dataset");
    _settingsHolder.getClusterSelectionAction().setToolTip("Cluster Selection");
    _settingsHolder.getGeneSelectionAction().setToolTip("Gene Selection");
    _settingsHolder.getStatusColorAction().setToolTip("Status Color");
    _computeHolder.getComputeAction().setToolTip("Compute Action");

    _settingsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox);
    _settingsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getStatusColorAction().setDefaultWidgetFlags(StringAction::Label);
    _computeHolder.getComputeAction().setDefaultWidgetFlags(TriggerAction::IconText);
    _computeHolder.getComputeAction().setIcon(mv::util::StyledIcon("play"));
    _computeHolder.getComputeAction().setText("Compute");

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

    


    _settingsHolder.getClusterDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == ClusterType;
        });
    _settingsHolder.getPointDatasetAction().setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });

    const auto updateClusterDataset = [this]() -> void {
        if (_settingsHolder.getClusterDatasetAction().getCurrentDataset().isValid())
        {
            auto clusterFullDataset = mv::data().getDataset<Clusters>(_settingsHolder.getClusterDatasetAction().getCurrentDataset().getDatasetId());
            auto clusters = clusterFullDataset->getClusters();
            QStringList clusterNames = {};
            if (!clusters.isEmpty())
            {
                for (auto cluster : clusters)
                {
                    clusterNames.append(cluster.getName());
                }
            }
            
            _settingsHolder.getClusterSelectionAction().setOptions(clusterNames);

        }
        else
        {
            _settingsHolder.getClusterSelectionAction().setOptions({});
        }

        };

    connect(&_settingsHolder.getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset);

    const auto updatePointDataset = [this]() -> void {
        if (_settingsHolder.getPointDatasetAction().getCurrentDataset().isValid())
        {
            _settingsHolder.getGeneSelectionAction().setPointsDataset(_settingsHolder.getPointDatasetAction().getCurrentDataset());
        }
        else
        {
            _settingsHolder.getGeneSelectionAction().setPointsDataset({});
        }
        
        };

    connect(&_settingsHolder.getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset);
}

inline SettingsAction::SettingsHolder::SettingsHolder(SettingsAction& settingsAction) :
    HorizontalGroupAction(&settingsAction, "Settings options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _clusterSelectionAction(this, "Cluster selection"),
    _geneSelectionAction(this, "Gene selection"),
    _statusColorAction(this, "Status color")
{
    setText("Settings Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_clusterSelectionAction);
    addAction(&_geneSelectionAction);
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
    _settingsHolder.getClusterSelectionAction().fromParentVariantMap(variantMap);
    _settingsHolder.getGeneSelectionAction().fromParentVariantMap(variantMap);

}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _settingsHolder.getClusterDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getPointDatasetAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getClusterSelectionAction().insertIntoVariantMap(variantMap);
    _settingsHolder.getGeneSelectionAction().insertIntoVariantMap(variantMap);

    return variantMap;
}