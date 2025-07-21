#include "LayerSurferPlugin.h"

#include <event/Event.h>
#include <QApplication>
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
    qApp->setStyleSheet("QToolTip { color: black; background: #ffffe1; border: 1px solid black; }");

    // Settings panel on the left
    GroupsAction* groupForAction = new GroupsAction(this, "Settings");
    _settingsAction.getDataset1OptionsHolder().setExpanded(false);
    groupForAction->addGroupAction(&_settingsAction.getDataset1OptionsHolder());
    _settingsAction.getDataset2OptionsHolder().setExpanded(false);
    groupForAction->addGroupAction(&_settingsAction.getDataset2OptionsHolder());
    _settingsAction.getCommonOptionsHolder().setExpanded(true);
    groupForAction->addGroupAction(&_settingsAction.getCommonOptionsHolder());
    addDockingAction(groupForAction, nullptr, gui::DockAreaFlag::Left, true, gui::AutoHideLocation::Left);

    // Set a dummy model if none is set, so the table expands
    auto ensureTableModel = [](QTableView* table) {
        if (table && !table->model()) {
            table->setModel(new QStandardItemModel(5, 3, table));
        }
        };
    ensureTableModel(_settingsAction.getTableView1Action());
    ensureTableModel(_settingsAction.getTableView2Action());
    ensureTableModel(_settingsAction.getTableView3Action());

    // Helper: create a vertical widget with a label and a table view
    auto createTableWidget = [](const QString& labelText, QTableView* tableView, QWidget* parent = nullptr) {
        QWidget* contentWidget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(contentWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        QLabel* label = new QLabel(labelText, contentWidget);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-weight: bold; padding: 0px; font-size: 11px;");
        label->setWordWrap(true);
        label->setMinimumHeight(32);
        label->setMaximumHeight(32);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        layout->addWidget(label, 0);

        tableView->setParent(contentWidget);
        tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(tableView, 1);

        contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QScrollArea* scrollArea = new QScrollArea(parent);
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(contentWidget);
        scrollArea->setFrameShape(QFrame::NoFrame);

        // Helper to update min height based on screen
        auto updateMinHeight = [scrollArea]() {
            QScreen* screen = scrollArea->screen();
            if (!screen) screen = QGuiApplication::primaryScreen();
            if (screen) {
                int minHeight = screen->availableGeometry().height();
                scrollArea->setMinimumHeight(minHeight);
            }
            };

        updateMinHeight();

        // Connect to screen geometry changes for dynamic updates
        QScreen* screen = scrollArea->screen();
        if (!screen) screen = QGuiApplication::primaryScreen();
        if (screen) {
            QObject::connect(screen, &QScreen::geometryChanged, scrollArea, [updateMinHeight]() {
                updateMinHeight();
                });
        }

        return scrollArea;
        };
    // 1. Add the first table to the main layout (plugin's main widget)
    {
        QWidget& mainWidget = getWidget(); // Use reference, not pointer
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->addWidget(
            createTableWidget("Gene Table: Dataset 1 Selected Cells", _settingsAction.getTableView1Action(), &mainWidget)
        );
        mainWidget.setLayout(mainLayout);
    }

    // 2. Dock the second table ("Dataset 1 All Cells") on the right
    auto* table2Action = new mv::gui::WidgetAction(this, "Gene Table: Dataset 1 All Cells");
    table2Action->setWidgetConfigurationFunction(
        [createTableWidget, this](mv::gui::WidgetAction*, QWidget* parent) {
            QWidget* tableWidget = createTableWidget("Gene Table: Dataset 1 All Cells", _settingsAction.getTableView3Action(), parent);
            tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            return tableWidget;
        }
    );
    addDockingAction(table2Action, nullptr, gui::DockAreaFlag::Right, true, gui::AutoHideLocation::Right, QSize(256, QWIDGETSIZE_MAX));

    // 3. Dock the third table ("Dataset 2 All Cells") on the right
    auto* table3Action = new mv::gui::WidgetAction(this, "Gene Table: Dataset 2 All Cells");
    table3Action->setWidgetConfigurationFunction(
        [createTableWidget, this](mv::gui::WidgetAction*, QWidget* parent) {
            QWidget* tableWidget = createTableWidget("Gene Table: Dataset 2 All Cells", _settingsAction.getTableView2Action(), parent);
            tableWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            return tableWidget;
        }
    );
    addDockingAction(table3Action, nullptr, gui::DockAreaFlag::Right, true, gui::AutoHideLocation::Right, QSize(256, QWIDGETSIZE_MAX));

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
    setIconByName("layer-group");
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