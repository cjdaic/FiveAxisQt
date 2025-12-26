#include "DrawingPanel.h"

#include <QButtonGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

DrawingPanel::DrawingPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void DrawingPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* buttonsLayout = new QHBoxLayout();
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);

    addToolButton(group, buttonsLayout, DrawingView::Mode::Pointer, tr("Pointer"), true);
    addToolButton(group, buttonsLayout, DrawingView::Mode::Line, tr("Line"));
    addToolButton(group, buttonsLayout, DrawingView::Mode::Circle, tr("Circle"));
    addToolButton(group, buttonsLayout, DrawingView::Mode::Rectangle, tr("Rect"));
    addToolButton(group, buttonsLayout, DrawingView::Mode::Ellipse, tr("ellipse"));
    addToolButton(group, buttonsLayout, DrawingView::Mode::Rectangle3D, tr("Rect3D"));

    layout->addLayout(buttonsLayout);

    m_view = new DrawingView(this);
    m_view->setMinimumHeight(300);
    layout->addWidget(m_view, 1);

    connect(group, &QButtonGroup::idClicked, this, [this](int id) {
        m_view->setMode(static_cast<DrawingView::Mode>(id));
        });
    connect(m_view, &DrawingView::shapeCreated, this, [this](const QString& id, const QString& type, QGraphicsItem* item) {
        if (item) {
            m_items.insert(id, item);
        }
        emit shapeCreated(id, type);
        });
    connect(m_view, &DrawingView::shapeSelected, this, &DrawingPanel::shapeSelected);
    connect(m_view, &DrawingView::shapeMoved, this, &DrawingPanel::shapeMoved);
    connect(m_view, &DrawingView::shapeRemoved, this, [this](const QString& id, const QString& type) {
        m_items.remove(id);
        emit shapeRemoved(id, type);
        });
}

void DrawingPanel::addToolButton(QButtonGroup* group, QHBoxLayout* buttonsLayout, DrawingView::Mode mode, const QString& text, bool checked) {
    auto* btn = new QToolButton(this);
    btn->setText(text);
    btn->setCheckable(true);
    btn->setChecked(checked);
    group->addButton(btn, static_cast<int>(mode));
    buttonsLayout->addWidget(btn);
}

// 3D/model loading removed per latest requirements; DrawingPanel stays 2D.

void DrawingPanel::removeShape(const QString& id) {
    auto it = m_items.find(id);
    if (it == m_items.end()) {
        return;
    }
    auto* item = it.value();
    m_view->scene()->removeItem(item);
    delete item;
    m_items.erase(it);
    emit shapeRemoved(id, QString());
}

void DrawingPanel::selectShape(const QString& id) {
    auto it = m_items.constFind(id);
    if (it == m_items.constEnd()) {
        return;
    }
    auto* item = it.value();
    m_view->scene()->clearSelection();
    item->setSelected(true);
    m_view->centerOn(item);
    emit shapeSelected(id, item->data(0).toString());
}

void DrawingPanel::setMode(DrawingView::Mode mode) {
    m_view->setMode(mode);
}

bool DrawingPanel::shapeInfo(const QString& id, ShapeInfo& info) const {
    auto it = m_items.constFind(id);
    if (it == m_items.constEnd()) {
        return false;
    }
    auto* item = it.value();
    info.id = id;
    info.type = item->data(0).toString();
    info.rect = item->boundingRect().translated(item->pos());
    if (auto* line = qgraphicsitem_cast<QGraphicsLineItem*>(item)) {
        const auto lineObj = line->line();
        info.p1 = lineObj.p1() + line->pos();
        info.p2 = lineObj.p2() + line->pos();
    }
    else if (auto* path = qgraphicsitem_cast<QGraphicsPathItem*>(item)) {
        info.p1 = path->pos();
        info.p2 = path->pos();
    }
    else {
        info.p1 = info.rect.topLeft();
        info.p2 = info.rect.bottomRight();
    }
    return true;
}