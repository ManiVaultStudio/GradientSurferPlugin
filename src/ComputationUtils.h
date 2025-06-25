#pragma once
#include <vector>
#include <QString>
#include <string>
#include <QElapsedTimer>
#include<QTableView>
#include<QLabel>
#include<QBuffer>
enum class NormalizationType {
    None,
    ZScore,
    MinMax,
    Log2
};

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
class FunctionTimer {
public:
    FunctionTimer(const QString& functionName);
    ~FunctionTimer();
private:
    QString _functionName;
    QElapsedTimer _timer;
};

std::vector<float> rotateAxis(const std::vector<float>& xyCoordinates, float degreeValue);
std::vector<float> getCoordinatesOfCells(const std::vector<float>& rotatedXYCoordinates, const std::string& axis);
std::vector<int> getSortedCellIndices(const std::vector<float>& yCoordinates);