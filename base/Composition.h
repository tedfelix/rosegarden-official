// -*- c-basic-offset: 4 -*-

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
#include "Quantizer.h"

namespace Rosegarden 
{
    
/**
 * Composition contains a complete representation of a piece of music.
 * It is a container for multiple Tracks, as well as any associated
 * non-Event data.
 * 
 * The Composition owns the Tracks it holds, and deletes them on
 * destruction.  When tracks are removed, it will also delete them.
 */

//!!! This could usefully do with a bit more tidying up.  We're
// gradually increasing the amount of stuff stored in the Composition
// as opposed to in individual Tracks.

class Composition : public TrackObserver
{
    
public:
    static const std::string BarEventType;

    typedef std::set<Track*, Track::TrackCmp> trackcontainer;

    typedef trackcontainer::iterator iterator;
    typedef trackcontainer::const_iterator const_iterator;

    Composition();
    virtual ~Composition();

    /// swap the contents with another composition
    void swap(Composition&);

    /**
     * Returns the track storing Bar and TimeSignature events
     */
    Track *getReferenceTrack() {
	referenceTrackRequested(0);
	return &m_timeReference;
    }

    const Quantizer *getQuantizer() const {
	return &m_quantizer;
    }

    trackcontainer& getTracks() { return m_tracks; }
    const trackcontainer& getTracks() const { return m_tracks; }

    /**
     * Add a new track and return an iterator pointing to it
     * The inserted Track is owned by the Composition object
     */
    iterator addTrack(Track*);

    /**
     * Delete the track pointed to by the specified iterator
     *
     * NOTE: The track is deleted from the composition and
     * destroyed
     */
    void deleteTrack(iterator);

    /**
     * Delete the track if it is part of the Composition
     * \return true if the track was found and deleted
     *
     * NOTE: The track is deleted from the composition and
     * destroyed
     */
    bool deleteTrack(Track*);

    unsigned int getNbTracks() const { return m_tracks.size(); }

    /// returns the absolute end time of the track that ends last
    timeT getDuration() const;

    /// returns the total number of bars in the composition
    unsigned int getNbBars() { return getBarNumber(getDuration()) + 1; }

    /// Removes all Tracks from the Composition and destroys them
    void clear();


    //!!! The following four functions are currently entirely untested

    /**
     * Return the number of the bar that starts at or contains time t
     */
    int getBarNumber(timeT t);

    /**
     * Return the starting time of the bar that contains time t
     */
    timeT getBarStart(timeT t);

    /**
     * Return the ending time of the bar that contains time t
     */
    timeT getBarEnd(timeT t);

    /**
     * Return the time range of bar n.  Relatively inefficient.
     * If truncate is true, will stop at end of track and return last
     * real bar if n is out of range; otherwise will happily return
     * theoretical timings for bars beyond the real end of composition
     * (this is used when extending tracks on the track editor).
     */
    std::pair<timeT, timeT> getBarRange(int n, bool truncate = false);


    //!!! these should go, as the results they return are entirely
    // arbitrary -- but they're used in transport code, so we need to
    // convert the transport to use the bar start and end instead
    unsigned int getNbTicksPerBar() const { return m_nbTicksPerBar; }
    void setNbTicksPerBar(unsigned int n) { m_nbTicksPerBar = n; }

    // Some set<> API delegation
    iterator       begin()       { return m_tracks.begin(); }
    const_iterator begin() const { return m_tracks.begin(); }
    iterator       end()         { return m_tracks.end(); }
    const_iterator end() const   { return m_tracks.end(); }

    // Tempo here is only our current Transport tempo which we use on
    // the GUI and is sent to the Sequencer.
    //
    double getTempo() const { return m_tempo; }
    void setTempo(const double &tempo) { m_tempo = tempo; }

    /// Get playback position
    const timeT& getPosition() { return m_position; }

    /// Set playback position
    void setPosition(const timeT& position) { m_position = position; }


    // TrackObserver methods:

    virtual void eventAdded(const Track *, Event *);
    virtual void eventRemoved(const Track *, Event *);
    virtual void referenceTrackRequested(const Track *);

protected:
    trackcontainer m_tracks;

    /// Contains time signature and new-bar events.
    Track m_timeReference;

    // called from calculateBarPositions
    Track::iterator addNewBar(timeT time);

    Quantizer m_quantizer;

    unsigned int m_nbTicksPerBar; //!!! must lose this
    double m_tempo;

    timeT m_position;

    /// affects the reference track in m_timeReference
    void calculateBarPositions();
    bool m_barPositionsNeedCalculating;

private:
    Composition(const Composition &);
    Composition &operator=(const Composition &);
};

}


#endif

