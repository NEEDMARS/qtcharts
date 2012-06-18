//  W A R N I N G
//  -------------
//
// This file is not part of the QtCommercial Chart API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef STACKEDBARANIMATION_P_H
#define STACKEDBARANIMATION_P_H

#include "chartanimation_p.h"
#include "barchartitem_p.h"

QTCOMMERCIALCHART_BEGIN_NAMESPACE

class StackedBarChartItem;
class QBarSet;
class BarSetAnimation;

class StackedBarAnimation : public ChartAnimation
{
    Q_OBJECT
public:
    StackedBarAnimation(BarChartItem *item);
    ~StackedBarAnimation();

public:
    virtual QVariant interpolated(const QVariant &from, const QVariant &to, qreal progress) const;
    virtual void updateCurrentValue(const QVariant &value);

    void updateLayout(const QVector<QRectF> &oldLayout, const QVector<QRectF> &newLayout);

private:
    BarChartItem *m_item;
    QHash<QBarSet *, BarSetAnimation *> m_animations;

};

QTCOMMERCIALCHART_END_NAMESPACE

#endif // STACKEDBARANIMATION_P_H