
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CHORDNAMERULER_H
#define RG_CHORDNAMERULER_H

#include "base/PropertyName.h"
#include <map>
#include <QFont>
#include <QFontMetrics>
#include <QSize>
#include <QWidget>
#include <vector>
#include "base/Event.h"


class QPaintEvent;


namespace Rosegarden
{

class Studio;
class Segment;
class RulerScale;
class RosegardenDocument;
class Composition;


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
     * RulerScale.  Be aware that it will not be refreshed until
     * setReady is called (because the first refresh is expensive).
     */
    ChordNameRuler(RulerScale *rulerScale,
                   RosegardenDocument *doc,
                   double xorigin = 0.0,
                   int height = 0,
                   QWidget* parent = 0);

    /**
     * Construct a ChordNameRuler that displays the chords in the
     * given Segments at positions calculated by the given
     * RulerScale.  Be aware that it will not be refreshed until
     * setReady is called (because the first refresh is expensive).
     */
    ChordNameRuler(RulerScale *rulerScale,
                   RosegardenDocument *doc,
                   std::vector<Segment *> &segments,
                   double xorigin = 0.0,
                   int height = 0,
                   QWidget* parent = 0);

    ~ChordNameRuler();

    /// Indicate that the chord-name ruler should make itself ready and refresh
    void setReady();

    // may have one of these; can be changed at any time (to any in given composition):
    void setCurrentSegment(Segment *segment);

    // may have one of these (to avoid using percussion tracks in chords):
    void setStudio(Studio *studio);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent *);

private:
    void recalculate(timeT from = 0,
                     timeT to = 0);

    double m_xorigin;
    int    m_height;
    int    m_currentXOffset;
    int    m_width;
    bool   m_ready;

    RulerScale  *m_rulerScale;

    Composition *m_composition;
    unsigned int m_compositionRefreshStatusId;

    typedef std::map<Segment *, int> SegmentRefreshMap;
    SegmentRefreshMap m_segments; // map to refresh status id
    bool m_regetSegmentsOnChange;

    Segment *m_currentSegment;
    Studio *m_studio;

    Segment *m_chordSegment;

    QFont m_font;
    QFont m_boldFont;
    QFontMetrics m_fontMetrics;

    const PropertyName TEXT_FORMAL_X;
    const PropertyName TEXT_ACTUAL_X;
};


}

#endif
