#include "LayerSurferPlugin.h"

#include <event/Event.h>

#include <DatasetsMimeData.h>

#include <QDebug>
#include <QMimeData>
#include <QLabel> 

Q_PLUGIN_METADATA(IID "studio.manivault.LayerSurferPlugin")

using namespace mv;

LayerSurferPlugin::LayerSurferPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _settingsAction(*this),
    _computation(*this, _settingsAction)
{

    getLearningCenterAction().addVideos(QStringList({ "Practitioner", "Developer" }));
}

void LayerSurferPlugin::init()
{

    GroupsAction* groupForAction = new GroupsAction(this, "Settings");
    _settingsAction.getDataset1OptionsHolder().setExpanded(false);
    groupForAction->addGroupAction(&_settingsAction.getDataset1OptionsHolder());
    _settingsAction.getDataset2OptionsHolder().setExpanded(false);
    groupForAction->addGroupAction(&_settingsAction.getDataset2OptionsHolder());
    _settingsAction.getCommonOptionsHolder().setExpanded(true);
    groupForAction->addGroupAction(&_settingsAction.getCommonOptionsHolder());
 
    addDockingAction(groupForAction, nullptr, gui::DockAreaFlag::Right, true, gui::AutoHideLocation::Right, QSize(500, 500));

    auto* mainLayout = new QVBoxLayout(&getWidget());
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    

    auto* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);


    auto* table1Layout = new QVBoxLayout();
    table1Layout->setContentsMargins(0, 0, 0, 0);
    table1Layout->setSpacing(2);
    auto* table1Label = new QLabel("Gene Table: Dataset 1 Selected Cells");
    table1Label->setAlignment(Qt::AlignCenter);
    table1Label->setStyleSheet("font-weight: bold; padding-bottom: 2px;");
    table1Layout->addWidget(table1Label);
    table1Layout->addWidget(_settingsAction.getTableView1Action());
    topLayout->addLayout(table1Layout, 4);
    topLayout->addWidget(_settingsAction.getLineChart1Widget(), 6);


    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);


    auto* table3Layout = new QVBoxLayout();
    table3Layout->setContentsMargins(0, 0, 0, 0);
    table3Layout->setSpacing(2);
    auto* table3Label = new QLabel("Gene Table: Dataset 2 All Cells");
    table3Label->setAlignment(Qt::AlignCenter);
    table3Label->setStyleSheet("font-weight: bold; padding-bottom: 2px;");
    table3Layout->addWidget(table3Label);
    table3Layout->addWidget(_settingsAction.getTableView2Action());
    bottomLayout->addLayout(table3Layout, 1);


    auto* table2Layout = new QVBoxLayout();
    table2Layout->setContentsMargins(0, 0, 0, 0);
    table2Layout->setSpacing(2);
    auto* table2Label = new QLabel("Gene Table: Dataset 1 All Cells");
    table2Label->setAlignment(Qt::AlignCenter);
    table2Label->setStyleSheet("font-weight: bold; padding-bottom: 2px;");
    table2Layout->addWidget(table2Label);
    table2Layout->addWidget(_settingsAction.getTableView0Action());
    bottomLayout->addLayout(table2Layout, 1);




    auto* combinedLayout = new QVBoxLayout();
    

    combinedLayout->addLayout(topLayout);



    auto* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain); // Use plain instead of sunken
    separator->setLineWidth(1);
    separator->setFixedHeight(1); // Make the line thinner
    separator->setStyleSheet("color: #e0e0e0; background-color: #e0e0e0;"); // Light gray
    combinedLayout->addWidget(separator);

    combinedLayout->addLayout(bottomLayout);

    mainLayout->addLayout(combinedLayout,90);

    

    auto* computeStatusLayout = new QHBoxLayout();
    computeStatusLayout->setContentsMargins(0, 0, 0, 0);
    computeStatusLayout->setSpacing(0);
    computeStatusLayout->addWidget(_settingsAction.getStatusHolder().getStatusBarAction(),90);
    computeStatusLayout->addWidget(_settingsAction.getStatusHolder().createWidget(&getWidget()));

    mainLayout->addLayout(computeStatusLayout);

    getWidget().setLayout(mainLayout);

    _eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetAdded));
    _eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetDataChanged));
    _eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetRemoved));
    _eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetDataSelectionChanged));
    _eventListener.registerDataEventByType(PointType, std::bind(&LayerSurferPlugin::onDataEvent, this, std::placeholders::_1));
}

void LayerSurferPlugin::onDataEvent(mv::DatasetEvent* dataEvent)
{

}

LayerSurferPluginFactory::LayerSurferPluginFactory()
{
    getPluginMetadata().setDescription("Layer surfer plugin");
    getPluginMetadata().setSummary("This plugin shows layer surfer plugin in ManiVault Studio.");
    getPluginMetadata().setCopyrightHolder({ "BioVault (Biomedical Visual Analytics Unit LUMC - TU Delft)" });
    getPluginMetadata().setAuthors({
	});
    getPluginMetadata().setOrganizations({
        { "LUMC", "Leiden University Medical Center", "https://www.lumc.nl/en/" },
        { "TU Delft", "Delft university of technology", "https://www.tudelft.nl/" }
	});
    getPluginMetadata().setLicenseText("This plugin is distributed under the [LGPL v3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html) license.");
}

ViewPlugin* LayerSurferPluginFactory::produce()
{
    return new LayerSurferPlugin(this);
}

mv::DataTypes LayerSurferPluginFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;

    // This layer surfer plugin is compatible with points datasets
    supportedTypes.append(PointType);

    return supportedTypes;
}

mv::gui::PluginTriggerActions LayerSurferPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;
/*
    const auto getPluginInstance = [this]() -> LayerSurferPlugin* {
        return dynamic_cast<LayerSurferPlugin*>(plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<LayerSurferPluginFactory*>(this), this, "Layer", "Surfer", icon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance();
        });

        pluginTriggerActions << pluginTriggerAction;
    }
    */
    return pluginTriggerActions;
}
void LayerSurferPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);
    mv::util::variantMapMustContain(variantMap, "LayerSurfer:Settings");
    _settingsAction.fromVariantMap(variantMap["LayerSurfer:Settings"].toMap());
}

QVariantMap LayerSurferPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();

    _settingsAction.insertIntoVariantMap(variantMap);
    return variantMap;
}