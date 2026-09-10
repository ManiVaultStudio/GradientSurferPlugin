#include "Experiments.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include "ComputationUtils.h"
#include "Computation.h" 

using namespace mv;

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
        [this](mv::Dataset<mv::DatasetImpl> dataset) {
            return isValidPointChildDataset(dataset);
        }
    );
    expOpts.getSelectionDatasetAction().setFilterFunction(
        [this](mv::Dataset<mv::DatasetImpl> dataset) {
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
    expOpts.getLoadCoordinatesAction().setDefaultWidgetFlags(TriggerAction::IconText);
    expOpts.getLoadCoordinatesAction().setIconByName("document-open");
    expOpts.getLoadCoordinatesAction().setIconText("Load Coordinates");
    expOpts.getLoadCoordinatesAction().setText("Load Coordinates");
    
    expOpts.getAutoComputationAction().setToolTip("Auto Compute");
    expOpts.getAutoComputationAction().setDefaultWidgetFlags(ToggleAction::CheckBox);
    expOpts.getAutoComputationAction().setChecked(true);
    expOpts.getAutoComputationAction().setText("Auto Compute");
    expOpts.getAutoComputationAction().setIconByName("system-run");
    expOpts.getAutoComputationAction().setIconText("Auto Compute");
    connect(&expOpts.getAutoComputationAction(), &ToggleAction::toggled, this, [this]() {
        if (_settings->getExperimentOptionsHolder().getAutoComputationAction().isChecked()) {
            correctRangeAndAutoCompute();
        }
        });

    expOpts.getAutoComputationTriggerAction().setToolTip("Compute");
    expOpts.getAutoComputationTriggerAction().setDefaultWidgetFlags(TriggerAction::IconText);
    expOpts.getAutoComputationTriggerAction().setIconByName("system-run");
    expOpts.getAutoComputationTriggerAction().setIconText("Compute");
    expOpts.getAutoComputationTriggerAction().setText("Compute");
    connect(&expOpts.getAutoComputationTriggerAction(), &TriggerAction::triggered, this, [this]() { correctRangeAndAutoCompute(); });

    expOpts.getSaveGeneListAction().setToolTip("Save Selected Genes");
    expOpts.getSaveGeneListAction().setDefaultWidgetFlags(TriggerAction::IconText);
    expOpts.getSaveGeneListAction().setIconByName("document-save");
    expOpts.getSaveGeneListAction().setIconText("Save Selected Genes");
    expOpts.getSaveGeneListAction().setText("Save Selected Genes");
    connect(&expOpts.getSaveGeneListAction(), &TriggerAction::triggered, this, [this]() {
        saveSelectedGeneList();
        });


    expOpts.getStoreCoordinatesAction().setToolTip("Store Coordinates");
    expOpts.getStoreCoordinatesAction().setIconByName("document-save");
    expOpts.getStoreCoordinatesAction().setIconText("Store Coordinates");
    expOpts.getStoreCoordinatesAction().setText("Store Coordinates");
    expOpts.getStoreCoordinatesAction().setDefaultWidgetFlags(TriggerAction::IconText);

    expOpts.getLoadSelectionAction().setToolTip("Load Selection");
    expOpts.getLoadSelectionAction().setDefaultWidgetFlags(TriggerAction::IconText);
    expOpts.getLoadSelectionAction().setIconByName("document-open");
    expOpts.getLoadSelectionAction().setIconText("Load Selection");
    expOpts.getLoadSelectionAction().setText("Load Selection");

    expOpts.getStoreSelectionAction().setToolTip("Store Selection");
    expOpts.getStoreSelectionAction().setDefaultWidgetFlags(TriggerAction::IconText);
    expOpts.getStoreSelectionAction().setIconByName("document-save");
    expOpts.getStoreSelectionAction().setIconText("Store Selection");
    expOpts.getStoreSelectionAction().setText("Store Selection");

    expOpts.getSaveTablesAction().setToolTip("Save Tables");
    expOpts.getSaveTablesAction().setDefaultWidgetFlags(TriggerAction::IconText);
    expOpts.getSaveTablesAction().setIconByName("document-save");
    expOpts.getSaveTablesAction().setIconText("Save Tables");
    expOpts.getSaveTablesAction().setText("Save Tables");

    expOpts.getApplyFlipAction().setToolTip("Apply Flip");
    expOpts.getApplyFlipAction().setDefaultWidgetFlags(TriggerAction::IconText);
    expOpts.getApplyFlipAction().setIconByName("transform-move");
    expOpts.getApplyFlipAction().setIconText("Apply Flip");
    expOpts.getApplyFlipAction().setText("Apply Flip");

    expOpts.getFlipTypeAction().setToolTip("Select Flip Type");
    expOpts.getFlipTypeAction().setDefaultWidgetFlags(OptionAction::ComboBox);
    expOpts.getFlipTypeAction().initialize({ "horizontal", "vertical" },"horizontal");

    expOpts.getGeneListTableSaveNumberAction().setToolTip("Select Gene List Table");
    expOpts.getGeneListTableSaveNumberAction().setDefaultWidgetFlags(DatasetPickerAction::ComboBox);
    expOpts.getGeneListTableSaveNumberAction().initialize({
        "Gene Correlation Table 0",
        "Gene Correlation Table 1",
        "Gene Correlation Table 2"
        }, "Gene Correlation Table 0");

    expOpts.getGeneListTableSaveTopNAction().setToolTip("Top N Genes to Save");
    expOpts.getGeneListTableSaveTopNAction().setDefaultWidgetFlags(IntegralAction::SpinBox | IntegralAction::Slider);

    expOpts.getGeneListTableSaveBottomNAction().setToolTip("Bottom N Genes to Save");
    expOpts.getGeneListTableSaveBottomNAction().setDefaultWidgetFlags(IntegralAction::SpinBox | IntegralAction::Slider);




    connect(&expOpts.getSaveTablesAction(), &TriggerAction::triggered, this, [this]() { saveTables(); });
    connect(&expOpts.getStoreSelectionAction(), &TriggerAction::triggered, this, [this]() { storeSelection(); });
    connect(&expOpts.getLoadSelectionAction(), &TriggerAction::triggered, this, [this]() { loadSelection(); });
    connect(&expOpts.getStoreCoordinatesAction(), &TriggerAction::triggered, this, [this]() { storeCoordinates(); });
    connect(&expOpts.getLoadCoordinatesAction(), &TriggerAction::triggered, this, [this]() { loadCoordinates(); });
    connect(&expOpts.getApplyRotationAction(), &TriggerAction::triggered, this, [this]() { applyRotation(); });
    connect(&expOpts.getRotationDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, [this]() { applyRotation(); });

    connect(&expOpts.getApplyFlipAction(), &TriggerAction::triggered, this, [this]() { applyFlip(); });
    connect(&expOpts.getFlipTypeAction(), &OptionAction::currentIndexChanged, this, [this]() { applyFlip(); });
}

void Experiments::saveSelectedGeneList()
{
    const QString tableSelection = _settings->getExperimentOptionsHolder().getGeneListTableSaveNumberAction().getCurrentText();
    if (tableSelection.isEmpty()) {
        QMessageBox::warning(_parentWidget, "Save Selected Genes", "No gene correlation table selected.");
        return;
    }

    const std::vector<CorrelationGeneCoords>* geneCorrValues = nullptr;
    if (tableSelection == "Gene Correlation Table 0") {
        geneCorrValues = &_geneCorrValues0;
    }
    else if (tableSelection == "Gene Correlation Table 1") {
        geneCorrValues = &_geneCorrValues1;
    }
    else if (tableSelection == "Gene Correlation Table 2") {
        geneCorrValues = &_geneCorrValues2;
    }
    else {
        QMessageBox::warning(_parentWidget, "Save Selected Genes", "Invalid gene correlation table selected.");
        return;
    }

    if (!geneCorrValues || geneCorrValues->empty()) {
        QMessageBox::warning(_parentWidget, "Save Selected Genes", "No computed gene list available.");
        return;
    }

    int topN = _settings->getExperimentOptionsHolder().getGeneListTableSaveTopNAction().getValue();
    int bottomN = _settings->getExperimentOptionsHolder().getGeneListTableSaveBottomNAction().getValue();

    if (topN + bottomN <= 0) {
        QMessageBox::warning(_parentWidget, "Save Selected Genes", "Top N and Bottom N must be greater than zero.");
        return;
    }


    // Sort by correlationValue descending
    std::vector<CorrelationGeneCoords> sortedGenes = *geneCorrValues;
    std::sort(sortedGenes.begin(), sortedGenes.end(), [](const CorrelationGeneCoords& a, const CorrelationGeneCoords& b) {
        return a.correlationValue > b.correlationValue;
        });

    QStringList selectedGenes;
    // Top N
    for (int i = 0; i < std::min(topN, static_cast<int>(sortedGenes.size())); ++i) {
        selectedGenes << sortedGenes[i].geneName;
    }
    // Bottom N
    for (int i = 0; i < std::min(bottomN, static_cast<int>(sortedGenes.size())); ++i) {
        selectedGenes << sortedGenes[sortedGenes.size() - 1 - i].geneName;
    }

    if (selectedGenes.isEmpty()) {
        QMessageBox::warning(_parentWidget, "Save Selected Genes", "No genes are currently selected by computation.");
        return;
    }

    const QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save Selected Genes", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(_parentWidget, "Save Selected Genes", "Failed to open file for writing.");
        return;
    }

    QTextStream out(&file);
    for (const QString& geneName : selectedGenes)
        out << geneName << "\n";
    file.close();

    QMessageBox::information(_parentWidget, "Save Selected Genes", "Selected genes saved to:\n" + fileName);
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
    auto& expOpts = _settings->getExperimentOptionsHolder();
    mv::Dataset<Points> dataset = expOpts.getSelectionDatasetAction().getCurrentDataset();

    if (!dataset.isValid()) {
        QMessageBox::warning(_parentWidget, "Save Selection", "No valid dataset selected for saving selections.");
        return;
    }

    auto selectionIndices = dataset->getSelectionIndices();
    if (selectionIndices.empty()) {
        QMessageBox::warning(_parentWidget, "Save Selection", "No points selected in the dataset.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save Selection", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(_parentWidget, "Save Selection", "Failed to open file for writing.");
        return;
    }
    QTextStream out(&file);

    // Write indices as comma-separated values
    for (size_t i = 0; i < selectionIndices.size(); ++i) {
        out << selectionIndices[i];
        if (i < selectionIndices.size() - 1)
            out << ",";
    }
    out << "\n";
    file.close();

    QMessageBox::information(_parentWidget, "Save Selection", "Selection saved to:\n" + fileName);
}

void Experiments::loadSelection()
{
    auto& expOpts = _settings->getExperimentOptionsHolder();
    mv::Dataset<Points> dataset = expOpts.getSelectionDatasetAction().getCurrentDataset();

    if (!dataset.isValid()) {
        QMessageBox::warning(_parentWidget, "Load Selection", "No valid dataset selected for loading selections.");
        return;
    }

    int numofPoints = dataset->getNumPoints();

    QString fileName = QFileDialog::getOpenFileName(_parentWidget, "Load Selection", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(_parentWidget, "Load Selection", "Failed to open file for reading.");
        return;
    }
    QTextStream in(&file);

    QString line = in.readLine().trimmed();
    file.close();
    if (line.isEmpty()) {
        QMessageBox::warning(_parentWidget, "Load Selection", "File is empty.");
        return;
    }

    QStringList parts = line.split(",");
    std::vector<std::seed_seq::result_type> selectionIndices;
    selectionIndices.reserve(parts.size());
    for (const QString& part : parts) {
        bool ok = false;
        int idx = part.toInt(&ok);
        if (!ok) {
            QMessageBox::warning(_parentWidget, "Load Selection",
                QString("Invalid index '%1' in file. Not an integer.").arg(part));
            return;
        }
        if (idx < 0 || idx >= numofPoints) {
            QMessageBox::warning(_parentWidget, "Load Selection",
                QString("Invalid index '%1' in file. Must be between 0 and %2.").arg(idx).arg(numofPoints - 1));
            return;
        }
        selectionIndices.push_back(idx);
    }

    dataset->setSelectionIndices(selectionIndices);
    mv::events().notifyDatasetDataSelectionChanged(dataset);

    QMessageBox::information(_parentWidget, "Load Selection", "Selection loaded from:\n" + fileName);
    if (_settings->getExperimentOptionsHolder().getAutoComputationAction().isChecked()) {
        correctRangeAndAutoCompute();
    }
}

void Experiments::storeCoordinates()
{
    auto& expOpts = _settings->getExperimentOptionsHolder();
    mv::Dataset<Points> dataset = expOpts.getRotationDatasetAction().getCurrentDataset();

    if (!dataset.isValid()) {
        QMessageBox::warning(_parentWidget, "Save Rotation", "No valid dataset selected for saving coordinates.");
        return;
    }

    int numofPoints = dataset->getNumPoints();
    int numOfDims = dataset->getNumDimensions();
    if (numofPoints <= 0 || numOfDims <= 0) {
        QMessageBox::warning(_parentWidget, "Save Rotation", "Dataset must have at least one point and one dimension.");
        return;
    }

    std::vector<float> coordinates(numofPoints * numOfDims);
    try {
        std::vector<int> dimIndices(numOfDims);
        std::iota(dimIndices.begin(), dimIndices.end(), 0);
        std::vector<int> pointIndices(numofPoints);
        std::iota(pointIndices.begin(), pointIndices.end(), 0);
        dataset->populateDataForDimensions(
            coordinates,
            dimIndices,
            pointIndices
        );
    }
    catch (const std::exception& ex) {
        QMessageBox::warning(_parentWidget, "Save Rotation", QString("Error populating coordinates: %1").arg(ex.what()));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(_parentWidget, "Save Rotation", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(_parentWidget, "Save Rotation", "Failed to open file for writing.");
        return;
    }
    QTextStream out(&file);

    // Write header: numofPoints,numOfDims
    out << numofPoints << "," << numOfDims << "\n";
    // Write all coordinates in a single line: x1,y1,x2,y2,...
    for (int i = 0; i < numofPoints * numOfDims; ++i) {
        out << QString::number(coordinates[i], 'g', 8);
        if (i < numofPoints * numOfDims - 1)
            out << ",";
    }
    out << "\n";
    file.close();
    QMessageBox::information(_parentWidget, "Save Rotation", "Rotation saved to:\n" + fileName);
}

void Experiments::loadCoordinates()
{
    auto& expOpts = _settings->getExperimentOptionsHolder();
    mv::Dataset<Points> dataset = expOpts.getRotationDatasetAction().getCurrentDataset();

    if (!dataset.isValid()) {
        QMessageBox::warning(_parentWidget, "Load Rotation", "No valid dataset selected for loading coordinates.");
        return;
    }

    int currentNumofPoints = dataset->getNumPoints();
    int currentNumOfDims = dataset->getNumDimensions();

    QString fileName = QFileDialog::getOpenFileName(_parentWidget, "Load Rotation", QString(), "TXT Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(_parentWidget, "Load Rotation", "Failed to open file for reading.");
        return;
    }
    QTextStream in(&file);

    // Read header line
    QString header = in.readLine().trimmed();
    QStringList headerParts = header.split(",");
    if (headerParts.size() != 2) {
        QMessageBox::warning(_parentWidget, "Load Rotation", "Header line must contain numofPoints and numOfDims.");
        file.close();
        return;
    }
    bool ok1 = false, ok2 = false;
    int fileNumofPoints = headerParts[0].toInt(&ok1);
    int fileNumOfDims = headerParts[1].toInt(&ok2);
    if (!ok1 || !ok2 || fileNumofPoints <= 0 || fileNumOfDims <= 0) {
        QMessageBox::warning(_parentWidget, "Load Rotation", "Invalid numofPoints or numOfDims in header.");
        file.close();
        return;
    }

    // Check if current dataset matches file
    if (currentNumofPoints != fileNumofPoints || currentNumOfDims != fileNumOfDims) {
        QMessageBox::warning(_parentWidget, "Load Rotation",
            QString("Dataset shape mismatch.\nCurrent: %1 points, %2 dims\nFile: %3 points, %4 dims")
            .arg(currentNumofPoints).arg(currentNumOfDims)
            .arg(fileNumofPoints).arg(fileNumOfDims));
        file.close();
        return;
    }

    // Read coordinates line
    QString line = in.readLine().trimmed();
    file.close();
    if (line.isEmpty()) {
        QMessageBox::warning(_parentWidget, "Load Rotation", "File is missing coordinates line.");
        return;
    }

    QStringList parts = line.split(",");
    if (parts.size() != fileNumofPoints * fileNumOfDims) {
        QMessageBox::warning(_parentWidget, "Load Rotation",
            QString("Coordinate count mismatch.\nFile header: %1 points x %2 dims = %3 values\nFile contains: %4 values")
            .arg(fileNumofPoints).arg(fileNumOfDims).arg(fileNumofPoints * fileNumOfDims).arg(parts.size()));
        return;
    }

    std::vector<float> coordinates;
    coordinates.reserve(fileNumofPoints * fileNumOfDims);
    for (const QString& part : parts) {
        bool ok = false;
        float val = part.toFloat(&ok);
        if (!ok) {
            QMessageBox::warning(_parentWidget, "Load Rotation", "Invalid number in file.");
            return;
        }
        coordinates.push_back(val);
    }

    dataset->setData(coordinates.data(), fileNumofPoints, fileNumOfDims);
    mv::events().notifyDatasetDataChanged(dataset);

    QMessageBox::information(_parentWidget, "Load Rotation", "Rotation loaded from:\n" + fileName);
        if (_settings->getExperimentOptionsHolder().getAutoComputationAction().isChecked()) {
            correctRangeAndAutoCompute();
    }
}

void Experiments::applyRotation()
{
    auto& expOpts = _settings->getExperimentOptionsHolder();
    mv::Dataset<Points> dataset = expOpts.getRotationDatasetAction().getCurrentDataset();
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
        if (_settings->getExperimentOptionsHolder().getAutoComputationAction().isChecked()) {
            correctRangeAndAutoCompute();
        }

}

void Experiments::applyFlip()
{
    auto& expOpts = _settings->getExperimentOptionsHolder();
    mv::Dataset<Points> dataset = expOpts.getRotationDatasetAction().getCurrentDataset();
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
        if (_settings->getExperimentOptionsHolder().getAutoComputationAction().isChecked()) {
            correctRangeAndAutoCompute();
        }
}


void Experiments::correctRangeAndAutoCompute()
{
    // Dataset 1
    auto& ds1 = _settings->getDataset1OptionsHolder();
    auto& lower1 = ds1.getScatterplotLowerColoringRangeAction();
    auto& upper1 = ds1.getScatterplotUpperColoringRangeAction();
    auto pointDataset1 = ds1.getPointDatasetAction().getCurrentDataset();
    QString datasetId1 = pointDataset1.isValid() ? pointDataset1.getDatasetId() : "";
    QString scatterplotName1 = ds1.getScatterplotViewSelectionAction().getCurrentText();

    // Dataset 2
    auto& ds2 = _settings->getDataset2OptionsHolder();
    auto& lower2 = ds2.getScatterplotLowerColoringRangeAction();
    auto& upper2 = ds2.getScatterplotUpperColoringRangeAction();
    auto pointDataset2 = ds2.getPointDatasetAction().getCurrentDataset();
    QString datasetId2 = pointDataset2.isValid() ? pointDataset2.getDatasetId() : "";
    QString scatterplotName2 = ds2.getScatterplotViewSelectionAction().getCurrentText();

    // Get min/max from actual data (PointDataset)
    auto [min1, max1] = findDatasetLimits(datasetId1);
    auto [min2, max2] = findDatasetLimits(datasetId2);

    // Set min/max for range actions
    lower1.setMinimum(min1);
    lower1.setMaximum(max1);
    upper1.setMinimum(min1);
    upper1.setMaximum(max1);

    lower2.setMinimum(min2);
    lower2.setMaximum(max2);
    upper2.setMinimum(min2);
    upper2.setMaximum(max2);

    // Clamp values if needed
    float l1 = lower1.getValue();
    float u1 = upper1.getValue();
    if (l1 > u1) lower1.setValue(u1);
    if (l1 < min1) lower1.setValue(min1);
    if (u1 > max1) upper1.setValue(max1);

    float l2 = lower2.getValue();
    float u2 = upper2.getValue();
    if (l2 > u2) lower2.setValue(u2);
    if (l2 < min2) lower2.setValue(min2);
    if (u2 > max2) upper2.setValue(max2);

    // Update scatterplot plugin ranges
    rangeChangeScatterPlot(scatterplotName1, lower1.getValue(), upper1.getValue());
    rangeChangeScatterPlot(scatterplotName2, lower2.getValue(), upper2.getValue());

    auto& lineLower1 = ds1.getLineplotLowerColoringRangeAction();
    auto& lineUpper1 = ds1.getLineplotUpperColoringRangeAction();
    rangeChangeLinePlot(ds1.getLineplotViewSelectionAction().getCurrentText(), lineLower1.getValue(), lineUpper1.getValue());

    // Trigger auto-compute
    if (_computation) {
       _computation->setSelectionAndAutoCompute();
    }
}