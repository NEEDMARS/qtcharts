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

#include <private/scatterchartitem_p.h>
#include <QtCharts/QScatterSeries>
#include <private/qscatterseries_p.h>
#include <private/chartpresenter_p.h>
#include <private/abstractdomain_p.h>
#include <QtCharts/QChart>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsScene>
#include <QtCore/QDebug>
#include <QtWidgets/QGraphicsSceneMouseEvent>

QT_BEGIN_NAMESPACE

namespace {
constexpr short STAR_SPIKES = 5;
}

ScatterChartItem::ScatterChartItem(QScatterSeries *series, QGraphicsItem *item)
    : XYChart(series,item),
      m_series(series),
      m_items(this),
      m_visible(true),
      m_shape(QScatterSeries::MarkerShapeRectangle),
      m_size(15),
      m_pointLabelsVisible(false),
      m_pointLabelsFormat(series->pointLabelsFormat()),
      m_pointLabelsFont(series->pointLabelsFont()),
      m_pointLabelsColor(series->pointLabelsColor()),
      m_pointLabelsClipping(true),
      m_mousePressed(false)
{
    QObject::connect(m_series->d_func(), SIGNAL(updated()), this, SLOT(handleUpdated()));
    QObject::connect(m_series, SIGNAL(visibleChanged()), this, SLOT(handleUpdated()));
    QObject::connect(m_series, SIGNAL(opacityChanged()), this, SLOT(handleUpdated()));
    QObject::connect(series, SIGNAL(pointLabelsFormatChanged(QString)),
                     this, SLOT(handleUpdated()));
    QObject::connect(series, SIGNAL(pointLabelsVisibilityChanged(bool)),
                     this, SLOT(handleUpdated()));
    QObject::connect(series, SIGNAL(pointLabelsFontChanged(QFont)), this, SLOT(handleUpdated()));
    QObject::connect(series, SIGNAL(pointLabelsColorChanged(QColor)), this, SLOT(handleUpdated()));
    QObject::connect(series, SIGNAL(pointLabelsClippingChanged(bool)), this, SLOT(handleUpdated()));
    connect(series, &QXYSeries::selectedColorChanged, this, &ScatterChartItem::handleUpdated);
    connect(series, &QXYSeries::selectedPointsChanged, this, &ScatterChartItem::handleUpdated);
    QObject::connect(series, &QScatterSeries::pointsConfigurationChanged, this,
                     &ScatterChartItem::handleUpdated);

    setZValue(ChartPresenter::ScatterSeriesZValue);
    setFlags(QGraphicsItem::ItemClipsChildrenToShape);

    handleUpdated();

    m_items.setHandlesChildEvents(false);
}

QRectF ScatterChartItem::boundingRect() const
{
    return m_rect;
}

void ScatterChartItem::createPoints(int count)
{
    for (int i = 0; i < count; ++i) {

        QGraphicsItem *item = 0;

        switch (m_shape) {
        case QScatterSeries::MarkerShapeCircle: {
            item = new ChartMarker<QGraphicsEllipseItem>(0, 0, m_size, m_size, this);
            break;
        }
        case QScatterSeries::MarkerShapeRectangle: {
            item = new ChartMarker<QGraphicsRectItem>(0, 0, m_size, m_size, this);
            break;
        }
        case QScatterSeries::MarkerShapeRotatedRectangle: {
            item = new RotatedRectangleMarker(0, 0, m_size, m_size, this);
            break;
        }
        case QScatterSeries::MarkerShapeTriangle: {
            item = new TriangleMarker(0, 0, m_size, m_size, this);
            break;
        }
        case QScatterSeries::MarkerShapeStar: {
            item = new StarMarker(0, 0, m_size, m_size, this);
            break;
        }
        case QScatterSeries::MarkerShapePentagon: {
            item = new PentagonMarker(0, 0, m_size, m_size, this);
            break;
        }
        default:
            qWarning() << "Unsupported marker type";
            break;
        }

        m_items.addToGroup(item);
    }
}

void ScatterChartItem::deletePoints(int count)
{
    QList<QGraphicsItem *> items = m_items.childItems();

    for (int i = 0; i < count; ++i) {
        QGraphicsItem *item = items.takeLast();
        m_markerMap.remove(item);
        delete(item);
    }
}

void ScatterChartItem::resizeMarker(QGraphicsItem *marker, const int size)
{
    switch (m_shape) {
    case QScatterSeries::MarkerShapeCircle: {
        QGraphicsEllipseItem *item = static_cast<QGraphicsEllipseItem *>(marker);
        item->setRect(item->x(), item->y(), size, size);
        break;
    }
    case QScatterSeries::MarkerShapeRectangle: {
        QGraphicsRectItem *item = static_cast<QGraphicsRectItem *>(marker);
        item->setRect(item->x(), item->y(), size, size);
        break;
    }
    case QScatterSeries::MarkerShapeRotatedRectangle: {
        QGraphicsPolygonItem *item = static_cast<QGraphicsPolygonItem *>(marker);
        item->setPolygon(RotatedRectangleMarker::polygon(item->x(), item->y(), size, size));
        break;
    }
    case QScatterSeries::MarkerShapeTriangle: {
        QGraphicsPolygonItem *item = static_cast<QGraphicsPolygonItem *>(marker);
        item->setPolygon(TriangleMarker::polygon(item->x(), item->y(), size, size));
        break;
    }
    case QScatterSeries::MarkerShapeStar: {
        QGraphicsPolygonItem *item = static_cast<QGraphicsPolygonItem *>(marker);
        item->setPolygon(StarMarker::polygon(item->x(), item->y(), size, size));
        break;
    }
    case QScatterSeries::MarkerShapePentagon: {
        QGraphicsPolygonItem *item = static_cast<QGraphicsPolygonItem *>(marker);
        item->setPolygon(PentagonMarker::polygon(item->x(), item->y(), size, size));
        break;
    }
    default:
        qWarning() << "Unsupported marker type";
        break;
    }
}

void ScatterChartItem::markerSelected(QGraphicsItem *marker)
{
    emit XYChart::clicked(m_markerMap[marker]);
}

void ScatterChartItem::markerHovered(QGraphicsItem *marker, bool state)
{
    emit XYChart::hovered(m_markerMap[marker], state);
}

void ScatterChartItem::markerPressed(QGraphicsItem *marker)
{
    emit XYChart::pressed(m_markerMap[marker]);
}

void ScatterChartItem::markerReleased(QGraphicsItem *marker)
{
    emit XYChart::released(m_markerMap[marker]);
}

void ScatterChartItem::markerDoubleClicked(QGraphicsItem *marker)
{
    emit XYChart::doubleClicked(m_markerMap[marker]);
}

void ScatterChartItem::updateGeometry()
{
    if (m_series->useOpenGL()) {
        if (m_items.childItems().count())
            deletePoints(m_items.childItems().count());
        if (!m_rect.isEmpty()) {
            prepareGeometryChange();
            // Changed signal seems to trigger even with empty region
            m_rect = QRectF();
        }
        update();
        return;
    }

    const QList<QPointF> &points = geometryPoints();

    if (points.size() == 0) {
        deletePoints(m_items.childItems().count());
        return;
    }

    int diff = m_items.childItems().size() - points.size();

    if (diff > 0)
        deletePoints(diff);
    else if (diff < 0)
        createPoints(-diff);

    if (diff != 0)
        handleUpdated();

    QList<QGraphicsItem *> items = m_items.childItems();

    QRectF clipRect(QPointF(0,0),domain()->size());

    // Only zoom in if the clipRect fits inside int limits. QWidget::update() uses
    // a region that has to be compatible with QRect.
    if (clipRect.height() <= INT_MAX
            && clipRect.width() <= INT_MAX) {
        const QList<bool> offGridStatus = offGridStatusVector();
        const int seriesLastIndex = m_series->count() - 1;

        for (int i = 0; i < points.size(); i++) {
            QAbstractGraphicsShapeItem *item =
                    static_cast<QAbstractGraphicsShapeItem *>(items.at(i));
            const QPointF &point = points.at(i);

            if (m_pointsConfiguration.contains(i) && m_pointsConfigurationDirty) {
                const auto &conf = m_pointsConfiguration[i];
                if (conf.contains(QXYSeries::PointConfiguration::Size))
                    resizeMarker(
                            item,
                            m_pointsConfiguration[i][QXYSeries::PointConfiguration::Size].toReal());
            }

            const QRectF &rect = item->boundingRect();
            // During remove animation series may have different number of points,
            // so ensure we don't go over the index. Animation handling itself ensures that
            // if there is actually no points in the series, then it won't generate a fake point,
            // so we can be assured there is always at least one point in m_series here.
            // Note that marker map values can be technically incorrect during the animation,
            // if it was caused by an insert, but this shouldn't be a problem as the points are
            // fake anyway. After remove animation stops, geometry is updated to correct one.
            m_markerMap[item] = m_series->at(qMin(seriesLastIndex, i));
            QPointF position;
            position.setX(point.x() - rect.width() / 2);
            position.setY(point.y() - rect.height() / 2);
            item->setPos(position);

            if (!m_visible || offGridStatus.at(i)) {
                item->setVisible(false);
            } else {
                bool drawPoint = true;
                if (m_pointsConfiguration.contains(i)) {
                    const auto &conf = m_pointsConfiguration[i];

                    if (conf.contains(QXYSeries::PointConfiguration::Visibility)) {
                        drawPoint
                                = m_pointsConfiguration[i][QXYSeries::PointConfiguration::Visibility]
                                .toBool();
                    }

                    if (drawPoint && conf.contains(QXYSeries::PointConfiguration::Color)) {
                        item->setBrush(
                                m_pointsConfiguration[i][QXYSeries::PointConfiguration::Color]
                                        .value<QColor>());
                    }
                }

                item->setVisible(drawPoint);
            }
        }

        prepareGeometryChange();
        m_rect = clipRect;
    }
}

void ScatterChartItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_series->useOpenGL())
        return;

    // Draw markers if a marker has been set (set to QImage() to disable)
    if (!m_series->lightMarker().isNull()) {
        const QImage &marker = m_series->lightMarker();
        int markerHalfWidth = marker.width() / 2;
        int markerHalfHeight = marker.height() / 2;

        for (const auto &point : qAsConst(m_points)) {
            painter->drawImage(point.x() - markerHalfWidth,
                               point.y() - markerHalfHeight,
                               marker);
        }
    }

    QRectF clipRect = QRectF(QPointF(0, 0), domain()->size());

    painter->save();
    painter->setClipRect(clipRect);

    if (m_series->bestFitLineVisible())
        m_series->d_func()->drawBestFitLine(painter, clipRect);

    m_series->d_func()->drawPointLabels(painter, m_points, m_series->markerSize() / 2 + m_series->pen().width());

    painter->restore();
}

void ScatterChartItem::setPen(const QPen &pen)
{
    foreach (QGraphicsItem *item , m_items.childItems())
        static_cast<QAbstractGraphicsShapeItem*>(item)->setPen(pen);
}

void ScatterChartItem::setBrush(const QBrush &brush)
{

    const auto &items = m_items.childItems();
    for (auto item : items) {
        if (m_markerMap.contains(item)) {
            auto index = m_series->points().indexOf(m_markerMap[item]);
            if (m_selectedPoints.contains(index) && m_selectedColor.isValid()) {
                static_cast<QAbstractGraphicsShapeItem *>(item)->setBrush(m_selectedColor);
            } else {
                bool useBrush = true;
                if (m_pointsConfiguration.contains(index)) {
                    const auto &conf = m_pointsConfiguration[index];
                    if (conf.contains(QXYSeries::PointConfiguration::Color))
                        useBrush = false;
                }

                if (useBrush)
                    static_cast<QAbstractGraphicsShapeItem *>(item)->setBrush(brush);
            }
        } else {
            static_cast<QAbstractGraphicsShapeItem *>(item)->setBrush(brush);
        }
    }
}

void ScatterChartItem::handleUpdated()
{
    if (m_series->useOpenGL()) {
        if ((m_series->isVisible() != m_visible)) {
            m_visible = m_series->isVisible();
            refreshGlChart();
        }
        return;
    }

    int count = m_items.childItems().count();
    if (count == 0)
        return;

    m_pointsConfigurationDirty = m_series->pointsConfiguration() != m_pointsConfiguration;

    bool recreate = m_visible != m_series->isVisible()
                    || m_size != m_series->markerSize()
                    || m_shape != m_series->markerShape()
                    || m_selectedColor != m_series->selectedColor()
                    || m_selectedPoints != m_series->selectedPoints()
                    || m_pointsConfigurationDirty;
    m_visible = m_series->isVisible();
    m_size = m_series->markerSize();
    m_shape = m_series->markerShape();
    setVisible(m_visible);
    setOpacity(m_series->opacity());
    m_pointLabelsFormat = m_series->pointLabelsFormat();
    m_pointLabelsVisible = m_series->pointLabelsVisible();
    m_pointLabelsFont = m_series->pointLabelsFont();
    m_pointLabelsColor = m_series->pointLabelsColor();
    m_selectedColor = m_series->selectedColor();
    m_selectedPoints = m_series->selectedPoints();
    m_pointsConfiguration = m_series->pointsConfiguration();
    bool labelClippingChanged = m_pointLabelsClipping != m_series->pointLabelsClipping();
    m_pointLabelsClipping = m_series->pointLabelsClipping();

    if (recreate) {
        deletePoints(count);
        createPoints(count);

        // Updating geometry is now safe, because it won't call handleUpdated unless it creates/deletes points
        updateGeometry();
    }

    setPen(m_series->pen());
    setBrush(m_series->brush());
    // Update whole chart in case label clipping changed as labels can be outside series area
    if (labelClippingChanged)
        m_series->chart()->update();
    else
        update();
}

template<class T>
ChartMarker<T>::ChartMarker(qreal x, qreal y, qreal w, qreal h, ScatterChartItem *parent)
    : T(x, y, w, h, parent)
    , m_parent(parent)
{
    T::setAcceptHoverEvents(true);
    T::setFlag(QGraphicsItem::ItemIsSelectable);
}

template<class T>
ChartMarker<T>::ChartMarker(ScatterChartItem *parent)
    : T(parent)
    , m_parent(parent)
{
    T::setAcceptHoverEvents(true);
    T::setFlag(QGraphicsItem::ItemIsSelectable);
}

template<class T>
void ChartMarker<T>::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    T::mousePressEvent(event);
    m_parent->markerPressed(this);
    m_parent->setMousePressed();
}

template<class T>
void ChartMarker<T>::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    T::hoverEnterEvent(event);
    m_parent->markerHovered(this, true);
}

template<class T>
void ChartMarker<T>::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    T::hoverLeaveEvent(event);
    m_parent->markerHovered(this, false);
}

template<class T>
void ChartMarker<T>::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    T::mouseReleaseEvent(event);
    m_parent->markerReleased(this);
    if (m_parent->mousePressed())
        m_parent->markerSelected(this);
    m_parent->setMousePressed(false);
}

template<class T>
void ChartMarker<T>::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    T::mouseDoubleClickEvent(event);
    m_parent->markerDoubleClicked(this);
}

RotatedRectangleMarker::RotatedRectangleMarker(qreal x, qreal y, qreal w, qreal h,
                                               ScatterChartItem *parent)
    : ChartMarker<QGraphicsPolygonItem>(parent)
{
    setPolygon(RotatedRectangleMarker::polygon(x, y, w, h));
}

QPolygonF RotatedRectangleMarker::polygon(qreal x, qreal y, qreal w, qreal h)
{
    QPolygonF rotatedRectPoly;
    rotatedRectPoly << QPointF(x, y + h / 2.0);
    rotatedRectPoly << QPointF(x + w / 2.0, y + h);
    rotatedRectPoly << QPointF(x + w, y + h / 2.0);
    rotatedRectPoly << QPointF(x + w / 2.0, y);

    return rotatedRectPoly;
}

TriangleMarker::TriangleMarker(qreal x, qreal y, qreal w, qreal h, ScatterChartItem *parent)
    : ChartMarker<QGraphicsPolygonItem>(parent)
{
    setPolygon(TriangleMarker::polygon(x, y, w, h));
}

QPolygonF TriangleMarker::polygon(qreal x, qreal y, qreal w, qreal h)
{
    QPolygonF trianglePoly;
    trianglePoly << QPointF(x, y + h);
    trianglePoly << QPointF(x + w, y + h);
    trianglePoly << QPointF(x + w / 2.0, y);

    return trianglePoly;
}

StarMarker::StarMarker(qreal x, qreal y, qreal w, qreal h, ScatterChartItem *parent)
    : ChartMarker<QGraphicsPolygonItem>(parent)
{
    setPolygon(StarMarker::polygon(x, y, w, h));
}

QPolygonF StarMarker::polygon(qreal x, qreal y, qreal w, qreal h)
{
    QPolygonF starPoly;

    constexpr qreal step = M_PI / STAR_SPIKES;
    const qreal radius = w / 2.0;
    const qreal innerRadius = radius * 0.5;
    const QPointF &center = QPointF(x + w / 2.0, y + h / 2.0);
    qreal rot = M_PI / 2 * 3;

    for (int i = 0; i < STAR_SPIKES; ++i) {
        starPoly << QPointF(center.x() + std::cos(rot) * radius,
                            center.y() + std::sin(rot) * radius);
        rot += step;

        starPoly << QPointF(center.x() + std::cos(rot) * innerRadius,
                            center.y() + std::sin(rot) * innerRadius);
        rot += step;
    }

    return starPoly;
}

PentagonMarker::PentagonMarker(qreal x, qreal y, qreal w, qreal h, ScatterChartItem *parent)
    : ChartMarker<QGraphicsPolygonItem>(parent)
{
    setPolygon(PentagonMarker::polygon(x, y, w, h));
}

QPolygonF PentagonMarker::polygon(qreal x, qreal y, qreal w, qreal h)
{
    QPolygonF pentagonPoly;

    constexpr qreal step = 2 * M_PI / 5;
    const qreal radius = w / 2.0;
    const QPointF &center = QPointF(x + w / 2.0, y + h / 2.0);
    qreal rot = M_PI / 2 * 3;

    for (int i = 0; i < 5; ++i) {
        pentagonPoly << QPointF(center.x() + std::cos(rot) * radius,
                                center.y() + std::sin(rot) * radius);
        rot += step;
    }

    return pentagonPoly;
}

QT_END_NAMESPACE

#include "moc_scatterchartitem_p.cpp"
