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
#include "rulerscale.h"
#include "colours.h"
#include <qvbox.h>
#include <qlabel.h>
#include <qcanvas.h>


BarButtons::BarButtons(RosegardenGUIDoc* doc,
		       RulerScale *rulerScale,
                       int barHeight,
                       QWidget* parent,
                       const char* name,
                       WFlags /*f*/):
    QHBox(parent, name),
    m_barHeight(barHeight),
    m_doc(doc),
    m_rulerScale(rulerScale)
{
    m_offset = 4;

    setMinimumHeight(m_barHeight);
    setMaximumHeight(m_barHeight);

    Rosegarden::Composition &comp = doc->getComposition();

    m_firstBar = comp.getBarNumber(comp.getStartMarker(), false);
    m_lastBar  = comp.getBarNumber(comp.getEndMarker(),   false);

    drawButtons();
}

BarButtons::~BarButtons()
{
}

void
BarButtons::drawButtons()
{
    if (!m_doc) return;

    // Create a horizontal spacing label to jog everything
    // up with the main SegmentCanvas
    //
    QLabel *label = new QLabel(this);
    label->setText(QString(""));
    label->setMinimumWidth(m_offset);
    label->setMaximumWidth(m_offset);

    int loopBarHeight = 8; // the height of the loop bar
    QVBox *bar;

    // Create a vertical box for the loopBar and the bar buttons
    //
    QVBox *buttonBar = new QVBox(this);

    // Loop ruler works its bar spacing out from the scale just
    // like we do in this class.  Then connect up the LoopRuler
    // signals passing back through the outside world.
    //
    //
    LoopRuler *loopRuler = new LoopRuler(m_doc,
                                         m_rulerScale,
                                         loopBarHeight,
                                         buttonBar);

    connect(loopRuler, SIGNAL(setPointerPosition(Rosegarden::timeT)),
            this,      SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    connect(loopRuler, SIGNAL(setPlayPosition(Rosegarden::timeT)),
            this,      SLOT(slotSetPlayPosition(Rosegarden::timeT)));

    connect(loopRuler, SIGNAL(setLoop(Rosegarden::timeT, Rosegarden::timeT)),
            this,      SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));

    connect(this,      SIGNAL(signalSetLoopingMode(bool)),
            loopRuler, SLOT(setLoopingMode(bool)));

    connect(this,      SIGNAL(signalSetLoopMarker(Rosegarden::timeT, Rosegarden::timeT)),
            loopRuler, SLOT(setLoopMarker(Rosegarden::timeT, Rosegarden::timeT)));

    // Another horizontal layout box..
    //
    QHBox *hButtonBar = new QHBox(buttonBar);

    int x = 0;

    for (int i = m_firstBar; i <= m_lastBar; i++)
    {
        bar = new QVBox(hButtonBar);
        bar->setSpacing(0);

	// The (i < lastBar) case resynchronises against the absolute
	// bar position at each stage so as to avoid gradually increasing
	// error through integer rounding

	int width;
	if (i < m_lastBar) {
	    width = (int)(m_rulerScale->getBarPosition(i+1) - (double)x);
	    x += width;
	} else {
	    width = (int)(m_rulerScale->getBarWidth(i));
	}

	bar->setMinimumSize(width, m_barHeight - loopBarHeight);
	bar->setMaximumSize(width, m_barHeight - loopBarHeight);

        // attempt a style
        //
        bar->setFrameStyle(StyledPanel);
        bar->setFrameShape(StyledPanel);
        bar->setFrameShadow(Raised);

        label = new QLabel(bar);
        label->setText(QString("%1").arg(i));
        label->setAlignment(AlignLeft|AlignVCenter);
        label->setIndent(4);
        label->setMinimumHeight(m_barHeight - loopBarHeight - 2);
        label->setMaximumHeight(m_barHeight - loopBarHeight - 2);
    }
}


void
BarButtons::setLoopingMode(bool value)
{
    emit signalSetLoopingMode(value);
}


