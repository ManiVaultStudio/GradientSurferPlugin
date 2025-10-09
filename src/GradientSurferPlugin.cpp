#include "GradientSurferPlugin.h"

#include <event/Event.h>
#include <QApplication>
#include <DatasetsMimeData.h>
#include <QHeaderView>
#include <QDebug>
#include <QMimeData>
#include <QLabel> 

Q_PLUGIN_METADATA(IID "studio.manivault.GradientSurferPlugin")

using namespace mv;

GradientSurferPlugin::GradientSurferPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _settingsAction(*this),
    _computation(*this, _settingsAction),
    _experiments(*this, _settingsAction, _computation)
{

    getLearningCenterAction().addVideos(QStringList({ "Practitioner", "Developer" }));
}

void GradientSurferPlugin::init()
{
    qApp->setStyleSheet("QToolTip { color: black; background: #ffffe1; border: 1px solid black; }");

    // Settings panel on the left
    GroupsAction* groupForAction = new GroupsAction(this, "Settings");
    _settingsAction.getDataset1OptionsHolder().setExpanded(false);
    _settingsAction.getDataset2OptionsHolder().setExpanded(false);
    _settingsAction.getCustomizationOptionsHolder().setExpanded(false);
    _settingsAction.getExperimentOptionsHolder().setExpanded(false);
    _settingsAction.getCommonOptionsHolder().setExpanded(true);

    groupForAction->addGroupAction(&_settingsAction.getCommonOptionsHolder());
    groupForAction->addGroupAction(&_settingsAction.getCustomizationOptionsHolder());

    addDockingAction(groupForAction, nullptr, gui::DockAreaFlag::Left, true, gui::AutoHideLocation::Left);

    connect(&_settingsAction.getCommonOptionsHolder().getShowAdvancedOptions(), &ToggleAction::toggled, this, [this, groupForAction]() {
        bool visible = _settingsAction.getCommonOptionsHolder().getShowAdvancedOptions().isChecked();

        if (visible) {
            groupForAction->addGroupAction(&_settingsAction.getDataset1OptionsHolder());
            groupForAction->addGroupAction(&_settingsAction.getDataset2OptionsHolder());
            //groupForAction->addGroupAction(&_settingsAction.getExperimentOptionsHolder());

        }
        else {
                        groupForAction->removeGroupAction(&_settingsAction.getDataset1OptionsHolder());
                        groupForAction->removeGroupAction(&_settingsAction.getDataset2OptionsHolder());
                        //groupForAction->removeGroupAction(&_settingsAction.getExperimentOptionsHolder());
        }
        //_settingsAction.getDataset1OptionsHolder().setVisible(visible);
        //_settingsAction.getDataset2OptionsHolder().setVisible(visible);
        //QApplication::processEvents();
        });
    //_settingsAction.getDataset1OptionsHolder().setVisible(false);
    //_settingsAction.getDataset2OptionsHolder().setVisible(false);
    // Set a dummy model if none is set, so the table expands
    auto ensureTableModel = [](QTableView* table) {
        if (table && !table->model()) {
            table->setModel(new QStandardItemModel(5, 3, table));
        }
        };
    ensureTableModel(_settingsAction.getTableView0Action());
    ensureTableModel(_settingsAction.getTableView2Action());
    ensureTableModel(_settingsAction.getTableView1Action());



    auto createTableWidget = [this](const QString& labelText, QTableView* tableView, QWidget* parent = nullptr) {
        QWidget* contentWidget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(contentWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        //QLabel* label = new QLabel(labelText, contentWidget);
        //label->setAlignment(Qt::AlignCenter);
        //label->setStyleSheet("font-weight: bold; padding: 0px; font-size: 11px;");
        //label->setWordWrap(true);
        ////label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        //layout->addWidget(label, 0);


        // --- Set up filter proxy model ---
        QSortFilterProxyModel* proxyModel = new QSortFilterProxyModel(tableView);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setSourceModel(tableView->model());
        tableView->setModel(proxyModel);
        // --- Enforce fixed row height on model/layout changes ---
        auto enforceRowHeight = [tableView]() {
            tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
            tableView->verticalHeader()->setDefaultSectionSize(18);
            };
        QObject::connect(proxyModel, &QAbstractItemModel::modelReset, tableView, enforceRowHeight);
        QObject::connect(proxyModel, &QAbstractItemModel::layoutChanged, tableView, enforceRowHeight);
        enforceRowHeight();
        tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        tableView->verticalHeader()->setDefaultSectionSize(18);

        // --- Connect search box to filter ---
        QTimer* filterDelayTimer = new QTimer(tableView);
        filterDelayTimer->setSingleShot(true);
        filterDelayTimer->setInterval(300); // 300 ms delay

        QObject::connect(
            &_settingsAction.getCommonOptionsHolder().getTableSearchBoxAction(),
            &StringAction::stringChanged,
            proxyModel,
            [proxyModel, filterDelayTimer](const QString& text) {
                filterDelayTimer->stop();
                QObject::disconnect(filterDelayTimer, nullptr, nullptr, nullptr); // Remove previous connections
                QObject::connect(filterDelayTimer, &QTimer::timeout, proxyModel, [proxyModel, text]() {
                    proxyModel->setFilterFixedString(text);
                    });
                filterDelayTimer->start();
            }
        );

        layout->addWidget(tableView, 1);

        contentWidget->setLayout(layout);

        return contentWidget;
        };
    // 1. Add the first table to the main layout (plugin's main widget)
    {
        QWidget& mainWidget = getWidget(); // Use reference, not pointer

        // Add header label
        _settingsAction.getSelectionTableHeaderLabel() = new QLabel("Selected Cells Gene Table", &mainWidget);

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        _settingsAction.getSelectionTableHeaderLabel()->setAlignment(Qt::AlignCenter);
        _settingsAction.getSelectionTableHeaderLabel()->setWordWrap(true);
        _settingsAction.getSelectionTableHeaderLabel()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mainLayout->addWidget(_settingsAction.getSelectionTableHeaderLabel(), 0);

        mainLayout->addWidget(
            createTableWidget("Selected Cells Gene Table", _settingsAction.getTableView0Action(), &mainWidget),1
        );
        mainLayout->addWidget(_settingsAction.getCommonOptionsHolder().getStatusBarAction()->createWidget(&getWidget()), 0);


        mainWidget.setLayout(mainLayout);
    }

    // 2. Dock the second table ("Dataset 1 All Cells") on the right
    auto* table1Action = new mv::gui::WidgetAction(this, "All Cells Gene Table 1");
    table1Action->setWidgetConfigurationFunction(
        [createTableWidget, this](mv::gui::WidgetAction*, QWidget* parent) {
            auto layout = new QVBoxLayout(parent);

            layout->setContentsMargins(0, 0, 0, 0);
            parent->setLayout(layout);
            if (!_settingsAction.getTable1HeaderLabel())
            {
                _settingsAction.getTable1HeaderLabel() = new QLabel(parent);
            }
            _settingsAction.getTable1HeaderLabel()->setText("All Cells Gene Table 1");

            layout->addWidget(_settingsAction.getTable1HeaderLabel(), 0); 

            layout->addWidget(createTableWidget("All Cells Gene Table 1", _settingsAction.getTableView1Action(), parent));
        }
    );
    // Remove fixed height from QSize
    addDockingAction(table1Action, nullptr, gui::DockAreaFlag::Right, true, gui::AutoHideLocation::Right);
    // 3. Dock the third table ("Dataset 2 All Cells") on the right
    auto* table2Action = new mv::gui::WidgetAction(this, "All Cells Gene Table 2");
    table2Action->setWidgetConfigurationFunction(
        [createTableWidget, this](mv::gui::WidgetAction*, QWidget* parent) {
            auto layout = new QVBoxLayout(parent);

            layout->setContentsMargins(0, 0, 0, 0);
            parent->setLayout(layout);
            if (!_settingsAction.getTable2HeaderLabel())
            {
                _settingsAction.getTable2HeaderLabel() = new QLabel(parent);
            }
            _settingsAction.getTable2HeaderLabel()->setText("All Cells Gene Table 2");
            layout->addWidget(_settingsAction.getTable2HeaderLabel(), 0);
            layout->addWidget(createTableWidget("All Cells Gene Table 2", _settingsAction.getTableView2Action(), parent));
        }
    );
    addDockingAction(table2Action, nullptr, gui::DockAreaFlag::Right, true, gui::AutoHideLocation::Right);

    //_eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetAdded));
    //_eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetDataChanged));
    //_eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetRemoved));
    //_eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetDataSelectionChanged));
    //_eventListener.registerDataEventByType(PointType, std::bind(&GradientSurferPlugin::onDataEvent, this, std::placeholders::_1));
    //QApplication::processEvents();
}

/*void GradientSurferPlugin::onDataEvent(mv::DatasetEvent* dataEvent)
{

}*/

GradientSurferPluginFactory::GradientSurferPluginFactory()
{
    setIconByName("layer-group");
    getPluginMetadata().setDescription("Gradient surfer plugin");
    getPluginMetadata().setSummary("This plugin shows gradient surfer plugin in ManiVault Studio.");
    getPluginMetadata().setCopyrightHolder({ "BioVault (Biomedical Visual Analytics Unit LUMC - TU Delft)" });
    getPluginMetadata().setAuthors({
	});
    getPluginMetadata().setOrganizations({
        { "LUMC", "Leiden University Medical Center", "https://www.lumc.nl/en/" },
        { "TU Delft", "Delft university of technology", "https://www.tudelft.nl/" }
	});
    getPluginMetadata().setLicenseText("This plugin is distributed under the [LGPL v3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html) license.");
}

ViewPlugin* GradientSurferPluginFactory::produce()
{
    return new GradientSurferPlugin(this);
}

mv::DataTypes GradientSurferPluginFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;

    // This gradient surfer plugin is compatible with points datasets
    supportedTypes.append(PointType);

    return supportedTypes;
}

mv::gui::PluginTriggerActions GradientSurferPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    return pluginTriggerActions;
}
void GradientSurferPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);
    mv::util::variantMapMustContain(variantMap, "GradientSurfer:Settings");
    _settingsAction.fromVariantMap(variantMap["GradientSurfer:Settings"].toMap());
}

QVariantMap GradientSurferPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();

    _settingsAction.insertIntoVariantMap(variantMap);
    return variantMap;
}