#pragma once

#include "SettingsAction.h"
#include <QObject>
#include <QString>

class GradientSurferPlugin;
class Computation;

class Experiments : public QObject
{
    Q_OBJECT

public:
    Experiments(GradientSurferPlugin& plugin, SettingsAction& settings, Computation& computation, QWidget* parent = nullptr);

private:
    void setupExperimentActions();

    void saveTables();
    void storeSelection();
    void loadSelection();
    void loadRotation();
    void storeRotation();
    void applyRotation();
    void applyFlip();

    GradientSurferPlugin* _plugin;
    SettingsAction* _settings;
    Computation* _computation;
    QTimer _rotationDebounceTimer;
    QWidget* _parentWidget;
};