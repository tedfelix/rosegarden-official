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

#include <qvbox.h>
#include <qlabel.h>
#include <qcanvas.h>

#include "rosedebug.h"

using Rosegarden::RulerScale;


BarButtons::BarButtons(RosegardenGUIDoc* doc,
		       RulerScale *rulerScale,
                       int barHeight,
		       bool invert,
                       QWidget* parent,
                       const char* name,
                       WFlags /*f*/):
    QHBox(parent, name),
    m_invert(invert),
    m_barHeight(barHeight),
    m_loopRulerHeight(8),
    m_doc(doc),
    m_rulerScale(rulerScale),
    m_firstBar(0),
    m_hButtonBar(0)
{
    m_offset = 4;

    setMinimumHeight(m_barHeight);
    setMaximumHeight(m_barHeight);

    setFrameStyle(Plain);

    // Create a horizontal spacing label to jog everything
    // up with the main SegmentCanvas
    //
    QLabel *label = new QLabel(this);
    label->setText(QString(""));
    label->setMinimumWidth(m_offset);
    label->setMaximumWidth(m_offset);

    // Create a vertical box for the loopBar and the bar buttons
    //
    QVBox *buttonBar = new QVBox(this);
    buttonBar->setSpacing(0);

    if (m_invert) {
	m_hButtonBar = new QHBox(buttonBar);
    }

    // Loop ruler works its bar spacing out from the scale just
    // like we do in this class.  Then connect up the LoopRuler
    // signals passing back through the outside world.
    //
    //
    m_loopRuler = new LoopRuler
	(m_doc, m_rulerScale, m_loopRulerHeight, m_invert, buttonBar);

    connect(m_loopRuler, SIGNAL(setPointerPosition(Rosegarden::timeT)),
            this,      SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    connect(m_loopRuler, SIGNAL(setPlayPosition(Rosegarden::timeT)),
            this,      SLOT(slotSetPlayPosition(Rosegarden::timeT)));

    connect(m_loopRuler, SIGNAL(setLoop(Rosegarden::timeT, Rosegarden::timeT)),
            this,      SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));

    connect(this,      SIGNAL(signalSetLoopingMode(bool)),
            m_loopRuler, SLOT(setLoopingMode(bool)));

    connect(this,      SIGNAL(signalSetLoopMarker(Rosegarden::timeT, Rosegarden::timeT)),
            m_loopRuler, SLOT(setLoopMarker(Rosegarden::timeT, Rosegarden::timeT)));

    if (!m_invert) {
	m_hButtonBar = new QHBox(buttonBar);
    }

    drawButtons(true);
}

BarButtons::~BarButtons()
{
}

void
BarButtons::drawButtons(bool recalc)
{
    if (!m_doc) return;

    for (int i = 0; i < m_buttons.size(); ++i) delete m_buttons[i];
    m_buttons.clear();

    int firstBar = m_rulerScale->getFirstVisibleBar(),
	 lastBar = m_rulerScale->getLastVisibleBar();
    m_firstBar = firstBar;


/*
    kdDebug(KDEBUG_AREA) << "BarButtons::drawButtons: firstBar " << firstBar
			 << ", lastBar " << lastBar << ", x " << x << std::endl;

    kdDebug(KDEBUG_AREA) << "bar positions: " << std::endl;
    for (int j = firstBar; j <= lastBar; ++j) {
	kdDebug(KDEBUG_AREA) << j << ":" << m_rulerScale->getBarPosition(j) << endl;
    }
*/
    for (int i = firstBar; i <= lastBar; ++i)
    {

//        label->setMinimumHeight(m_barHeight - m_loopRulerHeight - 4);
//        label->setMaximumHeight(m_barHeight - m_loopRulerHeight - 4);

	m_buttons.push_back(makeBar(i));
    }

    if (recalc) recalculate();
}

QWidget *
BarButtons::makeBar(int n)
{
    QVBox *bar = new QVBox(m_hButtonBar);
    bar->setSpacing(0);

    // attempt a style
    //
    bar->setFrameStyle(StyledPanel);
    bar->setFrameShape(StyledPanel);
    bar->setFrameShadow(Raised);
    
    QLabel *label = new QLabel(bar);
    label->setText(QString("%1").arg(n));
    label->setAlignment(AlignLeft|AlignVCenter);
    label->setIndent(4);

    return bar;
}

void
BarButtons::recalculate()
{
    int firstBar = m_rulerScale->getFirstVisibleBar(),
	 lastBar = m_rulerScale->getLastVisibleBar();

    if (m_firstBar != firstBar) drawButtons(false);

    int x = (int)m_rulerScale->getBarPosition(firstBar);

//        label->setMinimumHeight(m_barHeight - m_loopRulerHeight - 4);
//        label->setMaximumHeight(m_barHeight - m_loopRulerHeight - 4);

    for (int i = firstBar; i <= lastBar; ++i) {

	if (i + firstBar >= m_buttons.size()) {
	    m_buttons.push_back(makeBar(i));
	}

	QWidget *bar = m_buttons[i - firstBar];

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

	kdDebug(KDEBUG_AREA) << "BarButtons::drawButtons: bar " << i
			     << ", width " << width << ", x " << x << std::endl;


//	if (width == 0) continue;
	bar->setMinimumSize(width, m_barHeight - m_loopRulerHeight);
	bar->setMaximumSize(width, m_barHeight - m_loopRulerHeight);
    }
}

void
BarButtons::setLoopingMode(bool value)
{
    emit signalSetLoopingMode(value);
}


