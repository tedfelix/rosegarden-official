
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef _COMPOSITION_H_
#define _COMPOSITION_H_

#include <set>

#include "Track.h"

namespace Rosegarden 
{
    
/**
 * Composition contains a complete representation of a piece of music.
 * It is a container for multiple Tracks, as well as any associated
 * non-Event data.
 * 
 * The Composition owns the Tracks it holds, and deletes them on
 * destruction.
 */

class Composition : public TrackObserver
{
    
public:
    typedef std::set<Track*, Track::TrackCmp> trackcontainer;

    typedef trackcontainer::iterator iterator;
    typedef trackcontainer::const_iterator const_iterator;

    Composition();
    virtual ~Composition();

    /// swap the contents with other composition
    void swap(Composition&);

    trackcontainer& tracks() { return m_tracks; }

    iterator addTrack(Track*);

    void deleteTrack(iterator);
    bool deleteTrack(Track*);

    unsigned int getNbTracks() const { return m_tracks.size(); }

    // returns the nb of time steps of the longest track
    unsigned int getDuration() const;
    void         clear();

    //!!! these should go, as the results they return are entirely
    //arbitrary -- but they're used in transport code, so work out how
    //the transport *should* do it
    unsigned int getNbTicksPerBar() const { return m_nbTicksPerBar; }
    void setNbTicksPerBar(unsigned int n) { m_nbTicksPerBar = n; }

    // Some set<> API delegation
    iterator       begin()       { return m_tracks.begin(); }
    const_iterator begin() const { return m_tracks.begin(); }
    iterator       end()         { return m_tracks.end(); }
    const_iterator end() const   { return m_tracks.end(); }


    //!!! This should arguably not be a single per-Composition value.
    // MIDI has a tempo meta-event which can change arbitrarily often,
    // and notation has metronome events that can also appear more than
    // once (and indeed per Track, rather than per Composition).
    // (We perhaps do need a per-Composition base tempo, though.)
    unsigned int getTempo() const { return m_tempo; }
    void setTempo(const int &tempo) { m_tempo = tempo; }


    const timeT& getPosition() { return m_position; }
    void setPosition(const timeT& position) { m_position = position; }


    // TrackObserver methods:

    virtual void eventAdded(Track *, Event *);
    virtual void eventRemoved(Track *, Event *);

protected:
    trackcontainer m_tracks;
    Track m_timeReference; // contains time signature events &c

    unsigned int m_nbTicksPerBar; //!!! must lose this
    unsigned int m_tempo;

    timeT m_position;

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);
};

}


#endif

