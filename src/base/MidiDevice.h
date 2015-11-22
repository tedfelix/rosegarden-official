/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDIDEVICE_H
#define RG_MIDIDEVICE_H

#include <string>
#include <vector>

#include "Device.h"
#include "Instrument.h"
#include "MidiMetronome.h"
#include "MidiProgram.h"
#include "ControlParameter.h"
#include "Controllable.h"

namespace Rosegarden
{

typedef std::vector<std::string> StringList;
typedef std::vector<MidiByte> MidiByteList;

class AllocateChannels;

class MidiDevice : public Device, public Controllable
{
public:
    enum DeviceDirection {
        Play = 0,
        Record = 1
    };

    enum VariationType {
        NoVariations,
        VariationFromLSB,
        VariationFromMSB
    };

    MidiDevice();
    MidiDevice(const MidiDevice &);
    MidiDevice(DeviceId id,
               InstrumentId ibase,
               const MidiDevice &);
    MidiDevice(DeviceId id,
               InstrumentId ibase,
               const std::string &name,
               DeviceDirection dir);
    MidiDevice(DeviceId id,
               InstrumentId ibase,
               const std::string &name,
               const std::string &label,
               DeviceDirection dir);
    virtual ~MidiDevice();

    // Assignment
    MidiDevice &operator=(const MidiDevice &);

    virtual AllocateChannels *getAllocator(void);

    // Instrument must be on heap; I take ownership of it
    virtual void addInstrument(Instrument*);

    void setMetronome(const MidiMetronome &);
    const MidiMetronome* getMetronome() const { return m_metronome; }

    void addProgram(const MidiProgram &program);
    void addBank(const MidiBank &bank);
    void addKeyMapping(const MidiKeyMapping &mapping); // I own the result!

    void clearBankList();
    void clearProgramList();
    void clearKeyMappingList();
    void clearControlList();

    const BankList &getBanks() const { return m_bankList; }
    BankList getBanks(bool percussion) const;
    BankList getBanksByMSB(bool percussion, MidiByte msb) const; 
    BankList getBanksByLSB(bool percussion, MidiByte lsb) const;
    const MidiBank *getBankByName(const std::string &) const;
    
    MidiByteList getDistinctMSBs(bool percussion, int lsb = -1) const;
    MidiByteList getDistinctLSBs(bool percussion, int msb = -1) const;

    const ProgramList &getPrograms() const { return m_programList; }
    ProgramList getPrograms(const MidiBank &bank) const;
    /// Used by the UI to display all programs in variations mode.
    ProgramList getPrograms0thVariation(bool percussion, const MidiBank &bank) const;

    const KeyMappingList &getKeyMappings() const { return m_keyMappingList; }
    const MidiKeyMapping *getKeyMappingByName(const std::string &) const;
    const MidiKeyMapping *getKeyMappingForProgram(const MidiProgram &program) const;
    void setKeyMappingForProgram(const MidiProgram &program, std::string mapping);

    std::string getBankName(const MidiBank &bank) const;
    std::string getProgramName(const MidiProgram &program) const;

    void replaceBankList(const BankList &bank);
    void replaceProgramList(const ProgramList &program);
    void replaceKeyMappingList(const KeyMappingList &mappings);

    void mergeBankList(const BankList &bank);
    void mergeProgramList(const ProgramList &program);
    void mergeKeyMappingList(const KeyMappingList &mappings);

    virtual InstrumentList getAllInstruments() const;
    virtual InstrumentList getPresentationInstruments() const;

    // Retrieve Librarian details
    //
    const std::string getLibrarianName() const { return m_librarian.first; }
    const std::string getLibrarianEmail() const { return m_librarian.second; }
    std::pair<std::string, std::string> getLibrarian() const
        { return m_librarian; }

    // Set Librarian details
    //
    void setLibrarian(const std::string &name, const std::string &email)
        { m_librarian = std::pair<std::string, std::string>(name, email); }

    DeviceDirection getDirection() const { return m_direction; }
    void setDirection(DeviceDirection dir) { m_direction = dir; }

    VariationType getVariationType() const { return m_variationType; }
    void setVariationType(VariationType v) { m_variationType = v; }

    // Controllers - for mapping Controller names to values for use in
    // the InstrumentParameterBoxes (IPBs) and Control rulers.
    //
    ControlList::const_iterator beginControllers() const
        { return m_controlList.begin(); }
    ControlList::const_iterator endControllers() const
        { return m_controlList.end(); }

    // implemented from Controllable interface
    //
    virtual const ControlList &getControlParameters() const { return m_controlList; }

    // Only those on the IPB list
    //
    ControlList getIPBControlParameters() const;

    // Access ControlParameters (read/write)
    //
    virtual ControlParameter *getControlParameter(int index);
    virtual const ControlParameter *getControlParameter(int index) const;
    virtual ControlParameter *getControlParameter(const std::string &type, MidiByte controllerNumber);
    virtual const ControlParameter *getControlParameter(const std::string &type, MidiByte controllerNumber) const;

    // Modify ControlParameters
    //
    void addControlParameter(const ControlParameter &con,
                             bool propagateToInstruments);
    void addControlParameter(const ControlParameter &con, int index,
                             bool propagateToInstruments);
    bool removeControlParameter(int index);
    bool modifyControlParameter(const ControlParameter &con, int index);

    void replaceControlParameters(const ControlList &);
    void refreshControlParameters(void);

    // Check to see if the passed ControlParameter is unique in
    // our ControlParameter list.
    //
    bool isUniqueControlParameter(const ControlParameter &con) const;
    
    const ControlParameter *
        findControlParameter(std::string type, MidiByte conNumber) const;
    
    // Check if controller is visible.
    //
    bool isVisibleControlParameter(const ControlParameter &con) const;

    // Generate some default controllers for the MidiDevice
    //
    void generateDefaultControllers();

    virtual std::string toXmlString() const;

    virtual void refreshForConnection(void);

    // Accessors for recording property
    bool isRecording() {return m_recording; }

    static bool isPercussionNumber(int channel)
    { return channel == 9; }

protected:
    void createInstruments(InstrumentId);
    void renameInstruments();
    void conformInstrumentControllers(void);

    void generatePresentationList();

    // Push the default IPB controllers to the device's Instruments.
    //
    void deviceToInstrControllerPush();
    
    // Add a new control to all of the device's Instruments.
    //
    void addControlToInstrument(const ControlParameter &con);

    // Remove a control from all of the device's Instruments.
    //
    void removeControlFromInstrument(const ControlParameter &con);

    ProgramList    m_programList;
    BankList       m_bankList;
    ControlList    m_controlList;
    KeyMappingList m_keyMappingList;
    MidiMetronome *m_metronome;
    
    // used when we're presenting the instruments
    InstrumentList  m_presentationInstrumentList;

    // Is this device Play or Record?
    //
    DeviceDirection m_direction; 
    
    // Is this device recording?
    //
    static const bool m_recording = true;   
    
    // Should we present LSB or MSB of bank info as a Variation number?
    //
    VariationType m_variationType;

    // Librarian contact details
    //
    std::pair<std::string, std::string> m_librarian; // name. email

    // The channel allocator.
    AllocateChannels  *m_allocator;
};

}

#endif // RG_MIDIDEVICE_H
