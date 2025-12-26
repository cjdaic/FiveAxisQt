#include "DrawingView.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QWheelEvent>

namespace {
    QPen shapePen() {
        QPen pen(Qt::blue);
        pen.setWidth(2);
        pen.setCosmetic(true);
        return pen;
    }

    QPainterPath buildRect3DPath(const QPointF& start, const QPointF& end) {
        QRectF rect(QPointF(qMin(start.x(), end.x()), qMin(start.y(), end.y())),
            QPointF(qMax(start.x(), end.x()), qMax(start.y(), end.y())));
        const QPointF offset(8.0, -8.0);

        QPainterPath path;
        path.addRect(rect);

        QRectF offsetRect = rect.translated(offset);
        path.addRect(offsetRect);

        path.moveTo(rect.topLeft());
        path.lineTo(offsetRect.topLeft());
        path.moveTo(rect.topRight());
        path.lineTo(offsetRect.topRight());
        path.moveTo(rect.bottomLeft());
        path.lineTo(offsetRect.bottomLeft());
        path.moveTo(rect.bottomRight());
        path.lineTo(offsetRect.bottomRight());
        return path;
    }
}

DrawingView::DrawingView(QWidget* parent)
    : QGraphicsView(parent) {
    auto* scenePtr = new QGraphicsScene(this);
    setScene(scenePtr);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setMouseTracking(true);
    setDragMode(QGraphicsView::NoDrag);
    scene()->setSceneRect(-1000, -1000, 2000, 2000);

    // lightweight grid to mirror the web preview panel
    const int step = 50;
    QPen gridPen(QColor(220, 220, 220));
    gridPen.setCosmetic(true);
    for (int x = -1000; x <= 1000; x += step) {
        scene()->addLine(x, -1000, x, 1000, gridPen);
    }
    for (int y = -1000; y <= 1000; y += step) {
        scene()->addLine(-1000, y, 1000, y, gridPen);
    }
}

QString DrawingView::currentTypeName() const {
    switch (m_mode) {
    case Mode::Line:
        return QStringLiteral("Line");
    case Mode::Circle:
        return QStringLiteral("Circle");
    case Mode::Rectangle:
        return QStringLiteral("Rectangle");
    case Mode::Ellipse:
        return QStringLiteral("Ellipse");
    case Mode::Rectangle3D:
        return QStringLiteral("Rectangle3D");
    case Mode::Pointer:
        return QString();
    }
    return QString();
}

QString DrawingView::ensureId(const QString& type) {
    const QString id = QStringLiteral("%1%2").arg(type.toLower()).arg(m_nextId++);
    return id;
}

QString DrawingView::idForItem(QGraphicsItem* item) const {
    return m_itemIds.value(item);
}

void DrawingView::setMode(DrawingView::Mode mode) {
    m_mode = mode;
    if (m_mode == Mode::Pointer) {
        setCursor(Qt::ArrowCursor);
        setDragMode(QGraphicsView::RubberBandDrag);
        setInteractive(true);
    }
    else {
        setCursor(Qt::CrossCursor);
        setDragMode(QGraphicsView::NoDrag);
        setInteractive(false);
    }
}

void DrawingView::wheelEvent(QWheelEvent* event) {
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    scale(factor, factor);
}

void DrawingView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (m_mode == Mode::Pointer && event->button() == Qt::LeftButton) {
        auto* item = itemAt(event->pos());
        if (item) {
            m_pointerItem = item;
            m_pointerStartPos = item->pos();
            m_pointerItemId = idForItem(item);
            m_lastPointerScenePos = mapToScene(event->pos());
            emit shapeSelected(m_pointerItemId, item->data(0).toString());
            m_pointerDragging = true;
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::LeftButton && m_mode != Mode::Pointer) {
        m_startPos = mapToScene(event->pos());
        beginShape(m_startPos);
        m_drawing = true;
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void DrawingView::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        translate(delta.x(), delta.y());
        event->accept();
        return;
    }

    if (m_drawing && m_activeItem) {
        updateShape(mapToScene(event->pos()));
        event->accept();
        return;
    }

    if (m_mode == Mode::Pointer && m_pointerDragging && m_pointerItem) {
        const QPointF scenePos = mapToScene(event->pos());
        const QPointF delta = scenePos - m_lastPointerScenePos;
        m_lastPointerScenePos = scenePos;
        m_pointerItem->moveBy(delta.x(), delta.y());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void DrawingView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        setCursor(m_mode == Mode::Pointer ? Qt::ArrowCursor : Qt::CrossCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_drawing) {
        updateShape(mapToScene(event->pos()));
        finishShape();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_mode == Mode::Pointer && m_pointerDragging) {
        if (m_pointerItem && m_pointerItem->pos() != m_pointerStartPos) {
            emit shapeMoved(m_pointerItemId, m_pointerItem->data(0).toString());
        }
        m_pointerDragging = false;
        m_pointerItem = nullptr;
        m_pointerItemId.clear();
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void DrawingView::beginShape(const QPointF& scenePos) {
    QPen pen = shapePen();
    switch (m_mode) {
    case Mode::Line: {
        auto* item = scene()->addLine(QLineF(scenePos, scenePos), pen);
        item->setData(0, currentTypeName());
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_activeItem = item;
        break;
    }
    case Mode::Circle: {
        auto* item = scene()->addEllipse(QRectF(scenePos, scenePos), pen);
        item->setData(0, currentTypeName());
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_activeItem = item;
        break;
    }
    case Mode::Rectangle: {
        auto* item = scene()->addRect(QRectF(scenePos, scenePos), pen);
        item->setData(0, currentTypeName());
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_activeItem = item;
        break;
    }
    case Mode::Ellipse: {
        auto* item = scene()->addEllipse(QRectF(scenePos, scenePos), pen);
        item->setData(0, currentTypeName());
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_activeItem = item;
        break;
    }
    case Mode::Rectangle3D: {
        auto* item = scene()->addPath(buildRect3DPath(scenePos, scenePos), pen);
        item->setData(0, currentTypeName());
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_activeItem = item;
        break;
    }
    case Mode::Pointer:
        break;
    }
}

void DrawingView::updateShape(const QPointF& scenePos) {
    if (!m_activeItem) {
        return;
    }
    switch (m_mode) {
    case Mode::Line: {
        auto* line = qgraphicsitem_cast<QGraphicsLineItem*>(m_activeItem);
        if (line) {
            line->setLine(QLineF(m_startPos, scenePos));
        }
        break;
    }
    case Mode::Circle: {
        auto* ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(m_activeItem);
        if (ellipse) {
            const qreal radius = QLineF(m_startPos, scenePos).length();
            QRectF rect(m_startPos.x() - radius, m_startPos.y() - radius, radius * 2, radius * 2);
            ellipse->setRect(rect.normalized());
        }
        break;
    }
    case Mode::Rectangle: {
        auto* rect = qgraphicsitem_cast<QGraphicsRectItem*>(m_activeItem);
        if (rect) {
            rect->setRect(QRectF(m_startPos, scenePos).normalized());
        }
        break;
    }
    case Mode::Ellipse: {
        auto* ellipse = qgraphicsitem_cast<QGraphicsEllipseItem*>(m_activeItem);
        if (ellipse) {
            ellipse->setRect(QRectF(m_startPos, scenePos).normalized());
        }
        break;
    }
    case Mode::Rectangle3D: {
        auto* pathItem = qgraphicsitem_cast<QGraphicsPathItem*>(m_activeItem);
        if (pathItem) {
            pathItem->setPath(buildRect3DPath(m_startPos, scenePos));
        }
        break;
    }
    case Mode::Pointer:
        break;
    }
}

void DrawingView::finishShape() {
    m_drawing = false;
    if (m_activeItem) {
        m_activeItem->setFlag(QGraphicsItem::ItemIsMovable, true);
        const QString typeName = currentTypeName();
        const QString id = ensureId(typeName);
        m_itemIds.insert(m_activeItem, id);
        emit shapeCreated(id, typeName, m_activeItem);
    }
    m_activeItem = nullptr;
}