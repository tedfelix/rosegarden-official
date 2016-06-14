/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

// Representation of a Track
//
//


#ifndef RG_TRACK_H
#define RG_TRACK_H

#include "rosegardenprivate_export.h"

#include <string>

#include "XmlExportable.h"
#include "Instrument.h"
#include "Device.h"

#define NO_TRACK 0xDEADBEEF


namespace Rosegarden
{
class Composition;

typedef unsigned int TrackId;

/**
 * A Track represents a line on the SegmentCanvas on the
 * Rosegarden GUI.  A Track is owned by a Composition and
 * has reference to an Instrument from which the playback
 * characteristics of the Track can be derived.  A Track
 * has no type itself - the type comes only from the
 * Instrument relationship.
 *
 */
class ROSEGARDENPRIVATE_EXPORT Track : public XmlExportable
{
    friend class Composition;

public:
    //Track();
    Track(TrackId id,
          InstrumentId instrument = 0,
          int position =0 ,
          const std::string &label = "",
          bool muted = false);

private:
    Track(const Track &);
    Track operator=(const Track &);

public:

    ~Track();

    TrackId getId() const { return m_id; }

    void setMuted(bool muted);
    bool isMuted() const { return m_muted; }

    void setSolo(bool solo)  { m_solo = solo; }
    bool isSolo() const  { return m_solo; }

    void setPosition(int position) { m_position = position; }
    int getPosition() const { return m_position; }

    void setLabel(const std::string &label);
    std::string getLabel() const { return m_label; }

    void setShortLabel(const std::string &shortLabel);
    std::string getShortLabel() const { return m_shortLabel; }

    void setPresetLabel(const std::string &label);
    std::string getPresetLabel() const { return m_presetLabel; }

    void setInstrument(InstrumentId instrument);
    InstrumentId getInstrument() const { return m_instrument; }

    // Implementation of virtual
    //
    virtual std::string toXmlString() const;

    Composition* getOwningComposition() { return m_owningComposition; }

    void setMidiInputDevice(DeviceId id);
    DeviceId getMidiInputDevice() const { return m_input_device; }

    void setMidiInputChannel(char ic);
    char getMidiInputChannel() const { return m_input_channel; }

    enum ThruRouting { Auto, On, Off, WhenArmed };
    void setThruRouting(ThruRouting thruRouting);
    ThruRouting getThruRouting() const  { return m_thruRouting; }

    int getClef() { return m_clef; }
    void setClef(int clef) { m_clef = clef; }

    int getTranspose() { return m_transpose; }
    void setTranspose(int transpose) { m_transpose = transpose; }

    int getColor() { return m_color; }
    void setColor(int color) { m_color = color; }

    int getHighestPlayable() { return m_highestPlayable; }
    void setHighestPlayable(int pitch) { m_highestPlayable = pitch; }
    
    int getLowestPlayable() { return m_lowestPlayable; }
    void setLowestPlayable(int pitch) { m_lowestPlayable = pitch; }

    // Controls size of exported staff in LilyPond
    int getStaffSize() { return m_staffSize; }
    void setStaffSize(int index) { m_staffSize = index; }
    
    // Staff bracketing in LilyPond
    int getStaffBracket() { return m_staffBracket; }
    void setStaffBracket(int index) { m_staffBracket = index; }
    
    bool isArmed() const { return m_armed; }
    /// This routine should only be called by Composition::setTrackRecording().
    /**
     * Composition maintains a list of tracks that are recording.  Calling
     * this routine directly will bypass that.
     */
    void setArmed(bool armed);

protected: // For Composition use only
    void setOwningComposition(Composition* comp) { m_owningComposition = comp; }

private:
    //--------------- Data members ---------------------------------

    TrackId        m_id;
    bool           m_muted;
    bool           m_solo;
    std::string    m_label;
    std::string    m_shortLabel;
    std::string    m_presetLabel;
    int            m_position;
    InstrumentId   m_instrument;

    Composition*   m_owningComposition;

    DeviceId       m_input_device;
    char           m_input_channel;
    ThruRouting    m_thruRouting;
    bool           m_armed;

    // default parameters for new segments created belonging to this track
    int            m_clef;
    int            m_transpose;
    int            m_color;
    int            m_highestPlayable;
    int            m_lowestPlayable;

    // staff parameters for LilyPond export
    int            m_staffSize;
    int            m_staffBracket;
};

}

#endif // RG_TRACK_H
 
