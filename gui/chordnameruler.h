// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include "Segment.h"
#include "Selection.h"

namespace Rosegarden {
    class RulerScale;
    class Composition;
    class Studio;
}

class QFont;
class QFontMetrics;


/**
 * ChordNameRuler is a widget that shows a strip of text strings
 * describing the chords in a composition.
 */

class ChordNameRuler : public QWidget, public Rosegarden::SegmentObserver
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
		   double xorigin = 0.0,
		   int height = 0,
		   QWidget* parent = 0,
		   const char *name = 0);

    ~ChordNameRuler();

    // must have one of these:
    void setComposition(Rosegarden::Composition *composition);

    // may have one of these; can be changed at any time (to any in given composition):
    void setCurrentSegment(Rosegarden::Segment *segment);

    // may have one of these (to avoid using percussion tracks in chords):
    void setStudio(Rosegarden::Studio *studio);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

    virtual void eventAdded(const Rosegarden::Segment *, Rosegarden::Event *);
    virtual void eventRemoved(const Rosegarden::Segment *, Rosegarden::Event *);
    virtual void endMarkerTimeChanged(const Rosegarden::Segment *, bool);
    virtual void segmentDeleted(const Rosegarden::Segment *);

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent *);

private:
    double m_xorigin;
    int    m_height;
    int    m_currentXOffset;
    int    m_width;

    Rosegarden::RulerScale  *m_rulerScale;

    Rosegarden::Composition *m_composition;
    unsigned int m_compositionRefreshStatusId;
    bool m_needsRecalculate;

    Rosegarden::SegmentSelection m_segments;
    Rosegarden::Segment *m_currentSegment;
    Rosegarden::Studio *m_studio;

    Rosegarden::Segment *m_chordSegment;

    QFont m_font;
    QFont m_boldFont;
    QFontMetrics m_fontMetrics;

    const Rosegarden::PropertyName TEXT_FORMAL_X;
    const Rosegarden::PropertyName TEXT_ACTUAL_X;

    void recalculate(bool regetSegments);
};

#endif // _LOOPRULER_H_

