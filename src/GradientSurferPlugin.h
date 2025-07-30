#pragma once

#include <ViewPlugin.h>

#include <Dataset.h>
#include <widgets/DropWidget.h>

#include <PointData/PointData.h>

#include <QWidget>

#include "SettingsAction.h"
#include "Computation.h"

/** All plugin related classes are in the ManiVault plugin namespace */
using namespace mv::plugin;

/** Drop widget used in this plugin is located in the ManiVault gui namespace */
using namespace mv::gui;

/** Dataset reference used in this plugin is located in the ManiVault util namespace */
using namespace mv::util;

class QLabel;
class MinHeightEventFilter : public QObject {
    std::function<void()> _callback;
public:
    MinHeightEventFilter(std::function<void()> callback, QObject* parent = nullptr)
        : QObject(parent), _callback(std::move(callback)) {
    }
protected:
    bool eventFilter(QObject* obj, QEvent* event) override {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
            if (_callback) _callback();
        }
        return QObject::eventFilter(obj, event);
    }
};
/**
 * Gradient surfer plugin class
 *
 * This view plugin class provides skeleton code that shows how to develop
 * a view plugin in ManiVault. It shows how to use the built-in drag and drop
 * behavior.
 *
 * To see the plugin in action, please follow the steps below:
 *
 * 1. Go to the visualization menu in ManiVault
 * 2. Choose the gradient surfer menu item, the view will be added to the layout
 *
 */
class GradientSurferPlugin : public ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    GradientSurferPlugin(const PluginFactory* factory);

    /** Destructor */
    ~GradientSurferPlugin() override = default;
    
    /** This function is called by the core after the view plugin has been created */
    void init() override;

    /**
     * Invoked when a data event occurs
     * @param dataEvent Data event which occurred
     */
    void onDataEvent(mv::DatasetEvent* dataEvent);

public: // Serialization

    /**
     * Load widget action from variant map
     * @param Variant map representation of the widget action
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save widget action to variant map
     * @return Variant map representation of the widget action
     */
    QVariantMap toVariantMap() const override;

public:
    //SettingsAction& getSettingsAction() const { return _settingsAction; }

protected:
    SettingsAction        _settingsAction;
    Computation        _computation;
};

/**
 * Gradient explorer plugin factory class
 *
 * Note: Factory does not need to be altered (merely responsible for generating new plugins when requested)
 */
class GradientSurferPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.GradientSurferPlugin"
                      FILE  "PluginInfo.json")

public:

    /** Default constructor */
    GradientSurferPluginFactory();

    /** Creates an instance of the gradient surfer plugin */
    ViewPlugin* produce() override;

    /** Returns the data types that are supported by the gradient surfer plugin */
    mv::DataTypes supportedDataTypes() const override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;
};
