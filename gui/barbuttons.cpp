// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "barbuttons.h"
#include "loopruler.h"
#include "RulerScale.h"
#include "colours.h"

#include <qcanvas.h>

#include "rosedebug.h"

using Rosegarden::RulerScale;



class BarButtonsWidget : public QWidget
{
public:
    BarButtonsWidget(Rosegarden::RulerScale *rulerScale,
                     int buttonHeight,
                     QWidget* parent = 0,
                     const char* name = 0,
                     WFlags f=0);
    
    virtual QSize sizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent*);

    //--------------- Data members ---------------------------------
    int m_barHeight;
    Rosegarden::RulerScale *m_rulerScale;

};


BarButtons::BarButtons(RosegardenGUIDoc* doc,
		       RulerScale *rulerScale,
                       int barHeight,
		       bool invert,
                       QWidget* parent,
                       const char* name,
                       WFlags f):
    QVBox(parent, name, f),
    m_invert(invert),
    m_loopRulerHeight(8),
    m_offset(4),
    m_doc(doc),
    m_rulerScale(rulerScale),
    m_hButtonBar(0)
{

    setSpacing(0);

    if (m_invert) {
	m_hButtonBar = new BarButtonsWidget(m_rulerScale,
                                            barHeight - m_loopRulerHeight, this);
    }

    // Loop ruler works its bar spacing out from the scale just
    // like we do in this class.  Then connect up the LoopRuler
    // signals passing back through the outside world.
    //
    //
    m_loopRuler = new LoopRuler(m_rulerScale, m_loopRulerHeight, m_invert, this);

    if (!m_invert) {
	m_hButtonBar = new BarButtonsWidget(m_rulerScale,
                                            barHeight - m_loopRulerHeight, this);
    }
}



BarButtonsWidget::BarButtonsWidget(RulerScale *rulerScale,
                                   int barHeight,
                                   QWidget* parent,
                                   const char* name,
                                   WFlags f)
    : QWidget(parent, name, f),
      m_barHeight(barHeight),
      m_rulerScale(rulerScale)
{
    
}

QSize BarButtonsWidget::sizeHint() const
{
    int nbBars = m_rulerScale->getLastVisibleBar() - m_rulerScale->getFirstVisibleBar();
    double firstBarWidth = m_rulerScale->getBarWidth(0);

    return QSize(nbBars * firstBarWidth, m_barHeight);
}

void BarButtonsWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

//     painter.setBrush(red);
//     painter.setPen(red);

    int firstBar = m_rulerScale->getFirstVisibleBar(),
	 lastBar = m_rulerScale->getLastVisibleBar();

    kdDebug(KDEBUG_AREA) << "BarButtons::paintEvent: firstBar = "
			 << firstBar << std::endl;
    int x = 0;

    painter.drawLine(0, 0, visibleRect().width(), 0);

    for (int i = firstBar; i <= lastBar; ++i) {

        painter.drawLine(x, 0, x, m_barHeight);
        painter.drawText(x + 4, m_barHeight / 2, QString("%1").arg(i));

	// The (i < lastBar) case resynchronises against the absolute
	// bar position at each stage so as to avoid gradually increasing
	// error through integer rounding

	int width;
	if (i < lastBar) {
	    width = (int)(m_rulerScale->getBarPosition(i+1) - (double)x);
	    x += width;
	} else {
	    width = (int)(m_rulerScale->getBarWidth(i));
	}

	kdDebug(KDEBUG_AREA) << "BarButtons::paintEvent: bar " << i
			     << ", width " << width << ", x " << x << std::endl;

    }
}
