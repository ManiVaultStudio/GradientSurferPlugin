#include "ComputationUtils.h"
#include "ColorUtils.h"

#include <QString>
#include <QElapsedTimer>
#include <QDebug>
#include <QColor>
#include <QtGlobal>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
std::vector<CorrelationGeneCoords> _geneCorrValues1;
std::vector<CorrelationGeneCoords> _geneCorrValues2;
std::vector<CorrelationGeneCoords> _geneCorrValues3;
std::vector<int> _geneOfInterestIndices1;
std::vector<int> _geneOfInterestIndices2;
std::vector<int> _geneOfInterestIndices3;
QList<QString> _commonGeneNames;
QStringList _commonGeneListSelectedInTables;


void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char* colorCode = "\033[0m"; // Default

    switch (type) {
    case QtDebugMsg:
        colorCode = "\033[1;36m"; // Cyan
        break;
    case QtInfoMsg:
        colorCode = "\033[1;32m"; // Green
        break;
    case QtWarningMsg:
        colorCode = "\033[1;33m"; // Yellow
        break;
    case QtCriticalMsg:
        colorCode = "\033[1;31m"; // Red
        break;
    case QtFatalMsg:
        colorCode = "\033[1;41m"; // Red background
        break;
    }

    fprintf(stderr, "%s[%s] %s (%s:%u, %s)%s\n",
        colorCode,
        (type == QtDebugMsg ? "DEBUG" :
            type == QtInfoMsg ? "INFO" :
            type == QtWarningMsg ? "WARNING" :
            type == QtCriticalMsg ? "CRITICAL" : "FATAL"),
        localMsg.constData(),
        context.file ? context.file : "",
        context.line,
        context.function ? context.function : "",
        "\033[0m"); // Reset color

    if (type == QtFatalMsg)
        abort();
}


FunctionTimer::FunctionTimer(const QString& functionName)
    : _functionName(functionName)
{
    _timer.start();
}

FunctionTimer::~FunctionTimer()
{
    //std::cout<< _functionName.toStdString()<< ":"<< QString::number(_timer.elapsed() / 1000.0, 'f', 3).toStdString() << "sec"<<"\n";
}

std::vector<std::seed_seq::result_type> filterSelectionIndicesByClusters(
    const std::vector<std::seed_seq::result_type>& selectionIndices,
    const QVector<Cluster>& clusters,
    const QStringList& selectedClusters)
{

    std::vector<std::seed_seq::result_type> filtered;
    filtered.reserve(selectionIndices.size());
    for (const auto& index : selectionIndices) {
        filtered.push_back(static_cast<int>(index));
    }
    for (const auto& cluster : clusters) {
        if (!selectedClusters.contains(cluster.getName())) {
            auto clusterindices = cluster.getIndices();
            filtered.erase(std::remove_if(filtered.begin(), filtered.end(),
                [&clusterindices](int index) {
                    return std::find(clusterindices.begin(), clusterindices.end(), index) != clusterindices.end();
                }), filtered.end());
        }
    }
    return filtered;
}

std::vector<int> getIndicesBySelectedClusters(
    const QVector<Cluster>& clusters,
    const QStringList& selectedClusters)
{
    std::vector<int> filtered;

    for (const auto& cluster : clusters) {
        if (selectedClusters.contains(cluster.getName())) {
            const auto& clusterIndices = cluster.getIndices();
            filtered.insert(filtered.end(),
                clusterIndices.begin(),
                clusterIndices.end());
        }
    }

    return filtered;
}

void clearTable(QTableView* tableView)
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(tableView->model());
    if (model) {
        model->clear();
    }
}

void syncTableSelections(QTableView* sourceTable, const QList<QTableView*>& allTables, const QList<DimensionPickerAction*>& allActions)
{
    static bool isSyncing = false;
    if (isSyncing) return;
    isSyncing = true;

    // Block signals for all tables during sync
    std::vector<std::unique_ptr<QSignalBlocker>> blockers;
    for (QTableView* table : allTables) {
        if (table && table->selectionModel())
            blockers.emplace_back(std::make_unique<QSignalBlocker>(table->selectionModel()));
    }

    auto* mdl = sourceTable->model();
    if (!mdl) {
        isSyncing = false;
        return;
    }

    QModelIndexList selectedIndexes = sourceTable->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) {
        // If selection is cleared in source, clear in all others
        for (QTableView* table : allTables) {
            if (table != sourceTable) {
                deselectAllRows(table);
            }
        }
        isSyncing = false;
        return;
    }

    const QModelIndex& index = selectedIndexes.first();
    if (!index.isValid()) {
        isSyncing = false;
        return;
    }

    QString value = mdl->data(mdl->index(index.row(), 0), Qt::DisplayRole).toString();

    // Explicitly set selection in the sourceTable as well
    makeSelection(value, sourceTable);

    // Sync selection in all other tables
    for (QTableView* table : allTables) {
        if (table != sourceTable) {
            makeSelection(value, table);
        }
    }
    // Sync all actions
    for (DimensionPickerAction* action : allActions) {
        if (action) action->setCurrentDimensionName(value);
    }

    isSyncing = false;
}

void lineplotModeChange(QString linePlotName, QString positionDatasetID, QString colorDatasetID, QString colordatasetType, int colorDatasetDimensionIndx,  QString xPositionString, QString yPositionString, QString titleText, QString smoothingType, int smoothingWindow, QString colorMapStringValue, float minValue, float maxValue)
{

    auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
    if (lineplotViewFactory)
    {
        if (linePlotName.isEmpty() || positionDatasetID.isEmpty() || xPositionString.isEmpty() || yPositionString.isEmpty())
        {
            qWarning() << "LinePlot name or position dataset ID is empty";
            bool pluginfound = false;
            for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
            {
                if (plugin->getGuiName() == linePlotName)
                {
                    pluginfound = true;
                    mv::gui::DatasetPickerAction* pointDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Point dataset");
                    mv::gui::DatasetPickerAction* clusterDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Color dataset");
                    if (clusterDatasetAction)
                    {
                        clusterDatasetAction->setCurrentIndex(-1);
                    }
                    if (pointDatasetAction)
                    {
                        pointDatasetAction->setCurrentIndex(-1);
                    }
                    mv::gui::StringAction* chartTitle = findActionByPath<StringAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Chart Title");
                    if (chartTitle)
                    {
                        chartTitle->setString(titleText);
                    }
                }
            }
            if (!pluginfound)
            {
                qWarning() << "LinePlot View plugin not found with name:" << linePlotName;
            }

            return;
        }
        else
        {
            bool pluginfound = false;
            for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
            {
                
                if (plugin->getGuiName() == linePlotName)
                {
                    pluginfound = true;
                    Dataset<Points> pointDataset = mv::data().getDataset<Points>(positionDatasetID);
                    if (pointDataset.isValid())
                    {
                        mv::gui::DatasetPickerAction* pointDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Point dataset");
                        if (pointDatasetAction)
                        {
                            pointDatasetAction->setCurrentDataset(pointDataset);

                            mv::gui::OptionAction* dimensionXAction = findActionByPath<OptionAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Data Dimension X Selection/Select dimension");
                            mv::gui::OptionAction* dimensionYAction = findActionByPath<OptionAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Data Dimension Y Selection/Select dimension");
                            if (dimensionXAction && dimensionYAction)
                            {
                                dimensionXAction->setCurrentText(xPositionString);
                                dimensionYAction->setCurrentText(yPositionString);
                                if (colordatasetType == "Clusters")
                                {
                                    mv::Dataset<Clusters> colorDataset = mv::data().getDataset<Clusters>(colorDatasetID);

                                    if (colorDataset.isValid())
                                    {
                                        mv::gui::DatasetPickerAction* clusterDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Color dataset");
                                        if (clusterDatasetAction)
                                        {
                                            clusterDatasetAction->setCurrentDataset(colorDataset);
                                        }
                                        else
                                        {
                                            qWarning() << "LinePlot color dataset action not found";
                                        }
                                    }

                                }
                                else if (colordatasetType == "Points")
                                {
                                    mv::Dataset<Points> colorDataset = mv::data().getDataset<Points>(colorDatasetID);

                                    if (colorDataset.isValid())
                                    {
                                        mv::gui::DatasetPickerAction* clusterDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Color dataset");
                                        if (clusterDatasetAction)
                                        {
                                            clusterDatasetAction->setCurrentDataset(colorDataset);
                                            if (colorDatasetDimensionIndx >= 0)
                                            {
                                                DimensionPickerAction* colorDimAction = findActionByPath<DimensionPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Color Point Dataset Dimension");
                                                if (colorDimAction)
                                                {

                                                    colorDimAction->setCurrentDimensionIndex(colorDatasetDimensionIndx);
                                                }
                                                else
                                                {
                                                    qWarning() << "LinePlot color dimension action not found";
                                                }
                                                if (!colorMapStringValue.isEmpty())
                                                {
                                                    auto colormapValAction = findActionByPath<OptionAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Point Dataset Dimension Color Map/Current color map");
                                                    if (colormapValAction)
                                                    {
                                                        colormapValAction->setCurrentText(colorMapStringValue);
                                                        DecimalAction* minValAction = findActionByPath<DecimalAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Lower Color Limit");
                                                        DecimalAction* maxValAction = findActionByPath<DecimalAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Upper Color Limit");
                                                        if (minValAction && maxValAction)
                                                        {
                                                            minValAction->setValue(minValue);
                                                            maxValAction->setValue(maxValue);
                                                        }
                                                        else
                                                        {
                                                            qWarning() << "LinePlot color map min/max value actions not found";
                                                            minValAction->setValue(0);
                                                            maxValAction->setValue(0);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        qWarning() << "LinePlot color map action not found";
                                                    }
                                                }
                                                else
                                                {
                                                    qWarning() << "LinePlot color map string value is empty";
                                                }
                                            }
                                            else
                                            {
                                                qWarning() << "LinePlot color dataset dimension is empty";
                                            }
                                        }
                                        else
                                        {
                                            qWarning() << "LinePlot color dataset action not found";
                                        }
                                    }

                                }
                                mv::gui::StringAction* chartTitle = findActionByPath<StringAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Chart Title");
                                if (chartTitle)
                                {
                                    chartTitle->setString(titleText);
                                }
                                mv::gui::OptionAction* smoothingTypeAction = findActionByPath<OptionAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Smoothing Type");
                                mv::gui::IntegralAction* smoothingWindowAction = findActionByPath<IntegralAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Smoothing Window");
                                if (smoothingTypeAction && smoothingWindowAction)
                                {
                                    smoothingTypeAction->setCurrentText(smoothingType);
                                    int maxVal = (pointDataset->getNumPoints());
                                    int currentVal = static_cast<int>(std::round(maxVal * 0.50));
                                    smoothingWindowAction->setMaximum(maxVal);
                                    smoothingWindowAction->setMinimum(1);
                                    smoothingWindowAction->setValue(currentVal);
                                }
                                else
                                {
                                    qWarning() << "Smoothing type or window action not found";
                                }
                            }
                            else
                            {
                                qWarning() << "LinePlot dimension actions not found";
                            }

                        }
                        else
                        {
                            qWarning() << "LinePlot point dataset action not found";
                        }
                    }
                    else
                    {
                        qWarning() << "LinePlot position dataset is not valid";
                    }
                }
            }
            if (!pluginfound)
            {
                qWarning() << "LinePlot View plugin not found with name:" << linePlotName;
            }
        }
    }
    else
    {
        qWarning() << "LinePlot View plugin not found";
    }
}

void scatterplotModeChange(QString scatterplotName, QString positionDatasetID, QString colorDatasetID, QString xPositionString, QString yPositionString, bool disableSelection, QString dimensionForColoring, QString colormapValue, float minValue, float maxValue)
{
    auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
    mv::gui::DatasetPickerAction* colorDatasetPickerValueAction;
    mv::gui::DatasetPickerAction* pointDatasetPickerValueAction;
    if ( positionDatasetID == "" || scatterplotName =="") {
        qWarning() << "Invalid parameters for scatterplot mode change:" 
                   << ", positionDatasetID:" << positionDatasetID
                   << ", scatterplotName:" << scatterplotName;
        return;
    }
    {
        if (scatterplotViewFactory)
        {
            bool pluginfound = false;
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName)
                {
                    pluginfound = true;
                    pointDatasetPickerValueAction = findActionByPath<DatasetPickerAction>(plugin, "Settings/Datasets/Position");
                    if (pointDatasetPickerValueAction)
                    {
                        if (!pointDatasetPickerValueAction->getCurrentDataset().isValid())
                        {
                            auto embeddingDataset = mv::data().getDataset(positionDatasetID);
                            if (embeddingDataset.isValid())
                            {
                                pointDatasetPickerValueAction->setCurrentDataset(embeddingDataset);
                            }
                        }
                        if (pointDatasetPickerValueAction->getCurrentDataset().isValid())
                        {
                            setScatterplotDimensions(plugin, xPositionString, yPositionString);
                            if (colorDatasetID != "")
                            {
                                colorDatasetPickerValueAction = findActionByPath<DatasetPickerAction>(plugin, "Settings/Datasets/Color");
                                if (colorDatasetPickerValueAction)
                                {
                                    auto clusterDataset = mv::data().getDataset(colorDatasetID);
                                    if (clusterDataset.isValid())
                                    {
                                        auto options = colorDatasetPickerValueAction->getDatasets();
                                        if (options.size() > 0)
                                        {
                                            try {
                                                colorDatasetPickerValueAction->setCurrentDataset(clusterDataset);
                                            }
                                            catch (const std::exception& e)
                                            {
                                                qCritical() << "Error setting current dataset:" << e.what();
                                            }



                                        }

                                    }

                                }
                            }


                        }



                    }
                    else
                    {
                        qCritical() << "Position dataset action not found in plugin:" << plugin->getGuiName();
                    }
                    auto freezeSelectionValueAction = findActionByPath<ToggleAction>(plugin, "Settings/Selection/Freeze selection");
                    if (freezeSelectionValueAction)
                    {
                        if (disableSelection)
                        {
                            freezeSelectionValueAction->setChecked(true);
                        }
                        else {
                            freezeSelectionValueAction->setChecked(false);
                        }

                    }
                    else
                    {
                        qCritical() << "Freeze selection action not found in plugin:" << plugin->getGuiName();
                    }
                    if (dimensionForColoring != "")
                    {
                        auto geneSelectionValueAction = findActionByPath<DimensionPickerAction>(plugin, "Settings/Coloring/Dimension");
                        if (geneSelectionValueAction) {
                            geneSelectionValueAction->setCurrentDimensionName(dimensionForColoring);
                        }
                    }

                    if (colormapValue != "")
                    {
                        auto colorMapValueAction = findActionByPath<OptionAction>(plugin, "Settings/Coloring/1D Color map/Current color map");
                        if (colorMapValueAction)
                        {
                            colorMapValueAction->setCurrentText(colormapValue);
                        }
                    }

                    break;
                }

            }
            if (!pluginfound)
            {
                qCritical() << "Scatterplot plugin not found with name:" << scatterplotName;
            }
        }
    }


}

void setupTableModelAndSelection(
    QTableView* tableViewMain,
    std::vector<CorrelationGeneCoords>& geneCorrValues,
    DimensionPickerAction& geneSelectionAction,
    QTableView* tableViewOther1,
    QTableView* tableViewOther2,
    DimensionPickerAction& geneSelectionAction2
)
{
    FunctionTimer timer(Q_FUNC_INFO);
    auto rowCount = static_cast<int>(geneCorrValues.size());
    QStandardItemModel* model = new QStandardItemModel(rowCount, 3, tableViewMain);
    model->setHorizontalHeaderLabels(QStringList() << "Gene" << "Correlation" << "Rank");

    float minCorr = std::numeric_limits<float>::max();
    float maxCorr = std::numeric_limits<float>::lowest();
    for (const auto& entry : geneCorrValues) {
        minCorr = std::min(minCorr, entry.correlationValue);
        maxCorr = std::max(maxCorr, entry.correlationValue);
    }
    _commonGeneNames.clear();


    QVector<QStandardItem*> rowItems(3);
    for (int i = 0; i < rowCount; ++i) {
        const auto& entry = geneCorrValues[i];

        QStandardItem* geneItem = new QStandardItem(entry.geneName);
        QStandardItem* corrItem = new QStandardItem;
        QStandardItem* rankItem = new QStandardItem(QString::number(entry.rank));

        corrItem->setData(QVariant::fromValue(entry.correlationValue), Qt::UserRole + 1);
        corrItem->setData(QVariant::fromValue(entry.correlationValue), Qt::DisplayRole);

        if (entry.isWithinRange) {
            QFont fontG = geneItem->font();
            fontG.setBold(true);
            geneItem->setFont(fontG);
            geneItem->setBackground(QColor("#FFB6C1"));
            geneItem->setForeground(QColor("#000000"));

            QFont fontR = rankItem->font();
            fontR.setBold(true);
            rankItem->setFont(fontR);
            rankItem->setBackground(QColor("#FFB6C1"));
            rankItem->setForeground(QColor("#000000"));

            _commonGeneNames.append(entry.geneName);
        }

        corrItem->setForeground(QColor("#000000"));

        rowItems[0] = geneItem;
        rowItems[1] = corrItem;
        rowItems[2] = rankItem;
        for (int col = 0; col < 3; ++col)
            model->setItem(i, col, rowItems[col]);
    }

    tableViewMain->setModel(model);
    tableViewMain->setColumnHidden(2, true);
    //deselectAllRows(tableViewMain);
    // Ensure only one row can be selected at a time and the full row gets selected
    tableViewMain->setSelectionMode(QAbstractItemView::SingleSelection);
    tableViewMain->setSelectionBehavior(QAbstractItemView::SelectRows);

    tableViewMain->setItemDelegateForColumn(1, new CorrelationBarDelegate(minCorr, maxCorr, model));
    tableViewMain->resizeColumnsToContents();
    tableViewMain->resizeRowsToContents();
    tableViewMain->setSortingEnabled(true);
    tableViewMain->sortByColumn(1, Qt::DescendingOrder);

    // Disconnect previous connections for all tables
    auto disconnectSelection = [](QTableView* table) {
        if (table && table->selectionModel())
            QObject::disconnect(table->selectionModel(), nullptr, nullptr, nullptr);
    };
    disconnectSelection(tableViewMain);
    disconnectSelection(tableViewOther1);
    disconnectSelection(tableViewOther2);

    // Prepare lists for sync helper
    QList<QTableView*> allTables = { tableViewMain, tableViewOther1, tableViewOther2 };
    QList<DimensionPickerAction*> allActions = { &geneSelectionAction, &geneSelectionAction2 };

    // Connect selectionChanged for all tables
    for (QTableView* table : allTables) {
        if (!table) continue;
        QObject::connect(
            table->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            table,
            [table, allTables, allActions](const QItemSelection&, const QItemSelection&) {
                syncTableSelections(table, allTables, allActions);
            }
        );
    }

}

void makeSelection(QString value, QTableView* tableView)
{
    if (!tableView || !tableView->selectionModel()) return;

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(tableView->model());
    if (!model) return;

    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    QSignalBlocker blocker(tableView->selectionModel());
    tableView->selectionModel()->clearSelection();

    for (int row = 0; row < model->rowCount(); ++row) {
        QString geneName = model->data(model->index(row, 0), Qt::DisplayRole).toString();
        if (geneName == value) {
            QModelIndex startIndex = model->index(row, 0);

            tableView->selectionModel()->setCurrentIndex(
                startIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
            );

            if (!startIndex.isValid()) {
                qWarning() << "Invalid index for selection, skipping scroll.";
                return;
            }

            tableView->scrollTo(startIndex, QAbstractItemView::EnsureVisible);

            // Defer the update to avoid crash
            QTimer::singleShot(0, tableView, [tableView]() { tableView->viewport()->update(); tableView->repaint(); });

            break;
        }
    }
}

void selectFirstRow(QTableView* tableView)
{
    if (!tableView || !tableView->selectionModel()) return;

    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* model = tableView->model();
    if (!model || model->rowCount() <= 0) return;

    QModelIndex firstIndex = model->index(0, 0);
    if (!firstIndex.isValid()) return;

    tableView->selectionModel()->clearSelection();

    tableView->selectionModel()->setCurrentIndex(
        firstIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
    );

    if (!firstIndex.isValid()) {
        qWarning() << "Invalid index for selection, skipping scroll.";
        return;
    }

    tableView->scrollTo(firstIndex, QAbstractItemView::EnsureVisible);
    QTimer::singleShot(0, tableView, [tableView]() { tableView->viewport()->update(); tableView->repaint(); });

}

void deselectAllRows(QTableView* tableView)
{
    if (!tableView || !tableView->selectionModel()) return;
    if (!tableView->viewport()) return;
    if (!tableView->model()) return;

    QSignalBlocker blocker(tableView->selectionModel());
    tableView->selectionModel()->clearSelection();
    tableView->clearFocus();
    tableView->setCurrentIndex(QModelIndex());
    QTimer::singleShot(0, tableView, [tableView]() { tableView->viewport()->update(); tableView->repaint(); });
}

void populateGenesByCorrelationRange(
    const std::vector<float>& correlationVector,
    int topNRows,
    int bottomNRows,
    const std::vector<QString>& geneNameOrdered,
    std::vector<CorrelationGeneCoords>& geneCorrValues,
    std::vector<int>& geneOfInterestIndices,
    bool isMainTable)
{
    constexpr float epsilon = 1e-6f;
    const int count = std::min(static_cast<int>(correlationVector.size()), static_cast<int>(geneNameOrdered.size()));
    if (count == 0 || topNRows < 0 || bottomNRows < 0) return;

    std::vector<int> indices(count);
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(),
        [&correlationVector](int a, int b) {
            return correlationVector[a] > correlationVector[b];
        });

    geneCorrValues.clear();
    geneCorrValues.reserve(count);
    geneOfInterestIndices.clear();
    geneOfInterestIndices.reserve(count);

    if (isMainTable) {
        _commonGeneListSelectedInTables.clear();
    }

    int rank = 1;
    float prevCorr = std::numeric_limits<float>::quiet_NaN();
    for (int i = 0; i < count; ++i) {
        int idx = indices[i];
        float corr = correlationVector[idx];

        // Assign rank: same correlation values get the same rank
        if (i == 0 || std::abs(corr - prevCorr) > epsilon) {
            rank = i + 1;
            prevCorr = corr;
        }

        bool isWithinRange = false;
        // Only mark as within range if topNRows or bottomNRows > 0 and within those bounds
        if (isMainTable) {
            if ((topNRows > 0 && rank <= topNRows) ||
                (bottomNRows > 0 && rank > (count - bottomNRows))) {
                isWithinRange = true;
                _commonGeneListSelectedInTables.push_back(geneNameOrdered[idx]);
            }
        } else {
            isWithinRange = _commonGeneListSelectedInTables.contains(geneNameOrdered[idx]);
        }

        geneCorrValues.emplace_back(CorrelationGeneCoords{
            idx,
            geneNameOrdered[idx],
            corr,
            rank,
            isWithinRange
        });
        if (isWithinRange) {
            geneOfInterestIndices.push_back(idx);
        }
    }
}

void resetupCorrelationTable(std::vector<CorrelationGeneCoords>& geneCorrValues, std::vector<int>& geneOfInterestIndices, QTableView* tableViewMain, int topNRows, int bottomNRows, DimensionPickerAction& geneSelectionAction, QTableView* tableViewOther1, QTableView* tableViewOther2, DimensionPickerAction& geneSelectionAction2, bool isMainTable)
{
    constexpr float epsilon = 1e-6f;
    geneOfInterestIndices.clear();
    geneOfInterestIndices.reserve(geneCorrValues.size());

    int count = static_cast<int>(geneCorrValues.size());

    if (isMainTable)
    {
        _commonGeneListSelectedInTables.clear();
    }
    for (auto& gene : geneCorrValues) {

        bool isWithinRange = false;
        // Only mark as within range if topNRows or bottomNRows > 0 and within those bounds
        if (isMainTable) {
            if ((topNRows > 0 && gene.rank <= topNRows) ||
                (bottomNRows > 0 && gene.rank > (count - bottomNRows))) {
                isWithinRange = true;
                _commonGeneListSelectedInTables.push_back(gene.geneName);
            }
        }
        else {
            isWithinRange = _commonGeneListSelectedInTables.contains(gene.geneName);
        }

        gene.isWithinRange = isWithinRange;
        if (isWithinRange) {
            geneOfInterestIndices.push_back(gene.geneIndex);

        }
    }
    setupTableModelAndSelection(tableViewMain, geneCorrValues, geneSelectionAction, tableViewOther1, tableViewOther2, geneSelectionAction2);

}

void computeCorrelationGeneCoords(const std::vector<int>& cellIndices, const mv::Dataset<Points> dataset, const std::vector<float>& coords, std::vector<float>& corrVector, QString corrType)
{
    FunctionTimer timer(Q_FUNC_INFO);
    const int numGenes = dataset->getNumDimensions();
    const int numCells = cellIndices.size();
    const int numCoords = coords.size();

    assert(numCells == numCoords);



    std::vector<std::pair<float, int>> coordIndexPairs;
    for (size_t i = 0; i < cellIndices.size(); ++i) {
        coordIndexPairs.emplace_back(coords[i], cellIndices[i]);
    }


    std::sort(coordIndexPairs.begin(), coordIndexPairs.end());


    std::vector<int> sortedCellIndices;
    std::vector<float> sortedCoords;
    sortedCellIndices.reserve(cellIndices.size());
    sortedCoords.reserve(coords.size());

    for (const auto& pair : coordIndexPairs) {
        sortedCoords.push_back(pair.first);
        sortedCellIndices.push_back(pair.second);
    }



    float meanCoord = computeMean(sortedCoords);

    std::vector<float> centeredCoords(numCells);
    float normCoord = 0.0f;
    for (int i = 0; i < numCells; ++i) {
        float c = sortedCoords[i] - meanCoord;
        centeredCoords[i] = c;
        normCoord += c * c;
    }

    corrVector.clear();
    corrVector.resize(numGenes);

#pragma omp parallel for
    for (int geneIdx = 0; geneIdx < numGenes; ++geneIdx) {
        std::vector<float> geneVals(numCells);
        float meanGene = 0.0f;

        for (int i = 0; i < numCells; ++i) {
            int rowIdx = sortedCellIndices[i];
            float val = dataset->getValueAt(rowIdx * numGenes + geneIdx);
            geneVals[i] = val;
            meanGene += val;
        }
        meanGene /= numCells;

        float normGene = 0.0f;
        float dotProd = 0.0f;
        for (int i = 0; i < numCells; ++i) {
            float centered = geneVals[i] - meanGene;
            normGene += centered * centered;
            dotProd += centered * centeredCoords[i];
        }

        float corr = (normGene > 0.0f && normCoord > 0.0f)
            ? (dotProd / std::sqrt(normGene * normCoord))
            : 0.0f;

        if (std::isnan(corr)) corr = 0.0f;

        if (corrType == "Absolute") {
            corrVector[geneIdx] = std::abs(corr);
        }
        else if (corrType == "Positive") {
            corrVector[geneIdx] = (corr > 0.0f) ? corr : 0.0f;
        }
        else if (corrType == "Negative") {
            corrVector[geneIdx] = (corr < 0.0f) ? corr : 0.0f;
        }
        else {
            corrVector[geneIdx] = corr;
        }

    }


}
void computeCorrelationGenePC(const mv::Dataset<Points>& fullDataset, const std::vector<float>& selectedPC, std::vector<float>& corrVector, std::vector<int>& allClusterFilteredIndices, QString normTypeStr, bool normalizeData)
{
    FunctionTimer timer(Q_FUNC_INFO);

    const int numGenes = fullDataset->getNumDimensions();
    const int numAllCells = fullDataset->getNumPoints();
    const int numCellsFilt = allClusterFilteredIndices.size();

    const int numCellsInPC = selectedPC.size();

    assert(numCellsFilt == numCellsInPC);

    float meanCoord = computeMean(selectedPC);

    std::vector<float> centeredCoords(numCellsFilt);
    float normCoord = 0.0f;
    for (int i = 0; i < numCellsFilt; ++i) {
        float c = selectedPC[i] - meanCoord;
        centeredCoords[i] = c;
        normCoord += c * c;
    }

    corrVector.clear();
    corrVector.resize(numGenes);

    for (int geneIdx = 0; geneIdx < numGenes; ++geneIdx) {
        std::vector<float> geneVals(numCellsFilt);

        //fullDataset->extractDataForDimension(geneVals, geneIdx);
        std::vector<int> geneIndex = { geneIdx };
        fullDataset->populateDataForDimensions(geneVals,geneIndex, allClusterFilteredIndices);
        if (normalizeData)
        {
            NormalizationType normType = normalizationTypeFromString(normTypeStr);

            normalizeVector(geneVals, normType);
        }
        float sumGene = 0.0f;
        for (float v : geneVals) sumGene += v;
        float meanGene = sumGene / numCellsFilt;

        float normGene = 0.0f;
        float dotProd = 0.0f;
        for (int i = 0; i < numCellsFilt; ++i) {
            float centered = geneVals[i] - meanGene;
            normGene += centered * centered;
            dotProd += centered * centeredCoords[i];
        }

        float corr = (normGene > 0.0f && normCoord > 0.0f)
            ? (dotProd / std::sqrt(normGene * normCoord))
            : 0.0f;

        if (std::isnan(corr)) corr = 0.0f;

        corrVector[geneIdx] = corr;
    }


}

std::pair<QFont, int> getStatusLabelFontAndIconSize(const QString& labelText) {
    QLabel tempLabel;
    tempLabel.setText(labelText);
    QFont font = tempLabel.font();
    QFontMetrics fm(font);
    int textHeight = fm.height();
    int iconSize = std::max(12, textHeight - 2);
    return { font, iconSize };
}
float computeMean(const std::vector<float>& values) {
    if (values.empty()) return 0.0f;
    float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    return sum / static_cast<float>(values.size());
}

void setScatterplotDimensions(mv::plugin::Plugin* plugin, const QString& xName, const QString& yName) {
    auto embeddingXVal = findActionByPath<OptionAction>(plugin, "Settings/Position/X/Select dimension");
    auto embeddingYVal = findActionByPath<OptionAction>(plugin, "Settings/Position/Y/Select dimension");
    if (embeddingXVal && embeddingYVal) {
        if (embeddingXVal->getCurrentIndex() != -1){ 
            embeddingXVal->setCurrentText("");
        }
        if (embeddingYVal->getCurrentIndex() != -1) {
            embeddingYVal->setCurrentText("");
        }
        if (xName != "") {
            if (embeddingXVal->getOptions().contains(xName)) {
                embeddingXVal->setCurrentText(xName);
            }
        }
        else
        {
            if (embeddingXVal->getOptions().size() > 0)
            {
                embeddingXVal->setCurrentIndex(0);
            }
        }
        if (yName != "") {
            if (embeddingYVal->getOptions().contains(yName)) {
                embeddingYVal->setCurrentText(yName);
            }
        }
        else
        {
            if (embeddingYVal->getOptions().size() > 1)
            {
                embeddingYVal->setCurrentIndex(1);
            }
            else if (embeddingYVal->getOptions().size() > 0)
            {
                embeddingYVal->setCurrentIndex(0);
            }
        }
    }
}
bool isCorrelationWithinRange(float corr, float minCorr, float maxCorr, float epsilon)
{
    return (corr <= minCorr - epsilon) || (corr >= maxCorr + epsilon);
}
std::pair<float, float> minMax(const std::vector<float>& vec)
{
    auto [minVal, maxVal] = std::minmax_element(vec.begin(), vec.end());
    if (minVal == vec.end() || maxVal == vec.end()) {
        throw std::invalid_argument("Input vector is empty.");
    }
    return { *minVal, *maxVal };
}
QLabel* createStatusLabel(const QString& labelText, const QString& backgroundColor, const QString& textColor, const QPixmap& iconPixmap, const QFont& font) {
    QLabel* statusLabel = new QLabel;
    statusLabel->setTextFormat(Qt::RichText);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    iconPixmap.save(&buffer, "PNG");
    QString imgTag = QString("<img style='vertical-align:middle;' width='%1' height='%2' src='data:image/png;base64,%3'/>")
        .arg(iconPixmap.width())
        .arg(iconPixmap.height())
        .arg(QString::fromLatin1(ba.toBase64()));
    statusLabel->setText(imgTag + "&nbsp;" + labelText);
    statusLabel->setStyleSheet(
        "QLabel{padding-left:8px;background:" + backgroundColor + ";color:" + textColor + ";}"
    );
    statusLabel->setFont(font);
    return statusLabel;
}
QStringList getCommonSortedNames(const QSet<QString>& names1, const QSet<QString>& names2)
{
    QStringList clusterNamesCommon = (names1 & names2).values();
    std::sort(clusterNamesCommon.begin(), clusterNamesCommon.end(),
        [](const QString& a, const QString& b) {
            return a.compare(b, Qt::CaseInsensitive) < 0;
        });
    return clusterNamesCommon;
}
void normalizeVector(std::vector<float>& data, NormalizationType type) {
    if (type == NormalizationType::None || data.empty()) return;

    auto cpm = [](std::vector<float>& vec) {
        float sum = std::accumulate(vec.begin(), vec.end(), 0.0f);
        if (sum > std::numeric_limits<float>::epsilon()) {
            for (auto& v : vec) v = (v / sum) * 1e6f;
        }
        };

    auto log1p = [](std::vector<float>& vec) {
        for (auto& v : vec) v = std::log1p(std::max(v, 0.0f));  // log(1 + x)
        };

    auto zscore = [](std::vector<float>& vec) {
        float mean = std::accumulate(vec.begin(), vec.end(), 0.0f) / vec.size();
        float sq_sum = std::inner_product(vec.begin(), vec.end(), vec.begin(), 0.0f);
        float variance = sq_sum / vec.size() - mean * mean;
        float stdev = std::sqrt(std::max(variance, 0.0f));
        if (stdev > std::numeric_limits<float>::epsilon()) {
            for (auto& v : vec) v = (v - mean) / stdev;
        }
        };

    switch (type) {
    case NormalizationType::CPM:
        cpm(data);
        break;

    case NormalizationType::Log1p:
        log1p(data);
        break;

    case NormalizationType::CPM_Log1p:
        cpm(data);
        log1p(data);
        break;

    case NormalizationType::CPM_Log1p_ZScore:
        cpm(data);
        log1p(data);
        zscore(data);
        break;

    case NormalizationType::ZScore:
        zscore(data);
        break;

    case NormalizationType::MinMax: {
        auto [minIt, maxIt] = std::minmax_element(data.begin(), data.end());
        float minV = *minIt, maxV = *maxIt;
        float range = maxV - minV;
        if (range > std::numeric_limits<float>::epsilon()) {
            for (auto& v : data) v = (v - minV) / range;
        }
        break;
    }

    case NormalizationType::Log2:
        for (auto& v : data) v = std::log2(std::max(v + 1.0f, 1e-6f));
        break;

    case NormalizationType::MaxAbs: {
        float maxAbs = 0.0f;
        for (const auto& v : data) maxAbs = std::max(maxAbs, std::fabs(v));
        if (maxAbs > std::numeric_limits<float>::epsilon()) {
            for (auto& v : data) v /= maxAbs;
        }
        break;
    }

    case NormalizationType::L2: {
        float sumSquares = std::inner_product(data.begin(), data.end(), data.begin(), 0.0f);
        float norm = std::sqrt(sumSquares);
        if (norm > std::numeric_limits<float>::epsilon()) {
            for (auto& v : data) v /= norm;
        }
        break;
    }

    case NormalizationType::L1: {
        float sumAbs = std::accumulate(data.begin(), data.end(), 0.0f, [](float a, float b) {
            return a + std::fabs(b);
            });
        if (sumAbs > std::numeric_limits<float>::epsilon()) {
            for (auto& v : data) v /= sumAbs;
        }
        break;
    }

    case NormalizationType::Mean: {
        float mean = std::accumulate(data.begin(), data.end(), 0.0f) / data.size();
        for (auto& v : data) v -= mean;
        break;
    }

    case NormalizationType::Softmax: {
        // Softmax: exp(x_i) / sum(exp(x_j))
        std::vector<float> expVals(data.size());
        float maxVal = *std::max_element(data.begin(), data.end());
        float sumExp = 0.0f;
        for (size_t i = 0; i < data.size(); ++i) {
            expVals[i] = std::exp(data[i] - maxVal); // for numerical stability
            sumExp += expVals[i];
        }
        if (sumExp > std::numeric_limits<float>::epsilon()) {
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] = expVals[i] / sumExp;
            }
        }
        break;
    }

    case NormalizationType::Robust: {
        // Robust: (x - median) / IQR
        std::vector<float> sorted(data);
        std::sort(sorted.begin(), sorted.end());
        float median = sorted[sorted.size() / 2];
        float q1 = sorted[sorted.size() / 4];
        float q3 = sorted[(3 * sorted.size()) / 4];
        float iqr = q3 - q1;
        if (iqr > std::numeric_limits<float>::epsilon()) {
            for (auto& v : data) v = (v - median) / iqr;
        }
        break;
    }

    case NormalizationType::UnitRange: {
        // UnitRange: scale to [-1, 1]
        auto [minIt, maxIt] = std::minmax_element(data.begin(), data.end());
        float minV = *minIt, maxV = *maxIt;
        float range = maxV - minV;
        if (range > std::numeric_limits<float>::epsilon()) {
            for (auto& v : data) v = 2.0f * (v - minV) / range - 1.0f;
        }
        break;
    }

    case NormalizationType::Binarize: {
        // Binarize: set to 1 if above mean, else 0
        float mean = std::accumulate(data.begin(), data.end(), 0.0f) / data.size();
        for (auto& v : data) v = (v > mean) ? 1.0f : 0.0f;
        break;
    }

    default:
        break;
    }
}


NormalizationType normalizationTypeFromString(const QString& str) {
    if (str == "ZScore") return NormalizationType::ZScore;
    if (str == "MinMax") return NormalizationType::MinMax;
    if (str == "Log2") return NormalizationType::Log2;
    if (str == "MaxAbs") return NormalizationType::MaxAbs;
    if (str == "L2") return NormalizationType::L2;
    if (str == "L1") return NormalizationType::L1;
    if (str == "Mean") return NormalizationType::Mean;
    if (str == "Softmax") return NormalizationType::Softmax;
    if (str == "Robust") return NormalizationType::Robust;
    if (str == "UnitRange") return NormalizationType::UnitRange;
    if (str == "Binarize") return NormalizationType::Binarize;
    if (str == "CPM") return NormalizationType::CPM;
    if (str == "Log1p") return NormalizationType::Log1p;
    if (str == "CPM_Log1p") return NormalizationType::CPM_Log1p;
    if (str == "CPM_Log1p_ZScore") return NormalizationType::CPM_Log1p_ZScore;
    return NormalizationType::None;
}


float computeGradientScore(const std::vector<float>& corrArray, const std::vector<float>& expArray) {
    if (corrArray.size() != expArray.size()) {
        qWarning() << "Correlation and expression arrays must have the same size for gradient score calculation.";
        return 0.0f;
    }
    float score = 0.0f;
    int N = std::min(corrArray.size(), expArray.size());
    for (int i = 0; i < N; ++i) {
        score += corrArray[i] * expArray[i];
    }
    return score;
}

QString getContrastingTextColor(const QString& bgColor) {
    QColor color(bgColor);
    if (!color.isValid()) return "white";
    double luminance = (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) / 255.0;
    return (luminance > 0.5) ? "black" : "white";
}

QString statusToIconName(const QString& status) {
    if (status == "C") return "circle-check"; 
    if (status == "M") return "circle-exclamation";
    if (status == "E") return "circle-xmark";      
    if (status == "R") return "spinner";          
    return "circle-question";                      
}

std::vector<float> rotateAxis(const std::vector<float>& xyCoordinates, float degreeValue) {
    std::vector<float> rotated(xyCoordinates.size());
    const double angle = degreeValue * M_PI / 180.0;
    const double cosAngle = std::cos(angle);
    const double sinAngle = std::sin(angle);

    if (xyCoordinates.size() % 2 != 0) {
        throw std::invalid_argument("xyCoordinates vector must have an even number of elements (x, y pairs).");
    }

    float minX = xyCoordinates[0], maxX = xyCoordinates[0];
    float minY = xyCoordinates[1], maxY = xyCoordinates[1];
    for (std::size_t i = 0; i < xyCoordinates.size(); i += 2) {
        minX = std::min(minX, xyCoordinates[i]);
        maxX = std::max(maxX, xyCoordinates[i]);
        minY = std::min(minY, xyCoordinates[i + 1]);
        maxY = std::max(maxY, xyCoordinates[i + 1]);
    }

    const double centerX = (minX + maxX) / 2.0;
    const double centerY = (minY + maxY) / 2.0;

    for (std::size_t i = 0; i < xyCoordinates.size(); i += 2) {
        const double originalX = xyCoordinates[i];
        const double originalY = xyCoordinates[i + 1];

        const double x = originalX - centerX;
        const double y = originalY - centerY;
        const double rotatedX = x * cosAngle - y * sinAngle;
        const double rotatedY = x * sinAngle + y * cosAngle;

        rotated[i] = static_cast<float>(rotatedX + centerX);
        rotated[i + 1] = static_cast<float>(rotatedY + centerY);
    }

    return rotated;
}
QString selectValidOption(const QStringList& options, const QString& current, int fallbackIndex) {
    if (options.contains(current)) return current;
    if (options.size() > fallbackIndex) return options[fallbackIndex];
    return options.isEmpty() ? "" : options[0];
}
StatusInfo parseStatusMessage(const QString& statusString) {
    StatusInfo info;
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(statusString.toUtf8(), &parseError);

    if (!doc.isNull() && doc.isObject()) {
        QJsonObject obj = doc.object();
        info.status = obj.value("status").toString();
        info.message = obj.value("message").toString();
        info.color = obj.value("color").toString();
    }
    else {
        auto parts = statusString.split(",");
        info.status = parts.value(0).trimmed();
        info.message = parts.value(1).trimmed();
        info.color = "#6c757d";
    }

    // Set background color based on status
    if (info.status == "C") {
        info.backgroundColor = "#28a745";
        if (info.message.isEmpty()) info.message = "Updated";
    }
    else if (info.status == "M") {
        info.backgroundColor = "#ffc107";
        if (info.message.isEmpty()) info.message = "Outdated";
    }
    else if (info.status == "E") {
        info.backgroundColor = "#dc3545";
        if (info.message.isEmpty()) info.message = "Error";
    }
    else if (info.status == "R") {
        info.backgroundColor = "#007bff";
        if (info.message.isEmpty()) info.message = "Processing";
    }
    else {
        info.backgroundColor = "#6c757d";
        if (info.message.isEmpty()) info.message = "Unknown";
    }

    info.textColor = getContrastingTextColor(info.backgroundColor);
    info.iconName = statusToIconName(info.status);

    return info;
}
bool isValidClusterDataset(const mv::Dataset<DatasetImpl>& dataset, const mv::Dataset<Points>& sibling) {
    if (!sibling.isValid()) return false;
    return (sibling->getParent() == dataset->getParent()) &&
        (dataset->getDataType() == ClusterType) &&
        (dataset->getGroupIndex() != 666 && dataset->getGroupIndex() != 667);
}

bool isValidPointDataset(const mv::Dataset<DatasetImpl>& dataset) {
    return dataset->getDataType() == PointType;
}

bool isValidEmbeddingDataset(const mv::Dataset<DatasetImpl>& dataset) {
    auto parent = dataset->getParent();
    bool checkGroup = (dataset->getGroupIndex() != 666 && dataset->getGroupIndex() != 667);
    return parent.isValid() && dataset->getDataType() == PointType && checkGroup;
}
std::vector<float> getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis) {
    std::vector<float> coords;
    int offset = (axis == "Y") ? 1 : 0;
    for (size_t i = offset; i < rotatedXYCoordinates.size(); i += 2) {
        coords.push_back(rotatedXYCoordinates[i]);
    }
    return coords;
}

std::vector<int> getSortedCellIndices(const std::vector<float>& yCoordinates) {
    std::vector<int> indices(yCoordinates.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return yCoordinates[a] < yCoordinates[b];
    });
    return indices;
}



void createLineChartClusterColors(QString coloringDataset,QString colorMapValue, std::vector<int>& selectedIndices, QString pointValuesDataset, int numOfTotalPoints, QString dimensionVal, float minValue, float maxValue)
{

    FunctionTimer timer(Q_FUNC_INFO);

    if (coloringDataset.isEmpty() || colorMapValue.isEmpty() || selectedIndices.empty() || pointValuesDataset.isEmpty() || dimensionVal.isEmpty()) {
        qWarning() << "Invalid parameters for PCA cluster colors creation.";
        return;
    }
    if (numOfTotalPoints <= 0) {
        qWarning() << "Invalid number of total points for PCA cluster colors creation.";
        return;
    }

    Dataset<Clusters> coloringDatasetFull = mv::data().getDataset<Clusters>(coloringDataset);
    Dataset<Points> pointValuesDatasetFull = mv::data().getDataset<Points>(pointValuesDataset);
    if (!coloringDatasetFull.isValid() || !pointValuesDatasetFull.isValid()) {
        qWarning() << "Invalid datasets provided for PCA cluster colors creation.";
        return;
    }

    // Clear previous clusters in the PCA coloring dataset
    coloringDatasetFull->getClusters() = QVector<Cluster>();
    //events().notifyDatasetDataChanged(coloringDatasetFull);
    QVector<Cluster> newClusters;
    newClusters.reserve(selectedIndices.size());

    std::vector<float> geneExpValues(selectedIndices.size());
    ColormapTypeValue colormap = getColorMapFromString(colorMapValue);
    std::vector<QString> getdimensionValues = pointValuesDatasetFull->getDimensionNames();
    auto it = std::find(getdimensionValues.begin(), getdimensionValues.end(), dimensionVal);
    int dimensionIndex = (it != getdimensionValues.end()) ? std::distance(getdimensionValues.begin(), it) : -1;
    pointValuesDatasetFull->extractDataForDimension(geneExpValues, dimensionIndex);

    std::map<float, std::vector<std::seed_seq::result_type>> valueToIndices;
    for (size_t i = 0; i < geneExpValues.size(); ++i) {
        valueToIndices[geneExpValues[i]].push_back(static_cast<std::seed_seq::result_type>(selectedIndices.at(i)));
    }

    int clusterIdx = 0;
    for (const auto& [value, indices] : valueToIndices) {
        QColor color = getColorFromColormap(value, colormap, minValue, maxValue);
        Cluster cluster;
        cluster.setName(QString("Cluster %1").arg(clusterIdx++));
        cluster.setColor(color);
        cluster.setIndices(indices);
        newClusters.append(cluster);
    }

    coloringDatasetFull->getClusters() = newClusters;
    events().notifyDatasetDataChanged(coloringDatasetFull);
}

void createClusterLineChartClusterColors(QString coloringChangeDataset, QString coloringReferenceDataset, std::vector<int>& selectedIndices)
{

    FunctionTimer timer(Q_FUNC_INFO);

    if (coloringChangeDataset.isEmpty() || coloringReferenceDataset.isEmpty() || selectedIndices.empty() ) {
        qWarning() << "Invalid parameters for PCA cluster colors creation.";
        return;
    }


    Dataset<Clusters> coloringChangeDatasetFull = mv::data().getDataset<Clusters>(coloringChangeDataset);
    Dataset<Clusters> coloringReferenceDatasetFull = mv::data().getDataset<Clusters>(coloringReferenceDataset);
    if (!coloringChangeDatasetFull.isValid() ||!coloringReferenceDatasetFull.isValid()) {
        qWarning() << "Invalid datasets provided for PCA cluster colors creation.";
        return;
    }

    // Clear previous clusters in the PCA coloring dataset
    coloringChangeDatasetFull->getClusters() = QVector<Cluster>();
    //events().notifyDatasetDataChanged(coloringChangeDatasetFull);

        auto clusters = coloringReferenceDatasetFull->getClusters();
        for (const auto& cluster : clusters)
        {
            std::vector<std::seed_seq::result_type> newClusterIndices;
            auto clusterIndices = cluster.getIndices();
            for (int index : clusterIndices) {
                auto it = std::find(selectedIndices.begin(), selectedIndices.end(), index);
                if (it != selectedIndices.end()) {
                    newClusterIndices.push_back(static_cast<int>(std::distance(selectedIndices.begin(), it)));
                }
            }

            if (newClusterIndices.size() > 0)
            {
                Cluster remappedCluster;
                remappedCluster.setName(cluster.getName());
                remappedCluster.setColor(cluster.getColor());
                remappedCluster.setIndices(newClusterIndices);
                coloringChangeDatasetFull->addCluster(remappedCluster);
            }


        }
        events().notifyDatasetDataChanged(coloringChangeDatasetFull);
    
}


void createScatterplotClusterColors(QString coloringDataset, QString colorMapValue, std::vector<int>& selectedIndices, QString pointValuesDataset, int numOfTotalPoints, QString dimensionVal, float minValue, float maxValue)
{

    FunctionTimer timer(Q_FUNC_INFO);

    if (coloringDataset.isEmpty() || colorMapValue.isEmpty() || selectedIndices.empty() || pointValuesDataset.isEmpty() || dimensionVal.isEmpty()) {
        qWarning() << "Invalid parameters for PCA cluster colors creation.";
        return;
    }
    if (numOfTotalPoints <= 0) {
        qWarning() << "Invalid number of total points for PCA cluster colors creation.";
        return;
    }

    Dataset<Clusters> coloringDatasetFull = mv::data().getDataset<Clusters>(coloringDataset);
    Dataset<Points> pointValuesDatasetFull = mv::data().getDataset<Points>(pointValuesDataset);
    if (!coloringDatasetFull.isValid() || !pointValuesDatasetFull.isValid()) {
        qWarning() << "Invalid datasets provided for PCA cluster colors creation.";
        return;
    }

    // Clear previous clusters in the PCA coloring dataset
    coloringDatasetFull->getClusters() = QVector<Cluster>();

    std::vector<std::seed_seq::result_type> notSelectedIndices;
    notSelectedIndices.reserve(numOfTotalPoints - selectedIndices.size());
    for (int i = 0; i < numOfTotalPoints; ++i) {
        if (std::find(selectedIndices.begin(), selectedIndices.end(), i) == selectedIndices.end()) {
            notSelectedIndices.push_back(i);
        }
    }
    QColor contrastiveColor("#D3D3D3"); 
    contrastiveColor.setAlpha(255); //0 to 255
    QVector<Cluster> newClusters;
    newClusters.reserve(selectedIndices.size() + 1);
    if (!notSelectedIndices.empty()) {
        Cluster notSelectedCluster;
        notSelectedCluster.setName("Not Selected");
        notSelectedCluster.setColor(QColor(contrastiveColor)); // Creme color
        notSelectedCluster.setIndices(notSelectedIndices);
        newClusters.append(notSelectedCluster);
    }

    std::vector<float> geneExpValues(selectedIndices.size());
    ColormapTypeValue colormap = getColorMapFromString(colorMapValue);
    std::vector<QString> getdimensionValues = pointValuesDatasetFull->getDimensionNames();
    auto it = std::find(getdimensionValues.begin(), getdimensionValues.end(), dimensionVal);
    int dimensionIndex = (it != getdimensionValues.end()) ? std::distance(getdimensionValues.begin(), it) : -1;
    pointValuesDatasetFull->extractDataForDimension(geneExpValues, dimensionIndex);

    // Group indices by unique PCA value
    std::map<float, std::vector<std::seed_seq::result_type>> valueToIndices;
    for (size_t i = 0; i < geneExpValues.size(); ++i) {
        valueToIndices[geneExpValues[i]].push_back(static_cast<std::seed_seq::result_type>(selectedIndices.at(i)));
    }

    int clusterIdx = 0;
    for (const auto& [value, indices] : valueToIndices) {
        QColor color = getColorFromColormap(value, colormap, minValue, maxValue);
        Cluster cluster;
        cluster.setName(QString("Cluster %1").arg(clusterIdx++));
        cluster.setColor(color);
        cluster.setIndices(indices);
        newClusters.append(cluster);
    }

    coloringDatasetFull->getClusters() = newClusters;
    events().notifyDatasetDataChanged(coloringDatasetFull);
}
