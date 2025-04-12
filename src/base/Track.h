/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TRACK_H
#define RG_TRACK_H

#include "rosegardenprivate_export.h"

#include "XmlExportable.h"
#include "Instrument.h"
#include "Device.h"

#include <string>


namespace Rosegarden
{


class Composition;

typedef unsigned int TrackId;
constexpr TrackId NoTrack = 0xDEADBEEF;

/// Representation of a Track.
/**
 * A Track represents a line on the SegmentCanvas on the
 * Rosegarden GUI.  A Track is owned by a Composition and
 * has reference to an Instrument from which the playback
 * characteristics of the Track can be derived.  A Track
 * has no type (Audio or MIDI) itself - the type comes only from the
 * Instrument relationship.
 *
 */
class ROSEGARDENPRIVATE_EXPORT Track : public XmlExportable
{
public:
    explicit Track(TrackId id,
                   InstrumentId instrument = 0,
                   int position = 0,
                   const std::string &label = "",
                   bool muted = false);
    ~Track() override  { }

    // Accessors/Mutators

    TrackId getId() const { return m_id; }

    void setMuted(bool muted)  { m_muted = muted; }
    bool isMuted() const { return m_muted; }

    /// Set the Track to be archived.
    /**
     * @param[in] refreshComp Whether to refresh Composition::m_recordTracks.
     *   Refreshing m_recordTracks would wreak havoc on .rg file read, so
     *   RoseXmlHandler should set it to false.  The UI (TrackParameterBox)
     *   should set it to true so that the list will always be in sync if the
     *   user archives or unarchives a track.
     */
    void setArchived(bool archived, bool refreshComp);
    bool isArchived() const { return m_archived; }

    void setSolo(bool solo)  { m_solo = solo; }
    bool isSolo() const  { return m_solo; }

    /// Zero-based Track position on the UI.
    void setPosition(int position) { m_position = position; }
    /// Zero-based Track position on the UI.
    int getPosition() const { return m_position; }

    void setLabel(const std::string &label)  { m_label = label; }
    std::string getLabel() const { return m_label; }

    void setShortLabel(const std::string &shortLabel)
            { m_shortLabel = shortLabel; }
    std::string getShortLabel() const { return m_shortLabel; }

    void setPresetLabel(const std::string &label);
    std::string getPresetLabel() const { return m_presetLabel; }

    void setInstrument(InstrumentId instrument);
    InstrumentId getInstrument() const { return m_instrument; }

    // For Composition use only
    void setOwningComposition(Composition *comp) { m_owningComposition = comp; }
    Composition *getOwningComposition() { return m_owningComposition; }

    void setMidiInputDevice(DeviceId id);
    DeviceId getMidiInputDevice() const { return m_input_device; }

    void setMidiInputChannel(char ic);
    char getMidiInputChannel() const { return m_input_channel; }

    enum ThruRouting { Auto, On, Off, WhenArmed };
    void setThruRouting(ThruRouting thruRouting);
    ThruRouting getThruRouting() const  { return m_thruRouting; }

    int getClef() const  { return m_clef; }
    void setClef(int clef) { m_clef = clef; }

    int getTranspose() const  { return m_transpose; }
    void setTranspose(int transpose) { m_transpose = transpose; }

    int getColor() const  { return m_color; }
    void setColor(int color) { m_color = color; }

    int getHighestPlayable() const  { return m_highestPlayable; }
    void setHighestPlayable(int pitch) { m_highestPlayable = pitch; }

    int getLowestPlayable() const  { return m_lowestPlayable; }
    void setLowestPlayable(int pitch) { m_lowestPlayable = pitch; }

    // Controls size of exported staff in LilyPond
    int getStaffSize() const  { return m_staffSize; }
    void setStaffSize(int index) { m_staffSize = index; }

    // Staff bracketing in LilyPond
    int getStaffBracket() const  { return m_staffBracket; }
    void setStaffBracket(int index) { m_staffBracket = index; }

    /// Returns false for archived Track objects.
    bool isArmed() const;
    /// Without consideration of archived.  Use for writing .rg files.
    bool isReallyArmed() const  { return m_armed; }
    /// Arm or disarm the Track.
    /**
     * This routine should only be called by Composition::setTrackRecording()
     * and RoseXmlHandler::startElement().
     *
     * Composition maintains a list of tracks that are recording.  Calling
     * this routine directly will bypass that.
     */
    void setArmed(bool armed)  { m_armed = armed; }


    // XmlExportable override.

    /// For writing.  See RoseXmlHandler for reading.
    std::string toXmlString() const override;

private:
    // Hide copy ctor and op=.
    Track(const Track &);
    Track operator=(const Track &);

    //--------------- Data members ---------------------------------

    TrackId        m_id;
    bool           m_muted;
    bool           m_archived;
    bool           m_solo;
    std::string    m_label;
    std::string    m_shortLabel;
    std::string    m_presetLabel;
    /// Zero-based Track position on the UI.
    int m_position;
    InstrumentId   m_instrument;

    Composition   *m_owningComposition;

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
