#ifndef GVTEST_H_
#define GVTEST_H_

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QMouseEvent>

class QTimer;
class FadeItem;

class GVTest : public QGraphicsView
{
    Q_OBJECT

public:
    GVTest(QWidget * parent = 0);
    void setup();

protected:

    virtual void mouseMoveEvent ( QMouseEvent * e );
//     virtual void mousePressEvent ( QMouseEvent * e );
    virtual void mouseReleaseEvent ( QMouseEvent * e );

    
public slots:
    void fadeIn();
    void fadeOut();
    
protected slots:
    void autoScroll();

protected:    
    virtual void timerEvent(QTimerEvent *event);
    
    QPoint m_currentPos;
    FadeItem* m_fadeItem;
    
    int m_autoScrollTimerId;
    int m_fadeTimerId;
};


#endif /*GVTEST_H_*/
