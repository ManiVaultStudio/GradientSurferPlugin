#include "LayerSurferPlugin.h"

#include <event/Event.h>

#include <DatasetsMimeData.h>

#include <QDebug>
#include <QMimeData>

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
    groupForAction->addGroupAction(&_settingsAction.getDataset1OptionsHolder());
    groupForAction->addGroupAction(&_settingsAction.getDataset2OptionsHolder());
    groupForAction->addGroupAction(&_settingsAction.getSelectionOptionsHolder());
    groupForAction->addGroupAction(&_settingsAction.getScatterplotOptionsHolder());
    groupForAction->addGroupAction(&_settingsAction.getLineChartOptionsHolder());
    groupForAction->addGroupAction(&_settingsAction.getTableOptionsHolder(),false);

    addDockingAction(groupForAction, nullptr, gui::DockAreaFlag::Right, true, gui::AutoHideLocation::Right, QSize(500, 500));

    auto* mainLayout = new QVBoxLayout(&getWidget());
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(_settingsAction.getTableViewAction(), 70);
    mainLayout->addWidget(_settingsAction.getLineChartWidget(), 90);

    auto* computeStatusLayout = new QHBoxLayout();
    computeStatusLayout->setContentsMargins(0, 0, 0, 0);
    computeStatusLayout->setSpacing(0);
    computeStatusLayout->addWidget(_settingsAction.getComputeHolder().getStatusBarAction(),90);
    computeStatusLayout->addWidget(_settingsAction.getComputeHolder().createWidget(&getWidget()));

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
    //// Get smart pointer to dataset that changed
    //const auto changedDataSet = dataEvent->getDataset();

    //// Get GUI name of the dataset that changed
    //const auto datasetGuiName = changedDataSet->getGuiName();

    //// The data event has a type so that we know what type of data event occurred (e.g. data added, changed, removed, renamed, selection changes)
    //switch (dataEvent->getType()) {

    //    // A points dataset was added
    //    case EventType::DatasetAdded:
    //    {
    //        // Cast the data event to a data added event
    //        const auto dataAddedEvent = static_cast<DatasetAddedEvent*>(dataEvent);

    //        // Get the GUI name of the added points dataset and print to the console
    //        qDebug() << datasetGuiName << "was added";

    //        break;
    //    }

    //    // Points dataset data has changed
    //    case EventType::DatasetDataChanged:
    //    {
    //        // Cast the data event to a data changed event
    //        const auto dataChangedEvent = static_cast<DatasetDataChangedEvent*>(dataEvent);

    //        // Get the name of the points dataset of which the data changed and print to the console
    //        qDebug() << datasetGuiName << "data changed";

    //        break;
    //    }

    //    // Points dataset data was removed
    //    case EventType::DatasetRemoved:
    //    {
    //        // Cast the data event to a data removed event
    //        const auto dataRemovedEvent = static_cast<DatasetRemovedEvent*>(dataEvent);

    //        // Get the name of the removed points dataset and print to the console
    //        qDebug() << datasetGuiName << "was removed";

    //        break;
    //    }

    //    // Points dataset selection has changed
    //    case EventType::DatasetDataSelectionChanged:
    //    {
    //        // Cast the data event to a data selection changed event
    //        const auto dataSelectionChangedEvent = static_cast<DatasetDataSelectionChangedEvent*>(dataEvent);

    //        // Get the selection set that changed
    //        const auto& selectionSet = changedDataSet->getSelection<Points>();

    //        // Print to the console
    //        qDebug() << datasetGuiName << "selection has changed";

    //        break;
    //    }

    //    default:
    //        break;
    //}
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