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
#include <algorithm>
#include <cmath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


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

std::vector<int> filterSelectionIndicesByClusters(
    const std::vector<std::seed_seq::result_type>& selectionIndices,
    const QVector<Cluster>& clusters,
    const QStringList& selectedClusters)
{

    std::vector<int> filtered;
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
        if (embeddingXVal->getCurrentIndex() != -1) embeddingXVal->setCurrentText("");
        if (embeddingYVal->getCurrentIndex() != -1) embeddingYVal->setCurrentText("");
        if (embeddingXVal->getOptions().contains(xName)) embeddingXVal->setCurrentText(xName);
        if (embeddingYVal->getOptions().contains(yName)) embeddingYVal->setCurrentText(yName);
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
    if (type == NormalizationType::None) return;
    if (data.empty()) return;
    if (type == NormalizationType::ZScore) {
        float mean = std::accumulate(data.begin(), data.end(), 0.0f) / data.size();
        float sq_sum = std::inner_product(data.begin(), data.end(), data.begin(), 0.0f);
        float stdev = std::sqrt(sq_sum / data.size() - mean * mean);
        if (stdev > 0.0f) {
            for (auto& v : data) v = (v - mean) / stdev;
        }
    }
    else if (type == NormalizationType::MinMax) {
        auto [minIt, maxIt] = std::minmax_element(data.begin(), data.end());
        float minV = *minIt, maxV = *maxIt;
        float range = maxV - minV;
        if (range > 0.0f) {
            for (auto& v : data) v = (v - minV) / range;
        }
    }
    else if (type == NormalizationType::Log2) {
        for (auto& v : data) v = std::log2(v + 1.0f);
    }
}

NormalizationType normalizationTypeFromString(const QString& str) {
    if (str == "ZScore") return NormalizationType::ZScore;
    if (str == "MinMax") return NormalizationType::MinMax;
    if (str == "Log2") return NormalizationType::Log2;
    return NormalizationType::None;
}
void selectFirstRow(QTableView* tableView)
{
    if (tableView) {
        auto* model = tableView->model();
        if (model && model->rowCount() > 0) {
            QModelIndex firstIndex = model->index(0, 0);
            if (firstIndex.isValid()) {
                tableView->setCurrentIndex(firstIndex);
                tableView->selectionModel()->select(
                    firstIndex,
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
                );
            }
        }
    }
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