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

#ifndef RG_INSTRUMENT_H
#define RG_INSTRUMENT_H

#include "XmlExportable.h"
#include "PluginContainer.h"
#include "Buss.h"
#include "InstrumentStaticSignals.h"
#include "MidiProgram.h"  // for MidiByte

#include <QString>
#include <QSharedPointer>

#include <climits>  // UINT_MAX
#include <string>
#include <vector>

namespace Rosegarden
{


class Device;


typedef unsigned int InstrumentId;
constexpr InstrumentId NoInstrument = UINT_MAX;

// Instrument number groups
constexpr InstrumentId SystemInstrumentBase     = 0;
constexpr InstrumentId AudioInstrumentBase      = 1000;
constexpr InstrumentId MidiInstrumentBase       = 2000;
constexpr InstrumentId SoftSynthInstrumentBase  = 10000;

constexpr unsigned int AudioInstrumentCount     = 16;
constexpr unsigned int SoftSynthInstrumentCount = 24;

// ??? std::map anyone?
typedef std::pair<MidiByte /*controller*/, MidiByte /*value*/>
        ControllerValuePair;
typedef std::vector<ControllerValuePair> StaticControllers;


/// Typically represents a channel on a Device.
/**
 * An Instrument connects a Track (which itself contains a list of Segments)
 * to a Device that can play that Track.
 *
 * An Instrument is either MIDI or Audio (or whatever else we decide to
 * implement).
 */
class Instrument : public QObject, public XmlExportable, public PluginContainer
{
    Q_OBJECT

public:
    static constexpr unsigned SYNTH_PLUGIN_POSITION = 999;

    enum InstrumentType { Midi, Audio, SoftSynth, InvalidInstrument = -1 };

    Instrument(InstrumentId id,
               InstrumentType it,
               const std::string &name,
               Device *device);

    // Copy constructor
    // ??? This is not a copy ctor.  It is a partialCopy() routine.
    //     Get rid of this and replace with a partialCopy() to make
    //     this clear.  QObject instances cannot be copied.
    Instrument(const Instrument &);

    ~Instrument() override;

    std::string getName() const override { return m_name; }
    std::string getPresentationName() const override;

    /** Returns a translated QString suitable for presentation to the user */
    virtual QString getLocalizedPresentationName() const;

    /** Returns a number derived from the presentation name of the instrument,
     * eg. "General MIDI #15" returns 15, which corresponds with the MIDI
     * channel this Instrument uses if this instrument is a MIDI instrument.
     */
    // unused virtual unsigned int getPresentationNumber() const;

    std::string getAlias() const override;

    void setId(InstrumentId id) { m_id = id; }
    InstrumentId getId() const override { return m_id; }

    void setName(const std::string &name) { m_name = name; }
    void setAlias(const std::string &alias) { m_alias = alias; }

    void setType(InstrumentType type) { m_type = type; }
    InstrumentType getType() const { return m_type; }


    // ---------------- Fixed channels -----------------

    void setFixedChannel();
    // Release this instrument's fixed channel, if any.
    void releaseFixedChannel();
    bool hasFixedChannel() const { return m_fixed; }


    // ---------------- MIDI Controllers -----------------
    //

    void setNaturalMidiChannel(MidiByte channelId)
    {
        Q_ASSERT(m_type == Midi ||
                 (channelId == 0));
        m_midiChannel = channelId;
    }

    // Get the "natural" channel with regard to its device.  May not
    // be the same channel instrument is playing on.
    MidiByte getNaturalMidiChannel() const
    {
        return m_midiChannel;
    }

    void setMidiTranspose(MidiByte mT) { m_transpose = mT; }
    MidiByte getMidiTranspose() const { return m_transpose; }

    // Pan is 0-127 for MIDI instruments, and (for some
    // unfathomable reason) 0-200 for audio instruments.
    //
    void setPan(MidiByte pan) { m_pan = pan; }
    MidiByte getPan() const { return m_pan; }
    /// Get a 0-127 pan for both MIDI and Audio Instrument's.
    /**
     * For a MIDI Instrument, this is the same as calling getPan().  For
     * an Audio Instrument, this performs the math that is necessary to return
     * a value between 0 and 127 representing the level as returned by
     * getPan().
     */
    MidiByte getPanCC() const;

    // Volume is 0-127 for MIDI instruments.  It's not used for
    // audio instruments -- see "level" instead.
    //
    void setVolume(MidiByte volume) { m_volume = volume; }
    MidiByte getVolume() const { return m_volume; }
    /// Get a 0-127 volume for both MIDI and Audio Instrument's.
    /**
     * For a MIDI Instrument, this is the same as calling getVolume().  For
     * an Audio Instrument, this performs the math that is necessary to return
     * a value between 0 and 127 representing the level as returned by
     * getLevel().
     */
    MidiByte getVolumeCC() const;

    void setProgram(const MidiProgram &program);
    const MidiProgram &getProgram() const { return m_program; }
    /// Checks the bank and program change against the Device.
    bool isProgramValid() const;

    void setSendBankSelect(bool value) {
        m_sendBankSelect = value;
        if (value) { emit changedChannelSetup(); }
    }
    bool sendsBankSelect() const { return m_sendBankSelect; }

    void setSendProgramChange(bool value) {
        m_sendProgramChange = value;
        if (value) { emit changedChannelSetup(); }
    }
    bool sendsProgramChange() const;

    void setControllerValue(MidiByte controller, MidiByte value);
    MidiByte getControllerValue(MidiByte controller) const;
    bool hasController(MidiByte controlNumber) const;

    void sendController(MidiByte controller, MidiByte value);

    // This is retrieved from the reference MidiProgram in the Device
    const MidiKeyMapping *getKeyMapping() const;

    // Convenience functions (strictly redundant with get/setProgram):
    //
    void setProgramChange(MidiByte program);
    MidiByte getProgramChange() const;

    void setMSB(MidiByte msb);
    MidiByte getMSB() const;

    void setLSB(MidiByte lsb);
    MidiByte getLSB() const;

    /// Pick the first valid program in the connected Device.
    void pickFirstProgram(bool percussion);

    void setPercussion(bool percussion);
    bool isPercussion() const;

    // --------------- Audio Controllers -----------------
    //
    void setLevel(float dB) { m_level = dB; }
    float getLevel() const { return m_level; }

    void setRecordLevel(float dB) { m_recordLevel = dB; }
    float getRecordLevel() const { return m_recordLevel; }

    void setNumAudioChannels(unsigned int ch) { m_numAudioChannels = ch; }
    unsigned int getNumAudioChannels() const { return m_numAudioChannels; }

    // An audio input can be a buss or a record input. The channel number
    // is required for mono instruments, ignored for stereo ones.
    void setAudioInputToBuss(BussId buss, int channel = 0);
    void setAudioInputToRecord(int recordIn, int channel = 0);
    int getAudioInput(bool &isBuss, int &channel) const;

    void setAudioOutput(BussId buss) { m_audioOutput = buss; }
    BussId getAudioOutput() const { return m_audioOutput; }


    // -------------- Miscellaneous -----------------------

    // XmlExportable override
    std::string toXmlString() const override;

    // Get and set the parent device
    //
    Device *getDevice() const { return m_device; }
    void setDevice(Device* dev) { m_device = dev; }

    // Return a string describing the current program for
    // this Instrument
    //
    std::string getProgramName() const;

    // MappedId management - should typedef this type once
    // we have the energy to shake this all out.
    //
    int getMappedId() const { return m_mappedId; }
    void setMappedId(int id) { m_mappedId = id; }

    /// Get CCs at time 0 for this Instrument.
    const StaticControllers &getStaticControllers() const
            { return m_staticControllers; }

    // Clears down the instruments controls.
    //
    void clearStaticControllers() { m_staticControllers.clear(); };

    /// Removes the given controller from the list of Static Controllers.
    void removeStaticController(MidiByte controllerNumber);

    void sendWholeDeviceDestroyed()  { emit wholeDeviceDestroyed(); }

    /// Send out program changes, etc..., for fixed channels.
    /**
     * See StudioControl::sendChannelSetup().
     */
    void sendChannelSetup();

    /// For connecting to Instrument's static signals.
    /**
     * It's a good idea to hold on to a copy of this QSharedPointer in
     * a member variable to make sure the InstrumentStaticSignals
     * instance is still around when your object is destroyed.
     */
    static QSharedPointer<InstrumentStaticSignals> getStaticSignals();

    /// Emit InstrumentStaticSignals::controlChange().
    static void emitControlChange(Instrument *instrument, int cc)
            { getStaticSignals()->emitControlChange(instrument, cc); }

 signals:
    // Like QObject::destroyed, but implies that the whole device is
    // being destroyed so we needn't bother freeing channels on it.
    void wholeDeviceDestroyed();

    // Emitted when we change how we set up the MIDI channel.
    // Notifies ChannelManagers that use the instrument to refresh
    // channel
    void changedChannelSetup();

    // Emitted when we lose/gain a fixed MIDI channel.  Notifies
    // ChannelManagers that use the instrument to modify their channel
    // allocation accordingly.
    void channelBecomesFixed();
    void channelBecomesUnfixed();

private:
    // ??? Hiding to keep this simple.
    Instrument &operator=(const Instrument &) = delete;

    InstrumentId    m_id;
    std::string     m_name;
    std::string     m_alias;
    InstrumentType  m_type;

    // Standard MIDI controllers and parameters
    MidiByte        m_midiChannel;
    MidiProgram     m_program;
    MidiByte        m_transpose;
    MidiByte        m_pan;  // required by audio
    MidiByte        m_volume;

    // Whether this instrument uses a fixed channel.
    // "fixed==false" mode is experimental and usually disabled.
    bool m_fixed{true};

    // Used for Audio volume (dB value)
    //
    float           m_level;

    // Record level for Audio recording (dB value)
    //
    float           m_recordLevel;

    Device         *m_device;

    // Do we send at this intrument or do we leave these
    // things up to the parent device and God?  These are
    // directly relatable to GUI elements
    //
    bool             m_sendBankSelect;
    bool             m_sendProgramChange;

    // Instruments are directly related to faders for volume
    // control.  Here we can store the remote fader id.
    //
    int              m_mappedId;

    // Which input terminal we're connected to.  This is a BussId if
    // less than 1000 or a record input number (plus 1000) if >= 1000.
    // The channel number is only used for mono instruments.
    //
    int              m_audioInput;
    int              m_audioInputChannel;

    unsigned int     m_numAudioChannels;

    // Which buss we output to.  Zero is always the master.
    //
    BussId           m_audioOutput;

    // A static controller map that can be saved/loaded and queried along with this instrument.
    // These values are modified from the IPB and MidiMixerWindow.
    //
    StaticControllers m_staticControllers;
};


}

#endif // RG_INSTRUMENT_H
