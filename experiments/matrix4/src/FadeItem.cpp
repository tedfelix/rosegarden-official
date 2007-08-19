#include "FadeItem.h"
#include <QPen>
#include <QBrush>

FadeItem::FadeItem(const QRectF & rect, QGraphicsItem * parent)
    : QGraphicsRectItem(rect, parent),
    m_alpha(255),
    m_fade(false),
    m_fadeIn(false),
    m_step(10)
{
    setBrush(pen().color());
}

FadeItem::~FadeItem()
{
}

void FadeItem::startFade()
{
    m_fade = true;
    m_fadeIn = pen().color().alpha() == 0;
}

void FadeItem::advance(int phase)
{   
    if (phase == 0 || !m_fade)
        return;

    
    QColor currentPenColor = pen().color();
    QColor currentBrushColor = brush().color();
    qDebug("FadeItem::advance - alpha = %d", currentPenColor.alpha());
    
    if (m_fadeIn) {
        if (currentPenColor.alpha() == 255) {
            m_fade = false;
            return;
        }
        currentPenColor.setAlpha(currentPenColor.alpha() + m_step);
        currentBrushColor.setAlpha(currentBrushColor.alpha() + m_step);
    } else { // well, fade out...
        if (currentPenColor.alpha() == 0) {
            m_fade = false;
            return;
        }
        currentPenColor.setAlpha(currentPenColor.alpha() - m_step);
        currentBrushColor.setAlpha(currentBrushColor.alpha() - m_step);        
    }
    
    QPen gpen = pen();
    gpen.setColor(currentPenColor);
    QBrush gbrush = brush();
    gbrush.setColor(currentBrushColor);
    setPen(gpen);
    setBrush(gbrush);
}
