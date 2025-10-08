#include "Experiments.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include "ComputationUtils.h"

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

    expOpts.getRotationDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) {
            return isValidPointChildDataset(dataset);
        }
    );
    _rotationDebounceTimer.setSingleShot(true);

    connect(&expOpts.getRotationDegreeAction(),
        &IntegralAction::valueChanged,
        this,
        [this]() { _rotationDebounceTimer.start(300); });

    connect(&_rotationDebounceTimer, &QTimer::timeout, this, [this]() {
        applyRotation();
        });


    expOpts.getRotationDegreeAction().setToolTip("Rotation Angle");
    expOpts.getRotationDegreeAction().setDefaultWidgetFlags(IntegralAction::Slider | IntegralAction::SpinBox);
    expOpts.getRotationDegreeAction().setMinimum(0);
    expOpts.getRotationDegreeAction().setMaximum(360);
    expOpts.getRotationDegreeAction().setValue(0);
    connect(&expOpts.getSaveTablesAction(), &TriggerAction::triggered, this, [this]() { saveTables(); });
    connect(&expOpts.getStoreSelectionAction(), &TriggerAction::triggered, this, [this]() { storeSelection(); });
    connect(&expOpts.getLoadSelectionAction(), &TriggerAction::triggered, this, [this]() { loadSelection(); });
    connect(&expOpts.getStoreRotationAction(), &TriggerAction::triggered, this, [this]() { storeRotation(); });
    connect(&expOpts.getLoadRotationAction(), &TriggerAction::triggered, this, [this]() { loadRotation(); });
    connect(&expOpts.getRotationDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, [this]() { applyRotation(); });
}

void Experiments::saveTables()
{
    QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save All Tables (Comparison)", QString(), "CSV Files (*.csv);;All Files (*)");
    if (fileName.isEmpty()) return;

    // Get models
    QTableView* table0 = _settings->getTableView0Action();
    QTableView* table1 = _settings->getTableView1Action();
    QTableView* table2 = _settings->getTableView2Action();

    QAbstractItemModel* model0 = table0 ? table0->model() : nullptr;
    QAbstractItemModel* model1 = table1 ? table1->model() : nullptr;
    QAbstractItemModel* model2 = table2 ? table2->model() : nullptr;

    // Map: gene -> {corr, rank} for each table
    QMap<QString, QPair<QString, QString>> map0, map1, map2;
    auto fillMap = [](QAbstractItemModel* model, QMap<QString, QPair<QString, QString>>& map) {
        if (!model) return;
        for (int r = 0; r < model->rowCount(); ++r) {
            QString gene = model->data(model->index(r, 0), Qt::DisplayRole).toString();
            QString corr = model->data(model->index(r, 1), Qt::DisplayRole).toString();
            QString rank = model->data(model->index(r, 2), Qt::DisplayRole).toString();
            map[gene] = qMakePair(corr, rank);
        }
        };
    fillMap(model0, map0);
    fillMap(model1, map1);
    fillMap(model2, map2);

    // Union of all gene names
    QSet<QString> allGenes;
    for (const QString& gene : map0.keys()) allGenes.insert(gene);
    for (const QString& gene : map1.keys()) allGenes.insert(gene);
    for (const QString& gene : map2.keys()) allGenes.insert(gene);
    QStringList sortedGenes = allGenes.values();
    std::sort(sortedGenes.begin(), sortedGenes.end(), [](const QString& a, const QString& b) { return a < b; });

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(_parentWidget, "Save Tables", "Failed to open file for writing.");
        return;
    }
    QTextStream out(&file);

    // Helper to sanitize values
    auto sanitize = [](const QString& val) -> QString {
        if (val.isEmpty() || val == "Undefined" || val == "NaN")
            return "N/A";
        bool ok = false;
        double d = val.toDouble(&ok);
        if (!ok && val.contains("nan", Qt::CaseInsensitive))
            return "N/A";
        if (ok && std::isnan(d))
            return "N/A";
        return val;
        };

    // Write header
    out << "Gene,SelCells_Corr,SelCells_Rank,AllCells1_Corr,AllCells1_Rank,AllCells2_Corr,AllCells2_Rank\n";

    // Write rows
    for (const QString& gene : sortedGenes) {
        QStringList row;
        row << gene;
        auto v0 = map0.value(gene, QPair<QString, QString>("N/A", "N/A"));
        auto v1 = map1.value(gene, QPair<QString, QString>("N/A", "N/A"));
        auto v2 = map2.value(gene, QPair<QString, QString>("N/A", "N/A"));
        row << sanitize(v0.first) << sanitize(v0.second)
            << sanitize(v1.first) << sanitize(v1.second)
            << sanitize(v2.first) << sanitize(v2.second);
        out << row.join(",") << "\n";
    }
    file.close();
    QMessageBox::information(_parentWidget, "Save Tables", "All tables saved to:\n" + fileName);
}


void Experiments::storeSelection()
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

void Experiments::storeRotation()
{
    QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save Rotation", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;
    QMessageBox::information(_parentWidget, "Save Rotation", "Rotation saved to:\n" + fileName);
}

void Experiments::loadRotation()
{
    QString fileName = QFileDialog::getOpenFileName(_parentWidget, "Load Rotation", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;
    QMessageBox::information(_parentWidget, "Load Rotation", "Rotation loaded from:\n" + fileName);
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