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

    virtual ~BarButtonsWidget();
    
    virtual QSize sizeHint() const;

    void scrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent*);

    //--------------- Data members ---------------------------------
    int m_barHeight;
    int m_currentXOffset;

    QFont *m_barFont;

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
    m_currentXOffset(0),
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


void BarButtons::scrollHoriz(int x)
{
    // This can't work - we want a pseudo-scroll,
    // e.g. actually repainting a different part of the widgets
    //
//     int oldOffset = m_currentXOffset;
//     m_currentXOffset = x;

//     scroll(oldOffset - m_currentXOffset, 0);

    m_loopRuler->scrollHoriz(x);
    m_hButtonBar->scrollHoriz(x);
}


//----------------------------------------------------------------------


BarButtonsWidget::BarButtonsWidget(RulerScale *rulerScale,
                                   int barHeight,
                                   QWidget* parent,
                                   const char* name,
                                   WFlags f)
    : QWidget(parent, name, f),
      m_barHeight(barHeight),
      m_currentXOffset(0),
      m_rulerScale(rulerScale)
{
    m_barFont = new QFont("helvetica", 12);
    m_barFont->setPixelSize(12);
}

BarButtonsWidget::~BarButtonsWidget()
{
    delete m_barFont;
}

void BarButtonsWidget::scrollHoriz(int x)
{
    m_currentXOffset = -x;

    repaint();
}

QSize BarButtonsWidget::sizeHint() const
{
    double width = m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) -
	m_rulerScale->getBarPosition(m_rulerScale->getFirstVisibleBar());

    return QSize(int(width), m_barHeight);
}

void BarButtonsWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setFont(*m_barFont);

    QRect clipRect = visibleRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() - m_currentXOffset);
    int  lastBar = m_rulerScale->getLastVisibleBar();

    painter.drawLine(m_currentXOffset, 0, visibleRect().width(), 0);

    for (int i = firstBar; i <= lastBar; ++i) {

	double x = m_rulerScale->getBarPosition(i) + m_currentXOffset;
	if (x > clipRect.x() + clipRect.width()) break;

//        double width = m_rulerScale->getBarWidth(i);
//        if (width == 0) continue;

//	if (x + width < clipRect.x()) continue;

	painter.drawLine(x, 0, x, m_barHeight);
	painter.drawText(x + 4, 12, QString("%1").arg(i));
    }
}
