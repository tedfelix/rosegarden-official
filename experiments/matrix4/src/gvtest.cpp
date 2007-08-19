#include <QtGui>
#include <QTimer>
#include <QLinearGradient>

#include "gvtest.h"
#include "GraphicsItemPin.h"
#include "FadeItem.h"

GVTest::GVTest(QWidget * parent )
    : QGraphicsView(parent),
    m_fadeItem(0)
{
  QGraphicsScene *scene = new QGraphicsScene(this);
//  scene->setSceneRect(-200, -200, 2000, 2000);
  scene->setSceneRect(0, 0, 1000, 500);
  setScene(scene);

  setDragMode(RubberBandDrag);

}

void GVTest::setup()
{
    qDebug("GVTest::setup()");
    
    QGraphicsRectItem* rect = new QGraphicsRectItem(10.0, 10.0,
                                                    40.0, 40.0);
    QColor penColor(0, 0, 0, 255);
    rect->setPen(QPen(penColor, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    rect->setBrush(penColor);
    scene()->addItem(rect);
    rect->show();
    
    GraphicsItemPin *graphicsItemPin = new GraphicsItemPin(this);
    
    graphicsItemPin->pinItem(rect, QPointF(30, 30));
    
    m_fadeItem = new FadeItem(QRectF(30, 60, 40, 40));
    scene()->addItem(m_fadeItem);
    
}

void GVTest::fadeIn()
{
    qDebug("GVTest::fadeIn");
    m_fadeTimerId = startTimer(100);
    m_fadeItem->startFade();
}

void GVTest::fadeOut()
{
    killTimer(m_fadeTimerId);   
}

void GVTest::mouseMoveEvent ( QMouseEvent * e )
{
    QGraphicsView::mouseMoveEvent(e);

    if (e->buttons() != Qt::NoButton) {
        
        QPoint pos = e->pos();
        if (pos.x() < 50 || (width() - pos.x()) < 50 ||
            pos.y() < 50 || (height() - pos.y()) < 50)
            m_autoScrollTimerId = startTimer(500);
        else
            killTimer(m_autoScrollTimerId);
        m_currentPos = pos;

    }    
}

// void GVTest::mousePressEvent ( QMouseEvent * e )
// {
//   QGraphicsView::mousePressEvent(e);
// }

void GVTest::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_fadeTimerId) {
        qDebug("GVTest::timerEvent - fadeTimer");
        scene()->advance();
    } else if (event->timerId() ==m_autoScrollTimerId) {
        autoScroll();
    }
        
}

void GVTest::mouseReleaseEvent ( QMouseEvent * e )
{
  QGraphicsView::mouseReleaseEvent(e);
  killTimer(m_autoScrollTimerId);
}

void GVTest::autoScroll()
{
  if ((width() - m_currentPos.x()) < 50)
      horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
  else if (m_currentPos.x() < 50)
      horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);

  if ((height() - m_currentPos.y()) < 50)
      verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
  else if (m_currentPos.y() < 50)
      verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);

}

#include "gvtest.moc"

