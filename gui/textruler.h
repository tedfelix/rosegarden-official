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

namespace Rosegarden {
    class RulerScale;
    class Segment;
}

class QFont;
class QFontMetrics;


/**
 * TextRuler is a widget that shows a strip of text strings at
 * x-coordinates corresponding to specified times.  The strings
 * are obtained from a Segment, in which they are stored as
 * text events.  (The Segment does not have to be part of a
 * Composition.)
 *
 * By design, this is more suitable for the display of single-purpose
 * read-only data such as calculated chord names or (at a pinch)
 * lyrics; it's not really suitable for displaying text data
 * associated with a staff.
 */

class TextRuler : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct a TextRuler that displays the text events found
     * in the given Segment at positions calculated by the given
     * RulerScale.
     *
     * The Segment will be destroyed along with the TextRuler, if
     * it was not in a Composition when the TextRuler was created
     * and still is not when it's destroyed.  If the Segment was
     * found to be in a Composition on either occasion, it will be
     * assumed to be someone else's responsibility.
     *
     * The RulerScale will not be destroyed along with the TextRuler.
     */
    TextRuler(Rosegarden::RulerScale *rulerScale,
	      Rosegarden::Segment *segment,
              int height = 0,
              QWidget* parent = 0,
              const char *name = 0);

    ~TextRuler();

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

    bool m_mySegmentMaybe;

    Rosegarden::Segment    *m_segment;
    Rosegarden::RulerScale *m_rulerScale;

    QFont m_font;
    QFontMetrics m_fontMetrics;
};

#endif // _LOOPRULER_H_

