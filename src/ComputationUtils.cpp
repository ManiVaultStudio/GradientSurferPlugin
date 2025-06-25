#include "ComputationUtils.h"

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

FunctionTimer::FunctionTimer(const QString& functionName)
    : _functionName(functionName)
{
    _timer.start();
}
FunctionTimer::~FunctionTimer()
{
    qDebug() << _functionName << "took"
        << _timer.elapsed() / 1000.0 << "seconds";
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
void normalizeVector(std::vector<float>& vec, NormalizationType type) {
    if (vec.empty() || type == NormalizationType::None)
        return;

    const float EPSILON = 1e-8f; 

    switch (type) {
    case NormalizationType::ZScore: {
        float sum = std::accumulate(vec.begin(), vec.end(), 0.0f);
        float mean = sum / vec.size();

        float variance = 0.0f;
        for (float v : vec) {
            float diff = v - mean;
            variance += diff * diff;
        }
        variance /= vec.size();

        float stdev = std::sqrt(variance);
        if (stdev < EPSILON) stdev = 1.0f;

        for (auto& v : vec) {
            v = (v - mean) / stdev;
        }
        break;
    }
    case NormalizationType::MinMax: {
        auto [minIt, maxIt] = std::minmax_element(vec.begin(), vec.end());
        float minVal = *minIt;
        float maxVal = *maxIt;
        float range = maxVal - minVal;

        if (range < EPSILON) range = 1.0f;

        for (auto& v : vec) {
            v = (v - minVal) / range;
        }
        break;
    }
    case NormalizationType::Log2: {
        for (auto& v : vec) {
            v = (v > 0.0f) ? std::log2(v + 1.0f) : 0.0f;
        }
        break;
    }
    default:
        assert(false && "Unknown normalization type");
        break;
    }
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