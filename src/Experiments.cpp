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
        &DecimalAction::valueChanged,
        this,
        [this]() { _rotationDebounceTimer.start(300); });

    connect(&_rotationDebounceTimer, &QTimer::timeout, this, [this]() {
        applyRotation();
        });


    expOpts.getRotationDegreeAction().setToolTip("Rotation Angle");
    expOpts.getRotationDegreeAction().setDefaultWidgetFlags(DecimalAction::Slider | DecimalAction::SpinBox);
    expOpts.getRotationDegreeAction().setMaximum(360.00);
    expOpts.getRotationDegreeAction().setMinimum(0.00);
    expOpts.getRotationDegreeAction().setSingleStep(0.01);
    expOpts.getRotationDegreeAction().setNumberOfDecimals(2);

    expOpts.getApplyRotationAction().setToolTip("Apply Rotation");
    //expOpts.getApplyRotationAction().setDefaultWidgetFlags(TriggerAction::Trigger);

    expOpts.getLoadCoordinatesAction().setToolTip("Load Coordinates");
    //expOpts.getLoadCoordinatesAction().setDefaultWidgetFlags(TriggerAction::Trigger);

    expOpts.getStoreCoordinatesAction().setToolTip("Store Coordinates");
    //expOpts.getStoreCoordinatesAction().setDefaultWidgetFlags(TriggerAction::Trigger);

    expOpts.getLoadSelectionAction().setToolTip("Load Selection");
    //expOpts.getLoadSelectionAction().setDefaultWidgetFlags(TriggerAction::Trigger);

    expOpts.getStoreSelectionAction().setToolTip("Store Selection");
    //expOpts.getStoreSelectionAction().setDefaultWidgetFlags(TriggerAction::Trigger);

    expOpts.getSaveTablesAction().setToolTip("Save Tables");
    //expOpts.getSaveTablesAction().setDefaultWidgetFlags(TriggerAction::Trigger);

    expOpts.getApplyFlipAction().setToolTip("Apply Flip");
    //expOpts.getApplyFlipAction().setDefaultWidgetFlags(TriggerAction::Trigger);

    expOpts.getFlipTypeAction().setToolTip("Select Flip Type");
    expOpts.getFlipTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    expOpts.getFlipTypeAction().initialize({ "horizontal", "vertical" },"horizontal");

    connect(&expOpts.getSaveTablesAction(), &TriggerAction::triggered, this, [this]() { saveTables(); });
    connect(&expOpts.getStoreSelectionAction(), &TriggerAction::triggered, this, [this]() { storeSelection(); });
    connect(&expOpts.getLoadSelectionAction(), &TriggerAction::triggered, this, [this]() { loadSelection(); });
    connect(&expOpts.getStoreCoordinatesAction(), &TriggerAction::triggered, this, [this]() { storeRotation(); });
    connect(&expOpts.getLoadCoordinatesAction(), &TriggerAction::triggered, this, [this]() { loadRotation(); });
    connect(&expOpts.getApplyRotationAction(), &TriggerAction::triggered, this, [this]() { applyRotation(); });
    connect(&expOpts.getRotationDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, [this]() { applyRotation(); });

    connect(&expOpts.getApplyFlipAction(), &TriggerAction::triggered, this, [this]() { applyFlip(); });
    connect(&expOpts.getFlipTypeAction(), &OptionAction::currentIndexChanged, this, [this]() { applyFlip(); });
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
    Dataset<Points> dataset = expOpts.getRotationDatasetAction().getCurrentDataset();
    float degree = expOpts.getRotationDegreeAction().getValue();
    if (degree == 0.0f) {
        qDebug() << "Rotation degree is 0, no rotation applied.";
        return;
    }
    if (!dataset.isValid()) {
        qWarning() << "No valid dataset selected for rotation.";
        return;
    }
    int numofPoints = dataset->getNumPoints();
    int numOfDims = dataset->getNumDimensions();
    if (numOfDims < 2) {
        qWarning() << "Dataset has less than 2 dimensions, cannot apply rotation.";
        return;
    }

    if (numOfDims >2)
    {
        qWarning() << "Dataset has more than 2 dimensions, cannot apply rotation.";
        return;
    }
    if (numofPoints <= 0) {
        qWarning() << "Dataset has no points, cannot apply rotation.";
        return;
    }
    std::vector<float> XYCoordinates;
    std::vector<float> rotatedXYCordinates;

    qDebug() << "Applying rotation of" << degree << "degrees to dataset:" << dataset->getGuiName() + " (" + dataset.getDatasetId() + ")";

    try {
        XYCoordinates.resize(numofPoints * numOfDims);
        std::vector<int> dimIndices = { 0, 1 };
        std::vector<int> pointIndices(numofPoints);
        std::iota(pointIndices.begin(), pointIndices.end(), 0); 
        dataset->populateDataForDimensions(
            XYCoordinates,
            dimIndices,
            pointIndices
        );
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        return;
    }

    try {
        rotatedXYCordinates.resize(numofPoints * numOfDims);
        rotatedXYCordinates = rotateAxis(XYCoordinates, degree);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in rotateAxis:" << ex.what();
        return;
    }

    dataset->setData(rotatedXYCordinates.data(), numofPoints, numOfDims);
    mv::events().notifyDatasetDataChanged(dataset);

    qDebug() << "Rotation applied successfully.";

}

void Experiments::applyFlip()
{
    auto& expOpts = _settings->getExperimentOptionsHolder();
    Dataset<Points> dataset = expOpts.getRotationDatasetAction().getCurrentDataset();
    const QString& flipType = "vertical"; // "horizontal" or "vertical"

    if (!dataset.isValid()) {
        qWarning() << "No valid dataset selected for flip.";
        return;
    }
    int numofPoints = dataset->getNumPoints();
    int numOfDims = dataset->getNumDimensions();
    if (numOfDims < 2) {
        qWarning() << "Dataset has less than 2 dimensions, cannot apply flip.";
        return;
    }
    if (numOfDims > 2) {
        qWarning() << "Dataset has more than 2 dimensions, cannot apply flip.";
        return;
    }
    if (numofPoints <= 0) {
        qWarning() << "Dataset has no points, cannot apply flip.";
        return;
    }

    std::vector<float> XYCoordinates;
    XYCoordinates.resize(numofPoints * numOfDims);

    try {
        std::vector<int> dimIndices = { 0, 1 };
        std::vector<int> pointIndices(numofPoints);
        std::iota(pointIndices.begin(), pointIndices.end(), 0);
        dataset->populateDataForDimensions(
            XYCoordinates,
            dimIndices,
            pointIndices
        );
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        return;
    }

    // Flip logic
    for (int i = 0; i < numofPoints; ++i) {
        if (flipType.compare("vertical", Qt::CaseInsensitive) == 0) {
            // Flip Y
            XYCoordinates[i * 2 + 1] = -XYCoordinates[i * 2 + 1];
        }
        else if (flipType.compare("horizontal", Qt::CaseInsensitive) == 0) {
            // Flip X
            XYCoordinates[i * 2 + 0] = -XYCoordinates[i * 2 + 0];
        }
        else {
            qWarning() << "Unknown flip type:" << flipType;
            return;
        }
    }

    dataset->setData(XYCoordinates.data(), numofPoints, numOfDims);
    mv::events().notifyDatasetDataChanged(dataset);

    qDebug() << "Flip (" << flipType << ") applied successfully.";
}