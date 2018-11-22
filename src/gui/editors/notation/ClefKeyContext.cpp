/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#include "ClefKeyContext.h"
#include "NotationScene.h"
#include "NotationStaff.h"
#include "base/Segment.h"
#include "misc/Debug.h"

#include <map>
#include <utility>


namespace Rosegarden
{

ClefKeyContext::ClefKeyContext() :
    m_scene(nullptr),
    m_changed(true)
{
}

ClefKeyContext::~ClefKeyContext()
{
    ClefMaps::iterator icc;
    for (icc = m_clefMaps.begin(); icc != m_clefMaps.end(); ++icc) {
        delete (*icc).second;
    }
    m_clefMaps.clear();

    KeyMaps::iterator ikc;
    for (ikc = m_keyMaps.begin(); ikc != m_keyMaps.end(); ++ikc) {
        delete (*ikc).second;
    }
    m_keyMaps.clear();


    // Disconnect observer from segments :
    // NotationScene is now handling SegmentObserver management.

}

void
ClefKeyContext::setSegments(NotationScene *scene)
{
    ClefMaps::iterator icc;
    KeyMaps::iterator ikc;
    timeT ctime;

    m_scene = scene;


    // Delete existing clef/key maps

    for (icc = m_clefMaps.begin(); icc != m_clefMaps.end(); ++icc) {
        delete (*icc).second;
    }
    m_clefMaps.clear();

    for (ikc = m_keyMaps.begin(); ikc != m_keyMaps.end(); ++ikc) {
        delete (*ikc).second;
    }
    m_keyMaps.clear();


    // Create new clef/key maps from segments
    // Eventual inconsistancies (when segments are superimposed) are ignored
    // here: in such case, some clef/key are dropped and displayed notation
    // will be wrong. (With inconsistancies, notation is always wrong).

    std::vector<NotationStaff *> *staffs = scene->getStaffs();

    std::vector<NotationStaff *>::iterator staffsIt;
    for (staffsIt = staffs->begin(); staffsIt != staffs->end(); ++staffsIt) {
        TrackId trackId = (*staffsIt)->getSegment().getTrack();
        // RG_DEBUG << "Track=" << trackId << "\n";

        icc = m_clefMaps.find(trackId);
        if (icc == m_clefMaps.end()) {
            ClefMap *clefMap = new ClefMap;
            icc = m_clefMaps.insert(
                      ClefMaps::value_type(trackId, clefMap)).first;
//           RG_DEBUG << "INSERT track=" << trackId
//                     << " map@=" << clefMap << "\n";
        }

        ikc = m_keyMaps.find(trackId);
        if (ikc == m_keyMaps.end()) {
            KeyMap *keyMap = new KeyMap;
            ikc = m_keyMaps.insert(
                      KeyMaps::value_type(trackId, keyMap)).first;
        }

        Segment &s = (*staffsIt)->getSegment();
        bool again;

        // Set clefs and keys undefined outside segments
        (*icc).second->insert(ClefMap::value_type(s.getStartTime(),
                                                  Clef::UndefinedClef));
        (*icc).second->insert(ClefMap::value_type(s.getEndMarkerTime(),
                                                  Clef::UndefinedClef));
        (*ikc).second->insert(KeyMap::value_type(s.getStartTime(),
                                                  Key::UndefinedKey));
        (*ikc).second->insert(KeyMap::value_type(s.getEndMarkerTime(),
                                                  Key::UndefinedKey));

        Clef clef = s.getClefAtTime(s.getStartTime(), ctime);
        do {
            // If a clef value is already here, replace it.
            (*(*icc).second)[ctime] = clef;
            again = s.getNextClefTime(ctime, ctime);
            if (again) clef = s.getClefAtTime(ctime);
        } while(again);

        Key key = s.getKeyAtTime(s.getStartTime(), ctime);
        do {
            // If a key value is already here, replace it.
            (*(*ikc).second)[ctime] = key;
            again = s.getNextKeyTime(ctime, ctime);
            if (again) key = s.getKeyAtTime(ctime);
        } while(again);

        // ClefKeyContext have to observe the segments to know
        // about clef, key and segment size changes.
        // NotationScene is now handling SegmentObserver management.
        /// s.addObserver(this);
    }

    m_changed = false;
}

Clef
ClefKeyContext::getClefFromContext(TrackId track, timeT time)
{
    if (m_changed) setSegments(m_scene);

    ClefMaps::iterator i = m_clefMaps.find(track);
    if (i == m_clefMaps.end()) {
        RG_WARNING << "TrackId " << track << " not found in ClefKeyContext."
                  << "Probably this is a bug.";
        return Clef::UndefinedClef;
    }

    ClefMap::iterator j = (*i).second->lower_bound(time);
    if (j == (*i).second->begin()) return Clef::UndefinedClef;
    --j;
    return j->second;
}

Key
ClefKeyContext::getKeyFromContext(TrackId track, timeT time)
{
    if (m_changed) setSegments(m_scene);

    KeyMaps::iterator i = m_keyMaps.find(track);
    if (i == m_keyMaps.end()) {
        RG_WARNING << "TrackId" << track << "not found in ClefKeyContext.\n"
                   << "Probably this is a bug.";
        return Key::UndefinedKey;
    }

    KeyMap::iterator j = (*i).second->lower_bound(time);
    if (j == (*i).second->begin()) return Key::UndefinedKey;
    --j;
    return j->second;
}

// Only for debug
void
ClefKeyContext::dumpClefContext()
{
    ClefMaps::iterator i;
    ClefMap::iterator j;

    RG_DEBUG << "Begin of clef context dump =================";

    for (i = m_clefMaps.begin(); i != m_clefMaps.end(); ++i) {
        RG_DEBUG << "    Track = " << (*i).first;
        ClefMap *m = (*i).second;

        for (j = m->begin(); j != m->end(); ++j) {
            RG_DEBUG << "        Time = " << (*j).first
                      << " Clef = " << (*j).second.getClefType();
        }
    }

    RG_DEBUG << "End of clef context dump =================";
}

// Only for debug
void
ClefKeyContext::dumpKeyContext()
{
    KeyMaps::iterator i;
    KeyMap::iterator j;

    RG_DEBUG << "Begin of key context dump =================";

    for (i = m_keyMaps.begin(); i != m_keyMaps.end(); ++i) {
        RG_DEBUG << "    Track = " << (*i).first;
        KeyMap *m = (*i).second;

        for (j = m->begin(); j != m->end(); ++j) {
            RG_DEBUG << "        Time = " << (*j).first
                      << " Key = " << (*j).second.getName();
        }
    }

    RG_DEBUG << "End of key context dump =================";
}


void
ClefKeyContext::eventAdded(const Segment *s, Event *e)
{
    if (e->isa(Clef::EventType) || e->isa(Key::EventType)) {

        // Since the redundant clefs and keys may be hide, some changes
        // related to a clef or key modification may propagate across the
        // segments up to the end of the composition.
        // So next segments on the same track may need a refresh
        if (!m_changed) {   // Don't waste time if already done recently
            m_scene->updateRefreshStatuses(s->getTrack(), e->getAbsoluteTime());
        }

        // Rememember to compute the ClefKeyContext again
        m_changed = true;
    }
}

void
ClefKeyContext::eventRemoved(const Segment *s, Event *e)
{
    if (e->isa(Clef::EventType) || e->isa(Key::EventType)) {

        // Since the redundant clefs and keys may be hide, some changes
        // related to a clef or key modification may propagate across the
        // segments up to the end of the composition.
        // So next segments on the same track may need a refresh
        if (!m_changed) {   // Don't waste time if already done recently
            m_scene->updateRefreshStatuses(s->getTrack(), e->getAbsoluteTime());
        }

        // Rememember to compute the ClefKeyContext again
        m_changed = true;
    }
}

void
ClefKeyContext::startChanged(const Segment *, timeT)
{
        m_changed = true;
}

void
ClefKeyContext::endMarkerTimeChanged(const Segment *, bool /*shorten*/)
{
        m_changed = true;
}

}


