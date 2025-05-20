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
    _settingsHolder(*this)

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

    _settingsHolder.getClusterDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getPointDatasetAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    _settingsHolder.getClusterSelectionAction().setDefaultWidgetFlags(OptionsAction::ComboBox);
    _settingsHolder.getGeneSelectionAction().setDefaultWidgetFlags(OptionAction::ComboBox);


}

inline SettingsAction::SettingsHolder::SettingsHolder(SettingsAction& settingsAction) :
    VerticalGroupAction(&settingsAction, "Cluster data options"),
    _settingsOptions(settingsAction),
    _clusterDatasetAction(this, "Cluster dataset"),
    _pointDatasetAction(this, "Point dataset"),
    _clusterSelectionAction(this, "Cluster selection"),
    _geneSelectionAction(this, "Gene selection")
{
    setText("Color Options");
    setIcon(mv::util::StyledIcon("braille"));
    setPopupSizeHint(QSize(500, 0));
    setConfigurationFlag(WidgetAction::ConfigurationFlag::Default);
    addAction(&_pointDatasetAction);
    addAction(&_clusterDatasetAction);
    addAction(&_clusterSelectionAction);
    addAction(&_geneSelectionAction);
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