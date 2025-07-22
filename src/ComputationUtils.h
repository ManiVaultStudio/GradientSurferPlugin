#pragma once
#include <vector>
#include <QString>
#include <string>
#include <QElapsedTimer>
#include<QTableView>
#include<QLabel>
#include<QBuffer>
#include "ClusterData/ClusterData.h"
#include "PointData/PointData.h"
#include <./actions/OptionAction.h>
#include <./actions/ToggleAction.h>
#include <./actions/DecimalAction.h>
#include <./actions/StringAction.h>
#include "./actions/DatasetPickerAction.h"
#include <./actions/WidgetAction.h>
#include <./actions/IntegralAction.h>
#include <PointData/DimensionPickerAction.h>
#include <./actions/OptionsAction.h>
#include <./actions/VariantAction.h>
#include<./actions/ColorMap1DAction.h>
#include <./actions/HorizontalToolbarAction.h>
#include <./actions/VerticalToolbarAction.h>
#include "CorrelationBarDelegate.h"
enum class NormalizationType {
    None,
    ZScore,
    MinMax,
    Log2,
    MaxAbs,
    L2,
    L1,
    Mean,
    Softmax,
    Robust,
    UnitRange,
    Binarize,
    CPM,
    Log1p,
    CPM_Log1p,
    CPM_Log1p_ZScore
};
namespace mv
{
    class CoreInterface;
}
struct SelectedPCData
{
    std::vector<float> coordinates;
    int numberOfDimensions;
    int numberOfPoints;
};
struct CorrelationGeneCoords
{
    int geneIndex;
    QString geneName;
    float correlationValue;
    int   rank;
    bool  isWithinRange;
};
struct LoadingValues {
    int geneId1;
    int geneId2;
    QVector<float> loadings;
};
struct LoadingValuesContainer
{
    std::map<QString,LoadingValues> loadingValuesMap;
    int numberOfDimensions;
    int numberOfComponents;

};

using namespace mv::gui;
struct StatusInfo {
    QString status;
    QString message;
    QString color;
    QString backgroundColor;
    QString textColor;
    QString iconName;
};
class ScopedBlocker {
    bool& flag;
public:
    explicit ScopedBlocker(bool& f) : flag(f) { flag = true; }
    ~ScopedBlocker() { flag = false; }
};
template<typename ActionType>
ActionType* findActionByPath(mv::plugin::Plugin* plugin, const QString& path) {
    return dynamic_cast<ActionType*>(plugin->findChildByPath(path));
}
void setScatterplotDimensions(mv::plugin::Plugin* plugin, const QString& xName, const QString& yName);
std::vector<std::seed_seq::result_type> filterSelectionIndicesByClusters(
    const std::vector<std::seed_seq::result_type>& selectionIndices,
    const QVector<Cluster>& clusters,
    const QStringList& selectedClusters);
std::vector<int> getIndicesBySelectedClusters(
    const QVector<Cluster>& clusters,
    const QStringList& selectedClusters);
StatusInfo parseStatusMessage(const QString& statusString);
std::pair<QFont, int> getStatusLabelFontAndIconSize(const QString& labelText);

void computeCorrelationGenePC(const mv::Dataset<Points>& fullDataset, const std::vector<float>& selectedPC, std::vector<float>& corrVector, std::vector<int>& allClusterFilteredIndices);
void clearTable(QTableView* tableView);
float computeGradientScore(const std::vector<float>& corrArray, const std::vector<float>& expArray);
void normalizeVector(std::vector<float>& data, NormalizationType type);
NormalizationType normalizationTypeFromString(const QString& str);
QString getContrastingTextColor(const QString& bgColor);
QString statusToIconName(const QString& status);
void selectFirstRow(QTableView* tableView);
void deselectAllRows(QTableView* tableView);
void makeSelection(QString value, QTableView* tableView);
void populateGenesByCorrelationRange(const std::vector<float>& correlationVector, int topNRows, int bottomNRows, const std::vector<QString>& geneNameOrdered, std::vector<CorrelationGeneCoords>& geneCorrValues, std::vector<int>& geneOfInterestIndices, bool isMainTable);
void setupTableModelAndSelection(QTableView* tableViewMain, std::vector<CorrelationGeneCoords>& geneCorrValues,DimensionPickerAction& geneSelectionAction,  QTableView* tableViewOther1, QTableView* tableViewOther2, DimensionPickerAction& geneSelectionAction2);
void resetupCorrelationTable(std::vector<CorrelationGeneCoords>& geneCorrValues, std::vector<int>& geneOfInterestIndices,QTableView* tableViewMain, int topNRows, int bottomNRows, DimensionPickerAction& geneSelectionAction,  QTableView* tableViewOther1, QTableView* tableViewOther2, DimensionPickerAction& geneSelectionAction2, bool mainTable);
void computeCorrelationGeneCoords(const std::vector<int>& cellIndices, const mv::Dataset<Points> dataset, const std::vector<float>& coords, std::vector<float>& corrVector, QString corrType);
NormalizationType normalizationTypeFromString(const QString& normTypeStr);
bool isCorrelationWithinRange(float corr, float minCorr, float maxCorr, float epsilon = 1e-6f);
std::pair<float, float> minMax(const std::vector<float>& vec);
QStringList getCommonSortedNames(const QSet<QString>& names1, const QSet<QString>& names2);
QLabel* createStatusLabel(const QString& labelText, const QString& backgroundColor, const QString& textColor, const QPixmap& iconPixmap, const QFont& font);
float computeMean(const std::vector<float>& values);
class FunctionTimer {
public:
    FunctionTimer(const QString& functionName);
    ~FunctionTimer();
private:
    QString _functionName;
    QElapsedTimer _timer;
};
// Returns current if in options, otherwise returns options[fallbackIndex] or "" if empty.
QString selectValidOption(const QStringList& options, const QString& current, int fallbackIndex = 0);

// Checks if a dataset is a valid cluster dataset given a sibling embedding dataset.
bool isValidClusterDataset(const mv::Dataset<DatasetImpl>& dataset, const mv::Dataset<Points>& sibling);

// Checks if a dataset is a valid point dataset.
bool isValidPointDataset(const mv::Dataset<DatasetImpl>& dataset);
bool isValidPointChildDataset(const mv::Dataset<DatasetImpl>& dataset);
// Checks if a dataset is a valid embedding dataset.
bool isValidEmbeddingDataset(const mv::Dataset<DatasetImpl>& dataset);
std::vector<float> rotateAxis(const std::vector<float>& xyCoordinates, float degreeValue);
std::vector<float> getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis);
std::vector<int> getSortedCellIndices(const std::vector<float>& yCoordinates);


extern std::vector<CorrelationGeneCoords> _geneCorrValues1;
extern std::vector<CorrelationGeneCoords> _geneCorrValues2;
extern std::vector<CorrelationGeneCoords> _geneCorrValues3;
extern std::vector<int> _geneOfInterestIndices1;
extern std::vector<int> _geneOfInterestIndices2;
extern std::vector<int> _geneOfInterestIndices3;
extern QStringList _commonGeneNames;
extern QStringList   _commonGeneListSelectedInTables;

class QTableView;
class DimensionPickerAction;


void syncTableSelections(QTableView* sourceTable, const QList<QTableView*>& allTables, const QList<DimensionPickerAction*>& allActions);

void scatterplotModeChange(QString scatterplotName, QString positionDatasetID, QString colorDatasetID, QString xPositionString, QString yPositionString, bool disableSelection,QString dimensionForColoring,QString colormapValue, float minValue, float maxValue);
void lineplotModeChange(QString linePlotName, QString positionDatasetID, QString colorDatasetID, QString colordatasetType, int colorDatasetDimensionIndx, QString xPositionString, QString yPositionString, QString titleText, QString smoothingType, int smoothingWindow, QString colorMapStringValue, float minValue, float maxValue);

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

void createScatterplotClusterColors(QString coloringDataset, QString colorMapValue, std::vector<int>& selectedIndices, QString pointValuesDataset, int numOfTotalPoints, QString dimensionVal, float minValue, float maxValue);
std::pair<float, float> createScatterplotPointColors(QString coloringDataset, QString colorMapValue, std::vector<int>& selectedIndices, QString pointValuesDataset, int numOfTotalPoints, QString dimensionVal, float minValue, float maxValue, bool isFullData, std::pair<float, float> sentinelMinMax);

void createClusterLineChartClusterColors(QString coloringChangeDataset, QString coloringReferenceDataset, std::vector<int>& selectedIndices);
void createLineChartClusterColors(QString pcaColoringDataset, QString colorMapValue, std::vector<int>& selectedIndices, QString pcaValuesDataset, int numOfTotalPoints, QString pcaDimensionVal, float minValue, float maxValue, bool useFulldata);

void colormapChangeScatterplot(QString scatterplotName, QString colormapValue);
void rangeChangeLinePlot();
void colormapChangeLinePlot();
void rangeChangeScatterPlot(QString scatterplotName, float minValue, float maxValue);