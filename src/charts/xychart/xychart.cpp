/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/xychart_p.h>
#include <QtCharts/QXYSeries>
#include <private/qxyseries_p.h>
#include <private/chartpresenter_p.h>
#include <private/abstractdomain_p.h>
#include <private/chartdataset_p.h>
#include <private/glxyseriesdata_p.h>
#include <QtCharts/QXYModelMapper>
#include <private/qabstractaxis_p.h>
#include <QtGui/QPainter>
#include <QtCore/QAbstractItemModel>


QT_BEGIN_NAMESPACE

XYChart::XYChart(QXYSeries *series, QGraphicsItem *item):
      ChartItem(series->d_func(),item),
      m_series(series),
      m_animation(0),
      m_dirty(true)
{
    QObject::connect(series, SIGNAL(pointReplaced(int)), this, SLOT(handlePointReplaced(int)));
    QObject::connect(series, SIGNAL(pointsReplaced()), this, SLOT(handlePointsReplaced()));
    QObject::connect(series, SIGNAL(pointAdded(int)), this, SLOT(handlePointAdded(int)));
    QObject::connect(series, SIGNAL(pointRemoved(int)), this, SLOT(handlePointRemoved(int)));
    QObject::connect(series, SIGNAL(pointsRemoved(int, int)), this, SLOT(handlePointsRemoved(int, int)));
    QObject::connect(this, SIGNAL(clicked(QPointF)), series, SIGNAL(clicked(QPointF)));
    QObject::connect(this, SIGNAL(hovered(QPointF,bool)), series, SIGNAL(hovered(QPointF,bool)));
    QObject::connect(this, SIGNAL(pressed(QPointF)), series, SIGNAL(pressed(QPointF)));
    QObject::connect(this, SIGNAL(released(QPointF)), series, SIGNAL(released(QPointF)));
    QObject::connect(this, SIGNAL(doubleClicked(QPointF)), series, SIGNAL(doubleClicked(QPointF)));
    QObject::connect(series, &QAbstractSeries::useOpenGLChanged,
                     this, &XYChart::handleDomainUpdated);
}

void XYChart::setGeometryPoints(const QList<QPointF> &points)
{
    m_points = points;
}

void XYChart::setAnimation(XYAnimation *animation)
{
    m_animation = animation;
}

void XYChart::setDirty(bool dirty)
{
    m_dirty = dirty;
}

// Returns a list with same size as geometryPoints list, indicating
// the off grid status of points.
QList<bool> XYChart::offGridStatusVector()
{
    qreal minX = domain()->minX();
    qreal maxX = domain()->maxX();
    qreal minY = domain()->minY();
    qreal maxY = domain()->maxY();

    QList<bool> returnVector;
    returnVector.resize(m_points.size());
    // During remove animation series may have different number of points,
    // so ensure we don't go over the index. No need to check for zero points, this
    // will not be called in such a situation.
    const int seriesLastIndex = m_series->count() - 1;

    for (int i = 0; i < m_points.size(); i++) {
        const QPointF &seriesPoint = m_series->at(qMin(seriesLastIndex, i));
        if (seriesPoint.x() < minX
            || seriesPoint.x() > maxX
            || seriesPoint.y() < minY
            || seriesPoint.y() > maxY) {
            returnVector[i] = true;
        } else {
            returnVector[i] = false;
        }
    }
    return returnVector;
}

void XYChart::updateChart(const QList<QPointF> &oldPoints, const QList<QPointF> &newPoints,
                          int index)
{

    if (m_animation) {
        m_animation->setup(oldPoints, newPoints, index);
        m_points = newPoints;
        setDirty(false);
        presenter()->startAnimation(m_animation);
    } else {
        m_points = newPoints;
        updateGeometry();
    }
}

void XYChart::updateGlChart()
{
    dataSet()->glXYSeriesDataManager()->setPoints(m_series, domain());
    presenter()->updateGLWidget();
    updateGeometry();
}

// Doesn't update gl geometry, but refreshes the chart
void XYChart::refreshGlChart()
{
    if (presenter())
        presenter()->updateGLWidget();
}

//handlers

void XYChart::handlePointAdded(int index)
{
    Q_ASSERT(index < m_series->count());
    Q_ASSERT(index >= 0);

    if (m_series->useOpenGL()) {
        updateGlChart();
    } else {
        QList<QPointF> points;
        if (m_dirty || m_points.isEmpty()) {
            points = domain()->calculateGeometryPoints(m_series->points());
        } else {
            points = m_points;
            QPointF point =
                    domain()->calculateGeometryPoint(m_series->points().at(index), m_validData);
            if (!m_validData)
                m_points.clear();
            else
                points.insert(index, point);
        }
        updateChart(m_points, points, index);
    }
}

void XYChart::handlePointRemoved(int index)
{
    Q_ASSERT(index <= m_series->count());
    Q_ASSERT(index >= 0);

    if (m_series->useOpenGL()) {
        updateGlChart();
    } else {
        QList<QPointF> points;
        if (m_dirty || m_points.isEmpty()) {
            points = domain()->calculateGeometryPoints(m_series->points());
        } else {
            points = m_points;
            points.remove(index);
        }
        updateChart(m_points, points, index);
    }
}

void XYChart::handlePointsRemoved(int index, int count)
{
    Q_ASSERT(index <= m_series->count());
    Q_ASSERT(index >= 0);

    if (m_series->useOpenGL()) {
        updateGlChart();
    } else {
        QList<QPointF> points;
        if (m_dirty || m_points.isEmpty()) {
            points = domain()->calculateGeometryPoints(m_series->points());
        } else {
            points = m_points;
            points.remove(index, count);
        }
        updateChart(m_points, points, index);
    }
}

void XYChart::handlePointReplaced(int index)
{
    Q_ASSERT(index < m_series->count());
    Q_ASSERT(index >= 0);

    if (m_series->useOpenGL()) {
        updateGlChart();
    } else {
        QList<QPointF> points;
        if (m_dirty || m_points.isEmpty()) {
            points = domain()->calculateGeometryPoints(m_series->points());
        } else {
            QPointF point =
                    domain()->calculateGeometryPoint(m_series->points().at(index), m_validData);
            if (!m_validData)
                m_points.clear();
            points = m_points;
            if (m_validData)
                points.replace(index, point);
        }
        updateChart(m_points, points, index);
    }
}

void XYChart::handlePointsReplaced()
{
    if (m_series->useOpenGL()) {
        updateGlChart();
    } else {
        // All the points were replaced -> recalculate
        QList<QPointF> points = domain()->calculateGeometryPoints(m_series->points());
        updateChart(m_points, points, -1);
    }
}

void XYChart::handleDomainUpdated()
{
    if (m_series->useOpenGL()) {
        updateGlChart();
    } else {
        if (isEmpty()) return;
        QList<QPointF> points = domain()->calculateGeometryPoints(m_series->points());
        updateChart(m_points, points);
    }
}

bool XYChart::isEmpty()
{
    return domain()->isEmpty() || m_series->points().isEmpty();
}

QPointF XYChart::matchForLightMarker(const QPointF &eventPos)
{
    if (m_series->lightMarker().isNull())
        return QPointF(qQNaN(), qQNaN()); // 0,0 could actually be in points()

    int markerWidth =  m_series->lightMarker().width();
    int markerHeight =  m_series->lightMarker().height();

    for (const QPointF &dp : m_series->points()) {
        bool ok;
        const QPointF gp = domain()->calculateGeometryPoint(dp, ok);
        if (ok) {
            // '+2' and '+4': There is an addRect for the (mouse-)shape
            // in LineChartItem::updateGeometry()
            // This has a margin of 1 to make sure a press in the icon will always be detected,
            // but as there is a bunch of 'translations' and therefore inaccuracies,
            // so it is necessary to increase that margin to 2
            // (otherwise you can click next to an icon, get a click event but not match it)
            QRectF r(gp.x() - (markerWidth / 2 + 2),
                     gp.y() - (markerHeight / 2 + 2),
                     markerWidth + 4, markerHeight + 4);

            if (r.contains(eventPos))
                return dp;
        }
    }
    return QPointF(qQNaN(), qQNaN()); // 0,0 could actually be in points()
}

QT_END_NAMESPACE

#include "moc_xychart_p.cpp"
