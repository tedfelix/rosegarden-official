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

#include <qpainter.h>

#include "temporuler.h"
#include "colours.h"
#include "rosedebug.h"
#include "Composition.h"
#include "RulerScale.h"

using Rosegarden::RulerScale;
using Rosegarden::Composition;
using Rosegarden::timeT;


TempoRuler::TempoRuler(RulerScale *rulerScale,
		       Composition *composition,
		       int height,
		       QWidget *parent,
		       const char *name) :
    QWidget(parent, name),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_composition(composition),
    m_rulerScale(rulerScale),
    m_font("helvetica", 10),
    m_boldFont("helvetica", 10, QFont::Bold),
    m_fontMetrics(m_boldFont)
{
    setBackgroundColor(RosegardenGUIColours::TextRulerBackground);
}

TempoRuler::~TempoRuler()
{
    // nothing
}

void
TempoRuler::slotScrollHoriz(int x)
{
    m_currentXOffset = -x;
    repaint();
}

QSize
TempoRuler::sizeHint() const
{
    //!!! could be improved upon

    double width =
	m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
TempoRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
TempoRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);
    paint.setPen(RosegardenGUIColours::TextRulerForeground);

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
	(clipRect.x() - m_currentXOffset - 100);
    timeT   to = m_rulerScale->getTimeForX
	(clipRect.x() + clipRect.width() - m_currentXOffset + 100);

    QRect boundsForHeight = m_fontMetrics.boundingRect("^j|lM");
    int fontHeight = boundsForHeight.height();
    int textY = (height() - 6)/2 + fontHeight/2;

    double prevEndX = -1000.0;
    double prevTempo = 0.0;
    long prevBpm = 0;

    typedef std::map<timeT, bool> TimePoints;
    int tempoChangeHere = 1, timeSigChangeHere = 2;
    TimePoints timePoints;

    for (int tempoNo = m_composition->getTempoChangeNumberAt(from) + 1;
	 tempoNo <= m_composition->getTempoChangeNumberAt(to); ++tempoNo) {

	timePoints.insert
	    (TimePoints::value_type
	     (m_composition->getRawTempoChange(tempoNo).first,
	      tempoChangeHere));
    }
	
    for (int sigNo = m_composition->getTimeSignatureNumberAt(from) + 1;
	 sigNo <= m_composition->getTimeSignatureNumberAt(to); ++sigNo) {

	timeT time(m_composition->getTimeSignatureChange(sigNo).first);
	if (timePoints.find(time) != timePoints.end()) {
	    timePoints[time] += timeSigChangeHere;
	} else {
	    timePoints.insert(TimePoints::value_type(time, timeSigChangeHere));
	}
    }

    for (TimePoints::iterator i = timePoints.begin();
	 i != timePoints.end(); ++i) {

	timeT time = i->first;
	double x = m_rulerScale->getXForTime(time) + m_currentXOffset;
	paint.drawLine(x, height() - 4, x, height());
	
	if (i->second & timeSigChangeHere) {

	    Rosegarden::TimeSignature sig =
		m_composition->getTimeSignatureAt(time);

	    QString numStr = QString("%1").arg(sig.getNumerator());
	    QString denStr = QString("%1").arg(sig.getDenominator());

	    QRect numBounds = m_fontMetrics.boundingRect(numStr);
	    QRect denBounds = m_fontMetrics.boundingRect(denStr);

	    paint.setFont(m_boldFont);
	    paint.drawText(x - numBounds.width()/2,
			   numBounds.height(), numStr);
	    paint.drawText(x - denBounds.width()/2,
			   numBounds.height() + denBounds.height(), denStr);
	}

	if (i->second & tempoChangeHere) { 
	
	    double tempo = m_composition->getTempoAt(time);
	    long bpm = long(tempo);

	    QString tempoString = QString("%1").arg(bpm);
	    if (tempo == prevTempo) {
		tempoString = "=";
	    } else if (bpm == prevBpm) {
		tempoString = (tempo > prevTempo ? "+" : "-");
	    }
	    prevTempo = tempo;
	    prevBpm = bpm;

	    QRect bounds = m_fontMetrics.boundingRect(tempoString);

	    paint.setFont(m_font);
	    if (x > bounds.width() / 2) x -= bounds.width() / 2;
	    if (prevEndX >= x - 3) x = prevEndX + 3;
	    paint.drawText(x, textY, tempoString);
	    prevEndX = x + bounds.width();
	}
    }
}

