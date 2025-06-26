#pragma once
#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QStyleOptionFocusRect>
#include <QStyle>
#include <QModelIndex>
#include <QHelpEvent>
#include <QAbstractItemView>
#include <QToolTip>
#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QPen>
#include <QRect>
#include <QString>
#include <algorithm>
#include <cmath>

class CorrelationBarDelegate : public QStyledItemDelegate {
public:
    CorrelationBarDelegate(float minCorr, float maxCorr, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override;

    bool helpEvent(QHelpEvent* event, QAbstractItemView* view,
        const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    float minValue, maxValue;
};

