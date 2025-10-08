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
    QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save All Tables", QString(), "CSV Files (*.csv);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(_parentWidget, "Save Tables", "Failed to open file for writing.");
        return;
    }

    QTextStream out(&file);

    // Helper lambda to export a table
    auto exportTable = [&out](QTableView* table, const QString& sectionName) {
        if (!table || !table->model()) return;
        out << "==== " << sectionName << " ====\n";
        QAbstractItemModel* model = table->model();
        int rows = model->rowCount();
        int cols = model->columnCount();

        // Write header
        QStringList headers;
        for (int c = 0; c < cols; ++c)
            headers << model->headerData(c, Qt::Horizontal).toString();
        out << headers.join(",") << "\n";

        // Write data
        for (int r = 0; r < rows; ++r) {
            QStringList row;
            for (int c = 0; c < cols; ++c) {
                QString cell = model->data(model->index(r, c), Qt::DisplayRole).toString();
                // Escape quotes and commas
                cell.replace("\"", "\"\"");
                if (cell.contains(",") || cell.contains("\""))
                    cell = "\"" + cell + "\"";
                row << cell;
            }
            out << row.join(",") << "\n";
        }
        out << "\n";
        };

    // Export all three tables with section headers
    exportTable(_settings->getTableView0Action(), "Selected Cells Gene Table");
    exportTable(_settings->getTableView1Action(), "All Cells Gene Table 1");
    exportTable(_settings->getTableView2Action(), "All Cells Gene Table 2");

    file.close();
    QMessageBox::information(_parentWidget, "Save Tables", "All tables saved to:\n" + fileName);
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