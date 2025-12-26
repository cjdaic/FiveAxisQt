#pragma once

#include <QGraphicsView>

#include <QGraphicsItem>
#include <QHash>
#include <QSet>

class DrawingView : public QGraphicsView {
    Q_OBJECT
public:
    enum class Mode {
        Pointer,
        Line,
        Circle,
        Rectangle,
        Ellipse,
        Rectangle3D,
    };

    explicit DrawingView(QWidget* parent = nullptr);

    void setMode(Mode mode);

signals:
    void shapeCreated(const QString& id, const QString& type, QGraphicsItem* item);
    void shapeSelected(const QString& id, const QString& type);
    void shapeMoved(const QString& id, const QString& type);
    void shapeRemoved(const QString& id, const QString& type);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void beginShape(const QPointF& scenePos);
    void updateShape(const QPointF& scenePos);
    void finishShape();
    QString currentTypeName() const;
    QString ensureId(const QString& type);
    QString idForItem(QGraphicsItem* item) const;

    Mode m_mode{ Mode::Pointer };
    bool m_drawing{ false };
    bool m_panning{ false };
    bool m_pointerDragging{ false };
    QPoint m_lastPanPos;
    QPointF m_lastPointerScenePos;
    QPointF m_startPos;
    QGraphicsItem* m_activeItem{ nullptr };
    QGraphicsItem* m_pointerItem{ nullptr };
    QPointF m_pointerStartPos;
    QString m_pointerItemId;
    int m_nextId{ 1 };
    QHash<QGraphicsItem*, QString> m_itemIds;
};