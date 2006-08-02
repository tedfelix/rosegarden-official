// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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
#include <qtooltip.h>

#include <klocale.h>

#include "temporuler.h"
#include "colours.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardenguidoc.h"
#include "Composition.h"
#include "RulerScale.h"

#include "tempocolour.h"
#include "widgets.h"

using Rosegarden::RulerScale;
using Rosegarden::Composition;
using Rosegarden::timeT;
using Rosegarden::tempoT;


TempoRuler::TempoRuler(RulerScale *rulerScale,
		       RosegardenGUIDoc *doc,
		       double xorigin,
		       int height,
		       bool small,
		       QWidget *parent,
		       const char *name) :
    QWidget(parent, name),
    m_xorigin(xorigin),
    m_height(height),
    m_currentXOffset(0),
    m_width(-1),
    m_small(small),
    m_illuminate(-1),
    m_refreshLinesOnly(false),
    m_dragging(false),
    m_dragStartY(0),
    m_dragFine(false),
    m_dragStartTempo(-1),
    m_dragStartTarget(-1),
    m_dragOriginalTempo(-1),
    m_dragOriginalTarget(-1),
    m_composition(&doc->getComposition()),
    m_rulerScale(rulerScale),
    m_fontMetrics(m_boldFont)
{
//    m_font.setPointSize(m_small ? 9 : 11);
//    m_boldFont.setPointSize(m_small ? 9 : 11);

//    m_font.setPixelSize(m_height * 2 / 3);
//    m_boldFont.setPixelSize(m_height * 2 / 3);

    m_font.setPixelSize(m_height / 3);
    m_boldFont.setPixelSize(m_height * 2 / 5);
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);

    m_textFloat = new RosegardenTextFloat(this);
    m_textFloat->hide();

//    setBackgroundColor(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::TextRulerBackground));
    setBackgroundMode(Qt::NoBackground);

    QObject::connect
	(doc->getCommandHistory(), SIGNAL(commandExecuted()),
	 this, SLOT(update()));

//    QToolTip::add(this, i18n("Tempo and Time Signature Ruler.\nDouble click to insert an event."));
    setMouseTracking(false);
}

TempoRuler::~TempoRuler()
{
    // nothing
}

void
TempoRuler::slotScrollHoriz(int x)
{
    int w = width(), h = height();
    int dx = x - (-m_currentXOffset);
    m_currentXOffset = -x;

    if (dx > w*3/4 || dx < -w*3/4) {
	update();
	return;
    }

    if (dx > 0) { // moving right, so the existing stuff moves left
	bitBlt(this, 0, 0, this, dx, 0, w-dx, h);
	repaint(w-dx, 0, dx, h);
    } else {      // moving left, so the existing stuff moves right
	bitBlt(this, -dx, 0, this, 0, 0, w+dx, h);
	repaint(0, 0, -dx, h);
    }
}


void
TempoRuler::mousePressEvent(QMouseEvent *e)
{
    if (e->type() == QEvent::MouseButtonDblClick) {
	timeT t = m_rulerScale->getTimeForX
	    (e->x() - m_currentXOffset - m_xorigin);
	emit doubleClicked(t);
	return;
    }

    int x = e->x() + 1;
    int y = e->y();
    timeT t = m_rulerScale->getTimeForX(x - m_currentXOffset - m_xorigin);
    int tcn = m_composition->getTempoChangeNumberAt(t);

    if (tcn < 0 || tcn >= m_composition->getTempoChangeCount()) return;

    std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
    std::pair<bool, tempoT> tr = m_composition->getTempoRamping(tcn, false);

    m_dragStartY = y;
    m_dragStartTime = tc.first;
    m_dragStartTempo = tc.second;
    m_dragStartTarget = tr.first ? tr.second : -1;
    m_dragOriginalTempo = m_dragStartTempo;
    m_dragOriginalTarget = m_dragStartTarget;
    m_dragFine = ((e->state() & Qt::ShiftButton) != 0);

    m_dragging = true;
}

void
TempoRuler::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_dragging) {
	mouseMoveEvent(e);
	m_dragging = false;

	if (e->x() < 0 || e->x() >= width() ||
	    e->y() < 0 || e->y() >= height()) {
	    leaveEvent(0);
	}

	// First we make a note of the values that we just set and
	// restore the tempo to whatever it was previously, so that
	// the undo for any following command will work correctly.
	// Then we emit so that our user can issue the right command.
	
	int tcn = m_composition->getTempoChangeNumberAt(m_dragStartTime);
	std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
	std::pair<bool, tempoT> tr = m_composition->getTempoRamping(tcn, false);
	m_composition->addTempoAtTime(m_dragStartTime,
				      m_dragOriginalTempo,
				      m_dragOriginalTarget);
	emit changeTempo(m_dragStartTime, tc.second,
			 tr.first ? tr.second : -1,
			 TempoDialog::AddTempo);

	return;
    }
}

void
TempoRuler::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging) {

	bool shiftPressed = ((e->state() & Qt::ShiftButton) != 0);

	if (shiftPressed != m_dragFine) {

	    m_dragFine = shiftPressed;
	    m_dragStartY = e->y();

	    // reset the start tempi to whatever we last updated them
	    // to as we switch into or out of fine mode
	    int tcn = m_composition->getTempoChangeNumberAt(m_dragStartTime);
	    std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);
	    std::pair<bool, tempoT> tr = m_composition->getTempoRamping(tcn, false);
	    m_dragStartTempo = tc.second;
	    m_dragStartTarget = tr.first ? tr.second : -1;
	}

	int diff = m_dragStartY - e->y(); // +ve for upwards drag
	tempoT newTempo = m_dragStartTempo;
	tempoT newTarget = m_dragStartTarget;

	if (diff != 0) {

	    float qpm = m_composition->getTempoQpm(newTempo);
	    float qdiff = (m_dragFine ? diff * 0.05 : diff * 0.5);
	    qpm += qdiff;
	    if (qpm < 1) qpm = 1;
	    newTempo = m_composition->getTempoForQpm(qpm + 0.0001);

	    if (newTarget >= 0) {
		qpm = m_composition->getTempoQpm(newTarget);
		qpm += qdiff;
		if (qpm < 1) qpm = 1;
		newTarget = m_composition->getTempoForQpm(qpm + 0.0001);
	    }
	}

	showTextFloat(newTempo);
	m_composition->addTempoAtTime(m_dragStartTime, newTempo, newTarget);
	update();

    } else {
	
	int x = e->x() + 1;
	int y = e->y();
	timeT t = m_rulerScale->getTimeForX(x - m_currentXOffset - m_xorigin);
	int tcn = m_composition->getTempoChangeNumberAt(t);

	if (tcn < 0 || tcn >= m_composition->getTempoChangeCount()) return;

	std::pair<timeT, tempoT> tc = m_composition->getTempoChange(tcn);

	int bar, beat, fraction, remainder;
	m_composition->getMusicalTimeForAbsoluteTime(tc.first, bar, beat,
						     fraction, remainder);
	RG_DEBUG << "Tempo change: tempo " << m_composition->getTempoQpm(tc.second) << " at " << bar << ":" << beat << ":" << fraction << ":" << remainder << endl;

	m_illuminate = tcn;
	m_refreshLinesOnly = true;

	showTextFloat(tc.second);

	update();
    }
}

void
TempoRuler::wheelEvent(QWheelEvent *e)
{
}

void
TempoRuler::enterEvent(QEvent *)
{
    setMouseTracking(true);
}    

void
TempoRuler::leaveEvent(QEvent *)
{
    if (!m_dragging) {
	setMouseTracking(false);
	m_illuminate = -1;
	m_refreshLinesOnly = true;
	m_textFloat->hide();
	update();
    }
}    

void
TempoRuler::showTextFloat(tempoT tempo)
{
    float qpm = m_composition->getTempoQpm(tempo);
    int qi = int(qpm + 0.0001);
    int q0 = int(qpm * 10 + 0.0001) % 10;
    int q00 = int(qpm * 100 + 0.0001) % 10;

    m_textFloat->setText(i18n("%1.%2%3 bpm").arg(qi).arg(q0).arg(q00)); //!!! qpm

    QPoint cp = mapFromGlobal(QPoint(QCursor::pos()));
    std::cerr << "cp = " << cp.x() << "," << cp.y() << ", tempo = " << qpm << std::endl;
    QPoint mp = cp + pos();

    QWidget *parent = parentWidget();
    while (parent->parentWidget() &&
	   !parent->isTopLevel() &&
	   !parent->isDialog()) { 
	mp += parent->pos();
	parent = parent->parentWidget();
    }

    int yoff = cp.y() + m_textFloat->height() + 3;
    mp = QPoint(mp.x() + 10, mp.y() > yoff ? mp.y() - yoff : 0);

    m_textFloat->move(mp);
    m_textFloat->show();
}

QSize
TempoRuler::sizeHint() const
{
    double width =
	m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
	m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar()) +
	m_xorigin;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
TempoRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0) + m_xorigin;
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

int
TempoRuler::getYForTempo(tempoT tempo)
{
    int drawh = height() - 4;
    int y = drawh / 2;

    tempoT minTempo = m_composition->getMinTempo();
    tempoT maxTempo = m_composition->getMaxTempo();

    if (maxTempo > minTempo) {
	y = drawh - 
	    int((double(tempo - minTempo) / double(maxTempo - minTempo))
		* drawh + 0.5);
    }

    return y;
}

tempoT
TempoRuler::getTempoForY(int y)
{
    int drawh = height() - 4;

    tempoT minTempo = m_composition->getMinTempo();
    tempoT maxTempo = m_composition->getMaxTempo();

    tempoT tempo = minTempo;

    if (maxTempo > minTempo) {
	tempo = (maxTempo - minTempo) *
	    (double(drawh - y) / double(drawh)) + minTempo + 0.5;
    }

    return tempo;
}    

void
TempoRuler::paintEvent(QPaintEvent* e)
{
    QPainter paint(this);
    paint.setPen(Rosegarden::GUIPalette::getColour
		 (Rosegarden::GUIPalette::TextRulerForeground));

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalize());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
	(clipRect.x() - m_currentXOffset - 100 - m_xorigin);
    timeT   to = m_rulerScale->getTimeForX
	(clipRect.x() + clipRect.width() - m_currentXOffset + 100 - m_xorigin);

    QRect boundsForHeight = m_fontMetrics.boundingRect("019");
    int fontHeight = boundsForHeight.height();
    int textY = fontHeight + 2;

    double prevEndX = -1000.0;
    double prevTempo = 0.0;
    long prevBpm = 0;

    typedef std::map<timeT, int> TimePoints;
    int tempoChangeHere = 1;
    int timeSigChangeHere = 2;
    TimePoints timePoints;

    for (int tempoNo = m_composition->getTempoChangeNumberAt(from);
	 tempoNo <= m_composition->getTempoChangeNumberAt(to) + 1; ++tempoNo) {

	if (tempoNo >= 0 && tempoNo < m_composition->getTempoChangeCount()) {
	    timePoints.insert
		(TimePoints::value_type
		 (m_composition->getTempoChange(tempoNo).first,
		  tempoChangeHere));
	}
    }

    for (int sigNo = m_composition->getTimeSignatureNumberAt(from);
	 sigNo <= m_composition->getTimeSignatureNumberAt(to) + 1; ++sigNo) {

	if (sigNo >= 0 && sigNo < m_composition->getTimeSignatureCount()) {
	    timeT time(m_composition->getTimeSignatureChange(sigNo).first);
	    if (timePoints.find(time) != timePoints.end()) {
		timePoints[time] |= timeSigChangeHere;
	    } else {
		timePoints.insert(TimePoints::value_type(time, timeSigChangeHere));
	    }
	}
    }

    int lastx = 0, lasty = 0;
    bool haveSome = false;
//    tempoT minTempo = m_composition->getMinTempo();
//    tempoT maxTempo = m_composition->getMaxTempo();
    bool illuminate = false;

    if (m_illuminate >= 0) {
	int tcn = m_composition->getTempoChangeNumberAt(from);
	illuminate = (m_illuminate == tcn);
    }

    for (TimePoints::iterator i = timePoints.begin(); ; ++i) {

	timeT t0, t1;

	if (i == timePoints.begin()) {
	    t0 = from;
	} else {
	    TimePoints::iterator j(i);
	    --j;
	    t0 = j->first;
	}

	if (i == timePoints.end()) {
	    t1 = to;
	} else {
	    t1 = i->first;
	}

	if (t1 <= t0) t1 = to;

	int tcn = m_composition->getTempoChangeNumberAt(t0);
	tempoT tempo = m_composition->getTempoAtTime(t0);

	std::pair<bool, tempoT> ramping(false, tempo);
	if (tcn > 0 && tcn < m_composition->getTempoChangeCount() + 1) {
	    ramping = m_composition->getTempoRamping(tcn - 1);
	}

	QColor colour = TempoColour::getColour(m_composition->getTempoQpm(tempo));
        paint.setPen(colour);
        paint.setBrush(colour);

	double x0, x1;
	x0 = m_rulerScale->getXForTime(t0) + m_currentXOffset + m_xorigin;
	x1 = m_rulerScale->getXForTime(t1) + m_currentXOffset + m_xorigin;
	if (!m_refreshLinesOnly) {
// 	    RG_DEBUG << "TempoRuler: draw rect from " << x0 << " to " << x1 << endl;
	    paint.drawRect(int(x0), 0, int(x1 - x0) + 1, height());
	}

	int y = getYForTempo(tempo);
/*!!!
	int drawh = height() - 4;
	int y = drawh / 2;
	if (maxTempo > minTempo) {
	    y = drawh - 
		int((double(tempo - minTempo) / double(maxTempo - minTempo))
		    * drawh + 0.5);
	}
*/
	y += 2;

	if (haveSome) {

	    int x = int(x0) + 1;
	    int ry = lasty;

	    paint.setPen(illuminate ? Qt::white : Qt::black);

	    if (ramping.first) {
		ry = getYForTempo(ramping.second);
/*!!!
		ry = drawh - 
		    int((double(ramping.second - minTempo) /
			 double(maxTempo - minTempo))
			* drawh + 0.5);
*/
	    }

	    paint.drawLine(lastx + 1, lasty, x - 2, ry);

	    if (m_illuminate >= 0) {
		illuminate = (m_illuminate == tcn);
	    }

	    paint.setPen(illuminate ? Qt::white : Qt::black);
	    paint.drawRect(x - 1, y - 1, 3, 3);

	    paint.setPen(illuminate ? Qt::black : Qt::white);
	    paint.drawPoint(x, y);

//	    if (illuminate) {
//		showTextFloat(tempo);
//	    }
	}

	lastx = int(x0) + 1;
	lasty = y;
	if (i == timePoints.end()) break;
	haveSome = true;
    }

    if (haveSome) {
	paint.setPen(illuminate ? Qt::white : Qt::black);
	paint.drawLine(lastx + 1, lasty, width(), lasty);
    } else if (!m_refreshLinesOnly) {
	tempoT tempo = m_composition->getTempoAtTime(from);
	QColor colour = TempoColour::getColour(m_composition->getTempoQpm(tempo));
        paint.setPen(colour);
        paint.setBrush(colour);
	paint.drawRect(e->rect());
    }

    paint.setPen(Qt::black);
    paint.setBrush(Qt::black);
    paint.drawLine(0, 0, width(), 0);

    for (TimePoints::iterator i = timePoints.begin();
	 i != timePoints.end(); ++i) {

	timeT time = i->first;
	double x = m_rulerScale->getXForTime(time) + m_currentXOffset
                   + m_xorigin;

/*	
	paint.drawLine(static_cast<int>(x),
		       height() - (height()/4),
		       static_cast<int>(x),
		       height());
*/

	if ((i->second & timeSigChangeHere) && !m_refreshLinesOnly) {

	    Rosegarden::TimeSignature sig =
		m_composition->getTimeSignatureAt(time);

	    QString str = QString("%1/%2")
		.arg(sig.getNumerator())
		.arg(sig.getDenominator());

	    paint.setFont(m_boldFont);
	    paint.drawText(static_cast<int>(x) + 2, m_height - 2, str);
	}

	if ((i->second & tempoChangeHere) && !m_refreshLinesOnly) { 

	    double tempo = m_composition->getTempoQpm(m_composition->getTempoAtTime(time));
	    long bpm = long(tempo);
//	    long frac = long(tempo * 100 + 0.001) - 100 * bpm;

	    QString tempoString = QString("%1").arg(bpm);

	    if (tempo == prevTempo) {
		if (m_small) continue;
		tempoString = "=";
	    } else if (bpm == prevBpm) {
		tempoString = (tempo > prevTempo ? "+" : "-");
	    } else {
		if (m_small && (bpm != (bpm / 10 * 10))) {
		    if (bpm == prevBpm + 1) tempoString = "+";
		    else if (bpm == prevBpm - 1) tempoString = "-";
		}
	    }
	    prevTempo = tempo;
	    prevBpm = bpm;

	    QRect bounds = m_fontMetrics.boundingRect(tempoString);

	    paint.setFont(m_font);
	    if (time > 0) x -= bounds.width() / 2;
//	    if (x > bounds.width() / 2) x -= bounds.width() / 2;
	    if (prevEndX >= x - 3) x = prevEndX + 3;
	    paint.drawText(static_cast<int>(x), textY, tempoString);
	    prevEndX = x + bounds.width();
	}
    }

    m_refreshLinesOnly = false;
}

#include "temporuler.moc"
