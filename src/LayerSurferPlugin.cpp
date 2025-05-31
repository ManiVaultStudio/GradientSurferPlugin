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
    

    QWidget* composite = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(composite);

    // Improved collapsible area: professional header, full header clickable, no space when collapsed
    auto createCollapsibleArea = [](const QString& title, QWidget* content, QWidget* parent = nullptr) {
        QWidget* area = new QWidget(parent);
        QVBoxLayout* areaLayout = new QVBoxLayout(area);
        areaLayout->setContentsMargins(0, 0, 0, 0);
        areaLayout->setSpacing(0);

        // Header widget with professional look
        QWidget* headerWidget = new QWidget(area);
        QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
        headerLayout->setContentsMargins(8, 2, 8, 2);
        headerLayout->setSpacing(8);

        // Style: subtle border, rounded corners, bold font
        headerWidget->setStyleSheet(
            "QWidget {"
            " border-bottom: 1px solid #bbb;"
            " border-radius: 4px;"
            " background: transparent;"
            "}"
            "QLabel { font-weight: bold; font-size: 12pt; }"
        );

        QToolButton* toggleButton = new QToolButton(headerWidget);
        toggleButton->setText("-");
        toggleButton->setCheckable(true);
        toggleButton->setChecked(true);
        toggleButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        toggleButton->setStyleSheet("QToolButton { border: none; font-size: 12pt; }");

        QLabel* titleLabel = new QLabel(title, headerWidget);

        headerLayout->addWidget(toggleButton);
        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();

        areaLayout->addWidget(headerWidget);
        areaLayout->addWidget(content);

        // Store original size policy
        QSizePolicy originalPolicy = content->sizePolicy();

        // Make the whole header clickable
        headerWidget->setCursor(Qt::PointingHandCursor);

        // Remove the direct QObject::connect to QWidget::mouseReleaseEvent (not allowed)
        // Use only the event filter for header click
        class HeaderClickFilter : public QObject {
        public:
            QToolButton* btn;
            HeaderClickFilter(QToolButton* b) : btn(b) {}
            bool eventFilter(QObject* obj, QEvent* event) override {
                if (event->type() == QEvent::MouseButtonRelease) {
                    auto* me = static_cast<QMouseEvent*>(event);
                    if (me->button() == Qt::LeftButton) {
                        btn->toggle();
                        return true;
                    }
                }
                return QObject::eventFilter(obj, event);
            }
        };
        headerWidget->installEventFilter(new HeaderClickFilter(toggleButton));

        // Collapse/expand logic
        QObject::connect(toggleButton, &QToolButton::toggled, area, [content, toggleButton, originalPolicy](bool checked) {
            content->setVisible(checked);
            toggleButton->setText(checked ? "-" : "+");
            if (checked) {
                content->setSizePolicy(originalPolicy);
            } else {
                content->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
            }
            if (content->parentWidget() && content->parentWidget()->layout())
                content->parentWidget()->layout()->invalidate();
        });

        // Start expanded
        content->setVisible(true);

        return area;
    };

    QWidget* dataset1Widget = _settingsAction.getDataset1OptionsHolder().createWidget(&getWidget());
    dataset1Widget->setMaximumWidth(380);
    layout->addWidget(createCollapsibleArea("Dataset 1 Options", dataset1Widget, composite));

    QWidget* dataset2Widget = _settingsAction.getDataset2OptionsHolder().createWidget(&getWidget());
    dataset2Widget->setMaximumWidth(380);
    layout->addWidget(createCollapsibleArea("Dataset 2 Options", dataset2Widget, composite));

    QWidget* selectionWidget = _settingsAction.getSelectionOptionsHolder().createWidget(&getWidget());
    selectionWidget->setMaximumWidth(380);
    layout->addWidget(createCollapsibleArea("Selection Options", selectionWidget, composite));

    QWidget* scatterplotWidget = _settingsAction.getScatterplotOptionsHolder().createWidget(&getWidget());
    scatterplotWidget->setMaximumWidth(380);
    layout->addWidget(createCollapsibleArea("Scatterplot Options", scatterplotWidget, composite));

    QWidget* lineChartWidget = _settingsAction.getLineChartOptionsHolder().createWidget(&getWidget());
    lineChartWidget->setMaximumWidth(380);
    layout->addWidget(createCollapsibleArea("Line Chart Options", lineChartWidget, composite));


    auto* compositeAction = new mv::gui::WidgetAction(this, "Options");
    compositeAction->setWidgetConfigurationFunction(
        [composite](mv::gui::WidgetAction*, QWidget* parent) {
            composite->setParent(parent);
            return composite;
        }
    );

    addDockingAction(compositeAction, nullptr, gui::DockAreaFlag::Right, true, gui::AutoHideLocation::Right, QSize(500, 500));

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