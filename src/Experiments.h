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
    void saveSelectedGeneList();
    void storeSelection();
    void loadSelection();
    void loadCoordinates();
    void storeCoordinates();
    void applyRotation();
    void applyFlip();
    void correctRangeAndAutoCompute();

    GradientSurferPlugin* _plugin;
    SettingsAction* _settings;
    Computation* _computation;
    QTimer _rotationDebounceTimer;
    QWidget* _parentWidget;
};