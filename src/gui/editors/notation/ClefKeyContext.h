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


#ifndef RG_CLEF_KEY_CONTEXT_H
#define RG_CLEF_KEY_CONTEXT_H

/*
 * ClefKeyContext.h
 *
 * This file defines a class which maintains the clef and key context
 * of a set of segments currently displayed in the notation editor.
 *
 * This context is used to see the redundant clef/key and make them invisible.
 *
 * Such invisibility can't be an event property as it may depend of other staff
 * currently edited on the same track (ie the same clef/key event may be
 * visible in a notation editor and simultaneously may be invisible in another
 * one depending of segments opened in each editor).
 */

// !!! Currently, clefs and keys lists used by notation editor are maintained
// in three different places:
//     - 1) In NotationStaff (m_clefChanges and m_keyChanges)
//     - 2) In StaffHeader (m_clefOverlaps and m_keyOverlaps)
//     - 3) Here (referenced from NotationScene::m_clefKeyContext)
// It may worth to merge these three lists and the related methods inside
// one object only.

#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Track.h"

#include <vector>
#include <map>


namespace Rosegarden
{

class NotationScene;

class ClefKeyContext : public SegmentObserver
{

public:
    /**
     * Create an empty context
     */
    ClefKeyContext();

    ~ClefKeyContext();

    void setSegments(NotationScene *scene);

    /**
     * Returns the clef which should be in used on given track at given time
     * without looking at possible clef event on this precise place.
     * Returns undefinedClef if time is outside any segment or if there is
     * no clef before time in the segment.
     */
    Clef getClefFromContext(TrackId track, timeT time);

    /**
     * Returns the key which should be in used on given track at given time
     * without looking at possible clef event on this precise place.
     * Returns undefinedKey if time is outside any segment or if there is
     * no key before time in the segment.
     */
    Key getKeyFromContext(TrackId track, timeT time);

    /// Only for debug
    void dumpClefContext();
    void dumpKeyContext();

/** SegmentObserver methods **/
    virtual void eventAdded(const Segment *, Event *);

    virtual void eventRemoved(const Segment *, Event *);

    virtual void startChanged(const Segment *, timeT);

    virtual void endMarkerTimeChanged(const Segment *, bool /*shorten*/);

    virtual void segmentDeleted(const Segment *) { }
    // Nothing to do here : the cleanup is handled by NotationScene


public slots :

signals :

private :

    typedef std::map<timeT, Clef> ClefMap;
    typedef std::map<timeT, Key> KeyMap;

    typedef std::map<TrackId, ClefMap *> ClefMaps;
    typedef std::map<TrackId, KeyMap *> KeyMaps;

    ClefMaps m_clefMaps;
    KeyMaps m_keyMaps;

    NotationScene * m_scene;   // Only here for SegmentObserver methods
    bool m_changed;
};


}

#endif // RG_CLEF_KEY_CONTEXT_H
