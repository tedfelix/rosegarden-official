#ifndef FADEITEM_H_
#define FADEITEM_H_

#include <QGraphicsRectItem>

class FadeItem : public QGraphicsRectItem
{
public:
	FadeItem(const QRectF & rect, QGraphicsItem * parent = 0);
	virtual ~FadeItem();

	void startFade();
	virtual void advance(int phase);
	
protected:
    int m_alpha;
    bool m_fade;
    bool m_fadeIn;
    int m_step;
};

#endif /*FADEITEM_H_*/
