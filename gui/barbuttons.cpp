// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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
#include <qpainter.h>

#include "rosestrings.h"
#include "rosedebug.h"

using Rosegarden::RulerScale;



class BarButtonsWidget : public QWidget, public HZoomable
{
public:
    BarButtonsWidget(Rosegarden::RulerScale *rulerScale,
                     int buttonHeight,
		     double xorigin = 0.0,
                     QWidget* parent = 0,
                     const char* name = 0,
                     WFlags f=0);

    virtual ~BarButtonsWidget();
    
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void scrollHoriz(int x);

    void setWidth(int width) { m_width = width; }

protected:
    virtual void paintEvent(QPaintEvent*);

    //--------------- Data members ---------------------------------
    int m_barHeight;
    double m_xorigin;
    int m_currentXOffset;
    int m_width;

    QFont *m_barFont;

    Rosegarden::RulerScale *m_rulerScale;

};


BarButtons::BarButtons(RulerScale *rulerScale,
		       double xorigin,
                       int barHeight,
		       bool invert,
                       QWidget* parent,
                       const char* name,
                       WFlags f):
    QVBox(parent, name, f),
    m_invert(invert),
    m_loopRulerHeight(10),
    m_currentXOffset(0),
    m_rulerScale(rulerScale),
    m_hButtonBar(0)
{

    setSpacing(0);

    if (!m_invert) {
	m_hButtonBar = new BarButtonsWidget
	    (m_rulerScale, barHeight - m_loopRulerHeight, xorigin, this);
    }

    m_loopRuler = new LoopRuler
	(m_rulerScale, m_loopRulerHeight, xorigin, m_invert, this);

    if (m_invert) {
	m_hButtonBar = new BarButtonsWidget
	    (m_rulerScale, barHeight - m_loopRulerHeight, xorigin, this);
    }
}


void BarButtons::connectRulerToDocPointer(RosegardenGUIDoc *doc)
{
    QObject::connect
	(m_loopRuler, SIGNAL(setPointerPosition(Rosegarden::timeT)),
	 doc, SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    QObject::connect
	(m_loopRuler, SIGNAL(setPlayPosition(Rosegarden::timeT)),
	 doc, SLOT(slotSetPlayPosition(Rosegarden::timeT)));

    QObject::connect
	(m_loopRuler, SIGNAL(setLoop(Rosegarden::timeT, Rosegarden::timeT)),
	 doc, SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));

    /*
    QObject::connect
        (doc, SIGNAL(loopChanged(Rosegarden::timeT, Rosegarden::timeT)),
         m_loopRuler,
         SLOT(slotSetLoopMarker(Rosegarden::timeT, Rosegarden::timeT)));
         */

    m_loopRuler->setBackgroundColor(RosegardenGUIColours::PointerRuler);
}

void BarButtons::slotScrollHoriz(int x)
{
    m_loopRuler->scrollHoriz(x);
    m_hButtonBar->scrollHoriz(x);
}


void BarButtons::setMinimumWidth(int width)
{
    m_hButtonBar->setMinimumWidth(width);
    m_loopRuler->setMinimumWidth(width);
}

void BarButtons::setHScaleFactor(double dy)
{
    m_hButtonBar->setHScaleFactor(dy);
    m_loopRuler->setHScaleFactor(dy);
}



void BarButtons::paintEvent(QPaintEvent *e)
{
    m_hButtonBar->update();
    m_loopRuler->update();
    QWidget::paintEvent(e);
}



//----------------------------------------------------------------------


BarButtonsWidget::BarButtonsWidget(RulerScale *rulerScale,
                                   int barHeight,
				   double xorigin,
                                   QWidget* parent,
                                   const char* name,
                                   WFlags f)
    : QWidget(parent, name, f),
      m_barHeight(barHeight),
      m_xorigin(xorigin),
      m_currentXOffset(0),
      m_width(-1),
      m_rulerScale(rulerScale)
{
//    m_barFont = new QFont("helvetica", 12);
//    m_barFont->setPixelSize(12);
    m_barFont = new QFont();
    m_barFont->setPointSize(10);
}

BarButtonsWidget::~BarButtonsWidget()
{
    delete m_barFont;
}

void BarButtonsWidget::scrollHoriz(int x)
{
    m_currentXOffset = -x / getHScaleFactor();

    repaint();
}

QSize BarButtonsWidget::sizeHint() const
{
    int lastBar =
	m_rulerScale->getLastVisibleBar();
    double width =
	m_rulerScale->getBarPosition(lastBar) +
	m_rulerScale->getBarWidth(lastBar) + m_xorigin;

    return QSize(std::max(int(width), m_width), m_barHeight);
}

QSize BarButtonsWidget::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;

    return QSize(firstBarWidth, m_barHeight);
}

void BarButtonsWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setFont(*m_barFont);

    if (getHScaleFactor() != 1.0) painter.scale(getHScaleFactor(), 1.0);

    QRect clipRect = visibleRect();

    int firstBar = m_rulerScale->getBarForX(clipRect.x() -
					    m_currentXOffset -
					    m_xorigin);
    int  lastBar = m_rulerScale->getLastVisibleBar();
    if (firstBar < m_rulerScale->getFirstVisibleBar()) {
	firstBar = m_rulerScale->getFirstVisibleBar();
    }

    painter.drawLine(m_currentXOffset, 0, visibleRect().width() / getHScaleFactor(), 0);

    float minimumWidth = 25.0;
    float testSize = ((float)(m_rulerScale->getBarPosition(firstBar + 1) -
                              m_rulerScale->getBarPosition(firstBar)))
                               / minimumWidth;

    int every = 0;
    int count = 0;

    if (testSize < 1.0)
    {
        every = (int(1.0/testSize));

        if(every % 2 == 0)
            every++;
    }

    for (int i = firstBar; i <= lastBar; ++i) {

	double x = m_rulerScale->getBarPosition(i) + m_xorigin + m_currentXOffset;

	if ((x * getHScaleFactor()) > clipRect.x() + clipRect.width()) break;

        // always the first bar number
        if (every && i != firstBar)
        {
            if (count < every)
            {
                count++;
                continue;
            }

            // reset count if we passed
            count = 0;
        }

        // adjust count for first bar line
        if (every == firstBar) count++;

	painter.drawLine(x, 0, x, m_barHeight);

        // disable worldXForm for text
        QPoint textDrawPoint = painter.xForm(QPoint(x+4, 12));
        
        bool enableXForm = painter.hasWorldXForm();
        painter.setWorldXForm(false);

	if (i >= 0) painter.drawText(textDrawPoint, QString("%1").arg(i + 1));

        painter.setWorldXForm(enableXForm);

    }
}
