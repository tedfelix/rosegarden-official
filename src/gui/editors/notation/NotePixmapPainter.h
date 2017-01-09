/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTEPIXMAPPAINTER_H
#define RG_NOTEPIXMAPPAINTER_H

#include <QPainter>

#include "misc/Debug.h"


namespace Rosegarden {

class NotePixmapPainter
{
    // Just a trivial wrapper for QPainter.  We only duplicate those
    // methods we actually use in NotePixmapFactory

public:
    NotePixmapPainter() :
	m_painter(&m_myPainter) { }

    void beginExternal(QPainter *painter) {

	m_externalPainter = painter;

	painter->setPen(QPen(QColor(Qt::black), 1, Qt::SolidLine,
			     Qt::RoundCap, Qt::RoundJoin));

	if (m_externalPainter) {
	    m_painter = m_externalPainter;
	} else {
	    m_painter = &m_myPainter;
	}
    }

    bool begin(QPaintDevice *device) {

	m_externalPainter = 0;

        if (m_myPainter.isActive()) {
            NOTATION_DEBUG << "WARNING: NotePixmapPainter::begin(): Painter already active";
            assert(!m_myPainter.isActive());
        }
        
	m_painter = &m_myPainter;
	return m_painter->begin(device);
    }

    bool end() {
	return m_painter->end();
    }

    QPainter &painter() {
	return *m_painter;
    }

    void drawPoint(int x, int y) {
	m_painter->drawPoint(x, y);
    }

    void drawLine(int x1, int y1, int x2, int y2) {
	m_painter->drawLine(x1, y1, x2, y2);
    }

    void drawRect(int x, int y, int w, int h) {
	m_painter->drawRect(x, y, w, h);
    }
    
    void drawArc(int x, int y, int w, int h, int a, int alen) {
	m_painter->drawArc(x, y, w, h, a, alen);
    }

    void drawPolygon(const QPolygon &polygon, bool winding = false,
		     int index = 0, int count = -1) {

        int pointCount = (count == -1) ?  polygon.size() - index : count;
        Qt::FillRule fillRule = winding ? Qt::WindingFill : Qt::OddEvenFill;

        m_painter->drawPolygon(polygon.constData() + index, pointCount, fillRule);
    }

    void drawPolyline(const QPolygon &polygon, int index = 0, int count = -1) {

        int pointCount = (count == -1) ?  polygon.size() - index : count;

        m_painter->drawPolyline(polygon.constData() + index, pointCount);
    }

    void drawPixmap(int x, int y, const QPixmap &pm,
		    int sx = 0, int sy = 0, int sw = -1, int sh = -1) {
        m_painter->drawPixmap(x, y, pm, sx, sy, sw, sh);
    }

    void drawText(int x, int y, const QString &string) {
	m_painter->drawText(x, y, string);
    }

    void drawNoteCharacter(int x, int y, const NoteCharacter &character) {
	character.draw(m_painter, x, y);
    }

    void drawEllipse(int x, int y, int w, int h) {
        m_painter->drawEllipse(x, y, w, h);
    }

private:
    QPainter  m_myPainter;
    QPainter *m_externalPainter;
    QPainter *m_painter;
};

}

#endif
