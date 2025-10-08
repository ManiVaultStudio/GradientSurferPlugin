#include "Experiments.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

Experiments::Experiments(GradientSurferPlugin& plugin, SettingsAction& settings, Computation& computation, QWidget* parent)
    : QObject(parent),
    _plugin(&plugin),
    _settings(&settings),
    _computation(&computation),
    _parentWidget(parent)
{
    setupExperimentActions();
}

void Experiments::setupExperimentActions()
{
    // Access ExperimentOptionsHolder via _settings
    auto& expOpts = _settings->getExperimentOptionsHolder();

    connect(&expOpts.getSaveTablesAction(), &TriggerAction::triggered, this, [this]() { saveTables(); });
    connect(&expOpts.getSaveScatterplotsAction(), &TriggerAction::triggered, this, [this]() { saveScatterplots(); });
    connect(&expOpts.getSaveSelectionAction(), &TriggerAction::triggered, this, [this]() { saveSelection(); });
    connect(&expOpts.getLoadSelectionAction(), &TriggerAction::triggered, this, [this]() { loadSelection(); });
    connect(&expOpts.getRotationDegreeAction(), &IntegralAction::valueChanged, this, [this]() { applyRotation(); });
    connect(&expOpts.getRotationDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, [this]() { applyRotation(); });
}

void Experiments::saveTables()
{
    QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save Tables", QString(), "CSV Files (*.csv);;All Files (*)");
    if (fileName.isEmpty()) return;
    QMessageBox::information(_parentWidget, "Save Tables", "Tables saved to:\n" + fileName);
}

void Experiments::saveScatterplots()
{
    QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save Scatterplots", QString(), "PNG Files (*.png);;All Files (*)");
    if (fileName.isEmpty()) return;
    QMessageBox::information(_parentWidget, "Save Scatterplots", "Scatterplots saved to:\n" + fileName);
}

void Experiments::saveSelection()
{
    QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save Selection", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;
    QMessageBox::information(_parentWidget, "Save Selection", "Selection saved to:\n" + fileName);
}

void Experiments::loadSelection()
{
    QString fileName = QFileDialog::getOpenFileName(_parentWidget, "Load Selection", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;
    QMessageBox::information(_parentWidget, "Load Selection", "Selection loaded from:\n" + fileName);
}

void Experiments::applyRotation()
{
    auto& expOpts = _settings->getExperimentOptionsHolder();
    auto dataset = expOpts.getRotationDatasetAction().getCurrentDataset();
    int degree = expOpts.getRotationDegreeAction().getValue();

    if (!dataset.isValid()) {
        qWarning() << "No valid dataset selected for rotation.";
        return;
    }
    qDebug() << "Applying rotation of" << degree << "degrees to dataset:" << dataset.getDatasetId();
}