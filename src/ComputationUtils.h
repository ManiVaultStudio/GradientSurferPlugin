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

enum class NormalizationType {
    None,
    ZScore,
    MinMax,
    Log2
};
namespace mv
{
    class CoreInterface;
}
using namespace mv::gui;
struct StatusInfo {
    QString status;
    QString message;
    QString color;
    QString backgroundColor;
    QString textColor;
    QString iconName;
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
StatusInfo parseStatusMessage(const QString& statusString);
std::pair<QFont, int> getStatusLabelFontAndIconSize(const QString& labelText);

float computeGradientScore(const std::vector<float>& corrArray, const std::vector<float>& expArray);
void normalizeVector(std::vector<float>& data, NormalizationType type);
NormalizationType normalizationTypeFromString(const QString& str);
QString getContrastingTextColor(const QString& bgColor);
QString statusToIconName(const QString& status);
void selectFirstRow(QTableView* tableView);
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

// Checks if a dataset is a valid embedding dataset.
bool isValidEmbeddingDataset(const mv::Dataset<DatasetImpl>& dataset);
std::vector<float> rotateAxis(const std::vector<float>& xyCoordinates, float degreeValue);
std::vector<float> getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis);
std::vector<int> getSortedCellIndices(const std::vector<float>& yCoordinates);