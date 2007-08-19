#ifndef GRAPHICSITEMPIN_H_
#define GRAPHICSITEMPIN_H_

#include <QPointF>

#include <vector>

class QGraphicsItem;
class QGraphicsView;
class QGraphicsScene;

class GraphicsItemPin : public QObject
{
    Q_OBJECT
    
public:
	GraphicsItemPin(QGraphicsView*);
	virtual ~GraphicsItemPin();
	
	void pinItem(QGraphicsItem*, QPointF);
	void releaseItem(QGraphicsItem*);

public slots:
    void slotHorizontalValueChanged(int);
    void slotVerticalValueChanged(int);
	
protected:
    int m_currentX;
    int m_currentY;
    QGraphicsScene* m_scene;
    
    typedef std::vector<QGraphicsItem*> Items;
    
    Items m_items;
};

#endif /*GRAPHICSITEMPIN_H_*/
