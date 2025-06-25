#pragma once
#include <vector>
#include <QString>
#include <string>
#include <QElapsedTimer>

enum class NormalizationType {
    None,
    ZScore,
    MinMax,
    Log2
};

float computeGradientScore(const std::vector<float>& corrArray, const std::vector<float>& expArray);
void normalizeVector(std::vector<float>& vec, NormalizationType type);
QString getContrastingTextColor(const QString& bgColor);
QString statusToIconName(const QString& status);

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