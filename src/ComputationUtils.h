#pragma once
#include <vector>
#include <QString>

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