
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTEPIXMAPPAINTER_H_
#define _RG_NOTEPIXMAPPAINTER_H_

#include <qpainter.h>

namespace Rosegarden {

class NotePixmapPainter
{
    // Just a trivial class that instructs two painters to do the
    // same thing (one for the pixmap, one for the mask).  We only
    // duplicate those methods we actually use in NotePixmapFactory

public:
    NotePixmapPainter() :
	m_painter(&m_myPainter) { }

    void beginExternal(QPainter *painter) {

	m_externalPainter = painter;
	m_useMask = false;

	painter->setPen(QPen(Qt::black, 1, Qt::SolidLine,
			     Qt::RoundCap, Qt::RoundJoin));

	if (m_externalPainter) {
	    m_painter = m_externalPainter;
	} else {
	    m_painter = &m_myPainter;
	}
    }

    bool begin(QPaintDevice *device, QPaintDevice *mask = 0, bool unclipped = false) {

	m_externalPainter = 0;

	if (mask) {
	    m_useMask = true;
	    m_maskPainter.begin(mask, unclipped);
	} else {
	    m_useMask = false;
	}

	m_painter = &m_myPainter;
	return m_painter->begin(device, unclipped);
    }

    bool end() {
	if (m_useMask) m_maskPainter.end();
	return m_painter->end();
    }

    QPainter &painter() {
	return *m_painter;
    }

    QPainter &maskPainter() {
	return m_maskPainter;
    }

    void drawPoint(int x, int y) {
	m_painter->drawPoint(x, y);
	if (m_useMask) m_maskPainter.drawPoint(x, y);
    }

    void drawLine(int x1, int y1, int x2, int y2) {
	m_painter->drawLine(x1, y1, x2, y2);
	if (m_useMask) m_maskPainter.drawLine(x1, y1, x2, y2);
    }

    void drawRect(int x, int y, int w, int h) {
	m_painter->drawRect(x, y, w, h);
	if (m_useMask) m_maskPainter.drawRect(x, y, w, h);
    }
    
    void drawArc(int x, int y, int w, int h, int a, int alen) {
	m_painter->drawArc(x, y, w, h, a, alen);
	if (m_useMask) m_maskPainter.drawArc(x, y, w, h, a, alen);
    }

    void drawPolygon(const QPointArray &a, bool winding = false,
		     int index = 0, int n = -1) {
	m_painter->drawPolygon(a, winding, index, n);
	if (m_useMask) m_maskPainter.drawPolygon(a, winding, index, n);
    }

    void drawPolyline(const QPointArray &a, int index = 0, int n = -1) {
	m_painter->drawPolyline(a, index, n);
	if (m_useMask) m_maskPainter.drawPolyline(a, index, n);
    }

    void drawPixmap(int x, int y, const QPixmap &pm,
		    int sx = 0, int sy = 0, int sw = -1, int sh = -1) {
	m_painter->drawPixmap(x, y, pm, sx, sy, sw, sh);
	if (m_useMask) m_maskPainter.drawPixmap(x, y, *(pm.mask()), sx, sy, sw, sh);
    }

    void drawText(int x, int y, const QString &string) {
	m_painter->drawText(x, y, string);
	if (m_useMask) m_maskPainter.drawText(x, y, string);
    }

    void drawNoteCharacter(int x, int y, const NoteCharacter &character) {
	character.draw(m_painter, x, y);
	if (m_useMask) character.drawMask(&m_maskPainter, x, y);
    }

    void drawEllipse(int x, int y, int w, int h) {
      m_painter->drawEllipse(x, y, w, h);
      if (m_useMask) m_maskPainter.drawEllipse(x, y, w, h);
    }

private:
    bool m_useMask;
    QPainter  m_myPainter;
    QPainter  m_maskPainter;
    QPainter *m_externalPainter;
    QPainter *m_painter;
};

}

#endif
