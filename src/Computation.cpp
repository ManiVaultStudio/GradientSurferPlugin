#include "Computation.h"
#include "ComputationUtils.h"

#include <iostream>
#include <set>
#include "GradientSurferPlugin.h"
#include "SettingsAction.h"
#include<string>  
#include <QFileDialog>
#include <QPageLayout>
#include <QWebEngineView>
#include <chrono>
#include <typeinfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QSet>
#include <QJsonValue>
#include <Eigen/Dense>
#include <QtConcurrent>
#include <QColor>
#include "util/StyledIcon.h"
#include <QLabel>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QToolTip>
#include <QElapsedTimer>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QVector>
using namespace mv;
using namespace mv::gui;


Computation::Computation(GradientSurferPlugin& GradientSurferPlugin, SettingsAction& SettingsAction)
    : WidgetAction(&GradientSurferPlugin, "Computation"),
    _viewerPlugin(GradientSurferPlugin),
    _settingsAction(SettingsAction),
    _foregroundTask(this, "Computing", mv::Task::Status::Undefined)
{
    initializeState();
    setupDebounceTimers();
    setupStatusMessageHandler();
    setupPlotSelectionOptions();    
    setupDatasetFilters();
    setupClusterUpdateHandlers();
    setupPointDatasetHandlers();
    setupGeneSelectionHandlers();
    setupEmbeddingDatasetHandlers();
    setupEmbeddingDimensionHandlers();
    setupComputeHandlers();
    setupColoringHandlers();
    setupRangeColoringHandlers();   
    setupTableHandlers();           
    setupScatterplotHandlers();
    datasetNamesCheck();
    _pca1.setCompletionCallback([this](bool success, const std::string& message) {
        _isManualComputeComputing = false;
        _isPCATriggerComputing = false;
        if (success) {
            processAfterPCAComplete();
        }
        else {
            setStatusMessage("E", "PCA Failed", message.c_str());
        }
        });
}

void Computation::setupPlotSelectionOptions()
{
    const auto plotSelectionOptions = [this]() {
        QStringList scatterplotNames = {};
        QStringList lineplotNames = {};
        QString selectedCurrentText1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        QString selectedCurrentText2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory) {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
                scatterplotNames.append(plugin->getGuiName());
            }
        }

        QString lineplotView = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();
        auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
        if (lineplotViewFactory) {
            for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory)) {
                lineplotNames.append(plugin->getGuiName());
            }
        }

        selectedCurrentText1 = selectValidOption(scatterplotNames, selectedCurrentText1, 0);
        selectedCurrentText2 = selectValidOption(scatterplotNames, selectedCurrentText2, 1);
        lineplotView = selectValidOption(lineplotNames, lineplotView, 0);

        _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().setOptions(scatterplotNames);
        _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().setCurrentText(selectedCurrentText1);
        _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().setOptions(scatterplotNames);
        _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().setCurrentText(selectedCurrentText2);
        _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().setOptions(lineplotNames);
        _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().setCurrentText(lineplotView);
        };

    connect(&mv::plugins(), &AbstractPluginManager::pluginAdded, this, plotSelectionOptions);
}

void Computation::setupRangeColoringHandlers()
{
    // Expression coloring range handlers for scatterplot 1
    connect(&_settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction(),
        &DecimalAction::valueChanged,
        this,
        [this]() { _scatterplot1ExpressionColoringRangeDebounceTimer.start(50); });

    connect(&_settingsAction.getDataset1OptionsHolder().getScatterplotUpperColoringRangeAction(),
        &DecimalAction::valueChanged,
        this,
        [this]() { _scatterplot1ExpressionColoringRangeDebounceTimer.start(50); });

    // Expression coloring range handlers for lineplot
    connect(&_settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction(),
        &DecimalAction::valueChanged,
        this,
        [this]() { _lineExpressionColoringRangeDebounceTimer.start(50); });

    connect(&_settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction(),
        &DecimalAction::valueChanged,
        this,
        [this]() { _lineExpressionColoringRangeDebounceTimer.start(50); });

    // Expression coloring range handlers for scatterplot 2
    connect(&_settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction(),
        &DecimalAction::valueChanged,
        this,
        [this]() { _scatterplot2ExpressionColoringRangeDebounceTimer.start(50); });

    connect(&_settingsAction.getDataset2OptionsHolder().getScatterplotUpperColoringRangeAction(),
        &DecimalAction::valueChanged,
        this,
        [this]() { _scatterplot2ExpressionColoringRangeDebounceTimer.start(50); });

    // Lineplot expression coloring range debounce timer handler
    connect(&_lineExpressionColoringRangeDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!_projectOpened) return;

        if (_settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction().getValue() >
            _settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction().getValue()) {
            _settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction().setValue(
                _settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction().getValue());
            return;
        }

        auto linePlotName = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();
        float minVal = _settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction().getValue();
        float maxVal = _settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction().getValue();

        if (linePlotName.isEmpty()) {
            qWarning() << "No line plot view selected";
            return;
        }
        if (minVal > maxVal) {
            _settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction().setValue(maxVal);
            return;
        }

        rangeChangeLinePlot(linePlotName, minVal, maxVal);
        });

    // Scatterplot 1 expression coloring range debounce timer handler
    connect(&_scatterplot1ExpressionColoringRangeDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!_projectOpened) return;

        if (_settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction().getValue() >
            _settingsAction.getDataset1OptionsHolder().getScatterplotUpperColoringRangeAction().getValue()) {
            _settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction().setValue(
                _settingsAction.getDataset1OptionsHolder().getScatterplotUpperColoringRangeAction().getValue());
            return;
        }

        QString scatterplot1Name = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        float minValScatter1 = _settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction().getValue();
        float maxValScatter1 = _settingsAction.getDataset1OptionsHolder().getScatterplotUpperColoringRangeAction().getValue();

        if (scatterplot1Name.isEmpty()) {
            qWarning() << "No scatterplot view 1 selected";
            return;
        }
        if (minValScatter1 > maxValScatter1) {
            _settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction().setValue(maxValScatter1);
            return;
        }

        rangeChangeScatterPlot(scatterplot1Name, minValScatter1, maxValScatter1);
        });

    // Scatterplot 2 expression coloring range debounce timer handler
    connect(&_scatterplot2ExpressionColoringRangeDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!_projectOpened) return;

        if (_settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction().getValue() >
            _settingsAction.getDataset2OptionsHolder().getScatterplotUpperColoringRangeAction().getValue()) {
            _settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction().setValue(
                _settingsAction.getDataset2OptionsHolder().getScatterplotUpperColoringRangeAction().getValue());
            return;
        }

        QString scatterplot2Name = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        float minValScatter2 = _settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction().getValue();
        float maxValScatter2 = _settingsAction.getDataset2OptionsHolder().getScatterplotUpperColoringRangeAction().getValue();

        if (scatterplot2Name.isEmpty()) {
            qWarning() << "No scatterplot view 2 selected";
            return;
        }
        if (minValScatter2 > maxValScatter2) {
            _settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction().setValue(maxValScatter2);
            return;
        }

        rangeChangeScatterPlot(scatterplot2Name, minValScatter2, maxValScatter2);
        });
}

void Computation::setupTableHandlers()
{
    // N Range handlers for table updates
    connect(&_settingsAction.getCommonOptionsHolder().getTopNRowsAction(),
        &IntegralAction::valueChanged,
        this,
        [this]() { _NRangeDebounceTimer.start(300); });

    connect(&_settingsAction.getCommonOptionsHolder().getBottomNRowsAction(),
        &IntegralAction::valueChanged,
        this,
        [this]() { _NRangeDebounceTimer.start(300); });

    connect(&_NRangeDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!_projectOpened) return;

        // Get current values
        auto topN = _settingsAction.getCommonOptionsHolder().getTopNRowsAction().getValue();
        auto bottomN = _settingsAction.getCommonOptionsHolder().getBottomNRowsAction().getValue();
        bool valuesAdjusted = false;

        // Validate the values
        if (topN == 0 && bottomN == 0) {
            // Both are 0 - set topN to default minimum (2)
            topN = 2;
            _settingsAction.getCommonOptionsHolder().getTopNRowsAction().setValue(topN);
            valuesAdjusted = true;
        }
        else if (topN == 0 && bottomN < 2) {
            // topN is 0, bottomN is less than 2 - set bottomN to 2
            bottomN = 2;
            _settingsAction.getCommonOptionsHolder().getBottomNRowsAction().setValue(bottomN);
            valuesAdjusted = true;
        }
        else if (bottomN == 0 && topN < 2) {
            // bottomN is 0, topN is less than 2 - set topN to 2
            topN = 2;
            _settingsAction.getCommonOptionsHolder().getTopNRowsAction().setValue(topN);
            valuesAdjusted = true;
        }
        else if (topN < 2 && bottomN < 2) {
            // Both are non-zero but both are less than 2 - set the larger one to 2 and the other to 0
            if (topN >= bottomN) {
                topN = 2;
                bottomN = 0;
            }
            else {
                bottomN = 2;
                topN = 0;
            }
            _settingsAction.getCommonOptionsHolder().getTopNRowsAction().setValue(topN);
            _settingsAction.getCommonOptionsHolder().getBottomNRowsAction().setValue(bottomN);
            valuesAdjusted = true;
        }

        if (valuesAdjusted) {
            return; // Skip this execution, as the values are being adjusted
        }

        // If we get here, the values are valid
        setStatusMessage("R", "Updating table", "");

        // Update table 1
        resetupCorrelationTable(_geneCorrValues0, _geneOfInterestIndices0, _settingsAction.getTableView0Action(),
            topN,
            bottomN,
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(),
            _settingsAction.getTableView2Action(), _settingsAction.getTableView1Action(),
            _settingsAction.getDataset2OptionsHolder().getGeneSelectionAction(), true);

        // Update table 2
        resetupCorrelationTable(_geneCorrValues2, _geneOfInterestIndices2, _settingsAction.getTableView2Action(),
            topN,
            bottomN,
            _settingsAction.getDataset2OptionsHolder().getGeneSelectionAction(),
            _settingsAction.getTableView0Action(), _settingsAction.getTableView1Action(),
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), false);

        // Update table 3
        resetupCorrelationTable(_geneCorrValues1, _geneOfInterestIndices1, _settingsAction.getTableView1Action(),
            topN,
            bottomN,
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(),
            _settingsAction.getTableView0Action(), _settingsAction.getTableView2Action(),
            _settingsAction.getDataset2OptionsHolder().getGeneSelectionAction(), false);

        // Select first row in table 2
        selectFirstRow(_settingsAction.getTableView2Action());
        setStatusMessage("C", "Table updated", "");

        // Auto-compute if enabled
        if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked()) {
            setStatusMessage("R", "Computation in progress", "");
            disableOptions();
            ManualCompute();
            enableOptions();
        }
        });

}

void Computation::initializeState()
{
    if (mv::projects().isOpeningProject()) {
        _projectOpened = false;
        connect(&mv::projects(), &AbstractProjectManager::projectOpened, this, [this]() {
            _projectOpened = true;
            _taskDisable = true;
            colorScatterPlots();
            clearTables();
            clearLinePlots();
            findCommonGenes();
            datasetNamesCheck();
            _taskDisable = false;
           //selectionModeAltToggleLogic(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText());
            setStatusMessage("C", "Project opened", "");
            });
    }
    qInstallMessageHandler(customMessageHandler);
}

void Computation::setupDebounceTimers()
{
    _colorUpdateDebounceTimer.setSingleShot(true);
    connect(&_colorUpdateDebounceTimer, &QTimer::timeout, this, [this]() {
        colorLinePlots();
        colorScatterPlots();
        });
    //_lineplotColorUpdateDebounceTimer.setSingleShot(true);
    //connect(&_lineplotColorUpdateDebounceTimer, &QTimer::timeout, this, [this]() {
        //colorLinePlots();
        //});
    //_scatterplotColorUpdateDebounceTimer.setSingleShot(true);
    //connect(&_scatterplotColorUpdateDebounceTimer, &QTimer::timeout, this, [this]() {
        //colorScatterPlots();
        //});

    _NRangeDebounceTimer.setSingleShot(true);
    _clusterSelectionDebounceTimer.setSingleShot(true);
    _lineExpressionColoringRangeDebounceTimer.setSingleShot(true);
    _scatterplot1ExpressionColoringRangeDebounceTimer.setSingleShot(true);
    _scatterplot2ExpressionColoringRangeDebounceTimer.setSingleShot(true);
    _datasetNamesStringCheckDebounceTimer.setSingleShot(true);
}

void Computation::setupStatusMessageHandler()
{
    const auto updateStatus = [this]() {
        auto stringFull = _settingsAction.getCommonOptionsHolder().getStatusMessageAction().getString();
        StatusInfo info = parseStatusMessage(stringFull);

        auto [font, iconSize] = getStatusLabelFontAndIconSize(info.message);

        QColor iconColor(info.textColor);
        mv::util::StyledIcon icon(info.iconName);
        icon.withColor(iconColor);

        QPixmap iconPixmap = QIcon(icon).pixmap(iconSize, iconSize);
        QLabel* statusLabel = createStatusLabel(info.message, info.backgroundColor, info.textColor, iconPixmap, font);

        StatusBarAction* statusBarAction = _settingsAction.getCommonOptionsHolder().getStatusBarAction();
        if (statusBarAction && statusBarAction->statusBar()) {
            QStatusBar* statusBar = statusBarAction->statusBar();
            QList<QWidget*> children = statusBar->findChildren<QWidget*>();
            for (QWidget* child : children) {
                statusBar->removeWidget(child);
                child->deleteLater();
            }
            statusBar->addWidget(statusLabel, 1);
        }
        };
    connect(&_settingsAction.getCommonOptionsHolder().getStatusMessageAction(), &StringAction::stringChanged, this, updateStatus);
}

void Computation::setupDatasetFilters()
{
    // Dataset 1 filters
    _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) {
            return isValidClusterDataset(dataset, _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset());
        }
    );

    _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) {
            return isValidPointDataset(dataset);
        }
    );

    // Dataset 2 filters
    _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) {
            return isValidClusterDataset(dataset, _settingsAction.getDataset2OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset());
        }
    );

    _settingsAction.getDataset2OptionsHolder().getPointDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) {
            return isValidPointDataset(dataset);
        }
    );

    // Embedding dataset filters
    _settingsAction.getDataset2OptionsHolder().getEmbeddingDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) {
            return isValidPointChildDataset(dataset);
        }
    );

    _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) {
            return isValidPointChildDataset(dataset);
        }
    );
}

void Computation::setupClusterUpdateHandlers()
{
    const auto updateClusterDataset = [this](bool isDataset1) {
        updateClusterOptions();
        setStatusMessage("M", QString("Cluster dataset %1 updated").arg(isDataset1 ? "1" : "2"), "");
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged,
        this, [updateClusterDataset]() { updateClusterDataset(true); });

    connect(&_settingsAction.getDataset2OptionsHolder().getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged,
        this, [updateClusterDataset]() { updateClusterDataset(false); });
}

void Computation::setupPointDatasetHandlers()
{
    const auto updatePointDataset = [this](bool isDataset1) {
        auto& pointDatasetAction = isDataset1 ?
            _settingsAction.getDataset1OptionsHolder().getPointDatasetAction() :
            _settingsAction.getDataset2OptionsHolder().getPointDatasetAction();

        auto& geneSelectionAction = isDataset1 ?
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction() :
            _settingsAction.getDataset2OptionsHolder().getGeneSelectionAction();

        auto& scatterplotRangeUpper = isDataset1 ?
            _settingsAction.getDataset1OptionsHolder().getScatterplotUpperColoringRangeAction() :
            _settingsAction.getDataset2OptionsHolder().getScatterplotUpperColoringRangeAction();

        auto& scatterplotRangeLower = isDataset1 ?
            _settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction() :
            _settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction();

        if (pointDatasetAction.getCurrentDataset().isValid()) {
            auto datasetID = pointDatasetAction.getCurrentDataset().getDatasetId();
            auto dataset = mv::data().getDataset<Points>(datasetID);
            if (dataset.isValid()) {
                auto limits = findDatasetLimits(datasetID);
                scatterplotRangeUpper.setMaximum(limits.second);
                scatterplotRangeLower.setMaximum(limits.second);
                scatterplotRangeUpper.setMinimum(limits.first);
                scatterplotRangeLower.setMinimum(limits.first);

                if (isDataset1) {
                    auto& lineplotRangeUpper = _settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction();
                    auto& lineplotRangeLower = _settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction();
                    lineplotRangeUpper.setMaximum(limits.second);
                    lineplotRangeUpper.setMinimum(limits.first);  // Fixed: was limits.second
                    lineplotRangeLower.setMaximum(limits.second);  // Fixed: was limits.first
                    lineplotRangeLower.setMinimum(limits.first);
                }

                findCommonGenes();
                geneSelectionAction.setPointsDataset(dataset);
                auto dimensions = dataset->getNumDimensions(); 
            }
        }
        else {
            geneSelectionAction.setPointsDataset({});
        }
        setStatusMessage("M", QString("Point dataset %1 updated").arg(isDataset1 ? "1" : "2"), "");
        };

    const auto updateCommonClusterDatasetOptionNames= [this]() {
        auto pointDataset1 = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset();
        auto pointDataset2 = _settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset();
        if (pointDataset1.isValid() && pointDataset2.isValid()) {
            auto children1 = pointDataset1->getChildren();
            auto children2 = pointDataset2->getChildren();
            if (children1.isEmpty() || children2.isEmpty()) {
                _settingsAction.getCommonOptionsHolder().getClusterDatasetSelectionAction().setOptions({});
                
                return;
            }
            QStringList commonClusterDatasetNames;
            QSet<QString> clusterNames2;
            for (const Dataset<Clusters>& child2 : children2) {
                if (child2->getDataType() == ClusterType) {
                    clusterNames2.insert(child2->getGuiName());
                }
            }
            for (const Dataset<Clusters>& child1 : children1) {
                if (child1->getDataType() == ClusterType) {
                    const QString& name = child1->getGuiName();
                    if (clusterNames2.contains(name)) {
                        commonClusterDatasetNames.append(name);
                    }
                }
            }
            _settingsAction.getCommonOptionsHolder().getClusterDatasetSelectionAction().setOptions(commonClusterDatasetNames);
            if (commonClusterDatasetNames.size()>0)
            {
                _settingsAction.getCommonOptionsHolder().getClusterDatasetSelectionAction().setCurrentIndex(0);
            }


        }
        else {
            _settingsAction.getCommonOptionsHolder().getClusterDatasetSelectionAction().setOptions({});
        }
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged,
        this, [updatePointDataset, updateCommonClusterDatasetOptionNames]() { updatePointDataset(true); updateCommonClusterDatasetOptionNames(); });

    connect(&_settingsAction.getDataset2OptionsHolder().getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged,
        this, [updatePointDataset, updateCommonClusterDatasetOptionNames]() { updatePointDataset(false); updateCommonClusterDatasetOptionNames(); });
}

void Computation::setupGeneSelectionHandlers()
{
    const auto updateGeneSelection = [this](bool isDataset1) {
        auto& geneSelectionAction = isDataset1 ?
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction() :
            _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action();

        if (geneSelectionAction.getCurrentDimensionIndex() >= 0) {
            ScopedBlocker blocker(_blockToggle);
            _settingsAction.getCommonOptionsHolder().getPcaColoringAction().setChecked(false);
            //_settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(false);
        }
        _colorUpdateDebounceTimer.start(200);
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged,
        this, [updateGeneSelection]() { updateGeneSelection(true); });

    connect(&_settingsAction.getDataset2OptionsHolder().getGeneSelection2Action(), &DimensionPickerAction::currentDimensionIndexChanged,
        this, [updateGeneSelection]() { updateGeneSelection(false); });
}


void Computation::setupEmbeddingDatasetHandlers()
{
    const auto updateEmbeddingDataset1 = [this]() {
        if (_settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().isValid()) {
            _embeddingDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());

            if (_embeddingDataset1.isValid()) {
                _filteredSelectionIndices.clear();
                _selectedClusterIndices1.clear();
                _selectedClusterIndices2.clear();
                _geneCorrValues0.clear();
                _geneCorrValues2.clear();
                _geneCorrValues1.clear();
                _geneOfInterestIndices0.clear();
                _geneOfInterestIndices2.clear();
                _geneOfInterestIndices1.clear();
                clearTables();
                clearLinePlots();
                _selectionBlocker = true;
                _selectionDataset = -1;
                _embeddingDataset1->setSelectionIndices({});
                if (_embeddingDataset2.isValid())
                {
                    _embeddingDataset2->setSelectionIndices({});
                }

                _selectionBlocker = false;
                auto parent = _embeddingDataset1->getParent();
                if (parent.isValid()) {
                    _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().setCurrentDataset(parent);
                    auto children = parent->getChildren();
                    Datasets clusterDatasets;
                    for (const Dataset<Clusters>& child : children) {
                        if (child->getDataType() == ClusterType && (child->getGroupIndex() != 667 || child->getGroupIndex() != 666)) {
                            clusterDatasets.append(child);
                        }
                    }
                    auto selectedClusterDatasetName = _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset();

                    _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setDatasets(clusterDatasets);
                    if (clusterDatasets.contains(selectedClusterDatasetName)) {
                        _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setCurrentDataset(selectedClusterDatasetName);
                    }
                    else if (clusterDatasets.size() > 0) {
                        _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setCurrentDataset(clusterDatasets.first());
                    }
                }
                else {
                    _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setDatasets({});
                }

                _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setPointsDataset(_embeddingDataset1);
                _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setPointsDataset(_embeddingDataset1);

                if (_embeddingDataset1->getNumDimensions() >= 2) {
                    _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(0);
                    _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(1);
                }
                else if (_embeddingDataset1->getNumDimensions() == 1) {
                    _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(0);
                    _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(0);
                }
                else {
                    _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(-1);
                    _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(-1);
                }

                const auto mainSelectionChanged1 = [this]() {
                    if (_blockSelectionTrigger || _selectionBlocker) return;
                    _settingsAction.getSelectionTableHeaderLabel()->setText("Common Gene Table:Selection Dataset1");
                    _selectionBlocker = true;
                    _embeddingDataset2->setSelectionIndices({});
                    qDebug() << "Main selection changed for embedding dataset 1";
                    _selectionDataset = 1;
                    _selectionBlocker = false;
                    setStatusMessage("M", "Selected indices changed", "");
                    setStatusMessage("R", "Computation in progress", "");
                    disableOptions();
                    setSelectionAndAutoCompute();
                    enableOptions();
                    };
                if (_embeddingDataset1Connection != QMetaObject::Connection{}) {
                    disconnect(_embeddingDataset1Connection);
                }
                _embeddingDataset1Connection = connect(&_embeddingDataset1, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged1);
                if (!_embeddingDataset1Connection) {
                    qWarning() << "Failed to connect to dataSelectionChanged signal of fullEmbeddingDataset";
                }
                _selectionModeEnable = true;
                //_settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(true);
                colorScatterPlots();
            }
        }
        setStatusMessage("M", "Embedding dataset 1 updated", "");
        _selectionModeEnable = false;
        };

    const auto updateEmbeddingDataset2 = [this]() {
        if (_settingsAction.getDataset2OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().isValid()) {
            _embeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());

            if (_embeddingDataset2.isValid()) {
                _filteredSelectionIndices.clear();
                _selectedClusterIndices1.clear();
                _selectedClusterIndices2.clear();
                _geneCorrValues0.clear();
                _geneCorrValues2.clear();
                _geneCorrValues1.clear();
                _geneOfInterestIndices0.clear();
                _geneOfInterestIndices2.clear();
                _geneOfInterestIndices1.clear();
                clearTables();
                clearLinePlots();
                _selectionBlocker = true;
                _selectionDataset = -1;
                if (_embeddingDataset1.isValid())
                {
                    _embeddingDataset1->setSelectionIndices({});
                }
                _embeddingDataset2->setSelectionIndices({});
                _selectionBlocker = false;
                auto parent = _embeddingDataset2->getParent();
                if (parent.isValid()) {
                    _settingsAction.getDataset2OptionsHolder().getPointDatasetAction().setCurrentDataset(parent);
                    auto children = parent->getChildren();
                    Datasets clusterDatasets;
                    for (const Dataset<Clusters>& child : children) {
                        if (child->getDataType() == ClusterType && (child->getGroupIndex() != 667 || child->getGroupIndex() != 666)) {
                            clusterDatasets.append(child);
                        }
                    }

                    auto selectedClusterDatasetName = _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().getCurrentDataset();
                    _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().setDatasets(clusterDatasets);
                    if (clusterDatasets.contains(selectedClusterDatasetName)) {
                        _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().setCurrentDataset(selectedClusterDatasetName);
                    }
                    else if (clusterDatasets.size() > 0) {
                        _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().setCurrentDataset(clusterDatasets.first());
                    }
                }
                else {
                    _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().setDatasets({});
                }

                _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionXAction().setPointsDataset(_embeddingDataset2);
                _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionYAction().setPointsDataset(_embeddingDataset2);

                if (_embeddingDataset2->getNumDimensions() >= 2) {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(0);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(1);
                }
                else if (_embeddingDataset2->getNumDimensions() == 1) {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(0);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(0);
                }
                else {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(-1);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(-1);
                }

                //_settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(true);
                //coloringScatterPlots(); //TODO Check

                const auto mainSelectionChanged2 = [this]() {
                    if (_blockSelectionTrigger || _selectionBlocker) return;
                    _settingsAction.getSelectionTableHeaderLabel()->setText("Common Gene Table:Selection Dataset2");
                    _selectionBlocker = true;
                    _embeddingDataset1->setSelectionIndices({});
                    //qDebug() << "Main selection changed for embedding dataset 2";
                    _selectionDataset = 2;
                    _selectionBlocker = false;
                    setStatusMessage("M", "Selected indices changed", "");
                    setStatusMessage("R", "Computation in progress", "");
                    disableOptions();
                    setSelectionAndAutoCompute();
                    enableOptions();
                    };
                if (_embeddingDataset2Connection != QMetaObject::Connection{}) {
                    disconnect(_embeddingDataset2Connection);
                }
                _embeddingDataset2Connection = connect(&_embeddingDataset2, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged2);
                if (!_embeddingDataset2Connection) {
                    qWarning() << "Failed to connect to dataSelectionChanged signal of fullEmbeddingDataset";
                }
                _selectionModeEnable = true;
                //_settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(true);
                colorScatterPlots();
            }
        }
        setStatusMessage("M", "Embedding dataset 2 updated", "");
        _selectionModeEnable = false;
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset1);
    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset2);
    connect(&_settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction(), &OptionAction::currentIndexChanged, this, [this]() {

        //if (!_projectOpened) return;
        if (_settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().getCurrentText() != "")
        {
            auto availableDatasets = _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getDatasets();
            if (availableDatasets.size()>0) {

                for (const Dataset<Points>& dataset : availableDatasets) {
                    QString substring = _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().getCurrentText();
                    if (dataset->getGuiName().contains(substring, Qt::CaseInsensitive))
                    {
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().setCurrentDataset(dataset);
                        break;
                    }
                }


            }


        }

        });

    connect(&_settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction(), &OptionAction::currentIndexChanged, this, [this]() {

        //if (!_projectOpened) return;
        if (_settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().getCurrentText() != "")
        {
            auto availableDatasets = _settingsAction.getDataset2OptionsHolder().getEmbeddingDatasetAction().getDatasets();
            if (availableDatasets.size() > 0) {
                for (const Dataset<Points>& dataset : availableDatasets) {
                    QString substring = _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().getCurrentText();
                    if (dataset->getGuiName().contains(substring, Qt::CaseInsensitive))
                    {
                        _settingsAction.getDataset2OptionsHolder().getEmbeddingDatasetAction().setCurrentDataset(dataset);
                        break;
                    }
                }
            }
        }

        });
    const auto optionsforembeddingdataset = [this]() {

        datasetNamesCheck();

        
        };
    connect(&mv::data(), &AbstractDataManager::datasetAdded, this, optionsforembeddingdataset);
    connect(&mv::data(), &AbstractDataManager::datasetRemoved, this, optionsforembeddingdataset);
    connect(&_settingsAction.getCommonOptionsHolder().getDefaultDatasetNamesAction(),
        &StringAction::stringChanged,
        this,
        [this]() { _datasetNamesStringCheckDebounceTimer.start(500); });

    connect(&_datasetNamesStringCheckDebounceTimer, &QTimer::timeout, this, [this]() {
        datasetNamesCheck();
        setStatusMessage("M", "Dataset names checked", "");
        });

}

void Computation::datasetNamesCheck()
{
    if (!_projectOpened) return;
    auto fullString= _settingsAction.getCommonOptionsHolder().getDefaultDatasetNamesAction().getString();//"Human; Marmoset; Macaque; smFISH; MERFISH; Embryo1; Embryo2; Embryo3"
    if (fullString == "")
    {
        _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setOptions({});
        _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setOptions({});
        _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setDisabled(true);
        _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setDisabled(true);
        return;
    }
    auto allDatasets = mv::data().getAllDatasets();
    
    //split fullString based on ";" and set into substringList
    QStringList substringList;
    for (const QString& str : fullString.split(';', Qt::SkipEmptyParts)) {
        QString trimmed = str.trimmed();
        if (!trimmed.isEmpty())
            substringList.append(trimmed);
    }
    //qDebug() << "substringList:" << substringList;

    if (substringList.isEmpty()) {
        _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setOptions({});
        _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setOptions({});
        _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setDisabled(true);
        _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setDisabled(true);
        return;
    }
    QStringList finalSubStringList;

    for (const auto& dataset : allDatasets) {
        if (dataset->getDataType() == PointType) {
            for (const auto& substring : substringList) {
                if (dataset->getGuiName().contains(substring, Qt::CaseInsensitive)) {
                    if (!finalSubStringList.contains(substring, Qt::CaseInsensitive)) {
                        finalSubStringList.append(substring);
                    }
                }
            }
        }
    }
    _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setOptions(finalSubStringList);
    _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setOptions(finalSubStringList);

    if (finalSubStringList.size() > 1)
    {
        _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setCurrentIndex(0);
        _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setCurrentIndex(1);
        _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setDisabled(false);
        _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setDisabled(false);
    }
    else
    {
        _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setCurrentIndex(-1);
        _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setCurrentIndex(-1);
        _settingsAction.getCommonOptionsHolder().getComparatorDataset1SelectorAction().setDisabled(true);
        _settingsAction.getCommonOptionsHolder().getComparatorDataset2SelectorAction().setDisabled(true);
    }

}
void Computation::setupEmbeddingDimensionHandlers()
{
    // These were already consolidated correctly, but the message strings need to match original
    const auto embeddingDimensionYActionChanged1 = [this]() {
        setStatusMessage("M", "Embedding 1 Y dimension changed", "");
        _colorUpdateDebounceTimer.start(200);
        };

    const auto embeddingDimensionYActionChanged2 = [this]() {
        setStatusMessage("M", "Embedding 2 Y dimension changed", "");
        _colorUpdateDebounceTimer.start(200);
        };

    const auto embeddingDimensionXActionChanged1 = [this]() {
        setStatusMessage("M", "Embedding 1 X dimension changed", "");
        _colorUpdateDebounceTimer.start(200);
        };

    const auto embeddingDimensionXActionChanged2 = [this]() {
        setStatusMessage("M", "Embedding 2 X dimension changed", "");
        _colorUpdateDebounceTimer.start(200);
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionYActionChanged1);
    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionYAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionYActionChanged2);
    connect(&_settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionXActionChanged1);
    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionXAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionXActionChanged2);
}

void Computation::setupComputeHandlers()
{
    // Compute Info handler
    connect(&_settingsAction.getCommonOptionsHolder().getComputeAction(), &TriggerAction::triggered, this, [this]() {
        if (!_projectOpened) return;
        setStatusMessage("R", "Correlation computation in progress", "");
        /*if (_settingsAction.getCommonOptionsHolder().getSelectionModeAction().isChecked()) {
            ScopedBlocker blocker(_blockToggle);
            _settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(false);
        }*/
        disableOptions();
        correlationAfterPCA();
        enableOptions();
        });

    // Correlation calculation type handler
    connect(&_settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction(), &OptionAction::currentIndexChanged, this, [this]() {
        if (!_projectOpened) return;
        setStatusMessage("R", "Computation in progress", "");
        disableOptions();
        setSelectionAndAutoCompute();
        enableOptions();
        });

    // PCA trigger handler
    connect(&_settingsAction.getCommonOptionsHolder().getComputePCAAction(), &TriggerAction::triggered, this, [this]() {
        if (!_projectOpened) return;
        setStatusMessage("R", "PCA Computation in progress", "");
        /*if (_settingsAction.getCommonOptionsHolder().getSelectionModeAction().isChecked()) {
            ScopedBlocker blocker(_blockToggle);
            _settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(false);
        }*/
        disableOptions();
        ManualCompute();
        enableOptions();
        });

    // Computation type changed handler
    connect(&_settingsAction.getCommonOptionsHolder().getComputationTypeAction(), &OptionAction::currentIndexChanged, this, [this]() {
        _settingsAction.getCommonOptionsHolder().getComputePCAAction().trigger();
        });

    // Cluster selection debounce handler
    connect(&_settingsAction.getCommonOptionsHolder().getClusterValueSelectionAction(),
        &OptionsAction::selectedOptionsChanged,
        this,
        [this]() { _clusterSelectionDebounceTimer.start(500); });

    connect(&_clusterSelectionDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!_projectOpened) return;
        setStatusMessage("R", "Computation in progress", "");
        disableOptions();
        setSelectionAndAutoCompute();
        enableOptions();
        });

    connect(&_settingsAction.getCommonOptionsHolder().getClusterDatasetSelectionAction(), &OptionAction::currentIndexChanged, this, [this]() {
        
        //if (!_projectOpened) return;
        if (_settingsAction.getCommonOptionsHolder().getClusterDatasetSelectionAction().getCurrentText()!="")
        {
            _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setCurrentText(_settingsAction.getCommonOptionsHolder().getClusterDatasetSelectionAction().getCurrentText());
            _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().setCurrentText(_settingsAction.getCommonOptionsHolder().getClusterDatasetSelectionAction().getCurrentText());
        }

        });
}

void Computation::setupColoringHandlers()
{
    // PCA coloring handler
    connect(&_settingsAction.getCommonOptionsHolder().getPcaColoringAction(), &ToggleAction::toggled, this, [this]() {
        if (_blockToggle) return;

        if (_settingsAction.getCommonOptionsHolder().getPcaColoringAction().isChecked()) {
            ScopedBlocker blocker(_blockToggle);
            //_settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(false);
            //_settingsAction.getCommonOptionsHolder().getColorAllCellsWithExpressionAction().setChecked(false);
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);
            deselectAllRows(_settingsAction.getTableView0Action());
            deselectAllRows(_settingsAction.getTableView2Action());
            deselectAllRows(_settingsAction.getTableView1Action());
            setStatusMessage("M", "PCA selection changed. Click on \"Compute\" to update tables", "");
        }
        else 
        {
            ScopedBlocker blocker(_blockToggle);
            //_settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(true);
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);
            if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked())
            {
                _settingsAction.getCommonOptionsHolder().getComputeAction().trigger();
            }
        }
        _colorUpdateDebounceTimer.start(200);
        });

    // Colormap change handler
    connect(&_settingsAction.getCommonOptionsHolder().getColoringMapSelectionAction(), &ColorMap1DAction::imageChanged, this, [this]() {
        QString scatterplot1Name = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        QString scatterplot2Name = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        auto colorMapStringValue = _settingsAction.getCommonOptionsHolder().getColoringMapSelectionAction().getColorMap();
        auto linePlotName = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();

        if (colorMapStringValue.isEmpty()) {
            qWarning() << "No colormap selected";
            return;
        }

        if (!scatterplot1Name.isEmpty()) {
            colormapChangeScatterplot(scatterplot1Name, colorMapStringValue);
        }
        if (!scatterplot2Name.isEmpty()) {
            colormapChangeScatterplot(scatterplot2Name, colorMapStringValue);
        }
        if (!linePlotName.isEmpty()) {
            colormapChangeLinePlot(linePlotName, colorMapStringValue);
        }
        });

    // Percentile value change handlers
    auto changePercentileValue = [this]() { _colorUpdateDebounceTimer.start(200); };
    connect(&_settingsAction.getCommonOptionsHolder().getLowRangeColoringPercentileAction(), &DecimalAction::valueChanged, this, changePercentileValue);
    connect(&_settingsAction.getCommonOptionsHolder().getHighRangeColoringPercentileAction(), &DecimalAction::valueChanged, this, changePercentileValue);

    // Selection mode toggle handler
   /* connect(&_settingsAction.getCommonOptionsHolder().getSelectionModeAction(), &ToggleAction::toggled, this, [this]() {
        if (_blockToggle) return;

        if (_settingsAction.getCommonOptionsHolder().getSelectionModeAction().isChecked()) {
            ScopedBlocker blocker(_blockToggle);
            _settingsAction.getCommonOptionsHolder().getPcaColoringAction().setChecked(false);
            //_settingsAction.getCommonOptionsHolder().getColorAllCellsWithExpressionAction().setChecked(false);
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);
            deselectAllRows(_settingsAction.getTableView0Action());
            deselectAllRows(_settingsAction.getTableView2Action());
            deselectAllRows(_settingsAction.getTableView1Action());
        }
        else {
            ScopedBlocker blocker(_blockToggle);
            _settingsAction.getCommonOptionsHolder().getPcaColoringAction().setChecked(false);
            selectFirstRow(_settingsAction.getTableView0Action());
        }
        _colorUpdateDebounceTimer.start(200);
        });
    */
    // Color all cells with expression handler
    /*connect(&_settingsAction.getCommonOptionsHolder().getColorAllCellsWithExpressionAction(), &ToggleAction::toggled, this, [this]() {
        if (_blockToggle) return;
        _colorUpdateDebounceTimer.start(200);
        });*/

    // PCA dimension for coloring handler
    connect(&_settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, [this]() {
        if (_blockToggle) return;

        if(_settingsAction.getCommonOptionsHolder().getPcaColoringAction().isChecked()){
            _colorUpdateDebounceTimer.start(200);
        }
        else
        {
            if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked())
                {
                    _settingsAction.getCommonOptionsHolder().getComputeAction().trigger();
                }
        }
        });
}

void Computation::setupScatterplotHandlers()
{
    // Show point selection changed handler
    connect(&_settingsAction.getCommonOptionsHolder().getHidePointSelectionOutlineAction(), &ToggleAction::toggled, this, [this]() {
        QString scatterplot1Name = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        QString scatterplot2Name = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();

        if (scatterplot1Name.isEmpty()) {
            qWarning() << "No scatterplot view 1 selected";
        }
        if (scatterplot2Name.isEmpty()) {
            qWarning() << "No scatterplot view 2 selected";
        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory) {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
                if (plugin->getGuiName() == scatterplot1Name || plugin->getGuiName() == scatterplot2Name) {
                    auto pointSelectionScaleAction = findActionByPath<DecimalAction>(plugin, "Settings/Selection/Scale");
                    auto pointSelectionOpacityAction = findActionByPath<DecimalAction>(plugin, "Settings/Selection/Opacity");

                    if (pointSelectionScaleAction) {
                        if (_settingsAction.getCommonOptionsHolder().getHidePointSelectionOutlineAction().isChecked()) {
                            pointSelectionScaleAction->setValue(100.0);
                        }
                        else {
                            pointSelectionScaleAction->setValue(160.0);
                        }
                    }
                    else {
                        qWarning() << "Point selection scale action not found";
                    }

                    if (pointSelectionOpacityAction) {
                        if (_settingsAction.getCommonOptionsHolder().getHidePointSelectionOutlineAction().isChecked()) {
                            pointSelectionOpacityAction->setValue(0.0);
                        }
                        else {
                            pointSelectionOpacityAction->setValue(100.0);
                        }
                    }
                    else {
                        qWarning() << "Point selection opacity action not found";
                    }
                }
            }
        }
        });

    // Focus selection changed handler
    connect(&_settingsAction.getCommonOptionsHolder().getFocusSelectionAction(), &ToggleAction::toggled, this, [this]() {
        QString scatterplot1Name = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        QString scatterplot2Name = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();

        if (scatterplot1Name.isEmpty()) {
            qWarning() << "No scatterplot view 1 selected";
        }
        if (scatterplot2Name.isEmpty()) {
            qWarning() << "No scatterplot view 2 selected";
        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory) {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
                if (plugin->getGuiName() == scatterplot1Name || plugin->getGuiName() == scatterplot2Name) {
                    auto focusAction = findActionByPath<ToggleAction>(plugin, "Settings/Plot/Point/Focus selection");
                    if (focusAction) {
                        focusAction->setChecked(_settingsAction.getCommonOptionsHolder().getFocusSelectionAction().isChecked());
                    }
                    else {
                        qWarning() << "Focus selection action not found";
                    }
                }
            }
        }
        });

    // Scatterplot view selection changed handlers with project opened check
    const auto scatterplotviewSelectionChanged1 = [this]() {
        if (!_projectOpened) {
            return;
        }
        setStatusMessage("M", "Scatterplotview selection 1 updated", "");

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName1.isEmpty()) {
            qWarning() << "No scatterplot view selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory) {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
                if (plugin->getGuiName() == scatterplotName1) {
                    mv::gui::ToggleAction* notifyDuringSelectionAction = findActionByPath<ToggleAction>(plugin, "Settings/Selection/Point Selection/Notify during selection");
                    mv::gui::ToggleAction* haloAction = findActionByPath<ToggleAction>(plugin, "Settings/Selection/Halo");

                    if (notifyDuringSelectionAction) {
                        notifyDuringSelectionAction->setChecked(false);
                    }
                    else {
                        qWarning() << "Notify during selection action not found.";
                    }
                    if (haloAction) {
                        haloAction->setChecked(true);
                    }
                    else {
                        qWarning() << "Halo action not found.";
                    }

                    auto pointSelectionScaleAction = findActionByPath<DecimalAction>(plugin, "Settings/Selection/Scale");
                    auto pointSelectionOpacityAction = findActionByPath<DecimalAction>(plugin, "Settings/Selection/Opacity");
                    if (pointSelectionScaleAction) {
                        pointSelectionScaleAction->setValue(160.0);
                    }
                    else {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction) {
                        pointSelectionOpacityAction->setValue(100.0);
                    }
                    else {
                        qWarning() << "Point selection opacity action not found";
                    }
                }
            }
        }
        };

    const auto scatterplotviewSelectionChanged2 = [this]() {
        setStatusMessage("M", "Scatterplotview selection 2 updated", "");

        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName2.isEmpty()) {
            qWarning() << "No scatterplot view 2 selected";
            return;
        }
        //selectionModeAltToggleLogic(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText());
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory) {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
                if (plugin->getGuiName() == scatterplotName2) {
                    mv::gui::ToggleAction* notifyDuringSelectionAction = findActionByPath<ToggleAction>(plugin, "Settings/Selection/Point Selection/Notify during selection");
                    mv::gui::ToggleAction* haloAction = findActionByPath<ToggleAction>(plugin, "Settings/Selection/Halo");

                    if (notifyDuringSelectionAction) {
                        notifyDuringSelectionAction->setChecked(false);
                    }
                    else {
                        qWarning() << "Notify during selection action not found.";
                    }
                    if (haloAction) {
                        haloAction->setChecked(true);
                    }
                    else {
                        qWarning() << "Halo action not found.";
                    }

                    auto pointSelectionScaleAction = findActionByPath<DecimalAction>(plugin, "Settings/Selection/Scale");
                    auto pointSelectionOpacityAction = findActionByPath<DecimalAction>(plugin, "Settings/Selection/Opacity");

                    if (pointSelectionScaleAction) {
                        pointSelectionScaleAction->setValue(160.0);
                    }
                    else {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction) {
                        pointSelectionOpacityAction->setValue(100.0);
                    }
                    else {
                        qWarning() << "Point selection opacity action not found";
                    }
                }
            }
        }
        };

    const auto lineplotviewSelectionChanged1 = [this]() {
        setStatusMessage("M", "Lineplotview selection 1 updated", "");
        //TODO
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction(), &OptionAction::currentIndexChanged, this, scatterplotviewSelectionChanged1);
    connect(&_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction(), &OptionAction::currentIndexChanged, this, scatterplotviewSelectionChanged2);
    connect(&_settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction(), &OptionAction::currentIndexChanged, this, lineplotviewSelectionChanged1);
}
void Computation::clearLinePlots()
{

    if (_lineChartPointDataset1.isValid())
    {
        QString linePlotName = "";
        QString positionDatasetID = "";
        QString xPositionString = "";
        QString yPositionString = "";
        QString colorDatasetID = "";
        QString colordatasetType = "";
        int colorDatasetDimensionIndx = -1;
        QString titleText = "";
        QString smoothingType = "";
        int smoothingWindow = -1;
        QString colorMapStringValue = "";

        linePlotName = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();
        float minVal = _settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction().getValue();
        float maxVal = _settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction().getValue();
        lineplotModeChange(linePlotName, positionDatasetID, colorDatasetID, colordatasetType, colorDatasetDimensionIndx, xPositionString, yPositionString, titleText, smoothingType, smoothingWindow, colorMapStringValue, minVal, maxVal);

        events().notifyDatasetDataChanged(_lineChartPointDataset1);

    }
}

void Computation::clearTables()
{
    clearTable(_settingsAction.getTableView1Action());
    clearTable(_settingsAction.getTableView2Action());
    clearTable(_settingsAction.getTableView0Action());
}


void Computation::AutoCompute()
{
    if (_isAutoComputeComputing) return;
    _isAutoComputeComputing = true;
    auto guard = qScopeGuard([&] { _isAutoComputeComputing = false; });

    try
    {
        // Reset state
        clearTables();
        clearLinePlots();
        _numOfPointsInLine1 = 0;

        // Ensure normalized datasets are available
        /*if (!_normalizedFullDataset1.isValid() || !_normalizedFullDataset2.isValid()) {
            normalizeData();
            setSelectionAndAutoCompute();
            return;
        }*/
        if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()) {
            setStatusMessage("E", "No valid Point datasets", "");
            return;
        }
        auto pointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
        auto pointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
        if (!pointDataset1.isValid() || !pointDataset2.isValid()) {
            setStatusMessage("E", "No valid Point datasets", "");

            return;
        }
        // Retrieve and validate required datasets and parameters
        mv::Dataset<Clusters> fullClusterDataset1, fullClusterDataset2;
        int xEmbeddingIndex = -1, yEmbeddingIndex = -1, topNRows = -1, bottomNRows = -1;
        std::vector<int> selectedIndices;
        QString scatterplotViewName;
        if (!validateAndRetrieveDatasets(fullClusterDataset1, fullClusterDataset2,
            xEmbeddingIndex, yEmbeddingIndex,
            selectedIndices, scatterplotViewName,
            topNRows, bottomNRows))
            return;

        // Ensure all required datasets exist
        if (!createDatasetsIfNeeded())
            return;

        // Gather cluster and selection information
        const auto selectedClusters = _settingsAction.getCommonOptionsHolder().getClusterValueSelectionAction().getSelectedOptions();
        std::vector < std::seed_seq::result_type> selectIndices;
        if (_selectionDataset==1)
        {
            selectIndices = _embeddingDataset1->getSelectionIndices();
        }
        else if (_selectionDataset == 2)
        {
            selectIndices = _embeddingDataset2->getSelectionIndices();
        }
        else
        {
            setStatusMessage("E", "Invalid selection dataset", "");
            return;

        }

        
        const auto clusters1 = fullClusterDataset1->getClusters();
        const auto clusters2 = fullClusterDataset2->getClusters();

        if (selectedClusters.isEmpty() || clusters1.isEmpty() || clusters2.isEmpty()) {
            setStatusMessage("E", "No clusters selected for correlation computation", "");
            return;
        }

        if (selectIndices.empty()) {
            setStatusMessage("E", "No points selected for correlation computation", "");
            return;
        }
        std::vector < std::seed_seq::result_type> filtIndxs;
        // Filter selection indices by clusters
        if (_selectionDataset == 1)
        {
            filtIndxs = filterSelectionIndicesByClusters(selectIndices, clusters1, selectedClusters);
        }
        else if (_selectionDataset == 2)
        {
            filtIndxs = filterSelectionIndicesByClusters(selectIndices, clusters2, selectedClusters);
        }
        else
        {
            setStatusMessage("E", "Invalid selection dataset", "");
            return;

        }
        
        
        if (filtIndxs.empty()) {
            setStatusMessage("E", "No points selected for correlation computation after filtering by clusters", "");
            return;
        }
        if (filtIndxs.size() < 2) {
            setStatusMessage("E", "Not enough points selected for correlation computation after filtering by clusters", "");
            return;
        }

        // Update selected cluster indices
        _selectedClusterIndices1 = getIndicesBySelectedClusters(clusters1, selectedClusters);
        _selectedClusterIndices2 = getIndicesBySelectedClusters(clusters2, selectedClusters);
        std::sort(_selectedClusterIndices1.begin(), _selectedClusterIndices1.end());
        std::sort(_selectedClusterIndices2.begin(), _selectedClusterIndices2.end());

        // Update filtered selection indices and notify
        _filteredSelectionIndices.assign(filtIndxs.begin(), filtIndxs.end());
        
        //_settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(false);
        _blockSelectionTrigger = true;
        if (_selectionDataset == 1)
        {
            _embeddingDataset1->setSelectionIndices(filtIndxs);
            mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset1);
        }
        else if (_selectionDataset == 2)
        {
            _embeddingDataset2->setSelectionIndices(filtIndxs);
            mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset2);
        }
        else
        {
            setStatusMessage("E", "Invalid selection dataset", "");
            return;

        }
        

        if (_filteredSelectionIndices.size() < 2) {
            setStatusMessage("E", "Not enough points selected for correlation computation", "");
            return;
        }

        // Get rotation angle for embedding
        float rotationAngle = 0.0f;
        if (!getRotationAngle(scatterplotViewName, rotationAngle))
            return;

        // Populate embedding coordinates and rotate
        std::vector<float> XYCoordinates;
        if (!populateEmbeddingCoordinates(xEmbeddingIndex, yEmbeddingIndex, XYCoordinates)) {
            setStatusMessage("E", "Error occurred during embedding coordinates population", "");
            return;
        }

        std::vector<float> rotatedXYCordinates;
        try {
            rotatedXYCordinates = rotateAxis(XYCoordinates, rotationAngle);
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in rotateAxis:" << ex.what();
            setStatusMessage("E", "Error occurred during point rotation", "");
            return;
        }

        // Extract Y coordinates and sort cell indices
        std::vector<float> yCoordinates;
        try {
            yCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "Y");
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in getCoordinatesOfCells:" << ex.what();
            setStatusMessage("E", "Error occurred during fetching coordinates", "");
            return;
        }
        /*
        std::vector<int> sortedCellIndices;
        try {
            sortedCellIndices = getSortedCellIndices(yCoordinates);
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in getSortedCellIndices:" << ex.what();
            setStatusMessage("E", "Error occurred during sorting indices", "");
            return;
        }
        */
        // Prepare for dataset population
        int noOfDimensions = pointDataset1->getNumDimensions();
        const auto& dimensionNames = pointDataset1->getDimensionNames();
        std::vector<int> columnIndices(noOfDimensions);
        std::iota(columnIndices.begin(), columnIndices.end(), 0);


        if (_selectionDataset == 1)
        {
            if (!populateAndUpdateDatasets(pointDataset1, rotatedXYCordinates, noOfDimensions, columnIndices, dimensionNames)) {
                setStatusMessage("E", "Error occurred during dataset population", "");
                return;
            }
        }
        else if (_selectionDataset == 2)
        {
            if (!populateAndUpdateDatasets(pointDataset2, rotatedXYCordinates, noOfDimensions, columnIndices, dimensionNames)) {
                setStatusMessage("E", "Error occurred during dataset population", "");
                return;
            }
        }
        else
        {
            setStatusMessage("E", "Invalid selection dataset", "");
            return;

        }
        // Compute correlation gene coordinates
        std::vector<float> corrGeneCoords;
        const QString corrType = _settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction().getCurrentText();
        
        if (_selectionDataset == 1)
        {
            computeCorrelationGeneCoords(_filteredSelectionIndices, pointDataset1, yCoordinates, corrGeneCoords, corrType);
        }
        else if (_selectionDataset == 2)
        {
            computeCorrelationGeneCoords(_filteredSelectionIndices, pointDataset2, yCoordinates, corrGeneCoords, corrType);
        }
        else
        {
            setStatusMessage("E", "Invalid selection dataset", "");
            return;

        }
        try {
            populateGenesByCorrelationRange(corrGeneCoords, topNRows, bottomNRows, dimensionNames, _geneCorrValues0, _geneOfInterestIndices0, true);
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            setStatusMessage("E", "Error occurred during filtering genes", "");
            return;
        }      

        _geneCorrValues0.erase(
            std::remove_if(
                _geneCorrValues0.begin(),
                _geneCorrValues0.end(),
                [this](const auto& entry) {
                    // Remove if NOT in _commonGenesAcrossDatasets
                    return !_commonGenesAcrossDatasets.contains(entry.geneName, Qt::CaseInsensitive);
                }
            ),
            _geneCorrValues0.end()
        );

        // Setup table and trigger UI updates

        if (_selectionDataset == 1)
        {
            setupTableModelAndSelection(
                _settingsAction.getTableView0Action(),
                _geneCorrValues0,
                _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(),
                _settingsAction.getTableView2Action(),
                _settingsAction.getTableView1Action(),
                _settingsAction.getDataset2OptionsHolder().getGeneSelectionAction(),
                true
            );
        }
        else if (_selectionDataset == 2)
        {
            setupTableModelAndSelection(
                _settingsAction.getTableView0Action(),
                _geneCorrValues0,
                _settingsAction.getDataset2OptionsHolder().getGeneSelectionAction(),
                _settingsAction.getTableView2Action(),
                _settingsAction.getTableView1Action(),
                _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(),
                true
            );
        }
        else
        {
            setStatusMessage("E", "Invalid selection dataset", "");
            return;

        }
        selectFirstRow(_settingsAction.getTableView0Action());
        _colorUpdateDebounceTimer.start(200);

        setStatusMessage("C", "Computation completed", "#28a745");

        {
            ScopedBlocker blocker(_blockToggle);
            //_settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(false);
            _settingsAction.getCommonOptionsHolder().getPcaColoringAction().setChecked(false);
        }

        if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked()) {
            setStatusMessage("R", "Computation in progress", "");
            ManualCompute();
        }

        _foregroundTask.setProgress(1.0f, "This is a test!");
        _foregroundTask.setFinished();
    }
    catch (const std::exception& ex)
    {
        qWarning() << "Exception in Computation:" << ex.what();
        setStatusMessage("E", "Error occurred during computation: " + QString::fromStdString(ex.what()), "");
    }
    catch (...)
    {
        qWarning() << "Unknown exception in Computation";
        setStatusMessage("E", "Unknown error occurred during computation", "");
    }
    _isAutoComputeComputing = false;
}

void Computation::ManualCompute()
{
    if (_isManualComputeComputing) return;
    _isManualComputeComputing = true;
    _pcaDataset1Complete = false;
    _pcaDataset2Complete = false;

    if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()) {
        setStatusMessage("E", "No valid Point datasets", "");
        setSelectionAndAutoCompute();
        _isManualComputeComputing = false;
        return;
    }
    
    bool complete = setupAndTriggerPCA();
    if (!complete)
    {
        clearTable(_settingsAction.getTableView1Action());
        clearTable(_settingsAction.getTableView2Action());
        //setStatusMessage("E", "Error occurred during PCA setup", "");
        qCritical() << "Error occurred during PCA setup";
        _isManualComputeComputing = false;
        return;
    }
    
}

void Computation::setStatusMessage(const QString& status, const QString& message, const QString& color) {
    QString resolvedColor = color;
    if (resolvedColor.isEmpty()) {
        if (status == "C") resolvedColor = "#28a745";    
        else if (status == "M") resolvedColor = "#ffc107"; 
        else if (status == "E") resolvedColor = "#dc3545"; 
        else if (status == "R") resolvedColor = "#007bff"; 
        else resolvedColor = "#6c757d";                    
    }
    int brCount;
    QString wrappedMessage = wrapMessageWithBrCount(message, brCount,50);
    _settingsAction.getCommonOptionsHolder().getStatusBarAction()->statusBar()->setFixedHeight(30 + brCount * 20); // Adjust height based on number of <br> tags
    QJsonObject obj;
    obj["status"] = status;
    obj["message"] = wrappedMessage;
    obj["color"] = resolvedColor;

    if (status == "E")
    {
        enableOptions();
    }


    auto* statusHolder = &_settingsAction.getCommonOptionsHolder();
    if (!statusHolder) {
        qWarning() << "StatusHolder is null in setStatusMessage";
        return;
    }
    auto* statusMessageAction = &(statusHolder->getStatusMessageAction());
    if (!statusMessageAction) {
        qWarning() << "StatusMessageAction is null in setStatusMessage";
        return;
    }

    try {
        statusMessageAction->setString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    } catch (...) {
        qWarning() << "Exception thrown in setStatusMessage when calling setString";
    }
}


bool Computation::validateAndRetrieveDatasets(mv::Dataset<Clusters>& fullClusterDataset1, mv::Dataset<Clusters>& fullClusterDataset2, int& xEmbeddingIndex, int& yEmbeddingIndex, std::vector<int>& selectedIndices, QString& scatterplotViewName,  int& topNRows,int& bottomNRows )
{

    fullClusterDataset1 = mv::data().getDataset<Clusters>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
    fullClusterDataset2 = mv::data().getDataset<Clusters>(_settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());

    if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()) {
        setStatusMessage("E", "No valid Point datasets", "");

        return false;
    }
    auto pointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto pointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());


    if (!_embeddingDataset1.isValid() || !pointDataset1.isValid() || !pointDataset2.isValid() || !fullClusterDataset1.isValid() || !fullClusterDataset2.isValid()) {
        qWarning() << "Invalid datasets";
        setStatusMessage("E", "Error occured during dataset creation", "");
        return false;
    }

    xEmbeddingIndex = _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionIndex();
    yEmbeddingIndex = _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionIndex();
    if (xEmbeddingIndex == -1 || yEmbeddingIndex == -1) {
        qWarning() << "No embedding dimensions selected";
        setStatusMessage("E", "No embedding dataset dimensions selected", "");
        return false;
    }


    {
        std::vector < std::seed_seq::result_type> indices;
        if (_selectionDataset == 1)
        {
            indices = _embeddingDataset1->getSelectionIndices();
        }
        else if (_selectionDataset == 2)
        {
            indices = _embeddingDataset2->getSelectionIndices();
        }
        else
        {
            setStatusMessage("E", "Invalid selection dataset", "");
            return false;

        }
       
        selectedIndices.assign(indices.begin(), indices.end());
    }
    if (selectedIndices.empty()) {
        qWarning() << "No selected indices";
        setStatusMessage("E", "No indices selected", "");
        return false;
    }

    auto selectedClusterValues = _settingsAction.getCommonOptionsHolder().getClusterValueSelectionAction().getSelectedOptions();
    if (selectedClusterValues.isEmpty()) {
        qWarning() << "No selected cluster values";
        setStatusMessage("E", "No clusters selected", "");
        _blockSelectionTrigger = true;
        
        if (_selectionDataset == 1)
        {
            _embeddingDataset1->setSelectionIndices({});
            mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset1);
        }
        else if (_selectionDataset == 2)
        {
            _embeddingDataset2->setSelectionIndices({});
            mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset2);
        }
        else
        {
            setStatusMessage("E", "Invalid selection dataset", "");
            return false;

        }
        
        return false;
    }

    scatterplotViewName = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
    if (scatterplotViewName.isEmpty()) {
        qWarning() << "No scatterplot view selected";
        setStatusMessage("E", "No scatterplot view selected", "");
        return false;
    }

    bottomNRows = _settingsAction.getCommonOptionsHolder().getBottomNRowsAction().getValue();
      topNRows = _settingsAction.getCommonOptionsHolder().getTopNRowsAction().getValue();
    if (bottomNRows <0 || topNRows <0) {
        qWarning() << "Bottom or top N rows are negative";
        setStatusMessage("E", "Bottom or top N rows are negative", "");
        return false;
    }
    if (bottomNRows < 1 && topNRows < 1)
    {
        qWarning() << "Both bottom and top N rows are set to 0";
        setStatusMessage("E", "Both bottom and top N rows are set to 0", "");
        return false;
    }
    //qWarning() << "Selected indices size:" << selectedIndices.size() << "xEmbeddingIndex:" << xEmbeddingIndex << "yEmbeddingIndex:" << yEmbeddingIndex << "scatterplotViewName:" << scatterplotViewName << "bottomNRows:" << bottomNRows << "topNRows:" << topNRows;


    return true;
}


bool Computation::createDatasetsIfNeeded()
{
    if (!_selectedGenePointDataset1.isValid()) {
        _selectedGenePointDataset1 = mv::data().createDataset("Points", "Selected Gene Point Data 1");
        _selectedGenePointDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedGenePointDataset1);
    }

    if (!_selectedGenePointDataset2.isValid()) {
        _selectedGenePointDataset2 = mv::data().createDataset("Points", "Selected Gene Point Data 2");
        _selectedGenePointDataset2->setGroupIndex(668);
        mv::events().notifyDatasetAdded(_selectedGenePointDataset2);
    }

    /*if (!_selectedEmbeddingPointDataset1.isValid()) {
        _selectedEmbeddingPointDataset1 = mv::data().createDataset("Points", "Selected Embedding Point Data 1", _selectedGenePointDataset1);
        _selectedEmbeddingPointDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedEmbeddingPointDataset1);
    }

    if (!_selectedEmbeddingPointDataset2.isValid()) {
        _selectedEmbeddingPointDataset2 = mv::data().createDataset("Points", "Selected Embedding Point Data 2", _selectedGenePointDataset2);
        _selectedEmbeddingPointDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedEmbeddingPointDataset2);
    }*/
    
    if (!_selectedCellPointDataset.isValid()) {
        _selectedCellPointDataset = mv::data().createDataset("Points", "Selected Cell Point Data");
        _selectedCellPointDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_selectedCellPointDataset);
    }
    if (!_rotatedSelectedEmbeddingDataset.isValid()) {
        _rotatedSelectedEmbeddingDataset = mv::data().createDataset("Points", "Selected Rotated Embedding Data", _selectedCellPointDataset);
        _rotatedSelectedEmbeddingDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_rotatedSelectedEmbeddingDataset);
    }
    if (!_lineChartPointDataset1.isValid()) {
        _lineChartPointDataset1 = mv::data().createDataset("Points", "Line Chart Point Data 1", _selectedCellPointDataset);
        _lineChartPointDataset1->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_lineChartPointDataset1);
    }
    if(!_lineChartClusterColoringDataset1.isValid()) {
        _lineChartClusterColoringDataset1 = mv::data().createDataset("Cluster", "Line Chart Cluster Color Dataset 1", _lineChartPointDataset1);
        _lineChartClusterColoringDataset1->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_lineChartClusterColoringDataset1);
    }
    /*
    if (!_lineChartPointColoringDataset1.isValid()) {
        _lineChartPointColoringDataset1 = mv::data().createDataset("Cluster", "Line Chart Color Dataset 1", _lineChartPointDataset1);
        _lineChartPointColoringDataset1->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_lineChartPointColoringDataset1);
    }
    */
    //from here
    if (!_scatterplotPointColoringDataset1.isValid()) {
        _scatterplotPointColoringDataset1 = mv::data().createDataset("Points", "Scatterplot Point Color Dataset 1");
        _scatterplotPointColoringDataset1->setGroupIndex(555);
        mv::events().notifyDatasetAdded(_scatterplotPointColoringDataset1);
    }
    if (!_scatterplotPointColoringDataset2.isValid()) {
        _scatterplotPointColoringDataset2 = mv::data().createDataset("Points", "Scatterplot Point Color Dataset 2");
        _scatterplotPointColoringDataset2->setGroupIndex(556);
        mv::events().notifyDatasetAdded(_scatterplotPointColoringDataset2);
    }
    if (!_scatterplotClusterColoringDataset1.isValid()) {
        _scatterplotClusterColoringDataset1 = mv::data().createDataset("Cluster", "Scatterplot Cluster Color Dataset 1", _scatterplotPointColoringDataset1);
        _scatterplotClusterColoringDataset1->setGroupIndex(555);
        mv::events().notifyDatasetAdded(_scatterplotClusterColoringDataset1);
    }
    if (!_scatterplotClusterColoringDataset2.isValid()) {
        _scatterplotClusterColoringDataset2 = mv::data().createDataset("Cluster", "Scatterplot Cluster Color Dataset 2", _scatterplotPointColoringDataset2);
        _scatterplotClusterColoringDataset2->setGroupIndex(556);
        mv::events().notifyDatasetAdded(_scatterplotClusterColoringDataset2);
    }

    if (!_pcaSelectedEmbeddingDataset.isValid()) {
        _pcaSelectedEmbeddingDataset = mv::data().createDerivedDataset("PCA Dataset Selected Cells", _selectedCellPointDataset);
        _pcaSelectedEmbeddingDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_pcaSelectedEmbeddingDataset);
    }
    if (!_pcaCommonGenesDataset1.isValid()) {
        _pcaCommonGenesDataset1 = mv::data().createDerivedDataset("PCA Dataset 1", _selectedGenePointDataset1);
        _pcaCommonGenesDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset1);
        if (_selectedGenePointDataset1.isValid() && _pcaCommonGenesDataset1.isValid()) {
            _pcaAnalysisCommonGenesDataset1Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset1 }, { _pcaCommonGenesDataset1 });
        }
        else
        {
            qWarning() << "Selected gene point dataset 1 or PCA common genes dataset 1 is invalid";
            setStatusMessage("E", "Selected gene point dataset 1 or PCA common genes dataset 1 is invalid", "");
            return false;
        }


    }
    if (!_pcaCommonGenesDataset2.isValid()) {
        _pcaCommonGenesDataset2 = mv::data().createDerivedDataset("PCA Dataset 2", _selectedGenePointDataset2);
        _pcaCommonGenesDataset2->setGroupIndex(668);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset2);
        if (_selectedGenePointDataset2.isValid() && _pcaCommonGenesDataset2.isValid()) {
            _pcaAnalysisCommonGenesDataset2Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset2 }, { _pcaCommonGenesDataset2 });
        }
        else
        {
            qWarning() << "Selected gene point dataset 2 or PCA common genes dataset 2 is invalid";
            setStatusMessage("E", "Selected gene point dataset 2 or PCA common genes dataset 2 is invalid", "");
            return false;
        }


    }

    if (!_rotatedSelectedEmbeddingDataset.isValid() || !_selectedGenePointDataset1.isValid() || !_lineChartPointDataset1.isValid()||!_lineChartClusterColoringDataset1.isValid() || !_selectedGenePointDataset2.isValid() || !_selectedCellPointDataset.isValid() || !_scatterplotClusterColoringDataset1.isValid() || !_scatterplotClusterColoringDataset2.isValid() || !_scatterplotPointColoringDataset1.isValid() || !_scatterplotPointColoringDataset2.isValid() || !_pcaSelectedEmbeddingDataset.isValid() || !_pcaCommonGenesDataset1.isValid() || !_pcaCommonGenesDataset2.isValid()) {
        qWarning() << "Failed to create datasets";
        setStatusMessage("E", "Error occurred during dataset creation", "");
        return false;
    }

    return true;
}


void Computation::updateClusterOptions()
{

    QSet<QString> clusterNames1, clusterNames2;


    if (_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().isValid())
    {
        auto clusterFullDataset1 = mv::data().getDataset<Clusters>(
            _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
        const auto clusters1 = clusterFullDataset1->getClusters();
        for (const auto& cluster : clusters1)
            clusterNames1.insert(cluster.getName());
    }


    if (_settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().getCurrentDataset().isValid())
    {
        auto clusterFullDataset2 = mv::data().getDataset<Clusters>(
            _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
        const auto clusters2 = clusterFullDataset2->getClusters();
        for (const auto& cluster : clusters2)
            clusterNames2.insert(cluster.getName());
    }



    QStringList clusterNamesCommon = getCommonSortedNames(clusterNames1, clusterNames2);

    auto& clusterSelectionAction = _settingsAction.getCommonOptionsHolder().getClusterValueSelectionAction();
    clusterSelectionAction.setOptions(clusterNamesCommon);
    clusterSelectionAction.setSelectedOptions(clusterNamesCommon);
}

void Computation::processAfterPCAComplete()
{
    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("C", "PCA analysis completed", "#28a745");
    qDebug() << "PCA analysis completed successfully";

    if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()) {
        setStatusMessage("E", "No valid Point datasets", "");
        return;
    }

    auto pointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto pointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    if (!pointDataset1.isValid() || !pointDataset2.isValid()) {
        setStatusMessage("E", "No valid Point datasets", "");
        return;
    }
    const QString computationType = _settingsAction.getCommonOptionsHolder().getComputationTypeAction().getCurrentText();
    const bool isPartialPCA = (computationType == "Partial PCA");

    // Prepare PCA data
    _selectedPCData1.coordinates = _pca1.scores();
    _selectedPCData1.numberOfPoints = _pca1.numRows();
    _selectedPCData1.numberOfDimensions = _pca1.numDims();
    const int nComps = _pca1.numComponents();
    const auto& loadings1 = _pca1.loadings();

    // Validate all required datasets
    if (!_selectedGenePointDataset1.isValid() || !_selectedGenePointDataset2.isValid()) {
        qWarning() << "Selected gene point datasets are invalid";
        setStatusMessage("E", "Selected gene point datasets are invalid", "");
        return;
    }
    if (!_selectedCellPointDataset.isValid()) {
        qWarning() << "selected embedding dataset is invalid";
        setStatusMessage("E", "selected embedding dataset is invalid", "");
        return;
    }
    if (!_pcaSelectedEmbeddingDataset.isValid()) {
        qWarning() << "PCA selected embedding dataset is invalid";
        setStatusMessage("E", "PCA selected embedding dataset is invalid", "");
        return;
    }
    if (!_pcaCommonGenesDataset1.isValid()) {
        qWarning() << "PCA common genes dataset 1 is invalid";
        setStatusMessage("E", "PCA common genes dataset 1 is invalid", "");
        return;
    }
    if (!_pcaCommonGenesDataset2.isValid()) {
        qWarning() << "PCA common genes dataset 2 is invalid";
        setStatusMessage("E", "PCA common genes dataset 2 is invalid", "");
        return;
    }
    if (_selectedGenePointDataset2->getNumDimensions() != _selectedPCData1.numberOfDimensions) {
        qWarning() << "Selected gene point dataset 2 dimensions do not match PCA dimensions";
        setStatusMessage("E", "Selected gene point dataset 2 dimensions do not match PCA dimensions", "");
        return;
    }
    if (loadings1.empty()) {
        qWarning() << "PCA loadings are empty";
        setStatusMessage("E", "PCA loadings are empty", "");
        return;
    }
    if (_selectedPCData1.numberOfDimensions != nComps) {
        qWarning() << "Loadings matrix is not square: nDims =" << _selectedPCData1.numberOfDimensions << ", nComps =" << nComps;
        setStatusMessage("E", "Loadings matrix is not square", "");
        return;
    }
    if (static_cast<int>(loadings1.size()) != _selectedPCData1.numberOfDimensions * nComps) {
        qWarning() << "Loadings size mismatch: loadings1.size() =" << loadings1.size() << ", expected =" << _selectedPCData1.numberOfDimensions * nComps;
        setStatusMessage("E", "Loadings size mismatch", "");
        return;
    }
    if (_geneOfInterestIndices0.empty()) {
        qWarning() << "Gene of interest indices 1 are empty";
        setStatusMessage("E", "Gene of interest indices 1 are empty", "");
        return;
    }

    setStatusMessage("R", "Computation after PCA", "");

    // Set PCA embedding dataset
    _pcaSelectedEmbeddingDataset->setData(
        _selectedPCData1.coordinates.data(),
        _selectedPCData1.numberOfPoints,
        _selectedPCData1.numberOfDimensions
    );

    coorelationScoresForPCs();

    mv::events().notifyDatasetDataChanged(_pcaSelectedEmbeddingDataset);
    // Find common gene indices and names
    std::vector<int> geneOfInterestIndices2;
    QStringList commonGeneNames;
    const auto& genedims1 = pointDataset1->getDimensionNames();
    const auto& genedims2 = pointDataset2->getDimensionNames();
    geneOfInterestIndices2.reserve(_geneOfInterestIndices0.size());
    commonGeneNames.reserve(_geneOfInterestIndices0.size());

    // Build a map from gene name to index for genedims2
    QHash<QString, int> geneNameToIndex2;
    for (int i = 0; i < genedims2.size(); ++i) {
        geneNameToIndex2[genedims2[i]] = i;
    }

    for (int inddx : _geneOfInterestIndices0) {
        if (inddx < 0 || inddx >= genedims1.size()) {
            qWarning() << "Invalid index in _geneOfInterestIndices0:" << inddx;
            setStatusMessage("E", "Invalid index in gene of interest indices 1", "");
            return;
        }
        const QString& geneName = genedims1[inddx];
        auto it = geneNameToIndex2.find(geneName);
        if (it != geneNameToIndex2.end()) {
            geneOfInterestIndices2.push_back(it.value());
            commonGeneNames.push_back(geneName);
        }
    }
    if (commonGeneNames.empty()) {
        qWarning() << "No common genes found between datasets";
        setStatusMessage("E", "No common genes found between datasets", "");
        return;
    }
    const int nCommon = static_cast<int>(_geneOfInterestIndices0.size());
    if (geneOfInterestIndices2.size() != nCommon || commonGeneNames.size() != nCommon) {
        qWarning() << "Gene indices or common gene names size mismatch";
        setStatusMessage("E", "Gene indices or common gene names size mismatch", "");
        return;
    }
    if (nCommon != _selectedPCData1.numberOfDimensions) {
        qWarning() << "Gene indices size does not match nDims: indices1 =" << nCommon << ", nDims =" << _selectedPCData1.numberOfDimensions;
        setStatusMessage("E", "Gene indices size does not match PCA dimensions", "");
        return;
    }
    if (isPartialPCA)
    {  // Prepare loading values
        LoadingValuesContainer loadingValues;
        loadingValues.numberOfComponents = nComps;
        loadingValues.numberOfDimensions = _selectedPCData1.numberOfDimensions;

        for (int id = 0; id < _selectedPCData1.numberOfDimensions; ++id) {
            LoadingValues tempValues;
            tempValues.geneId1 = _geneOfInterestIndices0[id];
            tempValues.geneId2 = geneOfInterestIndices2[id];
            tempValues.loadings.resize(nComps);
            for (int j = 0; j < nComps; ++j) {
                tempValues.loadings[j] = loadings1[id * nComps + j];
            }
            loadingValues.loadingValuesMap[commonGeneNames[id]] = tempValues;
        }

        // Validate cluster indices
        const int numPointsReduced1 = static_cast<int>(_selectedClusterIndices1.size());
        const int numPointsReduced2 = static_cast<int>(_selectedClusterIndices2.size());
        if (numPointsReduced1 <= 0 || numPointsReduced2 <= 0) {
            qWarning() << "No points selected for PCA computation";
            setStatusMessage("E", "No points selected for PCA computation", "");
            return;
        }

        // Compute final gene-PC similarity vectors
        std::vector<float> finalGenePCSimVector1(numPointsReduced1 * nComps, 0.0f);
        std::vector<float> finalGenePCSimVector2(numPointsReduced2 * nComps, 0.0f);

        for (const auto& [geneName, value] : loadingValues.loadingValuesMap) {
            const int geneId1 = value.geneId1;
            const int geneId2 = value.geneId2;
            const QVector<float>& loadings = value.loadings;

            if (geneId1 < 0 || geneId1 >= pointDataset1->getNumDimensions() ||
                geneId2 < 0 || geneId2 >= pointDataset2->getNumDimensions()) {
                qWarning() << "Invalid gene indices for PCA computation: geneId1 =" << geneId1 << ", geneId2 =" << geneId2;
                setStatusMessage("E", "Invalid gene indices for PCA computation", "");
                return;
            }

            std::vector<float> genePointVector1(numPointsReduced1, 0.0f);
            std::vector<float> genePointVector2(numPointsReduced2, 0.0f);
            std::vector<int> geneIDVector1{ geneId1 };
            std::vector<int> geneIDVector2{ geneId2 };

            pointDataset1->populateDataForDimensions(genePointVector1, geneIDVector1, _selectedClusterIndices1);
            pointDataset2->populateDataForDimensions(genePointVector2, geneIDVector2, _selectedClusterIndices2);

            for (int j = 0; j < nComps; ++j) {
                float loadingVal = loadings[j];
                for (int k = 0; k < numPointsReduced1; ++k) {
                    finalGenePCSimVector1[k * nComps + j] += genePointVector1[k] * loadingVal;
                }
                for (int k = 0; k < numPointsReduced2; ++k) {
                    finalGenePCSimVector2[k * nComps + j] += genePointVector2[k] * loadingVal;
                }
            }
        }
        // Set PCA common genes datasets
        _pcaCommonGenesDataset1->setData(finalGenePCSimVector1.data(), numPointsReduced1, nComps);
        mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset1);
        _pcaCommonGenesDataset2->setData(finalGenePCSimVector2.data(), numPointsReduced2, nComps);
        mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset2);

        processAfterPCADatasetsPopulated();
    }
}


void Computation::coorelationScoresForPCs()
{
    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("R", "Running correlation between genes and PCs", "#007bff");

    // Validate datasets
    if (!_pcaSelectedEmbeddingDataset.isValid() || !_rotatedSelectedEmbeddingDataset.isValid()) {
        setStatusMessage("E", "PCA selected embedding dataset or rotated selected embedding dataset is invalid", "");
        return;
    }

    const int numDims = _pcaSelectedEmbeddingDataset->getNumDimensions();
    const int numPoints = _pcaSelectedEmbeddingDataset->getNumPoints();
    if (numDims <= 0 || numPoints <= 0) {
        setStatusMessage("E", "PCA selected embedding dataset is empty", "");
        return;
    }

    // Prepare cell indices
    std::vector<int> cellIndices(numPoints);
    std::iota(cellIndices.begin(), cellIndices.end(), 0);

    const int numRotatedPoints = _rotatedSelectedEmbeddingDataset->getNumPoints();
    if (numPoints != numRotatedPoints) {
        qWarning() << "Number of points mismatch: PCA selected embedding dataset has" << numPoints
            << "but rotated selected embedding dataset has" << numRotatedPoints;
        setStatusMessage("E", "Number of points mismatch between PCA selected and rotated selected embedding datasets", "");
        return;
    }
    if (static_cast<int>(cellIndices.size()) != numRotatedPoints) {
        qWarning() << "Cell indices size mismatch: expected" << numRotatedPoints
            << "but got" << cellIndices.size();
        setStatusMessage("E", "Cell indices size mismatch in PCA selected embedding dataset", "");
        return;
    }

    // Find Y dimension index
    const auto dimensionNames = _rotatedSelectedEmbeddingDataset->getDimensionNames();
    if (dimensionNames.empty()) {
        qWarning() << "Dimension names are empty in PCA selected embedding dataset";
        setStatusMessage("E", "Dimension names are empty in PCA selected embedding dataset", "");
        return;
    }
    const auto itY = std::find(dimensionNames.begin(), dimensionNames.end(), "Y");
    const int colorDimIdx = (itY != dimensionNames.end()) ? static_cast<int>(std::distance(dimensionNames.begin(), itY)) : -1;
    if (colorDimIdx == -1) {
        qWarning() << "No Y dimension found in PCA selected embedding dataset";
        setStatusMessage("E", "No Y dimension found in PCA selected embedding dataset", "");
        return;
    }
    if (numDims <= colorDimIdx) {
        qWarning() << "Y dimension index out of bounds in PCA selected embedding dataset";
        setStatusMessage("E", "Y dimension index out of bounds in PCA selected embedding dataset", "");
        return;
    }

    // Extract Y coordinates
    std::vector<float> yCoordinates(numRotatedPoints);
    _rotatedSelectedEmbeddingDataset->extractDataForDimension(yCoordinates, colorDimIdx);
    if (yCoordinates.empty() || static_cast<int>(yCoordinates.size()) != numRotatedPoints) {
        qWarning() << "Y coordinates size mismatch: expected" << numRotatedPoints
            << "but got" << yCoordinates.size();
        setStatusMessage("E", "Y coordinates size mismatch in PCA selected embedding dataset", "");
        return;
    }

    // Compute correlation scores
    std::vector<float> corrGeneCoords;
    const QString corrType = _settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction().getCurrentText();
    computeCorrelationGeneCoords(cellIndices, _pcaSelectedEmbeddingDataset, yCoordinates, corrGeneCoords, corrType);

    // Find index of highest absolute value
    float maxAbsValue = 0.0f;
    int maxIndex = -1;
    for (int i = 0; i < static_cast<int>(corrGeneCoords.size()); ++i) {
        float absValue = std::abs(corrGeneCoords[i]);
        if (absValue > maxAbsValue) {
            maxAbsValue = absValue;
            maxIndex = i;
        }
    }
    if (maxIndex == -1) {
        qWarning() << "No maximum absolute value found in correlation scores";
        setStatusMessage("E", "No maximum absolute value found in correlation scores", "");
        return;
    }
    if (static_cast<int>(corrGeneCoords.size()) != numDims) {
        qWarning() << "Correlation scores size mismatch: expected" << numDims
            << "but got" << corrGeneCoords.size();
        setStatusMessage("E", "Correlation scores size mismatch in PCA selected embedding dataset", "");
        return;
    }

    // Set dimension names with correlation scores
    std::vector<QString> PCCoordNames;
    PCCoordNames.reserve(numDims);
    for (int i = 0; i < numDims; ++i) {
        PCCoordNames.push_back(QString("PC %1 ||| Score: %2").arg(i).arg(corrGeneCoords[i], 0, 'g', 6));
    }
    if (numDims != static_cast<int>(PCCoordNames.size())) {
        qWarning() << "Number of dimensions mismatch: PCA selected embedding dataset has" << numDims
            << "but PCCoordNames has" << PCCoordNames.size();
        setStatusMessage("E", "Number of dimensions mismatch in PCA selected embedding dataset", "");
        return;
    }
    _pcaSelectedEmbeddingDataset->setDimensionNames(PCCoordNames);

    // Update coloring action
    _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setPointsDataset(_pcaSelectedEmbeddingDataset);
    _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setCurrentDimensionIndex(maxIndex);
    setStatusMessage("C", "Correlation scores for PCA dimensions computed successfully", "#28a745");
}
void Computation::processAfterPCADatasetsPopulated()
{
    static bool inProgress = false;
    if (inProgress) return;
    inProgress = true;
   // _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setPointsDataset(_pcaCommonGenesDataset2);

    int numDimsForColoring = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getNumberOfDimensions();


    if(_pcaCommonGenesDataset1.isValid() && _pcaCommonGenesDataset2.isValid() && numDimsForColoring > 0) {

        if (_pcaCommonGenesDataset1->getNumDimensions() != numDimsForColoring || _pcaCommonGenesDataset2->getNumDimensions() != numDimsForColoring) {
            qWarning() << "PCA common genes datasets dimensions do not match number of dimensions for coloring";
            setStatusMessage("E", "PCA common genes datasets dimensions do not match number of dimensions for coloring", "");
            return;
        }

        
        std::vector<QString> dimensionNamesPC;
        dimensionNamesPC.reserve(numDimsForColoring);
        for (int i = 0; i < numDimsForColoring; ++i) {
            dimensionNamesPC.push_back(QString("PC %1 ||| Score: %2").arg(i).arg(0.000000, 0, 'g', 6));
        }

        if (dimensionNamesPC.size() != static_cast<size_t>(numDimsForColoring)) {
            qWarning() << "Dimension names size mismatch: expected" << numDimsForColoring
                << "but got" << dimensionNamesPC.size();
            setStatusMessage("E", "Dimension names size mismatch in PCA common genes datasets", "");
            return;
        }


        // Set PCA common genes datasets
        //qCritical() << "Error Identified here:";
        _pcaCommonGenesDataset1->setDimensionNames(dimensionNamesPC);
        mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset1);
        //qCritical() << "Error Identified here:";
        _pcaCommonGenesDataset2->setDimensionNames(dimensionNamesPC);
        mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset2);
    }



    if (numDimsForColoring > 0) {
        //_settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setCurrentDimensionIndex(0);
    }
    else if (numDimsForColoring < 0) {
        qWarning() << "No PCA dimensions available for coloring";
        setStatusMessage("E", "No PCA dimensions available for coloring", "");
        clearTable(_settingsAction.getTableView1Action());
        clearTable(_settingsAction.getTableView2Action());
        return;
    }

    if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked()) {
        correlationAfterPCA();
    }
    inProgress = false;
}


void Computation::correlationAfterPCA()
{
    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("R", "Running correlation between genes and PC", "#28a745");
    if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()) {
        setStatusMessage("E", "No valid Point datasets", "");
        return;
    }
    auto pointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto pointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    if (!pointDataset1.isValid() || !pointDataset2.isValid()) {
        setStatusMessage("E", "No valid Point datasets", "");

        return;
    }

    int selectedPCDimension = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();


    if (selectedPCDimension < 0)
    {
        setStatusMessage("E", "No PCA dimension selected for dataset 2", "");
        return;
    }
    const int numAllCells2 = pointDataset2->getNumPoints();
    if (numAllCells2 <= 0) {
        setStatusMessage("E", "No cells in point dataset 2", "");
        return;
    }
    const int numAllCells1 = pointDataset1->getNumPoints();
    if (numAllCells1 <= 0) {
        setStatusMessage("E", "No cells in point dataset 1", "");
        return;
    }

    std::vector<float> selectedPC2(numAllCells2);
    _pcaCommonGenesDataset2->extractDataForDimension(selectedPC2, selectedPCDimension);

    std::vector<float> selectedPC1(numAllCells1);
    _pcaCommonGenesDataset1->extractDataForDimension(selectedPC1, selectedPCDimension);

    std::vector<float> correlationGenePC2;
    std::vector<float> correlationGenePC1;

    computeCorrelationGenePC(pointDataset2, selectedPC2, correlationGenePC2, _selectedClusterIndices2);
    computeCorrelationGenePC(pointDataset1, selectedPC1, correlationGenePC1, _selectedClusterIndices1);
    
    const int topNRows = _settingsAction.getCommonOptionsHolder().getTopNRowsAction().getValue();
    const int bottomNRows = _settingsAction.getCommonOptionsHolder().getBottomNRowsAction().getValue();
    const auto dimensionNames2 = pointDataset2->getDimensionNames();
    
    const auto dimensionNames1 = pointDataset1->getDimensionNames();

     try {
        populateGenesByCorrelationRange(correlationGenePC2, topNRows, bottomNRows, dimensionNames2, _geneCorrValues2, _geneOfInterestIndices2,false);
        populateGenesByCorrelationRange(correlationGenePC1, topNRows, bottomNRows, dimensionNames1, _geneCorrValues1, _geneOfInterestIndices1,false);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
        setStatusMessage("E", "Error occurred during filtering genes", "");
        return;
    }

    setupTableModelAndSelection(_settingsAction.getTableView2Action(), _geneCorrValues2, _settingsAction.getDataset2OptionsHolder().getGeneSelectionAction(), _settingsAction.getTableView0Action(), _settingsAction.getTableView1Action(), _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(),false);
    setupTableModelAndSelection(_settingsAction.getTableView1Action(), _geneCorrValues1, _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), _settingsAction.getTableView0Action(), _settingsAction.getTableView2Action(), _settingsAction.getDataset2OptionsHolder().getGeneSelectionAction(),false);
    selectFirstRow(_settingsAction.getTableView1Action());
    

    setStatusMessage("C", "Correlation between genes and PC completed", "#28a745");
    colorLinePlots();
}
bool Computation::getRotationAngle(const QString& scatterplotViewName, float& rotationAngle)
{
    auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
    if (!scatterplotViewFactory) {
        qWarning() << "Scatterplot view factory not found";
        setStatusMessage("E", "Scatterplot view factory not found", "");
        return false;
    }


    for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory)) {
        if (plugin->getGuiName() == scatterplotViewName) {
            auto* angleAction = findActionByPath<DecimalAction>(
                plugin,"Settings/Selection/Point Selection/Line angle");
            if (!angleAction) {
                qWarning() << "Angle action not found";
                setStatusMessage("E", "Angle action not found", "");
                return false;
            }
            rotationAngle = angleAction->getValue();
            return true;
        }
    }

    qWarning() << "Scatterplot plugin not found";
    setStatusMessage("E", "Scatterplot plugin not found", "");

    return false;
}

bool Computation::populateEmbeddingCoordinates(int xEmbeddingIndex, int yEmbeddingIndex, std::vector<float>& XYCoordinates)
{
    if (_filteredSelectionIndices.empty()) {
        qWarning() << "No filtered selection indices";
        setStatusMessage("E", "No indices selected", "");
        return false;
    }

    const size_t numPoints = _filteredSelectionIndices.size();
    XYCoordinates.resize(numPoints * 2);


    int embeddingIndices[2] = { xEmbeddingIndex, yEmbeddingIndex };
    if (_selectionDataset == 1)
    {
        _embeddingDataset1->populateDataForDimensions(
            XYCoordinates,
            std::vector<int>(embeddingIndices, embeddingIndices + 2),
            _filteredSelectionIndices
        );
    }
    else if (_selectionDataset == 2)
    {
        _embeddingDataset2->populateDataForDimensions(
            XYCoordinates,
            std::vector<int>(embeddingIndices, embeddingIndices + 2),
            _filteredSelectionIndices
        );
    }
    else
    {
        setStatusMessage("E", "Invalid selection dataset", "");
        return false;

    }



    return true;
}

bool Computation::populateAndUpdateDatasets(mv::Dataset<Points>& fullNormalizedPointDataset, const std::vector<float>& rotatedXYCordinates, /*const std::vector<float>& XYCordinates,*/ int noOfDimensions, const std::vector<int>& columnIndices, const std::vector<QString>& dimensionNames)
{

    if (!_selectedCellPointDataset.isValid() ||
        !_rotatedSelectedEmbeddingDataset.isValid() ||
        !_lineChartPointDataset1.isValid() ||
        !_lineChartClusterColoringDataset1.isValid() ||
        !_selectedGenePointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() ||
        /*!_selectedEmbeddingDataset.isValid() ||*/
        !fullNormalizedPointDataset.isValid() ||
        _filteredSelectionIndices.empty() ||
        columnIndices.empty() ||
        dimensionNames.size() != static_cast<size_t>(noOfDimensions))
    {
        qWarning() << "Invalid dataset pointers or input sizes!";
        setStatusMessage("E", "Internal error: invalid dataset pointer or input sizes", "");
        return false;
    }


    const size_t numCells = _filteredSelectionIndices.size();
    const size_t numGenes = columnIndices.size();
    std::vector<float> cellbygenematrix(numCells * numGenes);


    try {
        fullNormalizedPointDataset->populateDataForDimensions(cellbygenematrix, columnIndices, _filteredSelectionIndices);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }

    //TODO TEST START
    //float currentLowVal1 = _settingsAction.getDataset1OptionsHolder().getExpressionLowerColoringRangeAction().getValue();
    //float currentHighVal1 = _settingsAction.getDataset1OptionsHolder().getExpressionUpperColoringRangeAction().getValue();
    //float currentLowVal2 = _settingsAction.getDataset2OptionsHolder().getExpressionLowerColoringRangeAction().getValue();
    //float currentHighVal2 = _settingsAction.getDataset2OptionsHolder().getExpressionUpperColoringRangeAction().getValue();

    //float minValue = *std::min_element(cellbygenematrix.begin(), cellbygenematrix.end());
    //float maxValue = *std::max_element(cellbygenematrix.begin(), cellbygenematrix.end());

    //qInfo() << "Current Low Value:" << minValue;
    //qInfo() << "Current High Value:" << maxValue;


        //_settingsAction.getCommonOptionsHolder().getExpressionLowerColoringRangeAction().setValue((minValue));


        //_settingsAction.getCommonOptionsHolder().getExpressionUpperColoringRangeAction().setValue((maxValue));

    //TODO TEST END
    _selectedCellPointDataset->setData(cellbygenematrix.data(), static_cast<int>(numCells), noOfDimensions);
    _selectedCellPointDataset->setDimensionNames(dimensionNames);
    mv::events().notifyDatasetDataChanged(_selectedCellPointDataset);

    _rotatedSelectedEmbeddingDataset->setData(rotatedXYCordinates.data(), static_cast<int>(numCells), 2);
    _rotatedSelectedEmbeddingDataset->setDimensionNames({ QStringLiteral("X"), QStringLiteral("Y") });
    mv::events().notifyDatasetDataChanged(_rotatedSelectedEmbeddingDataset);


    std::vector<QString> dimensionNamesWithYCoords = { QStringLiteral("Empty") };
    dimensionNamesWithYCoords.push_back("Rotated Y Coordinates");
    dimensionNamesWithYCoords.push_back("Mean Expr All Genes");
    dimensionNamesWithYCoords.push_back("Mean Expr Selected Genes");
    dimensionNamesWithYCoords.insert(dimensionNamesWithYCoords.end(), dimensionNames.begin(), dimensionNames.end());


    std::vector<float> rotatedYCoord(numCells * 1);
    _rotatedSelectedEmbeddingDataset->extractDataForDimension(rotatedYCoord, 1);
    std::vector<float> meanSelectedGenes(numCells, 0.0f);
    std::vector<float> meanAllGenes(numCells, 0.0f);
    for (size_t iC = 0; iC < numCells; ++iC) {
        float sum = 0.0f;
        if (numGenes > 0) {
            for (size_t iG = 0; iG < numGenes; ++iG) {
                sum += _selectedCellPointDataset->getValueAt(iC * numGenes + iG);
            }
            meanAllGenes[iC] = sum / static_cast<float>(numGenes);
        }
        else {
            meanAllGenes[iC] = 0.0f;
        }
    }

    std::vector<float> linechartdata(numCells * (4 + numGenes));


    for (size_t i = 0; i < numCells; ++i) {
        // Zero column: blank values
        linechartdata[i * (4 + numGenes)] = 0.0f;
        // First column: Rotated Y coordinate
        linechartdata[i * (4 + numGenes)+1] = rotatedYCoord[i];
        // Second column: Mean of all genes
        linechartdata[i * (4 + numGenes) + 2] = meanAllGenes[i];
        // Third column: Mean of selected genes
        linechartdata[i * (4 + numGenes) + 3] = meanSelectedGenes[i];
        // Remaining columns: All gene values
        for (size_t j = 0; j < numGenes; ++j) {
            linechartdata[i * (4 + numGenes) + 4 + j] = cellbygenematrix[i * numGenes + j];
        }
    }


    _lineChartPointDataset1->setData(linechartdata.data(), static_cast<int>(numCells), noOfDimensions + 4);

    _lineChartPointDataset1->setDimensionNames(dimensionNamesWithYCoords);
    mv::events().notifyDatasetDataChanged(_lineChartPointDataset1);

    return true;
}

bool Computation::setupAndTriggerPCA()
{
    if (_isPCATriggerComputing) return false;
    _isPCATriggerComputing = true;
    auto guard = qScopeGuard([this]() { _isPCATriggerComputing = false; });

    FunctionTimer timer(Q_FUNC_INFO);
    if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()) {
        setStatusMessage("E", "No valid Point datasets", "");
        return false;
    }
    auto pointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto pointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());

    // Early exit for invalid datasets
    if (!_selectedGenePointDataset1.isValid() || !pointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() || !pointDataset2.isValid()) {
        qWarning() << "One or more datasets are invalid!";
        setStatusMessage("E", "Internal error: invalid dataset pointer", "");
        return false;
    }

    // Gather sizes and check for validity
    const int numCells0 = static_cast<int>(_filteredSelectionIndices.size());
    const int numAllCellsAll1 = pointDataset1->getNumPoints();
    const int numAllCellsAll2 = pointDataset2->getNumPoints();
    const int numAllFilteredCells1 = static_cast<int>(_selectedClusterIndices1.size());
    const int numAllFilteredCells2 = static_cast<int>(_selectedClusterIndices2.size());
    const int numOfIntGenes = static_cast<int>(_geneOfInterestIndices0.size());

    const auto& dimensionNames1 = pointDataset1->getDimensionNames();
    const auto& dimensionNames2 = pointDataset2->getDimensionNames();

    if (numCells0 <= 0 || numAllCellsAll1 <= 0 || numAllCellsAll2 <= 0 ||
        numAllFilteredCells1 <= 0 || numAllFilteredCells2 <= 0 || numOfIntGenes <= 0) {
        qWarning() << "Invalid dataset sizes: numCells0:" << numCells0
            << " numOfIntGenes:" << numOfIntGenes
            << " numAllFilteredCells1:" << numAllFilteredCells1
            << " numAllFilteredCells2:" << numAllFilteredCells2
            << " numAllCellsAll1:" << numAllCellsAll1
            << " numAllCellsAll2:" << numAllCellsAll2;
        setStatusMessage("E", "Invalid dataset sizes", "");
        return false;
    }
    if (numOfIntGenes > static_cast<int>(dimensionNames1.size()) ||
        numOfIntGenes > static_cast<int>(dimensionNames2.size())) {
        qWarning() << "Number of genes of interest exceeds dataset dimensions";
        setStatusMessage("E", "Number of genes of interest exceeds dataset dimensions", "");
        return false;
    }

    // Build gene index lists and names of interest
    std::vector<QString> dimensionNamesOfInterest;
    std::vector<int> geneDataset0, geneDataset1, geneDataset2;
    dimensionNamesOfInterest.reserve(numOfIntGenes);

    const QString computationType = _settingsAction.getCommonOptionsHolder().getComputationTypeAction().getCurrentText();
    const bool isPartialPCA = (computationType == "Partial PCA");

    geneDataset0.reserve(numOfIntGenes);
    geneDataset1.reserve(numOfIntGenes);
    geneDataset2.reserve(numOfIntGenes);

    for (int idx : _geneOfInterestIndices0) {
        if (idx < 0 || idx >= static_cast<int>(dimensionNames1.size()))
            continue;
        const QString& geneName = dimensionNames1[idx];
        auto it2 = std::find(dimensionNames2.begin(), dimensionNames2.end(), geneName);
        if (it2 != dimensionNames2.end()) {
            int geneId2 = static_cast<int>(std::distance(dimensionNames2.begin(), it2));
            dimensionNamesOfInterest.push_back(geneName);

            
            if (_selectionDataset == 1)
            {
                geneDataset0.push_back(idx);
            }
            else if (_selectionDataset == 2)
            {
                geneDataset0.push_back(geneId2);
            }
            else
            {
                setStatusMessage("E", "Invalid selection dataset", "");
                return false;

            }

            geneDataset1.push_back(idx);
            geneDataset2.push_back(geneId2);
        }
    }

    if (dimensionNamesOfInterest.empty()) {
        qWarning() << "No common genes found between datasets";
        setStatusMessage("E", "No common genes found between datasets", "");
        return false;
    }
    if (dimensionNamesOfInterest.size() > _filteredSelectionIndices.size()) {
        setStatusMessage("E", "Row selection exceeds available point selection size", "");
        return false;
    }
    const int numDimsOfInterest = static_cast<int>(dimensionNamesOfInterest.size());
    if (numDimsOfInterest <= 0) {
        qWarning() << "No genes of interest selected for PCA";
        setStatusMessage("E", "No genes of interest selected for PCA", "");
        return false;
    }

    std::vector<float> cellByGeneMatrix0;

        // Prepare cell indices
        std::vector<int> cellIndices0(_filteredSelectionIndices.begin(), _filteredSelectionIndices.end());

        // Prepare data matrices
        cellByGeneMatrix0.resize(numCells0 * numDimsOfInterest);
        try {
            
            if (_selectionDataset == 1)
            {
                pointDataset1->populateDataForDimensions(cellByGeneMatrix0, geneDataset0, cellIndices0);
            }
            else if (_selectionDataset == 2)
            {
                pointDataset2->populateDataForDimensions(cellByGeneMatrix0, geneDataset0, cellIndices0);
            }
            else
            {
                setStatusMessage("E", "Invalid selection dataset", "");
                return false;

            }
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in populateDataForDimensions:" << ex.what();
            setStatusMessage("E", "Error occurred during populating filtered data", "");
            return false;
        }
        /*if (!cellByGeneMatrix0.empty()) {
            auto [minVal, maxVal] = minMax(cellByGeneMatrix0);
            qInfo() << "PCA cellByGeneMatrix0 min:" << minVal << "max:" << maxVal;
        }*/
   

    std::vector<float> cellByGeneMatrix2(numAllFilteredCells2 * numDimsOfInterest, 0.0f);
    std::vector<float> cellByGeneMatrix1(numAllFilteredCells1 * numDimsOfInterest, 0.0f);

    if (_selectedClusterIndices1.empty() || _selectedClusterIndices2.empty()) {
        qWarning() << "No selected cluster indices for dataset 1 or 2";
        setStatusMessage("E", "No selected cluster indices for dataset 1 or 2", "");
        return false;
    }
    if (_selectedClusterIndices2.size() > static_cast<size_t>(numAllCellsAll2) ||
        _selectedClusterIndices1.size() > static_cast<size_t>(numAllCellsAll1)) {
        qWarning() << "Selected cluster indices exceed total number of cells in dataset 2 or 1";
        setStatusMessage("E", "Selected cluster indices exceed total number of cells in dataset 2 or 1", "");
        return false;
    }

    try {
        pointDataset2->populateDataForDimensions(cellByGeneMatrix2, geneDataset2, _selectedClusterIndices2);
        pointDataset1->populateDataForDimensions(cellByGeneMatrix1, geneDataset1, _selectedClusterIndices1);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }

    // Set data and dimension names for selected gene point datasets
    _selectedGenePointDataset2->setData(cellByGeneMatrix2.data(), numAllFilteredCells2, numDimsOfInterest);

    _selectedGenePointDataset2->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset2);

    _selectedGenePointDataset1->setData(cellByGeneMatrix1.data(), numAllFilteredCells1, numDimsOfInterest);

    _selectedGenePointDataset1->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset1);

    // Update line chart dataset with mean of selected genes
    if (_lineChartPointDataset1.isValid() &&
        !_geneOfInterestIndices0.empty() &&
        !_filteredSelectionIndices.empty()) {
        const int numRowsLine = _lineChartPointDataset1->getNumPoints();
        if (pointDataset1.isValid() &&
            static_cast<int>(_filteredSelectionIndices.size()) == numRowsLine) {
            int numOfGenes = static_cast<int>(_geneOfInterestIndices0.size());
            std::vector<float> expDataSelectedCellGene(numRowsLine * numOfGenes, 0.0f);
            pointDataset1->populateDataForDimensions(
                expDataSelectedCellGene, _geneOfInterestIndices0, _filteredSelectionIndices);

            // Compute mean expression for each cell
            std::vector<float> meanOfSelectedGenes(numRowsLine, 0.0f);
            for (int i = 0; i < numRowsLine; ++i) {
                float sum = 0.0f;
                for (int j = 0; j < numOfGenes; ++j) {
                    sum += expDataSelectedCellGene[i * numOfGenes + j];
                }
                meanOfSelectedGenes[i] = (numOfGenes > 0) ? (sum / static_cast<float>(numOfGenes)) : 0.0f;
            }
            const int stride = _lineChartPointDataset1->getNumDimensions();
            for (int i = 0; i < numRowsLine; ++i) {
                _lineChartPointDataset1->setValueAt(i * stride + 2, meanOfSelectedGenes[i]);
            }
            events().notifyDatasetDataChanged(_lineChartPointDataset1);
        }
    }


    disconnect(&_pcaCommonGenesDataset1, &Dataset<Points>::dataChanged, this, nullptr);
    disconnect(&_pcaCommonGenesDataset2, &Dataset<Points>::dataChanged, this, nullptr);

    setStatusMessage("R", "PCA Started", "");

    _pca1.fit(cellByGeneMatrix0, numCells0, numDimsOfInterest, numOfIntGenes);

    if (!isPartialPCA)
    {

        if (!_pcaAnalysisCommonGenesDataset1Plugin) {
            _pcaAnalysisCommonGenesDataset1Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset1 }, { _pcaCommonGenesDataset1 });
        }
        if (!_pcaAnalysisCommonGenesDataset2Plugin) {
            _pcaAnalysisCommonGenesDataset2Plugin = mv::plugins().requestPlugin<AnalysisPlugin>("PCA", { _selectedGenePointDataset2 }, { _pcaCommonGenesDataset2 });
        }

        if (!_pcaAnalysisCommonGenesDataset1Plugin) {
            qCritical() << "PCA Analysis Plugin not found";
            setStatusMessage("E", "PCA plugin not found", "");
            return false;
        }
        if (!_pcaAnalysisCommonGenesDataset2Plugin) {
            qCritical() << "PCA Analysis Plugin not found";
            setStatusMessage("E", "PCA plugin not found", "");
            return false;
        }
        connect(&_pcaCommonGenesDataset1, &Dataset<Points>::dataChanged, this, [this]() {
            _pcaDataset1Complete = true;
            if (_pcaDataset1Complete && _pcaDataset2Complete) {
                _isManualComputeComputing = false;
                processAfterPCADatasetsPopulated();
            }
            });
        connect(&_pcaCommonGenesDataset2, &Dataset<Points>::dataChanged, this, [this]() {
            _pcaDataset2Complete = true;
            if (_pcaDataset1Complete && _pcaDataset2Complete) {
                _isManualComputeComputing = false;
                processAfterPCADatasetsPopulated();
            }
            });
        auto componentsAction1 = dynamic_cast<IntegralAction*>(_pcaCommonGenesDataset1->findChildByPath("PCA/Number of PCA components"));
        if (!componentsAction1) {
            qCritical() << "Number of PCA components action not found";
            setStatusMessage("E", "PCA components action not found", "");
            return false;
        }
        componentsAction1->setMaximum(numOfIntGenes);
        componentsAction1->setValue(numOfIntGenes);

        auto componentsAction2 = dynamic_cast<IntegralAction*>(_pcaCommonGenesDataset2->findChildByPath("PCA/Number of PCA components"));
        if (!componentsAction2) {
            qCritical() << "Number of PCA components action not found";
            setStatusMessage("E", "PCA components action not found", "");
            return false;
        }
        componentsAction2->setMaximum(numOfIntGenes);
        componentsAction2->setValue(numOfIntGenes);


        auto startAction1 = dynamic_cast<TriggerAction*>(_pcaCommonGenesDataset1->findChildByPath("PCA/Start analysis"));
        auto startAction2 = dynamic_cast<TriggerAction*>(_pcaCommonGenesDataset2->findChildByPath("PCA/Start analysis"));
        if (startAction1) {
            startAction1->trigger();
            //qDebug() << "PCA analysis 1 started";
            QEventLoop loop;
            loop.processEvents();
            _pcaCommonGenesDataset1->setSelectionIndices({});
            mv::events().notifyDatasetDataSelectionChanged(_pcaCommonGenesDataset1);

        }
        else {
            qCritical() << "Start analysis action not found in PCA dataset";
            setStatusMessage("E", "PCA start action not found", "");
            return false;
        }
        if (startAction2) {
            startAction2->trigger();
            //qDebug() << "PCA analysis 2 started";
            QEventLoop loop;
            loop.processEvents();
            _pcaCommonGenesDataset2->setSelectionIndices({});
            mv::events().notifyDatasetDataSelectionChanged(_pcaCommonGenesDataset2);

        }
        else {
            qCritical() << "Start analysis action not found in PCA dataset";
            setStatusMessage("E", "PCA start action not found", "");
            return false;
        }
    }

    return true;
}
/*
void Computation::normalizeData()
{
    if (!_projectOpened)
        return;

    constexpr int CHUNK_SIZE = 10000; // Tune as needed for memory constraints

    if (!_normalizedFullDataset1.isValid()) {
        _normalizedFullDataset1 = mv::data().createDataset("Points", "Normalized Full Dataset 1");
        _normalizedFullDataset1->setGroupIndex(555);
        mv::events().notifyDatasetAdded(_normalizedFullDataset1);
    }
    if (!_normalizedFullDataset2.isValid()) {
        _normalizedFullDataset2 = mv::data().createDataset("Points", "Normalized Full Dataset 2");
        _normalizedFullDataset2->setGroupIndex(556);
        mv::events().notifyDatasetAdded(_normalizedFullDataset2);
    }

    if (!_normalizedFullDataset1.isValid() || !_normalizedFullDataset2.isValid()) {
        setStatusMessage("E", "No valid datasets for normalization", "");
        return;
    }

    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("R", "Normalizing data", "#28a745");

    auto fullPointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto fullPointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());

    if (!fullPointDataset1.isValid() || !fullPointDataset2.isValid()) {
        setStatusMessage("E", "Invalid point datasets for normalization", "");
        return;
    }

    const int numPoints1 = fullPointDataset1->getNumPoints();
    const int numDims1 = fullPointDataset1->getNumDimensions();
    const int numPoints2 = fullPointDataset2->getNumPoints();
    const int numDims2 = fullPointDataset2->getNumDimensions();

    std::vector<int> columns1(numDims1);
    std::vector<int> columns2(numDims2);
    std::iota(columns1.begin(), columns1.end(), 0);
    std::iota(columns2.begin(), columns2.end(), 0);

    QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
    NormalizationType normType = normalizationTypeFromString(normTypeStr);

    bool combined = _settingsAction.getCommonOptionsHolder().getNormalizeCombinedDatasetsAction().isChecked();

    float globalMin = std::numeric_limits<float>::infinity();
    float globalMax = -std::numeric_limits<float>::infinity();

    // First pass: find global min/max if combined normalization
    if (combined) {
        for (int offset = 0; offset < numPoints1; offset += CHUNK_SIZE) {
            int chunkSize = std::min(CHUNK_SIZE, numPoints1 - offset);
            std::vector<float> chunk(chunkSize * numDims1);
            std::vector<int> rowIndices(chunkSize);
            std::iota(rowIndices.begin(), rowIndices.end(), offset);
            fullPointDataset1->populateDataForDimensions(chunk, columns1, rowIndices);
            auto [minIt, maxIt] = std::minmax_element(chunk.begin(), chunk.end());
            if (minIt != chunk.end() && maxIt != chunk.end()) {
                globalMin = std::min(globalMin, *minIt);
                globalMax = std::max(globalMax, *maxIt);
            }
        }
        for (int offset = 0; offset < numPoints2; offset += CHUNK_SIZE) {
            int chunkSize = std::min(CHUNK_SIZE, numPoints2 - offset);
            std::vector<float> chunk(chunkSize * numDims2);
            std::vector<int> rowIndices(chunkSize);
            std::iota(rowIndices.begin(), rowIndices.end(), offset);
            fullPointDataset2->populateDataForDimensions(chunk, columns2, rowIndices);
            auto [minIt, maxIt] = std::minmax_element(chunk.begin(), chunk.end());
            if (minIt != chunk.end() && maxIt != chunk.end()) {
                globalMin = std::min(globalMin, *minIt);
                globalMax = std::max(globalMax, *maxIt);
            }
        }
    }

    // Second pass: normalize and write out in chunks (parallelized)
    std::vector<float> normalized1(numPoints1 * numDims1, 0.0f);
    std::vector<float> normalized2(numPoints2 * numDims2, 0.0f);

    // Helper struct for chunk info
    struct ChunkTask {
        int offset;
        int chunkSize;
        int numDims;
        std::vector<int> columns;
        mv::Dataset<Points> dataset;
        float globalMin;
        float globalMax;
        bool combined;
        NormalizationType normType;
        float* outPtr;
    };

    auto processChunk = [](ChunkTask& task) {
        std::vector<float> chunk(task.chunkSize * task.numDims);
        std::vector<int> rowIndices(task.chunkSize);
        std::iota(rowIndices.begin(), rowIndices.end(), task.offset);
        task.dataset->populateDataForDimensions(chunk, task.columns, rowIndices);

        if (task.combined) {
            for (auto& v : chunk) {
                v = (task.globalMax > task.globalMin) ? (v - task.globalMin) / (task.globalMax - task.globalMin) : 0.0f;
            }
        }
        else {
            normalizeVector(chunk, task.normType);
        }
        std::copy(chunk.begin(), chunk.end(), task.outPtr + task.offset * task.numDims);
        };

    // Prepare chunk tasks for dataset 1
    QList<ChunkTask> tasks1;
    for (int offset = 0; offset < numPoints1; offset += CHUNK_SIZE) {
        int chunkSize = std::min(CHUNK_SIZE, numPoints1 - offset);
        tasks1.append(ChunkTask{ offset, chunkSize, numDims1, columns1, fullPointDataset1, globalMin, globalMax, combined, normType, normalized1.data() });
    }
    // Prepare chunk tasks for dataset 2
    QList<ChunkTask> tasks2;
    for (int offset = 0; offset < numPoints2; offset += CHUNK_SIZE) {
        int chunkSize = std::min(CHUNK_SIZE, numPoints2 - offset);
        tasks2.append(ChunkTask{ offset, chunkSize, numDims2, columns2, fullPointDataset2, globalMin, globalMax, combined, normType, normalized2.data() });
    }

    // Run normalization in parallel
    QtConcurrent::blockingMap(tasks1, processChunk);
    QtConcurrent::blockingMap(tasks2, processChunk);

    // Compute min/max for UI
    float minValue = *std::min_element(normalized1.begin(), normalized1.end());
    float maxValue = *std::max_element(normalized1.begin(), normalized1.end());
    if (!normalized2.empty()) {
        minValue = std::min(minValue, *std::min_element(normalized2.begin(), normalized2.end()));
        maxValue = std::max(maxValue, *std::max_element(normalized2.begin(), normalized2.end()));
    }

    _normalizedFullDataset1->setData(normalized1.data(), numPoints1, numDims1);

    _normalizedFullDataset1->setDimensionNames(fullPointDataset1->getDimensionNames());

    mv::events().notifyDatasetDataChanged(_normalizedFullDataset1);

    _normalizedFullDataset2->setData(normalized2.data(), numPoints2, numDims2);

    _normalizedFullDataset2->setDimensionNames(fullPointDataset2->getDimensionNames());

    mv::events().notifyDatasetDataChanged(_normalizedFullDataset2);

    auto& scatterplotRangeUpper1 = _settingsAction.getDataset1OptionsHolder().getScatterplotUpperColoringRangeAction();
    auto& scatterplotRangeLower1 = _settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction();
    auto& scatterplotRangeUpper2 = _settingsAction.getDataset2OptionsHolder().getScatterplotUpperColoringRangeAction();
    auto& scatterplotRangeLower2 = _settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction();

    auto& lineplotRangeUpper1 = _settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction();
    auto& lineplotRangeLower1 = _settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction();

    scatterplotRangeUpper1.setMaximum(maxValue);
    scatterplotRangeUpper1.setMinimum(minValue);
    scatterplotRangeLower1.setMaximum(maxValue);
    scatterplotRangeLower1.setMinimum(minValue);

    scatterplotRangeUpper2.setMaximum(maxValue);
    scatterplotRangeUpper2.setMinimum(minValue);
    scatterplotRangeLower2.setMaximum(maxValue);
    scatterplotRangeLower2.setMinimum(minValue);

    lineplotRangeUpper1.setMaximum(maxValue);
    lineplotRangeUpper1.setMinimum(minValue);
    lineplotRangeLower1.setMaximum(maxValue);
    lineplotRangeLower1.setMinimum(minValue);

    int maxRankLimit = std::min(
        _normalizedFullDataset2->getNumDimensions(),
        _normalizedFullDataset1->getNumDimensions()
    );
    int minRankLimit = 0;

    auto& topNRowsAction = _settingsAction.getCommonOptionsHolder().getTopNRowsAction();
    auto& bottomNRowsAction = _settingsAction.getCommonOptionsHolder().getBottomNRowsAction();

    topNRowsAction.setMaximum(maxRankLimit);
    bottomNRowsAction.setMaximum(maxRankLimit);
    topNRowsAction.setMinimum(minRankLimit);
    bottomNRowsAction.setMinimum(minRankLimit);

    topNRowsAction.setValue(std::min(10, maxRankLimit));
    bottomNRowsAction.setValue(std::min(minRankLimit, maxRankLimit));
    setStatusMessage("C", "Data normalization completed", "#28a745");
}
*/


void Computation::setSelectionAndAutoCompute()
{
    if (!_projectOpened)
{
    //qDebug() << "Project opening";
    return;
}
    if (_selectionDataset < 1)
    {

        setStatusMessage("E", "No valid selection dataset", "");
        return;
    }
    disableOptions();
    AutoCompute();
    enableOptions();
    _blockSelectionTrigger = false;
}

void Computation::coloringLinePlotsOld()
{
    if (!_projectOpened)
    {
        //qDebug() << "Project opening";
        return;
    }
    QString linePlotName = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();


    if (!_lineChartPointDataset1.isValid() || linePlotName.isEmpty() || _filteredSelectionIndices.size()<1 || !_selectedCellPointDataset.isValid())
    {
        clearLinePlots();
        setStatusMessage("E", "No valid line chart dataset or line plot name", "");
        return;
    }
    if (_lineChartPointDataset1->getNumPoints() < 1)
    {
        clearLinePlots();
        setStatusMessage("E", "No valid line chart dataset", "");
        return;
    }
    if (_selectedCellPointDataset->getNumPoints() != _lineChartPointDataset1->getNumPoints())
    {
        clearLinePlots();
        setStatusMessage("E", "Selected cell dataset and line chart dataset do not match in number of points", "");
        return;
    }

    int totalSelectedPoints = _selectedCellPointDataset->getNumPoints();
    

    QString smoothingType = "Moving Average";
    QString xPositionString = "Rotated Y Coordinates";
    int smoothingWindow = static_cast<int>(std::round(totalSelectedPoints * 0.2));
    QString yPositionString = "";
    QString titleText = "";
    auto colorMapStringValue = _settingsAction.getCommonOptionsHolder().getColoringMapSelectionAction().getColorMap();
    QString colorDatasetID = "";
    QString colordatasetType = "";
    if (_lineChartClusterColoringDataset1.isValid())
    {
        colorDatasetID = _lineChartClusterColoringDataset1.getDatasetId();
        if (_lineChartClusterColoringDataset1->getDataType() == PointType)
        {
            colordatasetType = "Points";
        }
        else if (_lineChartClusterColoringDataset1->getDataType() == ClusterType)
        {
            colordatasetType = "Clusters";
        }
        /*if (_settingsAction.getCommonOptionsHolder().getSelectionModeAction().isChecked())
        {
            QString referenceClusterDatasetID = _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDatasetId();
            if (referenceClusterDatasetID.isEmpty())
            {
                setStatusMessage("E", "No valid cluster dataset selected for coloring", "");
                return;
            }
            createClusterLineChartClusterColors(colorDatasetID, referenceClusterDatasetID,_filteredSelectionIndices);
            yPositionString = "Mean Expr All Genes";
            titleText = QString("%1 vs %2 expression of selected cells (%3)")
                .arg(xPositionString)
                .arg(yPositionString)
                .arg(static_cast<int>(_filteredSelectionIndices.size()));
        }
        else*/
        {
            float minValLine = _settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction().getValue();
            float maxValLine = _settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction().getValue();

            //float minValScatter1 = _settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction().getValue();
            //float maxValScatter1 = _settingsAction.getDataset1OptionsHolder().getScatterplotUpperColoringRangeAction().getValue();

            //float minValScatter2 = _settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction().getValue();
            //float maxValScatter2 = _settingsAction.getDataset2OptionsHolder().getScatterplotUpperColoringRangeAction().getValue();

            if (_settingsAction.getCommonOptionsHolder().getPcaColoringAction().isChecked())
            {
                if (!_pcaSelectedEmbeddingDataset.isValid())
                {
                    setStatusMessage("E", "No valid PCA dataset selected for coloring", "");
                    return;
                }
                QString pcaDataset1ID = _pcaSelectedEmbeddingDataset.getDatasetId();
                if (pcaDataset1ID.isEmpty() )
                {
                    setStatusMessage("E", "No valid PCA datasets", "");
                    return;
                }
                
                QString selectedPCADimensionName = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionName();
                int selectedPCADimensionIndx = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();
                createLineChartClusterColors(colorDatasetID, colorMapStringValue, _filteredSelectionIndices, pcaDataset1ID, totalSelectedPoints, selectedPCADimensionName, minValLine, maxValLine,true);
                yPositionString = "Mean Expr All Genes";
                titleText = QString("%1 vs %2 expression of selected cells (%3)")
                    .arg(xPositionString)
                    .arg(yPositionString)
                    .arg(static_cast<int>(_filteredSelectionIndices.size()));

            }
            else
            {
                QString selectedGene = _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();

                if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid())
                {
                    setStatusMessage("E", "No valid point dataset selected for coloring", "");
                    return;
                }
                if (!selectedGene.isEmpty())
                {
                    QString geneDataset1ID = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId();
                    if (geneDataset1ID.isEmpty())
                    {
                        setStatusMessage("E", "No valid  datasets", "");
                        return;
                    }
                    yPositionString = selectedGene;
                    QString selectedGeneName = _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
                    int selectedGeneIndx = _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionIndex();
                    createLineChartClusterColors(colorDatasetID, colorMapStringValue, _selectedClusterIndices1, geneDataset1ID, totalSelectedPoints, selectedGeneName, minValLine, maxValLine,false);
                }
                else
                {
                    yPositionString = "Mean Expr All Genes";
                }
                titleText = QString("%1 vs %2 expression of selected cells (%3)")
                    .arg(xPositionString)
                    .arg(yPositionString)
                    .arg(static_cast<int>(_filteredSelectionIndices.size()));

            }
        }

    }

    lineplotModeChange(linePlotName, _lineChartPointDataset1.getDatasetId(), colorDatasetID, colordatasetType, -1, xPositionString, yPositionString, titleText, smoothingType, smoothingWindow, "", 0.0, 0.0);
    mv::events().notifyDatasetDataChanged(_lineChartPointDataset1);

    
}

void Computation::coloringScatterPlotsOld()
{
    if (!_projectOpened)
    {
        //qDebug() << "Project opening";
        return;
    }

    if (!_settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().isValid() ||
        !_settingsAction.getDataset2OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().isValid())
    {
        setStatusMessage("E", "No valid embedding datasets", "");
        return;
    }

    int totalPoints1 = 0;
    int totalPoints2 = 0;
    if (_embeddingDataset1.isValid())
    {
        totalPoints1 = _embeddingDataset1->getNumPoints();
    }
    if (_embeddingDataset2.isValid())
    {
        totalPoints2 = _embeddingDataset2->getNumPoints();
    }
    QString embeddingDataset1Id = _embeddingDataset1.getDatasetId();
    QString embeddingDataset2Id = _embeddingDataset2.getDatasetId();
    if (embeddingDataset1Id.isEmpty() || embeddingDataset2Id.isEmpty())
    {
        setStatusMessage("E", "No valid dataset ids", "");
        return;
    }
    QString embeddingDimensionX1 = _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName();
    QString embeddingDimensionY1 = _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName();
    QString embeddingDimensionX2 = _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName();
    QString embeddingDimensionY2 = _settingsAction.getDataset2OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName();
    if (embeddingDimensionX1.isEmpty() || embeddingDimensionY1.isEmpty() ||
        embeddingDimensionX2.isEmpty() || embeddingDimensionY2.isEmpty())
    {
        setStatusMessage("E", "No valid embedding dimensions selected", "");
        return;
    }
    
    QString scatterplot1Name = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
    QString scatterplot2Name = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
    if (scatterplot1Name.isEmpty() || scatterplot2Name.isEmpty())
    {
        setStatusMessage("E", "No valid plot names selected", "");
        return;
    }
    auto colorMapStringValue = _settingsAction.getCommonOptionsHolder().getColoringMapSelectionAction().getColorMap();
    /*if (_settingsAction.getCommonOptionsHolder().getSelectionModeAction().isChecked())
    {
        
        QString colorDataset1ID = _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDatasetId();
        QString colorDataset2ID = _settingsAction.getDataset2OptionsHolder().getClusterDatasetAction().getCurrentDatasetId();
            scatterplotModeChange(scatterplot1Name, embeddingDataset1Id, colorDataset1ID, embeddingDimensionX1, embeddingDimensionY1, false, "", "", 0.0, 0.0);

            scatterplotModeChange(scatterplot2Name, embeddingDataset2Id, colorDataset2ID, embeddingDimensionX2, embeddingDimensionY2, false, "", "", 0.0, 0.0);

    }
    else*/
    {
        if(!_scatterplotClusterColoringDataset1.isValid() || !_scatterplotClusterColoringDataset2.isValid())
        {
            setStatusMessage("E", "No valid coloring datasets", "");
            return;
        }
        QString scatterplotColorDataset1Id = _scatterplotClusterColoringDataset1.getDatasetId();
        QString scatterplotColorDataset2Id = _scatterplotClusterColoringDataset2.getDatasetId();
        if (scatterplotColorDataset1Id.isEmpty() || scatterplotColorDataset2Id.isEmpty())
        {
            setStatusMessage("E", "No valid coloring datasets", "");
            return;
        }
        if (_selectedClusterIndices1.empty() || _selectedClusterIndices2.empty())
        {
            setStatusMessage("E", "No selected cluster indices for dataset 1 or 2", "");
            return;
        }
        
        //float minValLine = _settingsAction.getDataset1OptionsHolder().getLineplotLowerColoringRangeAction().getValue();
        //float maxValLine = _settingsAction.getDataset1OptionsHolder().getLineplotUpperColoringRangeAction().getValue();

        float minValScatter1 = _settingsAction.getDataset1OptionsHolder().getScatterplotLowerColoringRangeAction().getValue();
        float maxValScatter1 = _settingsAction.getDataset1OptionsHolder().getScatterplotUpperColoringRangeAction().getValue();

        float minValScatter2 = _settingsAction.getDataset2OptionsHolder().getScatterplotLowerColoringRangeAction().getValue();
        float maxValScatter2 = _settingsAction.getDataset2OptionsHolder().getScatterplotUpperColoringRangeAction().getValue();

        if (_settingsAction.getCommonOptionsHolder().getPcaColoringAction().isChecked())
        {
            if (!_pcaCommonGenesDataset1.isValid() || !_pcaCommonGenesDataset2.isValid())
            {
                setStatusMessage("E", "No valid PCA datasets", "");
                return;
            }
            
            QString pcaDataset1ID = _pcaCommonGenesDataset1.getDatasetId();
            QString pcaDataset2ID = _pcaCommonGenesDataset2.getDatasetId();
            if (pcaDataset1ID.isEmpty() || pcaDataset2ID.isEmpty())
            {
                setStatusMessage("E", "No valid PCA datasets", "");
                return;
            }
            
            QString selectedPCADimensionName = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionName();
            int selectedPCADimensionIndx = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();
            createScatterplotClusterColors(scatterplotColorDataset1Id, colorMapStringValue, _selectedClusterIndices1, pcaDataset1ID, totalPoints1, selectedPCADimensionName, minValScatter1, maxValScatter1);
            createScatterplotClusterColors(scatterplotColorDataset2Id, colorMapStringValue, _selectedClusterIndices2, pcaDataset2ID, totalPoints2, selectedPCADimensionName, minValScatter2, maxValScatter2);

            scatterplotModeChange(scatterplot1Name, embeddingDataset1Id, scatterplotColorDataset1Id, embeddingDimensionX1, embeddingDimensionY1, true, "", "", 0.0, 0.0);
            scatterplotModeChange(scatterplot2Name, embeddingDataset2Id, scatterplotColorDataset2Id, embeddingDimensionX1, embeddingDimensionY1, true, "", "", 0.0, 0.0);

        }
        else
        {
            

            if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()) {
                setStatusMessage("E", "No valid Point datasets", "");
                return;
            }


            QString geneDataset1ID = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId();
            QString geneDataset2ID = _settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId();
            if (geneDataset1ID.isEmpty() || geneDataset2ID.isEmpty())
            {
                setStatusMessage("E", "No valid PCA datasets", "");
                return;
            }
            
            QString selectedGeneName = _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
            int selectedGeneIndx = _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionIndex();
            /*if (!_settingsAction.getCommonOptionsHolder().getColorAllCellsWithExpressionAction().isChecked())
            {
                createScatterplotClusterColors(scatterplotColorDataset1Id, colorMapStringValue, _selectedClusterIndices1, geneDataset1ID, totalPoints1, selectedGeneName, minValScatter1, maxValScatter1);
                createScatterplotClusterColors(scatterplotColorDataset2Id, colorMapStringValue, _selectedClusterIndices2, geneDataset2ID, totalPoints2, selectedGeneName, minValScatter2, maxValScatter2);

                scatterplotModeChange(scatterplot1Name, embeddingDataset1Id, scatterplotColorDataset1Id, embeddingDimensionX1, embeddingDimensionY1, true, "", "", 0.0, 0.0);
                scatterplotModeChange(scatterplot2Name, embeddingDataset2Id, scatterplotColorDataset2Id, embeddingDimensionX1, embeddingDimensionY1, true, "", "", 0.0, 0.0);
            }
            else*/
            {
                scatterplotModeChange(scatterplot1Name, embeddingDataset1Id, geneDataset1ID, embeddingDimensionX1, embeddingDimensionY1, true, "", "", 0.0, 0.0);
                scatterplotModeChange(scatterplot2Name, embeddingDataset2Id, geneDataset2ID, embeddingDimensionX1, embeddingDimensionY1, true, "", "", 0.0, 0.0);
            }

        }



    }
}

void Computation::enableOptions()
{
    if (!_projectOpened || !_optionsDisabled)
        return;
    if (!_taskDisable && 0)
    {
        if (_foregroundTask.isRunning()) 
        {
            _foregroundTask.setFinished();
            _foregroundTask.setUndefined();
        }
    }


    _settingsAction.getCommonOptionsHolder().setDisabled(false);
    _settingsAction.getDataset1OptionsHolder().setDisabled(false);
    _settingsAction.getDataset2OptionsHolder().setDisabled(false);
    QApplication::processEvents();
    _optionsDisabled = false;
}

void Computation::disableOptions()
{
    if (!_projectOpened || _optionsDisabled)
        return;

    if (!_taskDisable && 0)
    {
        if (_foregroundTask.isRunning()) {
            _foregroundTask.setFinished();
            _foregroundTask.setUndefined();
        }
        _foregroundTask.setProgressDescription("Computation in progress. Please wait.");
        //_foregroundTask.setRunning();
        _foregroundTask.setRunningIndeterminate();
    }


    _settingsAction.getCommonOptionsHolder().setDisabled(true);
    _settingsAction.getDataset1OptionsHolder().setDisabled(true);
    _settingsAction.getDataset2OptionsHolder().setDisabled(true);
    QApplication::processEvents();
    _optionsDisabled = true;
}

/*void Computation::selectionModeAltToggleLogic(QString scatterplotName)
{
    //QString scatterplotName = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
    if (_pixelSelectionStartedConnection != QMetaObject::Connection{}) {
        disconnect(_pixelSelectionStartedConnection);
    }
    if (scatterplotName.isEmpty())
    {
        qWarning() << "No scatterplot view selected";
        return;
    }


    auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
    if (scatterplotViewFactory)
    {
        for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
        {
            if (plugin->getGuiName() == scatterplotName)
            {

                mv::gui::PixelSelectionAction* pixelSelectionAction = findActionByPath<mv::gui::PixelSelectionAction>(plugin, "Settings/Selection/Point Selection");
                if (pixelSelectionAction) {
                    PixelSelectionTool* pixelSelectionTool = pixelSelectionAction->getPixelSelectionTool();
                    if (pixelSelectionTool) {
                        _pixelSelectionStartedConnection = connect(pixelSelectionTool, &PixelSelectionTool::started, this, [this]() {
                            _settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(true);
                            });
                        //bool ok2 = connect(pixelSelectionTool, &PixelSelectionTool::ended, this, [this]() {
                            //qInfo() << "Pixel selection ended";
                            //});
                        //qDebug() << "Connect started:" << ok1 << "ended:" << ok2;
                    }
                    else {
                        qWarning() << "PixelSelectionTool is null!";
                    }
                }
                else
                {
                    qWarning() << "Pixel selection tool not found.";
                }
            }
        }
    }
    else
    {
        qWarning() << "Scatterplot View factory not found.";
    }
}*/

void Computation::findCommonGenes()
{
    if (!_settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset
    ().isValid() || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset
    ().isValid())
    {
        setStatusMessage("E", "No valid point datasets for finding common genes", "");
        return;
    }

    QStringList datasetNames;
    datasetNames.append(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    datasetNames.append(_settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());

    updateCommonGeneNames(datasetNames);
    int maxRankLimit = _commonGenesAcrossDatasets.size();

    auto& topNRowsAction = _settingsAction.getCommonOptionsHolder().getTopNRowsAction();
    auto& bottomNRowsAction = _settingsAction.getCommonOptionsHolder().getBottomNRowsAction();

    topNRowsAction.setMaximum(maxRankLimit);
    bottomNRowsAction.setMaximum(maxRankLimit);

    int topValue = topNRowsAction.getValue();
    int bottomValue = bottomNRowsAction.getValue();

    topNRowsAction.setValue(std::min(topValue, maxRankLimit));
    bottomNRowsAction.setValue(std::min(bottomValue, maxRankLimit));

}



void Computation::colorLinePlots()
{
    if (!_projectOpened) return;

    if (_isColoringLineplots) return;

    _isColoringLineplots = true;
    const QString linePlotName = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();

    // Early exit for invalid or mismatched datasets
    if (!_lineChartPointDataset1.isValid() || linePlotName.isEmpty() ||
        _filteredSelectionIndices.empty() || !_selectedCellPointDataset.isValid() ||
        _lineChartPointDataset1->getNumPoints() < 1 ||
        _selectedCellPointDataset->getNumPoints() != _lineChartPointDataset1->getNumPoints())
    {
        clearLinePlots();
        setStatusMessage("E", "No valid line chart dataset or line plot name", "");
        _isColoringLineplots = false;
        return;
    }

    const int totalSelectedPoints = _selectedCellPointDataset->getNumPoints();
    constexpr auto smoothingType = "Moving Average";
    QString xPositionString = "Rotated Y Coordinates";
    const int smoothingWindow = static_cast<int>(std::round(totalSelectedPoints * 0.2));
    QString yPositionString, titleText, colorDatasetID, colordatasetType;
    int colordimensionIndx = -1;
    float minValue = 0.0f, maxValue = 0.0f;

    auto& commonOptions = _settingsAction.getCommonOptionsHolder();
    auto& dataset1Options = _settingsAction.getDataset1OptionsHolder();
    const auto colorMapStringValue = commonOptions.getColoringMapSelectionAction().getColorMap();

    /*if (commonOptions.getSelectionModeAction().isChecked())
    {
        if (_lineChartClusterColoringDataset1.isValid())
        {
            colorDatasetID = _lineChartClusterColoringDataset1.getDatasetId();
            auto dataType = _lineChartClusterColoringDataset1->getDataType();
            colordatasetType = (dataType == PointType) ? "Points" : (dataType == ClusterType) ? "Clusters" : "";
            const QString referenceClusterDatasetID = dataset1Options.getClusterDatasetAction().getCurrentDatasetId();
            if (referenceClusterDatasetID.isEmpty())
            {
                setStatusMessage("E", "No valid cluster dataset selected for coloring", "");
                _isColoringLineplots = false;
                return;
            }
            createClusterLineChartClusterColors(colorDatasetID, referenceClusterDatasetID, _filteredSelectionIndices);
            yPositionString = "Mean Expr All Genes";
            titleText = QString("%1 vs %2 expression of selected cells (%3)")
                .arg(xPositionString, yPositionString)
                .arg(_filteredSelectionIndices.size());
        }
    }
    else*/
    {
        if (commonOptions.getPcaColoringAction().isChecked())
        {
            if (!_pcaSelectedEmbeddingDataset.isValid())
            {
                setStatusMessage("E", "No valid PCA dataset selected for coloring", "");
                _isColoringLineplots = false;
                return;
            }
            colorDatasetID = _pcaSelectedEmbeddingDataset.getDatasetId();
            auto dataType = _pcaSelectedEmbeddingDataset->getDataType();
            colordatasetType = (dataType == PointType) ? "Points" : (dataType == ClusterType) ? "Clusters" : "";

            const QString selectedPCADimensionName = commonOptions.getPcaDimensionForColoringAction().getCurrentDimensionName();
            const auto& dimnames = _pcaSelectedEmbeddingDataset->getDimensionNames();
            auto it = std::find(dimnames.begin(), dimnames.end(), selectedPCADimensionName);
            colordimensionIndx = (it != dimnames.end()) ? static_cast<int>(std::distance(dimnames.begin(), it)) : -1;
            if (colordimensionIndx >= 0 && colordimensionIndx < _pcaSelectedEmbeddingDataset->getNumDimensions())
            {
                const int numbpts = _pcaSelectedEmbeddingDataset->getNumPoints();
                std::vector<float> selectedData(numbpts);
                _pcaSelectedEmbeddingDataset->extractDataForDimension(selectedData, colordimensionIndx);
                if (!selectedData.empty()) {
                    auto [minIt, maxIt] = std::minmax_element(selectedData.begin(), selectedData.end());
                    minValue = *minIt;
                    maxValue = *maxIt;
                }
            }
            yPositionString = "Mean Expr All Genes";
            titleText = QString("%1 vs %2 expression of selected cells (%3)")
                .arg(xPositionString, yPositionString)
                .arg(_filteredSelectionIndices.size());
        }
        else
        {
            const QString selectedGene = dataset1Options.getGeneSelectionAction().getCurrentDimensionName();
            if (!_lineChartPointDataset1.isValid())
            {
                setStatusMessage("E", "No valid point dataset selected for coloring", "");
                _isColoringLineplots = false;
                return;
            }
            if (!selectedGene.isEmpty())
            {
                colorDatasetID = _lineChartPointDataset1.getDatasetId();
                auto dataType = _lineChartPointDataset1->getDataType();
                colordatasetType = (dataType == PointType) ? "Points" : (dataType == ClusterType) ? "Clusters" : "";
                yPositionString = selectedGene;

                const auto& dimnames = _lineChartPointDataset1->getDimensionNames();
                auto it = std::find(dimnames.begin(), dimnames.end(), selectedGene);
                colordimensionIndx = (it != dimnames.end()) ? static_cast<int>(std::distance(dimnames.begin(), it)) : -1;
                if (colordimensionIndx >= 0 && colordimensionIndx < _lineChartPointDataset1->getNumDimensions())
                {
                    const int numbpts = _lineChartPointDataset1->getNumPoints();
                    std::vector<float> selectedData(numbpts);
                    _lineChartPointDataset1->extractDataForDimension(selectedData, colordimensionIndx);
                    if (!selectedData.empty()) {
                        minValue = percentile(selectedData, _settingsAction.getCommonOptionsHolder().getLowRangeColoringPercentileAction().getValue()); 
                        maxValue = percentile(selectedData, _settingsAction.getCommonOptionsHolder().getHighRangeColoringPercentileAction().getValue()); 
                    }
                }
                titleText = QString("%1 vs %2 expression of selected cells (%3)")
                    .arg(xPositionString, yPositionString)
                    .arg(_filteredSelectionIndices.size());
            }
            else
            {
                xPositionString= "Empty";
                colordimensionIndx = 0;
                yPositionString = "Empty";
                titleText = QString("Gene unavailable for selection dataset");
            }

        }
    }

    dataset1Options.getLineplotLowerColoringRangeAction().setValue(minValue);
    dataset1Options.getLineplotUpperColoringRangeAction().setValue(maxValue);

    lineplotModeChange(
        linePlotName,
        _lineChartPointDataset1.getDatasetId(),
        colorDatasetID,
        colordatasetType,
        colordimensionIndx,
        xPositionString,
        yPositionString,
        titleText,
        smoothingType,
        smoothingWindow,
        colorMapStringValue,
        minValue,
        maxValue
    );
    mv::events().notifyDatasetDataChanged(_lineChartPointDataset1);
    _isColoringLineplots = false;
}

void Computation::colorScatterPlots()
{
    if (!_projectOpened) return;
    if (_isColoringScatterplots) return;
    _isColoringScatterplots = true;
    // Validate embedding datasets
    auto& ds1Opts = _settingsAction.getDataset1OptionsHolder();
    auto& ds2Opts = _settingsAction.getDataset2OptionsHolder();
    auto& commonOpts = _settingsAction.getCommonOptionsHolder();

    const auto& embDs1 = ds1Opts.getEmbeddingDatasetAction().getCurrentDataset();
    const auto& embDs2 = ds2Opts.getEmbeddingDatasetAction().getCurrentDataset();
    if (!embDs1.isValid() || !embDs2.isValid()) {
        setStatusMessage("E", "No valid embedding datasets", "");
        _isColoringScatterplots = false;
        return;
    }


    if (!_embeddingDataset1.isValid() || !_embeddingDataset2.isValid()) {
        setStatusMessage("E", "No valid embedding datasets", "");
        _isColoringScatterplots = false;
        return;
    }

    const QString embeddingDataset1Id = _embeddingDataset1.getDatasetId();
    const QString embeddingDataset2Id = _embeddingDataset2.getDatasetId();
    if (embeddingDataset1Id.isEmpty() || embeddingDataset2Id.isEmpty()) {
        setStatusMessage("E", "No valid dataset ids", "");
        _isColoringScatterplots = false;
        return;
    }

    // Validate embedding dimensions
    const QString embeddingDimensionX1 = ds1Opts.getEmbeddingDimensionXAction().getCurrentDimensionName();
    const QString embeddingDimensionY1 = ds1Opts.getEmbeddingDimensionYAction().getCurrentDimensionName();
    const QString embeddingDimensionX2 = ds2Opts.getEmbeddingDimensionXAction().getCurrentDimensionName();
    const QString embeddingDimensionY2 = ds2Opts.getEmbeddingDimensionYAction().getCurrentDimensionName();
    if (embeddingDimensionX1.isEmpty() || embeddingDimensionY1.isEmpty() ||
        embeddingDimensionX2.isEmpty() || embeddingDimensionY2.isEmpty()) {
        setStatusMessage("E", "No valid embedding dimensions selected", "");
        _isColoringScatterplots = false;
        return;
    }

    // Validate scatterplot names
    const QString scatterplot1Name = ds1Opts.getScatterplotViewSelectionAction().getCurrentText();
    const QString scatterplot2Name = ds2Opts.getScatterplotViewSelectionAction().getCurrentText();
    if (scatterplot1Name.isEmpty() || scatterplot2Name.isEmpty()) {
        setStatusMessage("E", "No valid plot names selected", "");
        _isColoringScatterplots = false;
        return;
    }

    // Prepare coloring range actions and color map
    auto& minValScatter1 = ds1Opts.getScatterplotLowerColoringRangeAction();
    auto& maxValScatter1 = ds1Opts.getScatterplotUpperColoringRangeAction();
    auto& minValScatter2 = ds2Opts.getScatterplotLowerColoringRangeAction();
    auto& maxValScatter2 = ds2Opts.getScatterplotUpperColoringRangeAction();
    const auto colorMapStringValue = commonOpts.getColoringMapSelectionAction().getColorMap();

    float minVal1 = 0.0f, maxVal1 = 0.0f, minVal2 = 0.0f, maxVal2 = 0.0f;
    QString coloringdimension;

    /*if (commonOpts.getSelectionModeAction().isChecked()) */
    if(_selectionModeEnable)
    {
        // Selection mode: color by cluster datasets
        const QString colorDataset1ID = ds1Opts.getClusterDatasetAction().getCurrentDatasetId();
        const QString colorDataset2ID = ds2Opts.getClusterDatasetAction().getCurrentDatasetId();

        scatterplotModeChange(scatterplot1Name, embeddingDataset1Id, colorDataset1ID,
            embeddingDimensionX1, embeddingDimensionY1, false,
            coloringdimension, colorMapStringValue, minVal1, maxVal1);

        scatterplotModeChange(scatterplot2Name, embeddingDataset2Id, colorDataset2ID,
            embeddingDimensionX2, embeddingDimensionY2, false,
            coloringdimension, colorMapStringValue, minVal2, maxVal2);
        _isColoringScatterplots = false;
        return;
    }
    

    // Non-selection mode: color by point coloring datasets
    if (!_scatterplotPointColoringDataset1.isValid() || !_scatterplotPointColoringDataset2.isValid()) {
        setStatusMessage("E", "No valid coloring datasets", "");
        _isColoringScatterplots = false;
        return;
    }
    const QString scatterplotColorDataset1Id = _scatterplotPointColoringDataset1.getDatasetId();
    const QString scatterplotColorDataset2Id = _scatterplotPointColoringDataset2.getDatasetId();
    coloringdimension = "ColorDim";
    if (scatterplotColorDataset1Id.isEmpty() || scatterplotColorDataset2Id.isEmpty()) {
        setStatusMessage("E", "No valid coloring datasets", "");
        _isColoringScatterplots = false;
        return;
    }
    if (_selectedClusterIndices1.empty() || _selectedClusterIndices2.empty()) {
        setStatusMessage("E", "No selected cluster indices for dataset 1 or 2", "");
        qCritical() << "Here3";
        _isColoringScatterplots = false;
        return;
    }
    std::pair<float, float> sentinelValMinMax1{ minValScatter1.getMinimum(), maxValScatter1.getMaximum() };
    std::pair<float, float> sentinelValMinMax2{ minValScatter2.getMinimum(), maxValScatter2.getMaximum() };
    float lowRangePercentile = commonOpts.getLowRangeColoringPercentileAction().getValue();
    float highRangePercentile = commonOpts.getHighRangeColoringPercentileAction().getValue();
    // Determine coloring source: PCA or gene
    if (commonOpts.getPcaColoringAction().isChecked()) {
        if (!_pcaCommonGenesDataset1.isValid() || !_pcaCommonGenesDataset2.isValid()) {
            setStatusMessage("E", "No valid PCA datasets", "");
            _isColoringScatterplots = false;
            return;
        }
        const QString pcaDataset1ID = _pcaCommonGenesDataset1.getDatasetId();
        const QString pcaDataset2ID = _pcaCommonGenesDataset2.getDatasetId();
        if (pcaDataset1ID.isEmpty() || pcaDataset2ID.isEmpty()) {
            setStatusMessage("E", "No valid PCA datasets", "");
            _isColoringScatterplots = false;
            return;
        }
        const QString selectedPCADimensionName = commonOpts.getPcaDimensionForColoringAction().getCurrentDimensionName();

       auto value1= createScatterplotPointColors(scatterplotColorDataset1Id, colorMapStringValue, _selectedClusterIndices1,
            pcaDataset1ID, _embeddingDataset1->getNumPoints(),
            selectedPCADimensionName, minVal1, maxVal1, false, sentinelValMinMax1, lowRangePercentile,highRangePercentile);
       auto value2 = createScatterplotPointColors(scatterplotColorDataset2Id, colorMapStringValue, _selectedClusterIndices2,
            pcaDataset2ID, _embeddingDataset2->getNumPoints(),
            selectedPCADimensionName, minVal2, maxVal2, false, sentinelValMinMax2, lowRangePercentile, highRangePercentile);
       minVal1 = value1.first;
       maxVal1 = value1.second;
       minVal2 = value2.first;
       maxVal2 = value2.second;;
    }
    else {
        if (!_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()  || !_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid()) {
            setStatusMessage("E", "No valid Point datasets", "");
            _isColoringScatterplots = false;
            return;
        }

        const QString geneDataset1ID = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId();
        const QString geneDataset2ID = _settingsAction.getDataset2OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId();
        if (geneDataset1ID.isEmpty() || geneDataset2ID.isEmpty()) {
            setStatusMessage("E", "No valid PCA datasets", "");
            _isColoringScatterplots = false;
            return;
        }
        const QString selectedGeneName1 = ds1Opts.getGeneSelectionAction().getCurrentDimensionName();
        const QString selectedGeneName2 = ds2Opts.getGeneSelectionAction().getCurrentDimensionName();
        auto value1 = createScatterplotPointColors(scatterplotColorDataset1Id, colorMapStringValue, _selectedClusterIndices1,
            geneDataset1ID, _embeddingDataset1->getNumPoints(),
            selectedGeneName1, minVal1, maxVal1, true, sentinelValMinMax1, lowRangePercentile, highRangePercentile);
        auto value2 = createScatterplotPointColors(scatterplotColorDataset2Id, colorMapStringValue, _selectedClusterIndices2,
            geneDataset2ID, _embeddingDataset2->getNumPoints(),
            selectedGeneName2, minVal2, maxVal2, true, sentinelValMinMax2, lowRangePercentile, highRangePercentile);
        minVal1 = value1.first;
        maxVal1 = value1.second;
        minVal2 = value2.first;
        maxVal2 = value2.second;;
    }

    // Apply coloring and update ranges
    scatterplotModeChange(scatterplot1Name, embeddingDataset1Id, scatterplotColorDataset1Id,
        embeddingDimensionX1, embeddingDimensionY1, true,
        coloringdimension, colorMapStringValue, minVal1, maxVal1);
    scatterplotModeChange(scatterplot2Name, embeddingDataset2Id, scatterplotColorDataset2Id,
        embeddingDimensionX1, embeddingDimensionY1, true,
        coloringdimension, colorMapStringValue, minVal2, maxVal2);

    minValScatter1.setValue(minVal1);
    maxValScatter1.setValue(maxVal1);
    minValScatter2.setValue(minVal2);
    maxValScatter2.setValue(maxVal2);
    _isColoringScatterplots = false;
}

