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


#ifndef _TEXTRULER_H_
#define _TEXTRULER_H_

#include <qwidget.h>

#include "PropertyName.h"

namespace Rosegarden {
    class RulerScale;
    class Composition;
}

class QFont;
class QFontMetrics;


/**
 * ChordNameRuler is a widget that shows a strip of text strings
 * describing the chords in a composition.
 */

class ChordNameRuler : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct a ChordNameRuler that displays the chords in the
     * given Composition at positions calculated by the given
     * RulerScale.
     */
    ChordNameRuler(Rosegarden::RulerScale *rulerScale,
		   Rosegarden::Composition *composition,
		   int height = 0,
		   QWidget* parent = 0,
		   const char *name = 0);

    ~ChordNameRuler();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent *);

private:
    int  m_height;
    int  m_currentXOffset;
    int  m_width;

    Rosegarden::RulerScale  *m_rulerScale;
    Rosegarden::Composition *m_composition;

    QFont m_font;
    QFont m_boldFont;
    QFontMetrics m_fontMetrics;

    const Rosegarden::PropertyName TEXT_FORMAL_X;
    const Rosegarden::PropertyName TEXT_ACTUAL_X;
};

#endif // _LOOPRULER_H_

