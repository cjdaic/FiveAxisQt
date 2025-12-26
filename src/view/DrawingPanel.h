#pragma once

#include <QWidget>

#include <QGraphicsItem>
#include <QHash>

#include "DrawingView.h"

class QButtonGroup;
class QToolButton;
class QHBoxLayout;

class DrawingPanel : public QWidget {
    Q_OBJECT
public:
    explicit DrawingPanel(QWidget* parent = nullptr);
    void removeShape(const QString& id);
    void selectShape(const QString& id);
    void setMode(DrawingView::Mode mode);
    struct ShapeInfo {
        QString id;
        QString type;
        QPointF p1;
        QPointF p2;
        QRectF rect;
    };
    bool shapeInfo(const QString& id, ShapeInfo& info) const;

signals:
    void shapeCreated(const QString& id, const QString& type);
    void shapeSelected(const QString& id, const QString& type);
    void shapeMoved(const QString& id, const QString& type);
    void shapeRemoved(const QString& id, const QString& type);

private:
    void setupUi();
    void addToolButton(QButtonGroup* group, QHBoxLayout* buttonsLayout, DrawingView::Mode mode, const QString& text, bool checked = false);

    DrawingView* m_view{};
    QHash<QString, QGraphicsItem*> m_items;
};