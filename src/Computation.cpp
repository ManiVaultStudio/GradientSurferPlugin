#include "Computation.h"
#include "ComputationUtils.h"

#include <iostream>
#include <set>
#include "LayerSurferPlugin.h"
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

Computation::Computation(LayerSurferPlugin& LayerSurferPlugin, SettingsAction& SettingsAction)
    : WidgetAction(&LayerSurferPlugin, "Computation"),
    _viewerPlugin(LayerSurferPlugin),
    _settingsAction(SettingsAction)
{

    _correlationRangeDebounceTimer1.setSingleShot(true);
    _clusterSelectionDebounceTimer.setSingleShot(true);
    _correlationRangeDebounceTimer2.setSingleShot(true);


    qInstallMessageHandler(customMessageHandler);
    const auto updateStatus = [this]() -> void {
        auto stringFull = _settingsAction.getStatusHolder().getStatusMessageAction().getString();
        StatusInfo info = parseStatusMessage(stringFull);

        auto [font,iconSize]=getStatusLabelFontAndIconSize(info.message);

        QColor iconColor(info.textColor);
        mv::util::StyledIcon icon(info.iconName);
        icon.withColor(iconColor);

        QPixmap iconPixmap = QIcon(icon).pixmap(iconSize, iconSize);

        QLabel* statusLabel = createStatusLabel(info.message, info.backgroundColor, info.textColor, iconPixmap, font);
        QStatusBar* statusBar = _settingsAction.getStatusHolder().getStatusBarAction();
        if (statusBar) {
            QList<QWidget*> children = statusBar->findChildren<QWidget*>();
            for (QWidget* child : children) {
                statusBar->removeWidget(child);
                child->deleteLater();
            }
            statusBar->addWidget(statusLabel, 1);
        }
        };
    connect(&_settingsAction.getStatusHolder().getStatusMessageAction(), &StringAction::stringChanged, this, updateStatus);
    const auto plotSelectionOptions = [this]() -> void
        {
            QStringList scatterplotNames = {};
            QStringList lineplotNames = {};
            QString selectedCurrentText1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
            QString selectedCurrentText2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
            auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
            if (scatterplotViewFactory)
            {

                for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
                {
                    scatterplotNames.append(plugin->getGuiName());
                }
            }

            QString lineplotView = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();
            auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
            if (lineplotViewFactory)
            {
                for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
                {
                    lineplotNames.append(plugin->getGuiName());
                }
            }

            selectedCurrentText1 = selectValidOption(scatterplotNames, selectedCurrentText1, 0);
            selectedCurrentText2 = selectValidOption(scatterplotNames, selectedCurrentText2, 1);


            lineplotView = selectValidOption(lineplotNames, lineplotView, 0);

            _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().setOptions(scatterplotNames);
            _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().setCurrentText(selectedCurrentText1);
            _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().setOptions(scatterplotNames);
            _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().setCurrentText(selectedCurrentText2);


            _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().setOptions(lineplotNames);
            _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().setCurrentText(lineplotView);

        };

    connect(&mv::plugins(), &AbstractPluginManager::pluginAdded, this, plotSelectionOptions);


    _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) -> bool {
            return isValidClusterDataset(dataset, _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset());
        }
    );

    _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) -> bool {
            return isValidPointDataset(dataset);
        }
    );

    _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) -> bool {
            return isValidClusterDataset(dataset, _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset());
        }
    );

    _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) -> bool {
            return isValidPointDataset(dataset);
        }
    );

    _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) -> bool {
            return isValidPointDataset(dataset);
        }
    );

    _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().setFilterFunction(
        [this](mv::Dataset<DatasetImpl> dataset) -> bool {
            return isValidPointDataset(dataset);
        }
    );

    const auto updateClusterDataset1 = [this]() -> void {
        updateClusterOptions();
        setStatusMessage("M", "Cluster dataset updated", "");
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset1);

    const auto updateClusterDataset2 = [this]() -> void {
        updateClusterOptions();
        setStatusMessage("M", "Cluster dataset updated", "");
        };

    connect(&_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updateClusterDataset2);

    const auto updatePointDataset1 = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid())
        {
            auto dataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
            if (dataset.isValid())
            {
                 _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setPointsDataset(dataset);
                auto dimensions = dataset->getNumDimensions();
                
            }


        }
        else
        {
             _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setPointsDataset({});
        }
        setStatusMessage("M", "Point dataset 1 updated", "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getPointDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset1);

    const auto updatePointDataset2 = [this]() -> void {
        if (_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().isValid())
        {
            auto dataset = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId());
            if (dataset.isValid())
            {
                _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setPointsDataset(dataset);
                auto dimensions = dataset->getNumDimensions();

            }


        }
        else
        {
            _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setPointsDataset({});
        }
        setStatusMessage("M", "Point dataset 2 updated", "");
        };

    connect(&_settingsAction.getDataset2OptionsHolder().getPointDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updatePointDataset2);

    const auto updateGeneSelection1 = [this]() -> void {
        auto selectedGene = _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().getCurrentDimensionName();
        if (selectedGene == "")
        {
            modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), false, "");
        }
        else
        {
            modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), true, selectedGene);

            if (_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().isValid() && _filteredSelectionIndices.size() > 0)
            {
                //TODO
                auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
                if (lineplotViewFactory)
                {
                    auto lineplotView = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();
                    if (lineplotView.isEmpty())
                    {
                        qWarning() << "No scatterplot view 2 selected";
                    }


                    for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
                    {

                        if (plugin->getGuiName() == lineplotView)
                        {

                            _pacColoringPaused = true;
                            _settingsAction.getCommonOptionsHolder().getPcaColoringAction().setChecked(false);

                            mv::gui::DatasetPickerAction* pointDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Point dataset");
                            mv::gui::DatasetPickerAction* clusterDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Color dataset");
                            if (pointDatasetAction && _lineChartPointDataset1.isValid())
                            {
                                pointDatasetAction->setCurrentDataset(_lineChartPointDataset1);

                                if (clusterDatasetAction && _lineChartColorClusterDataset1.isValid())
                                {
                                    clusterDatasetAction->setCurrentDataset(_lineChartColorClusterDataset1);
                                }
                                mv::gui::StringAction* chartTitle = findActionByPath<StringAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Chart Title");
                                if (chartTitle)
                                {
                                    QString titleText = QString("%1 vs %2 expression of selected cells (%3)")
                                        .arg(QStringLiteral("Rotated Y Coordinates"))
                                        .arg(selectedGene)
                                        .arg(static_cast<int>(_filteredSelectionIndices.size()));
                                    chartTitle->setString(titleText);

                                }


                                mv::gui::OptionAction* dimensionX = findActionByPath<OptionAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Data Dimension X Selection/Select dimension");
                                mv::gui::OptionAction* dimensionY = findActionByPath<OptionAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Data Dimension Y Selection/Select dimension");
                                if (dimensionX)
                                {
                                    dimensionX->setCurrentText("Rotated Y Coordinates");
                                }
                                if (dimensionY)
                                {
                                    dimensionY->setCurrentText(selectedGene);
                                }

                                mv::gui::OptionAction* smoothingType = findActionByPath<OptionAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Smoothing Type");
                                mv::gui::IntegralAction* smoothingWindow = findActionByPath<IntegralAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Smoothing Window");
                                if (smoothingType && smoothingWindow)
                                {
                                    smoothingType->setCurrentText("Moving Average");
                                    int maxVal = _lineChartPointDataset1->getNumPoints() - 1;
                                    int currentVal = static_cast<int>(std::round(maxVal * 0.2));
                                    smoothingWindow->setMaximum(maxVal);
                                    smoothingWindow->setMinimum(2);
                                    smoothingWindow->setValue(currentVal);
                                }



                                events().notifyDatasetDataChanged(_lineChartPointDataset1);

                            }
                            _pacColoringPaused = false;

                        }
                    }
                }

            }


        }
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, updateGeneSelection1);


    const auto updateGeneSelection2 = [this]() -> void {

        auto selectedGene2 = _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().getCurrentDimensionName();
        if (selectedGene2 == "")
        {
            modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(),false,"");
        }
        else
        {
            modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(),true, selectedGene2);
        }
        

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getGeneSelection2Action(), &DimensionPickerAction::currentDimensionIndexChanged, this, updateGeneSelection2);


    const auto updateEmbeddingDataset1 = [this]() -> void {
        if ( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().isValid())
        {


            _embeddingDataset = mv::data().getDataset<Points>( _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());

            if (_embeddingDataset.isValid())
            {
                modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), "", "", "", false, "");
                               
                auto parent = _embeddingDataset->getParent();
                if (parent.isValid())
                {
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
                    if (clusterDatasets.contains(selectedClusterDatasetName))
                    {
                        _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setCurrentDataset(selectedClusterDatasetName);
                    }
                    else if (clusterDatasets.size() > 0)
                    {
                        _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setCurrentDataset(clusterDatasets.first());
                    }
                }
                else
                {
                    _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().setDatasets({});
                }
                
                
                
                _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setPointsDataset(_embeddingDataset);
                 _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setPointsDataset(_embeddingDataset);
                 if (_embeddingDataset->getNumDimensions() >= 2)
                 {
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(0);
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(1);
                    }
                    else if (_embeddingDataset->getNumDimensions() == 1)
                    {
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(0);
                        _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(0);
                    }
                    else
                 {
                     _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().setCurrentDimensionIndex(-1);
                     _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().setCurrentDimensionIndex(-1);
                 }
                const auto mainSelectionChanged = [this]() -> void {

                    if (_blockSelectionTrigger)
                    {
                        qDebug() << "Selection change blocked, returning early";
                        //_blockSelectionTrigger = false;
                        return;
                    }
  
                    setStatusMessage("M", "Selected indices changed", "");
                    setStatusMessage("R", "Computation in progress", "");
                    _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
                    QApplication::processEvents();
                    setSelectionAndAutoCompute();

                    _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
                    };


                bool ok = connect(&_embeddingDataset, &Dataset<Points>::dataSelectionChanged, this, mainSelectionChanged);
                if (!ok)
                {
                    qWarning() << "Failed to connect to dataSelectionChanged signal of fullEmbeddingDataset";
                }

            }


        }

        setStatusMessage("M", "Embedding dataset 1 updated", "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset1);

    const auto updateEmbeddingDataset2 = [this]() -> void {
        if (_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().isValid())
        {
            

            auto embeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId());

            if (embeddingDataset2.isValid())
            {
                modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), "", "", "", false, "");
                auto parent = embeddingDataset2->getParent();
                if (parent.isValid())
                {
                    _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().setCurrentDataset(parent);
                    auto children = parent->getChildren();
                    Datasets clusterDatasets;
                    for (const Dataset<Clusters>& child : children) {
                        if (child->getDataType() == ClusterType && (child->getGroupIndex() != 667|| child->getGroupIndex() != 666)) {
                            clusterDatasets.append(child);
                            
                        }
                    }

                    auto selectedClusterDatasetName = _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset();
                    _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setDatasets(clusterDatasets);
                    if (clusterDatasets.contains(selectedClusterDatasetName))
                    {
                        _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setCurrentDataset(selectedClusterDatasetName);
                    }
                    else if (clusterDatasets.size() > 0)
                    {
                        _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setCurrentDataset(clusterDatasets.first());
                    }
                }
                else
                {
                    _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().setDatasets({});
                }
                
                _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setPointsDataset(embeddingDataset2);
                _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setPointsDataset(embeddingDataset2);
                if (embeddingDataset2->getNumDimensions() >= 2)
                {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setCurrentDimensionIndex(0);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setCurrentDimensionIndex(1);
                }
                else if (embeddingDataset2->getNumDimensions() == 1)
                {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setCurrentDimensionIndex(0);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setCurrentDimensionIndex(0);
                }
                else
                {
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().setCurrentDimensionIndex(-1);
                    _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().setCurrentDimensionIndex(-1);
                }
            }


        }

        setStatusMessage("M", "Embedding dataset 2 updated", "");
        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action(), &DatasetPickerAction::currentIndexChanged, this, updateEmbeddingDataset2);

    const auto computeInfo = [this]() -> void {

        setStatusMessage("R", "Correlation computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        correlationAfterPCA(); 

        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        };

    connect(&_settingsAction.getCommonOptionsHolder().getComputeAction(), &TriggerAction::triggered, this, computeInfo);

    const auto correlationCalculationTypeActionChanged = [this]() -> void {

        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        setSelectionAndAutoCompute();

        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        };

    connect(&_settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction(), &OptionAction::currentIndexChanged, this, correlationCalculationTypeActionChanged);


    const auto computePCATrigger = [this]() -> void {

        setStatusMessage("R", "PCA Computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputePCAAction().setDisabled(true);
        QApplication::processEvents();
  
        ManualCompute();

        _settingsAction.getCommonOptionsHolder().getComputePCAAction().setDisabled(false);
        };

    connect(&_settingsAction.getCommonOptionsHolder().getComputePCAAction(), &TriggerAction::triggered, this, computePCATrigger);


    const auto pcaColorsToPlots = [this]() -> void {

        if (_pacColoringPaused) {
            qDebug() << "PCA coloring paused, returning early";
            return;
        }



        if (_settingsAction.getCommonOptionsHolder().getPcaColoringAction().isChecked())
        {
            QString colorDatasetID1 = "";
            QString colorDatasetID2 = "";
            QString selectedDimName = ""; 
            colorDatasetID1 = _pcaCommonGenesDataset1.getDatasetId();
            colorDatasetID2 = _pcaCommonGenesDataset2.getDatasetId();
            _blockSelectionMode = true;
            _settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(false);
            _blockSelectionMode = false;

            selectedDimName = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionName();
            modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _selectedEmbeddingPointDataset1.getDatasetId(), colorDatasetID1, _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), true, selectedDimName);

            modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _selectedEmbeddingPointDataset2.getDatasetId(), colorDatasetID2, _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(), true, selectedDimName);

        }
        else
        {
            _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);
            _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionIndex(-1);
            deselectAllRows(_settingsAction.getTableView1Action());
            deselectAllRows(_settingsAction.getTableView2Action());
            deselectAllRows(_settingsAction.getTableView3Action());

            modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), false, "");

            modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(), false, "");
        }





        
        auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
        if (lineplotViewFactory)
        {
            auto lineplotView = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();
            if (!lineplotView.isEmpty())
            {
                for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
                {

                    if (plugin->getGuiName() == lineplotView)
                    {
                        mv::gui::DatasetPickerAction* pointDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Point dataset");
                        mv::gui::DatasetPickerAction* clusterDatasetAction = findActionByPath<DatasetPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Color dataset");
                        if (pointDatasetAction && _lineChartPointDataset1.isValid())
                        {
                            pointDatasetAction->setCurrentDataset(_lineChartPointDataset1);
                            if (_settingsAction.getCommonOptionsHolder().getPcaColoringAction().isChecked())
                            {
                                if (clusterDatasetAction && _lineChartColorPointDataset1.isValid())
                                {
                                    clusterDatasetAction->setCurrentDataset(_lineChartColorPointDataset1);
                                    DimensionPickerAction* colorDim = findActionByPath<DimensionPickerAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Color Point Dataset Dimension");
                                    if (colorDim)
                                    {
                                        int selectedDimInd = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();
                                        QString selectedDimName = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionName();
                                        colorDim->setCurrentDimensionIndex(selectedDimInd);
                                    }
                                    else
                                    {
                                        qWarning() << "LinePlot color dimension action not found";
                                    }
                                }
                                else
                                {
                                    qWarning() << "LinePlot color dataset action not found";
                                }
                            }
                            else
                            {
                                if (clusterDatasetAction && _lineChartColorClusterDataset1.isValid())
                                {
                                    clusterDatasetAction->setCurrentDataset(_lineChartColorClusterDataset1);
                                }
                                else
                                {
                                    qWarning() << "LinePlot color dataset action not found";
                                }
                            }

                            int maxVal = _lineChartPointDataset1->getNumPoints();
                            mv::gui::OptionAction* smoothingType = findActionByPath<OptionAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Smoothing Type");
                            mv::gui::IntegralAction* smoothingWindow = findActionByPath<IntegralAction>(plugin, "LinePlotViewPlugin/Dataset1 Options/Smoothing Window");
                            if (smoothingType && smoothingWindow)
                            {
                                smoothingType->setCurrentText("Moving Average");
                                int maxVal = _lineChartPointDataset1->getNumPoints()-1;
                                int currentVal = static_cast<int>(std::round(maxVal * 0.2));
                                smoothingWindow->setMaximum(maxVal);
                                smoothingWindow->setMinimum(2);
                                smoothingWindow->setValue(currentVal);
                            }

                            events().notifyDatasetDataChanged(_lineChartPointDataset1);
                        }
                        else
                        {
                            qWarning() << "LinePlot point dataset action not found";
                        }

                    }
                }
                
            }
            else
            {
                qWarning() << "No lineplot view selected";
            }


        }
        else
        {
            qWarning() << "LinePlot View plugin not found";
        }

        };

    connect(&_settingsAction.getCommonOptionsHolder().getPcaColoringAction(), &ToggleAction::toggled, this, pcaColorsToPlots);

    const auto selectionModeToggled = [this]()
        {
            if(_blockSelectionMode)
                {
                qDebug() << "Selection mode change blocked, returning early";
                return;
            }
            if (_settingsAction.getCommonOptionsHolder().getSelectionModeAction().isChecked())
            {
                _pacColoringPaused = true;
                _settingsAction.getCommonOptionsHolder().getPcaColoringAction().setChecked(false);
                _pacColoringPaused = false;
                _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction().setCurrentDimensionIndex(-1);
                _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action().setCurrentDimensionIndex(-1);
                deselectAllRows(_settingsAction.getTableView1Action());
                deselectAllRows(_settingsAction.getTableView2Action());
                deselectAllRows(_settingsAction.getTableView3Action());

                modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(),_settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), false, "");

                modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(), false,"");
            }
            else
            {
                QString colorDatasetID1 = _selectedGenePointDataset1.getDatasetId();
                QString colorDatasetID2 = _selectedGenePointDataset2.getDatasetId();
                QString selectedDimName = "";
                
                modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _selectedEmbeddingPointDataset1.getDatasetId(), colorDatasetID1, _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), true, selectedDimName);

                modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _selectedEmbeddingPointDataset2.getDatasetId(), colorDatasetID2, _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(), true, selectedDimName);



                    selectFirstRow(_settingsAction.getTableView1Action());

            }


        };

    connect(&_settingsAction.getCommonOptionsHolder().getSelectionModeAction(), &ToggleAction::toggled, this, selectionModeToggled);


    connect(&_settingsAction.getCommonOptionsHolder().getDualPCAColoringAction(), &OptionsAction::selectedOptionsChanged, this, pcaColorsToPlots);

    connect(&_settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, pcaColorsToPlots);

    connect(& _settingsAction.getCommonOptionsHolder().getClusterSelectionAction(),
        &OptionsAction::selectedOptionsChanged,
        this,
        [this]() {
            _clusterSelectionDebounceTimer.start(800);
        });
    connect(&_clusterSelectionDebounceTimer, &QTimer::timeout, this, [this]() {
        setStatusMessage("R", "Computation in progress", "");
        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
        QApplication::processEvents();

        
        setSelectionAndAutoCompute();

        _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        });



    connect(& _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction(),
        &DecimalRangeAction::rangeChanged,
        this,
        [this]() {
            _correlationRangeDebounceTimer1.start(1000);
        });
    connect(&_correlationRangeDebounceTimer1, &QTimer::timeout, this, [this]() {
        setStatusMessage("R", "Updating correlation 1 table", "");
        resetupCorrelationTable(_geneCorrValues1, _geneOfInterestIndices1,_settingsAction.getTableView1Action(), _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMinimum(), _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMaximum(),_settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), _commonGeneNames, _settingsAction.getTableView2Action(), _settingsAction.getTableView3Action(), _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action());
        selectFirstRow(_settingsAction.getTableView1Action());
        setStatusMessage("C", "Correlation table 1 updated", "");
        if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked())
        {
            setStatusMessage("R", "Computation in progress", "");
            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
            QApplication::processEvents();

            ManualCompute();

            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        }
        });


    connect(&_settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action(),
        &DecimalRangeAction::rangeChanged,
        this,
        [this]() {
            _correlationRangeDebounceTimer2.start(300);
        });
    connect(&_correlationRangeDebounceTimer2, &QTimer::timeout, this, [this]() {
        setStatusMessage("R", "Updating correlation 2 table", "");
        resetupCorrelationTable(_geneCorrValues2, _geneOfInterestIndices2,_settingsAction.getTableView2Action(), _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMinimum(), _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMaximum(),_settingsAction.getDataset2OptionsHolder().getGeneSelection2Action(),_commonGeneNames, _settingsAction.getTableView1Action(), _settingsAction.getTableView3Action(),_settingsAction.getDataset1OptionsHolder().getGeneSelectionAction());
        resetupCorrelationTable(_geneCorrValues3, _geneOfInterestIndices3,_settingsAction.getTableView3Action(), _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMinimum(), _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMaximum(),_settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), _commonGeneNames, _settingsAction.getTableView1Action(), _settingsAction.getTableView2Action(), _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action());
        selectFirstRow(_settingsAction.getTableView3Action());
        setStatusMessage("C", "Correlation table 2 updated", "");
        });

    const auto showPointSelectionChanged1 = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();


        if (scatterplotName1.isEmpty())
        {
            qWarning() << "No scatterplot view 1 selected";

        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1)
                {
                    auto pointSelectionScaleAction = findActionByPath<DecimalAction>(plugin,"Settings/Selection/Scale");
                    auto pointSelectionOpacityAction = findActionByPath<DecimalAction>(plugin,"Settings/Selection/Opacity");
                    if (pointSelectionScaleAction)
                    {
                        if ( _settingsAction.getDataset1OptionsHolder().getHidePointSelectionOutlineAction().isChecked())
                        {
                            pointSelectionScaleAction->setValue(100.0);
                        }
                        else
                        {
                            pointSelectionScaleAction->setValue(160.0);
                        }

                    }
                    else
                    {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {
                        if ( _settingsAction.getDataset1OptionsHolder().getHidePointSelectionOutlineAction().isChecked())
                        {
                            pointSelectionOpacityAction->setValue(0.0);
                        }
                        else
                        {
                            pointSelectionOpacityAction->setValue(100.0);
                        }

                    }
                    else
                    {
                        qWarning() << "Point selection opacity action not found";
                    }
                }
            }
        }
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getHidePointSelectionOutlineAction(), &ToggleAction::toggled, this, showPointSelectionChanged1);


    const auto focusSelectionChanged1 = [this]() -> void {

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();


        if (scatterplotName1.isEmpty())
        {
            qWarning() << "No scatterplot view 1 selected";

        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1 )
                {
                    auto focusAction = findActionByPath<ToggleAction>(plugin,"Settings/Plot/Point/Focus selection");
                    if (focusAction)
                    {
                        focusAction->setChecked(_settingsAction.getDataset1OptionsHolder().getFocusSelectionAction().isChecked());
                    }
                    else
                    {
                        qWarning() << "Focus selection action not found";
                    }
                }
            }
        }

        };

    connect(& _settingsAction.getDataset1OptionsHolder().getFocusSelectionAction(), &ToggleAction::toggled, this, focusSelectionChanged1);



    const auto showPointSelectionChanged2 = [this]() -> void {

        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();


        if (scatterplotName2.isEmpty())
        {
            qWarning() << "No scatterplot view 2 selected";

        }

        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName2 )
                {
                    auto pointSelectionScaleAction = findActionByPath<DecimalAction>(plugin,"Settings/Selection/Scale");
                    auto pointSelectionOpacityAction = findActionByPath<DecimalAction>(plugin,"Settings/Selection/Opacity");
                    if (pointSelectionScaleAction)
                    {
                        if (_settingsAction.getDataset2OptionsHolder().getHidePointSelectionOutline2Action().isChecked())
                        {
                            pointSelectionScaleAction->setValue(100.0);
                        }
                        else
                        {
                            pointSelectionScaleAction->setValue(160.0);
                        }

                    }
                    else
                    {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {
                        if (_settingsAction.getDataset2OptionsHolder().getHidePointSelectionOutline2Action().isChecked())
                        {
                            pointSelectionOpacityAction->setValue(0.0);
                        }
                        else
                        {
                            pointSelectionOpacityAction->setValue(100.0);
                        }

                    }
                    else
                    {
                        qWarning() << "Point selection opacity action not found";
                    }
                }
            }
        }
        };

    connect(&_settingsAction.getDataset1OptionsHolder().getHidePointSelectionOutlineAction(), &ToggleAction::toggled, this, showPointSelectionChanged2);


    const auto focusSelectionChanged2 = [this]() -> void {

        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();

        if (scatterplotName2.isEmpty())
        {
            qWarning() << "No scatterplot view 2 selected";

        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName2)
                {
                    auto focusAction = findActionByPath<ToggleAction>(plugin,"Settings/Plot/Point/Focus selection");
                    if (focusAction)
                    {
                        focusAction->setChecked(_settingsAction.getDataset2OptionsHolder().getFocusSelection2Action().isChecked());
                    }
                    else
                    {
                        qWarning() << "Focus selection action not found";
                    }
                }
            }
        }

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getFocusSelection2Action(), &ToggleAction::toggled, this, focusSelectionChanged2);


    const auto embeddingDimensionYActionChanged1 = [this]() -> void {

        setStatusMessage("M", "Embedding 1 Y dimension changed", "");
        //TODO crash here with cluster dataset 
        //modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(),false,"");

        modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), "", _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), false, "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionYActionChanged1);


    const auto embeddingDimensionYActionChanged2 = [this]() -> void {

        setStatusMessage("M", "Embedding 2 Y dimension changed", "");
        //TODO crash here with cluster dataset 
        //modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(),false,"");

        modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), "", _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(), false, "");



        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionYActionChanged2);

    const auto embeddingDimensionXActionChanged1 = [this]() -> void {

        setStatusMessage("M", "Embedding 1 X dimension changed", "");
        //TODO crash here with cluster dataset 
        //modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), false, "");

        modeChange(_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId(), "", _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction().getCurrentDimensionName(), _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionYAction().getCurrentDimensionName(), false, "");
        };

    connect(& _settingsAction.getDataset1OptionsHolder().getEmbeddingDimensionXAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionXActionChanged1);


    const auto embeddingDimensionXActionChanged2 = [this]() -> void {

        setStatusMessage("M", "Embedding 2 X dimension changed", "");
        //TODO crash here with cluster dataset 
        //modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(),false,"");
        modeChange(_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId(), "", _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction().getCurrentDimensionName(), _settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2YAction().getCurrentDimensionName(), false, "");

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getEmbeddingDimension2XAction(), &DimensionPickerAction::currentDimensionIndexChanged, this, embeddingDimensionXActionChanged2);


   
    const auto scatterplotviewSelectionChanged1= [this]() -> void {

        setStatusMessage("M", "Scatterplotview selection 1 updated", "");

        QString scatterplotName1 = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
        if (scatterplotName1.isEmpty())
        {
            qWarning() << "No scatterplot view selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName1)
                {
                    mv::gui::ToggleAction* notifyDuringSelectionAction = findActionByPath<ToggleAction>(plugin,"Settings/Selection/Point Selection/Notify during selection");
                    mv::gui::ToggleAction* haloAction = findActionByPath<ToggleAction>(plugin,"Settings/Selection/Halo");

                    if (notifyDuringSelectionAction) {
                        notifyDuringSelectionAction->setChecked(false);
                    }
                    else
                    {
                        qWarning() << "Notify during selection action not found.";
                    }
                    if (haloAction) {
                        haloAction->setChecked(true);
                    }
                    else
                    {
                        qWarning() << "Halo action not found.";
                    }

                    auto pointSelectionScaleAction = findActionByPath<DecimalAction>(plugin,"Settings/Selection/Scale");
                    auto pointSelectionOpacityAction = findActionByPath<DecimalAction>(plugin,"Settings/Selection/Opacity");
                    if (pointSelectionScaleAction)
                    {

                        pointSelectionScaleAction->setValue(160.0);

                    }
                    else
                    {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {

                        pointSelectionOpacityAction->setValue(100.0);



                    }
                    else
                    {
                        qWarning() << "Point selection opacity action not found";
                    }

                }
            }
        }

        };

    connect(&_settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction(), &OptionAction::currentIndexChanged, this, scatterplotviewSelectionChanged1);
    
    const auto lineplotviewSelectionChanged1 = [this]() -> void {

        setStatusMessage("M", "Lineplotview selection 1 updated", "");
        //TODO
        };
    
    connect(&_settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction(), &OptionAction::currentIndexChanged, this, lineplotviewSelectionChanged1);





    const auto scatterplotviewSelectionChanged2 = [this]() -> void {

        setStatusMessage("M", "Scatterplotview selection 2 updated", "");

        QString scatterplotName2 = _settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action().getCurrentText();
        if (scatterplotName2.isEmpty())
        {
            qWarning() << "No scatterplot view 2 selected";
            return;
        }
        auto scatterplotViewFactory = mv::plugins().getPluginFactory("Scatterplot View");
        if (scatterplotViewFactory)
        {
            for (auto plugin : mv::plugins().getPluginsByFactory(scatterplotViewFactory))
            {
                if (plugin->getGuiName() == scatterplotName2)
                {
                    mv::gui::ToggleAction* notifyDuringSelectionAction = findActionByPath<ToggleAction>(plugin,"Settings/Selection/Point Selection/Notify during selection");
                    mv::gui::ToggleAction* haloAction = findActionByPath<ToggleAction>(plugin,"Settings/Selection/Halo");

                    if (notifyDuringSelectionAction) {
                        notifyDuringSelectionAction->setChecked(false);
                    }
                    else
                    {
                        qWarning() << "Notify during selection action not found.";
                    }
                    if (haloAction) {
                        haloAction->setChecked(true);
                    }
                    else
                    {
                        qWarning() << "Halo action not found.";
                    }

                    auto pointSelectionScaleAction = findActionByPath<DecimalAction>(plugin,"Settings/Selection/Scale");
                    auto pointSelectionOpacityAction = findActionByPath<DecimalAction>(plugin,"Settings/Selection/Opacity");
                    if (pointSelectionScaleAction)
                    {

                        pointSelectionScaleAction->setValue(160.0);

                    }
                    else
                    {
                        qWarning() << "Point selection scale action not found";
                    }
                    if (pointSelectionOpacityAction)
                    {

                        pointSelectionOpacityAction->setValue(100.0);



                    }
                    else
                    {
                        qWarning() << "Point selection opacity action not found";
                    }

                }
            }
        }

        };

    connect(&_settingsAction.getDataset2OptionsHolder().getScatterplotViewSelection2Action(), &OptionAction::currentIndexChanged, this, scatterplotviewSelectionChanged2);
    _pca1.setCompletionCallback(nullptr);
    _pca1.setCompletionCallback([this](bool success, const std::string& message) {
        if (success) {
            qDebug() << "PCA 1 finished successfully:" << message.c_str();
            processAfterPCAComplete();
            
        }
        else {
            setStatusMessage("E", "PCA Failed", message.c_str());
            qWarning() << "PCA 1 failed:" << message.c_str();
        }
        });
}


void Computation::AutoCompute()
{
    if (_isComputing) return;
    _isComputing = true;
    auto guard = qScopeGuard([&] { _isComputing = false; });
    try
    {
        
        clearTable(_settingsAction.getTableView3Action());
        clearTable(_settingsAction.getTableView2Action());
        clearTable(_settingsAction.getTableView1Action());

        auto lineplotViewFactory = mv::plugins().getPluginFactory("LinePlot View");
        if (lineplotViewFactory)
        {
            auto lineplotView = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();
            if (lineplotView.isEmpty())
            {
                qWarning() << "No scatterplot view 2 selected";
            }
            for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
            {

                if (plugin->getGuiName() == lineplotView)
                {

                    mv::gui::DatasetPickerAction* pointDatasetAction = findActionByPath<DatasetPickerAction>(plugin,"LinePlotViewPlugin/Dataset1 Options/Point dataset");
                    mv::gui::DatasetPickerAction* clusterDatasetAction = findActionByPath<DatasetPickerAction>(plugin,"LinePlotViewPlugin/Dataset1 Options/Color dataset");
                    if (clusterDatasetAction)
                    {
                        clusterDatasetAction->setCurrentIndex(-1);
                    }
                    if (pointDatasetAction)
                    {
                        pointDatasetAction->setCurrentIndex(-1);
                    }
                    if (_lineChartPointDataset1.isValid())
                    {
                        events().notifyDatasetDataChanged(_lineChartPointDataset1);
                    }

                }
            }
        }


        _numOfPointsInLine1 = 0;
        QApplication::processEvents();

        mv::Dataset<Points> fullPointDataset1;
        mv::Dataset<Clusters> fullClusterDataset1;
        mv::Dataset<Clusters> fullClusterDataset2;
        int xEmbeddingIndex = -1, yEmbeddingIndex = -1; float maxCorrelation = 1.0f, minCorrelation = 0.0f;
        std::vector<int> selectedIndices;
        QString scatterplotViewName;
        if (!validateAndRetrieveDatasets(fullPointDataset1, fullClusterDataset1, fullClusterDataset2,xEmbeddingIndex, yEmbeddingIndex, selectedIndices, scatterplotViewName, minCorrelation, maxCorrelation))
            return;

  
        if (!createDatasetsIfNeeded())
            return;


        auto selectedClusters = _settingsAction.getCommonOptionsHolder().getClusterSelectionAction().getSelectedOptions();
        auto selectIndices = _embeddingDataset->getSelectionIndices();
        auto clusters1 = fullClusterDataset1->getClusters();
        auto clusters2 = fullClusterDataset2->getClusters();
        if (selectedClusters.isEmpty() || clusters1.isEmpty() || clusters2.isEmpty())
        {
            setStatusMessage("E", "No clusters selected for correlation computation", "");
            return;
        }


        if (selectedIndices.empty())
        {
            return setStatusMessage("E", "No points selected for correlation computation", "");
        }
        auto filtIndxs = filterSelectionIndicesByClusters(selectIndices, clusters1, selectedClusters);
        if (filtIndxs.empty())
        {
            setStatusMessage("E", "No points selected for correlation computation after filtering by clusters", "");
            return;
        }
        if (filtIndxs.size() < 2)
        {
            setStatusMessage("E", "Not enough points selected for correlation computation after filtering by clusters", "");
            return;
        }
        _selectedClusterIndices1.clear();
        _selectedClusterIndices2.clear();
        _selectedClusterIndices1= getIndicesBySelectedClusters(clusters1, selectedClusters);
        _selectedClusterIndices2= getIndicesBySelectedClusters(clusters2, selectedClusters);

        std::sort(_selectedClusterIndices1.begin(), _selectedClusterIndices1.end());
        std::sort(_selectedClusterIndices2.begin(), _selectedClusterIndices2.end());

        _filteredSelectionIndices.clear();
        _filteredSelectionIndices.reserve(filtIndxs.size());
        for (auto idx : filtIndxs)
        {
            _filteredSelectionIndices.push_back(idx);
        }
        _embeddingDataset->setSelectionIndices(filtIndxs);
        _settingsAction.getCommonOptionsHolder().getSelectionModeAction().setChecked(false);
        _blockSelectionTrigger = true;
        mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset);

        if (_filteredSelectionIndices.empty())
        {
            setStatusMessage("E", "No points selected for correlation computation after filtering by clusters", "");
            return;
        }
        if (_filteredSelectionIndices.size() < 2)
        {
            setStatusMessage("E", "Not enough points selected for correlation computation", "");
            return;
        }
        
        float rotationAngle = 0.0f;
        if (!getRotationAngle(scatterplotViewName, rotationAngle))
            return;


        std::vector<float> XYCoordinates;
        if (!populateEmbeddingCoordinates(xEmbeddingIndex, yEmbeddingIndex, XYCoordinates))
        {
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

        std::vector<float> yCoordinates;
        try {
            yCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "Y");
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in getCoordinatesOfCells:" << ex.what();
            setStatusMessage("E", "Error occurred during fetching coordinates", "");
            return;
        }

        std::vector<int> sortedCellIndices;
        try {
            sortedCellIndices = getSortedCellIndices(yCoordinates);
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in getSortedCellIndices:" << ex.what();
            setStatusMessage("E", "Error occurred during sorting indices", "");
            return;
        }

        int noOfDimensions = fullPointDataset1->getNumDimensions();
        const auto& dimensionNames = fullPointDataset1->getDimensionNames();
        std::vector<int> columnIndices(noOfDimensions);
        std::iota(columnIndices.begin(), columnIndices.end(), 0);


        if (!populateAndUpdateDatasets(fullPointDataset1, rotatedXYCordinates, XYCoordinates, noOfDimensions, columnIndices, dimensionNames))
        {
            return setStatusMessage("E", "Error occurred during dataset population", "");
        }


        const auto xCoordinates = getCoordinatesOfCells(rotatedXYCordinates, "X");
        float minX = *std::min_element(xCoordinates.begin(), xCoordinates.end());
        float maxX = *std::max_element(xCoordinates.begin(), xCoordinates.end());
        float minY = *std::min_element(yCoordinates.begin(), yCoordinates.end());
        float maxY = *std::max_element(yCoordinates.begin(), yCoordinates.end());

        std::vector<float> corrGeneCoords;
        QString corrType = _settingsAction.getCommonOptionsHolder().getCorrelationCalculationTypeAction().getCurrentText();
        computeCorrelationGeneCoords(_filteredSelectionIndices, fullPointDataset1, yCoordinates, corrGeneCoords,corrType);

        
        try {
            populateGenesByCorrelationRange(corrGeneCoords, minCorrelation, maxCorrelation, dimensionNames, _geneCorrValues1, _geneOfInterestIndices1);
        }
        catch (const std::exception& ex) {
            qWarning() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
            setStatusMessage("E", "Error occurred during filtering genes", "");
            return;
        }


        setupTableModelAndSelection(_settingsAction.getTableView1Action(), _geneCorrValues1, _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), _commonGeneNames, _settingsAction.getTableView2Action(), _settingsAction.getTableView3Action(), _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action());

        selectFirstRow(_settingsAction.getTableView1Action());
        if (lineplotViewFactory)
        {
            auto lineplotView = _settingsAction.getDataset1OptionsHolder().getLineplotViewSelectionAction().getCurrentText();
            if (lineplotView.isEmpty())
            {
                qWarning() << "No scatterplot view 2 selected";
            }

            for (auto plugin : mv::plugins().getPluginsByFactory(lineplotViewFactory))
            {

                if (plugin->getGuiName() == lineplotView)
                {
                    
                   mv::gui::DatasetPickerAction* pointDatasetAction = findActionByPath<DatasetPickerAction>(plugin,"LinePlotViewPlugin/Dataset1 Options/Point dataset");
                   mv::gui::DatasetPickerAction* clusterDatasetAction = findActionByPath<DatasetPickerAction>(plugin,"LinePlotViewPlugin/Dataset1 Options/Color dataset");
                   if (pointDatasetAction && _lineChartPointDataset1.isValid())
                   {
                       pointDatasetAction->setCurrentDataset(_lineChartPointDataset1);
                       if (clusterDatasetAction && _lineChartColorClusterDataset1.isValid())
                       {
                           clusterDatasetAction->setCurrentDataset(_lineChartColorClusterDataset1);
                       }
                       events().notifyDatasetDataChanged(_lineChartPointDataset1);
                   }

                }
            }
        }


        setStatusMessage("C", "Computation completed", "#28a745");

        if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked())
        {
            setStatusMessage("R", "Computation in progress", "");
            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(true);
            QApplication::processEvents();

            ManualCompute();
            

            _settingsAction.getCommonOptionsHolder().getComputeAction().setDisabled(false);
        }

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
    _isComputing = false;
}

void Computation::ManualCompute()
{
    
    bool complete = setupAndTriggerPCA();
    if (!complete)
    {
        clearTable(_settingsAction.getTableView3Action());
        clearTable(_settingsAction.getTableView2Action());
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
    QJsonObject obj;
    obj["status"] = status;
    obj["message"] = message;
    obj["color"] = resolvedColor;
    _settingsAction.getStatusHolder().getStatusMessageAction().setString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}


bool Computation::validateAndRetrieveDatasets(mv::Dataset<Points>& fullPointDataset1, mv::Dataset<Clusters>& fullClusterDataset1, mv::Dataset<Clusters>& fullClusterDataset2, int& xEmbeddingIndex, int& yEmbeddingIndex, std::vector<int>& selectedIndices, QString& scatterplotViewName, float& minCorrelation, float& maxCorrelation)
{

    fullPointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    fullClusterDataset1 = mv::data().getDataset<Clusters>(_settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset().getDatasetId());
    fullClusterDataset2 = mv::data().getDataset<Clusters>(_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());

    if (!_embeddingDataset.isValid() || !fullPointDataset1.isValid() || !fullClusterDataset1.isValid() || !fullClusterDataset2.isValid()) {
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
        auto indices = _embeddingDataset->getSelectionIndices();
        selectedIndices.assign(indices.begin(), indices.end());
    }
    if (selectedIndices.empty()) {
        qWarning() << "No selected indices";
        setStatusMessage("E", "No indices selected", "");
        return false;
    }

    auto selectedClusterValues = _settingsAction.getCommonOptionsHolder().getClusterSelectionAction().getSelectedOptions();
    if (selectedClusterValues.isEmpty()) {
        qWarning() << "No selected cluster values";
        setStatusMessage("E", "No clusters selected", "");
        _blockSelectionTrigger = true;
        _embeddingDataset->setSelectionIndices({});
        mv::events().notifyDatasetDataSelectionChanged(_embeddingDataset);
        return false;
    }

    scatterplotViewName = _settingsAction.getDataset1OptionsHolder().getScatterplotViewSelectionAction().getCurrentText();
    if (scatterplotViewName.isEmpty()) {
        qWarning() << "No scatterplot view selected";
        setStatusMessage("E", "No scatterplot view selected", "");
        return false;
    }

    minCorrelation = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMinimum();
    maxCorrelation = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMaximum();
    if (minCorrelation < -1.0f || maxCorrelation > 1.0f || minCorrelation >= maxCorrelation) {
        qWarning() << "Invalid correlation range";
        setStatusMessage("E", "Invalid correlation range", "");
        return false;
    }
    qWarning() << "Selected indices size:" << selectedIndices.size() << "xEmbeddingIndex:" << xEmbeddingIndex << "yEmbeddingIndex:" << yEmbeddingIndex
        << "scatterplotViewName:" << scatterplotViewName << "minCorrelation:" << minCorrelation << "maxCorrelation:" << maxCorrelation;


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
        _selectedGenePointDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedGenePointDataset2);
    }

    if (!_selectedEmbeddingPointDataset1.isValid()) {
        _selectedEmbeddingPointDataset1 = mv::data().createDataset("Points", "Selected Embedding Point Data 1", _selectedGenePointDataset1);
        _selectedEmbeddingPointDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedEmbeddingPointDataset1);
    }

    if (!_selectedEmbeddingPointDataset2.isValid()) {
        _selectedEmbeddingPointDataset2 = mv::data().createDataset("Points", "Selected Embedding Point Data 2", _selectedGenePointDataset2);
        _selectedEmbeddingPointDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_selectedEmbeddingPointDataset2);
    }
    
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
        _lineChartPointDataset1 = mv::data().createDataset("Points", "Line Chart Point Data 1");
        _lineChartPointDataset1->setGroupIndex(670);
        mv::events().notifyDatasetAdded(_lineChartPointDataset1);
    }
    if(!_lineChartColorClusterDataset1.isValid()) {
        _lineChartColorClusterDataset1 = mv::data().createDataset("Cluster", "Line Chart Color Cluster Data 1", _lineChartPointDataset1);
        _lineChartColorClusterDataset1->setGroupIndex(670);
        mv::events().notifyDatasetAdded(_lineChartColorClusterDataset1);
    }
    if (!_lineChartColorPointDataset1.isValid()) {
        _lineChartColorPointDataset1 = mv::data().createDataset("Points", "Line Chart Color Point Data 1", _lineChartPointDataset1);
        _lineChartColorPointDataset1->setGroupIndex(670);
        mv::events().notifyDatasetAdded(_lineChartColorPointDataset1);
    }

    if (!_selectedEmbeddingDataset.isValid()) {
        _selectedEmbeddingDataset = mv::data().createDataset("Points", "Selected Embedding Data", _selectedCellPointDataset);
        _selectedEmbeddingDataset->setGroupIndex(666);
        mv::events().notifyDatasetAdded(_selectedEmbeddingDataset);
    }

    if (!_rotatedSelectedEmbeddingDataset.isValid() || !_selectedGenePointDataset1.isValid() || !_lineChartPointDataset1.isValid()||!_lineChartColorClusterDataset1.isValid() || !_lineChartColorPointDataset1.isValid() || !_selectedGenePointDataset2.isValid() || !_selectedEmbeddingDataset.isValid() || !_selectedCellPointDataset.isValid() ||!_selectedEmbeddingPointDataset1.isValid() ||!_selectedEmbeddingPointDataset2.isValid()) {
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


    if (_settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().isValid())
    {
        auto clusterFullDataset2 = mv::data().getDataset<Clusters>(
            _settingsAction.getDataset2OptionsHolder().getClusterDataset2Action().getCurrentDataset().getDatasetId());
        const auto clusters2 = clusterFullDataset2->getClusters();
        for (const auto& cluster : clusters2)
            clusterNames2.insert(cluster.getName());
    }



    QStringList clusterNamesCommon = getCommonSortedNames(clusterNames1, clusterNames2);

    auto& clusterSelectionAction = _settingsAction.getCommonOptionsHolder().getClusterSelectionAction();
    clusterSelectionAction.setOptions(clusterNamesCommon);
    clusterSelectionAction.setSelectedOptions(clusterNamesCommon);
}

void Computation::processAfterPCAComplete()
{
    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("C", "PCA analysis completed", "#28a745");
    qDebug() << "PCA analysis completed successfully";
    
    _selectedPCData1.coordinates.clear();
    _selectedPCData1.numberOfPoints = 0;
    _selectedPCData1.numberOfDimensions = 0;
    _selectedPCData1.coordinates = _pca1.scores();
    auto& loadings1 = _pca1.loadings();
    _selectedPCData1.numberOfPoints = _pca1.numRows();
    _selectedPCData1.numberOfDimensions = _pca1.numDims();
    const int nComps = _pca1.numComponents();

    if (!_selectedGenePointDataset1.isValid() || !_selectedGenePointDataset2.isValid()) {
        qWarning() << "Selected gene point datasets are invalid";
        setStatusMessage("E", "Selected gene point datasets are invalid", "");
        return;
    }

    Dataset<Points> fullDataset2 = _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset();
    Dataset<Points> fullDataset1 = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset();
    if (!fullDataset1.isValid() || !fullDataset2.isValid()) {
        qWarning() << "Full datasets are invalid";
        setStatusMessage("E", "Full datasets are invalid", "");
        return;
    }


    if (!_pcaCommonGenesDataset1.isValid()) {
        _pcaCommonGenesDataset1 = mv::data().createDataset("Points", "PCA Dataset 1", _selectedGenePointDataset1);
        _pcaCommonGenesDataset1->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset1);
    }
    if (!_pcaCommonGenesDataset2.isValid()) {
        _pcaCommonGenesDataset2 = mv::data().createDataset("Points", "PCA Dataset 2", _selectedGenePointDataset2);
        _pcaCommonGenesDataset2->setGroupIndex(667);
        mv::events().notifyDatasetAdded(_pcaCommonGenesDataset2);
    }

    setStatusMessage("R", "Computation after PCA", "");

    if (!_selectedGenePointDataset2.isValid()) {
        qWarning() << "Selected gene point dataset 2 is invalid";
        setStatusMessage("E", "Selected gene point dataset 2 is invalid", "");
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

    if (_geneOfInterestIndices1.empty()) {
        qWarning() << "Gene of interest indices 1 are empty";
        setStatusMessage("E", "Gene of interest indices 1 are empty", "");
        return;
    }


    std::vector<int> geneOfInterestIndices2;
    QStringList commonGeneNames;
    const auto& genedims1 = fullDataset1->getDimensionNames();
    const auto& genedims2 = fullDataset2->getDimensionNames();
    geneOfInterestIndices2.reserve(_geneOfInterestIndices1.size());
    commonGeneNames.reserve(_geneOfInterestIndices1.size());

    for (int inddx : _geneOfInterestIndices1) {
        if (inddx < 0 || inddx >= genedims1.size()) {
            qWarning() << "Invalid index in _geneOfInterestIndices1:" << inddx;
            setStatusMessage("E", "Invalid index in gene of interest indices 1", "");
            return;
        }
        const QString& geneName = genedims1[inddx];
        auto it = std::find(genedims2.begin(), genedims2.end(), geneName);
        if (it != genedims2.end()) {
            geneOfInterestIndices2.push_back(static_cast<int>(std::distance(genedims2.begin(), it)));
            commonGeneNames.push_back(geneName);
        }
        else {
            qWarning() << "Gene name not found in dataset 2:" << geneName;
            setStatusMessage("E", "Gene name not found in dataset 2", "");
            return;
        }
    }


    const int nCommon = static_cast<int>(_geneOfInterestIndices1.size());
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

 

    LoadingValuesContainer loadingValues;
    loadingValues.numberOfComponents = nComps;
    loadingValues.numberOfDimensions = _selectedPCData1.numberOfDimensions;

    for (int id = 0; id < _selectedPCData1.numberOfDimensions; ++id) {
        LoadingValues tempValus;
        tempValus.geneId1 = _geneOfInterestIndices1[id];
        tempValus.geneId2 = geneOfInterestIndices2[id];
        tempValus.loadings.resize(nComps);
        for (int j = 0; j < nComps; ++j) {
            tempValus.loadings[j] = loadings1[id * nComps + j];
        }
        loadingValues.loadingValuesMap[commonGeneNames[id]]=tempValus;
    }


    const int numPointsReduced1 = _selectedClusterIndices1.size();
    const int numPointsReduced2 = _selectedClusterIndices2.size();
    if (numPointsReduced1 <= 0 || numPointsReduced2 <= 0) {
        qWarning() << "No points selected for PCA computation";
        setStatusMessage("E", "No points selected for PCA computation", "");
        return;
    }



    //loadingValues qdebug
   // qDebug() << "\n\n""**********************************""\n\n";
    //qDebug() << "Loading values for PCA:";
    std::vector<float> finalGenePCSimVector1(numPointsReduced1 * loadingValues.numberOfComponents, 0.0f);
    std::vector<float> finalGenePCSimVector2(numPointsReduced2 * loadingValues.numberOfComponents, 0.0f);
    int i = 0;
    for (const auto& [key, value] : loadingValues.loadingValuesMap) {
        //get the key and values from loadingValues.loadingValuesMap
        const QString& geneName = key;
        const int& geneId1 = value.geneId1;
        const int& geneId2 = value.geneId2;
        const QVector<float>& loadings = value.loadings;

        //qDebug() << i<<")"<< "Gene Index:" << geneId1<< "Gene Index:" << geneId2<< "Gene Name:" << geneName<< "Number of Components:" << loadings.size();
        std::vector<float> genePointVector1(numPointsReduced1, 0.0f);
        std::vector<float> genePointVector2(numPointsReduced2, 0.0f);
        std::vector<int> geneIDVector1(1, geneId1);
        std::vector<int> geneIDVector2(1, geneId2);
        if (geneId1 < 0 || geneId1 >= fullDataset1->getNumDimensions() || geneId2 < 0 || geneId2 >= fullDataset2->getNumDimensions()) {
            qWarning() << "Invalid gene indices for PCA computation: geneId1 =" << geneId1 << ", geneId2 =" << geneId2;
            setStatusMessage("E", "Invalid gene indices for PCA computation", "");
            return;
        }
        //qDebug() << "Gene ID Vector 1:" << geneIDVector1 << "Gene ID Vector 2:" << geneIDVector2;

        fullDataset1->populateDataForDimensions(genePointVector1, geneIDVector1,_selectedClusterIndices1);
        fullDataset2->populateDataForDimensions(genePointVector2, geneIDVector2,_selectedClusterIndices2);
        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked())
        {
            QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
            NormalizationType normType = normalizationTypeFromString(normTypeStr);
            normalizeVector(genePointVector1, normType);
            normalizeVector(genePointVector2, normType);
        }
        for (int j = 0; j < loadings.size(); j++)
        {
            float loadingVal = loadings[j];


            for (int k = 0; k < numPointsReduced1; ++k) {


                finalGenePCSimVector1[k * loadingValues.numberOfComponents + j] += genePointVector1[k] * loadingVal;
                

            }



            for (int k = 0; k < numPointsReduced2; ++k) {
                finalGenePCSimVector2[k * loadingValues.numberOfComponents + j] += genePointVector2[k] * loadingVal;
            }

        }
        i++;
    }
    //qDebug() << "\n\n""**********************************""\n\n";


    _pcaCommonGenesDataset1->setData(finalGenePCSimVector1.data(), numPointsReduced1, _selectedPCData1.numberOfDimensions);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset1);
    _pcaCommonGenesDataset2->setData(finalGenePCSimVector2.data(), numPointsReduced2, _selectedPCData1.numberOfDimensions);
    mv::events().notifyDatasetDataChanged(_pcaCommonGenesDataset2);

    _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setPointsDataset(_pcaCommonGenesDataset2);

    int numDimsForColoring = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getNumberOfDimensions();
    if (numDimsForColoring > 0) {
        _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().setCurrentDimensionIndex(0);
    }
    else if (numDimsForColoring < 0) {
        qWarning() << "No PCA dimensions available for coloring";
        setStatusMessage("E", "No PCA dimensions available for coloring", "");
        clearTable(_settingsAction.getTableView3Action());
        clearTable(_settingsAction.getTableView2Action());
        return;
    }

    if (_settingsAction.getCommonOptionsHolder().getAutoComputeAction().isChecked()) {
        correlationAfterPCA();
    }
    populateLineChartColorPointData();
}

void Computation::correlationAfterPCA()
{
    FunctionTimer timer(Q_FUNC_INFO);
    setStatusMessage("R", "Running correlation between genes and PC", "#28a745");

    Dataset<Points> fullDataset2 = _settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset();
    Dataset<Points> fullDataset1 = _settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset();
    if (!fullDataset2.isValid()) {
        setStatusMessage("E", "Point dataset 2 is invalid", "");
        return;
    }
    if (!fullDataset1.isValid()) {
        setStatusMessage("E", "Point dataset 1 is invalid", "");
        return;
    }
  

    int selectedPCDimension = _settingsAction.getCommonOptionsHolder().getPcaDimensionForColoringAction().getCurrentDimensionIndex();


    if (selectedPCDimension < 0)
    {
        setStatusMessage("E", "No PCA dimension selected for dataset 2", "");
        return;
    }
    const int numAllCells2 = fullDataset2->getNumPoints();
    if (numAllCells2 <= 0) {
        setStatusMessage("E", "No cells in point dataset 2", "");
        return;
    }
    const int numAllCells1 = fullDataset1->getNumPoints();
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

    QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
    bool normalizeDataVal = _settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked();
    computeCorrelationGenePC(fullDataset2, selectedPC2, correlationGenePC2, _selectedClusterIndices2, normTypeStr, normalizeDataVal);
    computeCorrelationGenePC(fullDataset1, selectedPC1, correlationGenePC1, _selectedClusterIndices1, normTypeStr, normalizeDataVal);

    const float minCorrelation2 = _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMinimum();
    const float maxCorrelation2 = _settingsAction.getDataset2OptionsHolder().getCorrelationRange2Action().getMaximum();
    const auto dimensionNames2 = fullDataset2->getDimensionNames();
    
    const float minCorrelation1 = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMinimum();
    const float maxCorrelation1 = _settingsAction.getDataset1OptionsHolder().getCorrelationRangeAction().getMaximum();
    const auto dimensionNames1 = fullDataset1->getDimensionNames();

     try {
        populateGenesByCorrelationRange(correlationGenePC2, minCorrelation2, maxCorrelation2, dimensionNames2, _geneCorrValues2, _geneOfInterestIndices2);
        populateGenesByCorrelationRange(correlationGenePC1, minCorrelation1, maxCorrelation1, dimensionNames1, _geneCorrValues3, _geneOfInterestIndices3);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in filterTopNGenesByCorrelation:" << ex.what();
        setStatusMessage("E", "Error occurred during filtering genes", "");
        return;
    }

    setupTableModelAndSelection(_settingsAction.getTableView2Action(), _geneCorrValues2, _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action(), _commonGeneNames, _settingsAction.getTableView1Action(), _settingsAction.getTableView3Action(), _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction());
    setupTableModelAndSelection(_settingsAction.getTableView3Action(), _geneCorrValues3, _settingsAction.getDataset1OptionsHolder().getGeneSelectionAction(), _commonGeneNames, _settingsAction.getTableView1Action(), _settingsAction.getTableView2Action(), _settingsAction.getDataset2OptionsHolder().getGeneSelection2Action());
    selectFirstRow(_settingsAction.getTableView3Action());
    setStatusMessage("C", "Correlation between genes and PC completed", "#28a745");
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


    _embeddingDataset->populateDataForDimensions(
        XYCoordinates,
        std::vector<int>(embeddingIndices, embeddingIndices + 2),
        _filteredSelectionIndices
    );

    return true;
}

bool Computation::populateAndUpdateDatasets(mv::Dataset<Points>& fullPointDataset, const std::vector<float>& rotatedXYCordinates, const std::vector<float>& XYCordinates, int noOfDimensions, const std::vector<int>& columnIndices, const std::vector<QString>& dimensionNames)
{

    if (!_selectedCellPointDataset.isValid() ||
        !_rotatedSelectedEmbeddingDataset.isValid() ||
        !_lineChartPointDataset1.isValid() ||
        !_lineChartColorClusterDataset1.isValid() ||
        !_lineChartColorPointDataset1.isValid() ||
        !_selectedGenePointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() ||
        !_selectedEmbeddingDataset.isValid() ||
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
        fullPointDataset->populateDataForDimensions(cellbygenematrix, columnIndices, _filteredSelectionIndices);
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }
    if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked()) 
    { 
        QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
        NormalizationType normType = normalizationTypeFromString(normTypeStr);
        normalizeVector(cellbygenematrix, normType);
    }

    _selectedCellPointDataset->setData(cellbygenematrix.data(), static_cast<int>(numCells), noOfDimensions);
    _selectedCellPointDataset->setDimensionNames(dimensionNames);
    mv::events().notifyDatasetDataChanged(_selectedCellPointDataset);

    _rotatedSelectedEmbeddingDataset->setData(rotatedXYCordinates.data(), static_cast<int>(numCells), 2);
    _rotatedSelectedEmbeddingDataset->setDimensionNames({ QStringLiteral("X"), QStringLiteral("Y") });
    mv::events().notifyDatasetDataChanged(_rotatedSelectedEmbeddingDataset);
    
    
    _selectedEmbeddingDataset->setData(XYCordinates.data(), static_cast<int>(numCells), 2);
    _selectedEmbeddingDataset->setDimensionNames({ QStringLiteral("X"), QStringLiteral("Y") });
    mv::events().notifyDatasetDataChanged(_selectedEmbeddingDataset);


    std::vector<QString> dimensionNamesWithYCoords = { QStringLiteral("Rotated Y Coordinates") };
    dimensionNamesWithYCoords.insert(dimensionNamesWithYCoords.end(), dimensionNames.begin(), dimensionNames.end());


    std::vector<float> rotatedYCoord(numCells * 1);
    _rotatedSelectedEmbeddingDataset->extractDataForDimension(rotatedYCoord, 1);

    std::vector<float> linechartdata(numCells * (1 + numGenes));


    for (size_t i = 0; i < numCells; ++i) {
     
        linechartdata[i * (1 + numGenes)] = rotatedYCoord[i];

  
        for (size_t j = 0; j < numGenes; ++j) {
            linechartdata[i * (1 + numGenes) + 1 + j] = cellbygenematrix[i * numGenes + j];
        }
    }


    _lineChartPointDataset1->setData(linechartdata.data(), static_cast<int>(numCells), noOfDimensions + 1);
    _lineChartPointDataset1->setDimensionNames(dimensionNamesWithYCoords);
    mv::events().notifyDatasetDataChanged(_lineChartPointDataset1);

    Dataset<Clusters> clusterDataset = _settingsAction.getDataset1OptionsHolder().getClusterDatasetAction().getCurrentDataset();
    if (clusterDataset.isValid() && _lineChartColorClusterDataset1.isValid())
    {
        _lineChartColorClusterDataset1->getClusters() = QVector<Cluster>();
        events().notifyDatasetDataChanged(_lineChartColorClusterDataset1);
        auto clusters = clusterDataset->getClusters();
        for (const auto& cluster : clusters)
        {
            std::vector<std::seed_seq::result_type> newClusterIndices;
            auto clusterIndices = cluster.getIndices();
            for (int index : clusterIndices) {
                auto it = std::find(_filteredSelectionIndices.begin(), _filteredSelectionIndices.end(), index);
                if (it != _filteredSelectionIndices.end()) {
                    newClusterIndices.push_back(static_cast<int>(std::distance(_filteredSelectionIndices.begin(), it)));
                }
            }

            if (newClusterIndices.size() > 0)
            {
                Cluster remappedCluster;
                remappedCluster.setName(cluster.getName());
                remappedCluster.setColor(cluster.getColor());
                remappedCluster.setIndices(newClusterIndices);
                _lineChartColorClusterDataset1->addCluster(remappedCluster);
            }


        }
        events().notifyDatasetDataChanged(_lineChartColorClusterDataset1);
    }

    return true;
}

void Computation::populateLineChartColorPointData()
{
    if (!_selectedPCData1.coordinates.empty() && _selectedPCData1.numberOfPoints > 0 && _selectedPCData1.numberOfDimensions > 0 && _lineChartColorPointDataset1.isValid())
    {
        //std::vector<QString> dimensionNamesPC;
        //for (int i = 0; i < _selectedPCData1.numberOfDimensions; ++i) {
            //dimensionNamesPC.push_back(QString("PC%1").arg(i + 1));
        //}
        _lineChartColorPointDataset1->setData(_selectedPCData1.coordinates.data(), _selectedPCData1.numberOfPoints, _selectedPCData1.numberOfDimensions);

        //_lineChartColorPointDataset1->setDimensionNames(dimensionNamesPC);
        mv::events().notifyDatasetDataChanged(_lineChartColorPointDataset1);
    }
    else if (_lineChartColorPointDataset1.isValid())
    {
        _lineChartColorPointDataset1->setData(nullptr, 0, 0);
    }
}

bool Computation::setupAndTriggerPCA()
{
    FunctionTimer timer(Q_FUNC_INFO);

    auto fullPointDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getPointDatasetAction().getCurrentDataset().getDatasetId());
    auto fullPointDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getPointDataset2Action().getCurrentDataset().getDatasetId());

    auto fullEmbeddingDataset1 = mv::data().getDataset<Points>(_settingsAction.getDataset1OptionsHolder().getEmbeddingDatasetAction().getCurrentDataset().getDatasetId());
    auto fullEmbeddingDataset2 = mv::data().getDataset<Points>(_settingsAction.getDataset2OptionsHolder().getEmbeddingDataset2Action().getCurrentDataset().getDatasetId());

    if (!_selectedGenePointDataset1.isValid() || !fullPointDataset1.isValid() ||
        !_selectedGenePointDataset2.isValid() || !fullPointDataset2.isValid() ||!fullEmbeddingDataset1.isValid() || !fullEmbeddingDataset2.isValid()) {
        qWarning() << "One or more datasets are invalid!";
        setStatusMessage("E", "Internal error: invalid dataset pointer", "");
        return false;
    }

    const int numCells0 = static_cast<int>(_filteredSelectionIndices.size());
    const int numAllCellsAll2 = fullPointDataset2->getNumPoints();
    const int numAllCellsAll1 = fullPointDataset1->getNumPoints();
    const int numAllFilteredCells2 = _selectedClusterIndices2.size();
    const int numAllFilteredCells1 = _selectedClusterIndices1.size();
    const int numOfIntGenes = static_cast<int>(_geneOfInterestIndices1.size());

    const auto& dimensionNames1 = fullPointDataset1->getDimensionNames();
    const auto& dimensionNames2 = fullPointDataset2->getDimensionNames();

    if (numCells0 <= 0 || numAllCellsAll2 <= 0 || numOfIntGenes <= 0 || numAllCellsAll1 <=0 || numAllFilteredCells2<=0 || numAllFilteredCells1<=0) {
        qWarning() << "Invalid dataset sizes: numAllCells1:" << numCells0 << "numOfIntGenes:" << numOfIntGenes << "numAllFilteredCells2:" << numAllFilteredCells2 << "numAllFilteredCells1:" << numAllFilteredCells1 << "numAllCellsAll2:" << numAllCellsAll2 << "numAllCellsAll1:" << numAllCellsAll1;
        setStatusMessage("E", "Invalid dataset sizes", "");
        return false;
    }
    if (numOfIntGenes > static_cast<int>(dimensionNames1.size()) ||
        numOfIntGenes > static_cast<int>(dimensionNames2.size())) {
        qWarning() << "Number of genes of interest exceeds dataset dimensions";
        setStatusMessage("E", "Number of genes of interest exceeds dataset dimensions", "");
        return false;
    }

    std::vector<QString> dimensionNamesOfInterest;
    std::vector<int> geneDataset0, geneDataset1, geneDataset2;
    dimensionNamesOfInterest.reserve(numOfIntGenes);
    geneDataset0.reserve(numOfIntGenes);
    geneDataset1.reserve(numOfIntGenes);
    geneDataset2.reserve(numOfIntGenes);

    for (int idx : _geneOfInterestIndices1) {
        if (idx < 0 || idx >= static_cast<int>(dimensionNames1.size()))
            continue;
        const QString& geneName = dimensionNames1[idx];
        auto it2 = std::find(dimensionNames2.begin(), dimensionNames2.end(), geneName);
        if (it2 != dimensionNames2.end()) {
            int geneId2 = static_cast<int>(std::distance(dimensionNames2.begin(), it2));
            dimensionNamesOfInterest.push_back(geneName);
            geneDataset0.push_back(idx);
            geneDataset1.push_back(idx);
            geneDataset2.push_back(geneId2);
        }
    }

    const int numDimsOfInterest = static_cast<int>(dimensionNamesOfInterest.size());
    if (numDimsOfInterest <= 0) {
        qWarning() << "No genes of interest selected for PCA";
        setStatusMessage("E", "No genes of interest selected for PCA", "");
        return false;
    }

    std::vector<int> cellIndices0(_filteredSelectionIndices.begin(), _filteredSelectionIndices.end());

    std::vector<float> cellByGeneMatrix0(numCells0 * numDimsOfInterest);
    try {
        fullPointDataset1->populateDataForDimensions(cellByGeneMatrix0, geneDataset0, cellIndices0);
        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked())
        {
            QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
            NormalizationType normType = normalizationTypeFromString(normTypeStr);
            normalizeVector(cellByGeneMatrix0, normType);
        }
    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }
    auto [minVal, maxVal] = minMax(cellByGeneMatrix0);
    qInfo() << "PCA cellByGeneMatrix0 min:" << minVal << "max:" << maxVal;

    std::vector<float> cellByGeneMatrix2(numAllFilteredCells2 * numDimsOfInterest);
    std::vector<float> cellByGeneMatrix1(numAllFilteredCells1 * numDimsOfInterest);

    std::vector<float> embeddingMatrix1(numAllFilteredCells1 * fullEmbeddingDataset1->getNumDimensions());
    std::vector<float> embeddingMatrix2(numAllFilteredCells2 * fullEmbeddingDataset2->getNumDimensions());

    std::vector<int> embeddingGeneIndices1(fullEmbeddingDataset1->getNumDimensions());
    for (int i = 0; i < fullEmbeddingDataset1->getNumDimensions(); ++i) {
        embeddingGeneIndices1[i] = i;
    }
    std::vector<int> embeddingGeneIndices2(fullEmbeddingDataset2->getNumDimensions());
    for (int i = 0; i < fullEmbeddingDataset2->getNumDimensions(); ++i) {
        embeddingGeneIndices2[i] = i;
    }


    if(_selectedClusterIndices1 .empty() || _selectedClusterIndices2.empty())
    {
        qWarning() << "No selected cluster indices for dataset 1 or 2";
        setStatusMessage("E", "No selected cluster indices for dataset 1 or 2", "");
        return false;
    }
    if (_selectedClusterIndices2.size() > numAllCellsAll2 || _selectedClusterIndices1.size() > numAllCellsAll1) {
        qWarning() << "Selected cluster indices exceed total number of cells in dataset 2 or 1";
        setStatusMessage("E", "Selected cluster indices exceed total number of cells in dataset 2 or 1", "");
        return false;
    }


    try {
        fullPointDataset2->populateDataForDimensions(cellByGeneMatrix2, geneDataset2, _selectedClusterIndices2);
        fullPointDataset1->populateDataForDimensions(cellByGeneMatrix1, geneDataset1, _selectedClusterIndices1);
        fullEmbeddingDataset1->populateDataForDimensions(embeddingMatrix1, embeddingGeneIndices1, _selectedClusterIndices1);
        fullEmbeddingDataset2->populateDataForDimensions(embeddingMatrix2, embeddingGeneIndices2, _selectedClusterIndices2);


        if (_settingsAction.getCommonOptionsHolder().getNormalizeDataAction().isChecked()) 
        {
            QString normTypeStr = _settingsAction.getCommonOptionsHolder().getNormalizationTypeAction().getCurrentText();
            NormalizationType normType = normalizationTypeFromString(normTypeStr);
            normalizeVector(cellByGeneMatrix2, normType);
            normalizeVector(cellByGeneMatrix1, normType);
        }

    }
    catch (const std::exception& ex) {
        qWarning() << "Exception in populateDataForDimensions:" << ex.what();
        setStatusMessage("E", "Error occurred during populating filtered data", "");
        return false;
    }
    auto [minVal2, maxVal2] = std::minmax_element(cellByGeneMatrix2.begin(), cellByGeneMatrix2.end());
    auto [minVal1, maxVal1] = std::minmax_element(cellByGeneMatrix1.begin(), cellByGeneMatrix1.end());
    qInfo() << "PCA cellByGeneMatrix2 min:" << *minVal2 << "max:" << *maxVal2
        << "PCA cellByGeneMatrix1 min:" << *minVal1 << "max:" << *maxVal1;

    _selectedGenePointDataset2->setData(cellByGeneMatrix2.data(), numAllFilteredCells2, numDimsOfInterest);
    _selectedGenePointDataset2->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset2);

    _selectedGenePointDataset1->setData(cellByGeneMatrix1.data(), numAllFilteredCells1, numDimsOfInterest);
    _selectedGenePointDataset1->setDimensionNames(dimensionNamesOfInterest);
    mv::events().notifyDatasetDataChanged(_selectedGenePointDataset1);

    _selectedEmbeddingPointDataset1->setData(embeddingMatrix1.data(), numAllFilteredCells1, fullEmbeddingDataset1->getNumDimensions());
    _selectedEmbeddingPointDataset1->setDimensionNames(fullEmbeddingDataset1->getDimensionNames());
    mv::events().notifyDatasetDataChanged(_selectedEmbeddingPointDataset1);

    _selectedEmbeddingPointDataset2->setData(embeddingMatrix2.data(), numAllFilteredCells2, fullEmbeddingDataset2->getNumDimensions());
    _selectedEmbeddingPointDataset2->setDimensionNames(fullEmbeddingDataset2->getDimensionNames());
    mv::events().notifyDatasetDataChanged(_selectedEmbeddingPointDataset2);

    setStatusMessage("R", "PCA Started", "");
    _pca1.fit(cellByGeneMatrix0, numCells0, numDimsOfInterest, numOfIntGenes);
    return true;
}


void Computation::setSelectionAndAutoCompute()
{
    AutoCompute();
    _blockSelectionTrigger = false;
}





