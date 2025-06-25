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

class FunctionTimer {
public:
    FunctionTimer(const QString& functionName)
        : _functionName(functionName)
    {
        _timer.start();
    }
    ~FunctionTimer()
    {
        qDebug() << _functionName << "took"
            << _timer.elapsed() / 1000.0 << "seconds";
    }
private:
    QString _functionName;
    QElapsedTimer _timer;
};

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