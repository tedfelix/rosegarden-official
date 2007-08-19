#include <QGraphicsView>
#include <QScrollBar>
#include <QGraphicsItem>

#include "GraphicsItemPin.h"

GraphicsItemPin::GraphicsItemPin(QGraphicsView* view)
    : QObject(view),
    m_scene(view->scene()),
    m_currentX(view->horizontalScrollBar()->value()),
    m_currentY(view->verticalScrollBar()->value())
{
    connect(view->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotHorizontalValueChanged(int)));

    connect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalValueChanged(int)));
    
}

GraphicsItemPin::~GraphicsItemPin()
{
}

void GraphicsItemPin::pinItem(QGraphicsItem* item, QPointF pointf)
{
    m_items.push_back(item);
}

void GraphicsItemPin::releaseItem(QGraphicsItem* item)
{
    Items::iterator it = std::find(m_items.begin(), m_items.end(), item);
    if (it != m_items.end()) {
        m_items.erase(it);
    }
}

void GraphicsItemPin::slotHorizontalValueChanged(int v)
{
    int dx = v - m_currentX;

    for(Items::iterator i = m_items.begin(); i != m_items.end(); ++i) {
        QGraphicsItem* item = *i;        
        item->moveBy(dx, 0);
    }
    
    m_currentX = v;
    m_scene->update();
}

void GraphicsItemPin::slotVerticalValueChanged(int v)
{
    int dy = v - m_currentY;

    for(Items::iterator i = m_items.begin(); i != m_items.end(); ++i) {
        QGraphicsItem* item = *i;
        item->moveBy(0, dy);
    }
    
    m_currentY = v;    
    m_scene->update();
}

#include "GraphicsItemPin.moc"
