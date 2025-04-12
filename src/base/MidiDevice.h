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

    MidiDevice(DeviceId id,
               InstrumentId ibase,
               const std::string &name,
               DeviceDirection dir);
    ~MidiDevice() override;

    // Copy ctor.
    // ??? Switch pointers to objects and rely on bitwise copy to take
    //     care of this.  If that's workable, then this can go.
    // ??? Only used by ImportDeviceDialog.
    MidiDevice(const MidiDevice &);

    //MidiDevice();
    //MidiDevice(DeviceId id,
    //           InstrumentId ibase,
    //           const MidiDevice &);
    //MidiDevice(DeviceId id,
    //           InstrumentId ibase,
    //           const std::string &name,
    //           const std::string &label,
    //           DeviceDirection dir);

    AllocateChannels *getAllocator() const override;

    // Instrument must be on heap; I take ownership of it
    void addInstrument(Instrument*) override;

    void setMetronome(const MidiMetronome &);
    const MidiMetronome* getMetronome() const { return m_metronome; }

    void addProgram(const MidiProgram &prog);
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
    // unused std::string getBankName(const MidiBank &bank) const;
    // Generate an unused "new bank" name.
    std::string makeNewBankName() const;

    MidiByteList getDistinctMSBs(bool percussion, int lsb = -1) const;
    MidiByteList getDistinctLSBs(bool percussion, int msb = -1) const;

    const ProgramList &getPrograms() const { return m_programList; }
    ProgramList getPrograms(const MidiBank &bank) const;
    /// Used by the UI to display all programs in variations mode.
    ProgramList getPrograms0thVariation(bool percussion, const MidiBank &bank) const;

    const KeyMappingList &getKeyMappings() const { return m_keyMappingList; }
    const MidiKeyMapping *getKeyMappingByName(const std::string &) const;
    const MidiKeyMapping *getKeyMappingForProgram(const MidiProgram &program) const;
    std::string makeNewKeyMappingName() const;

    std::string getProgramName(const MidiProgram &program) const;

    void replaceBankList(const BankList &bankList);
    void replaceProgramList(const ProgramList &programList);
    void replaceKeyMappingList(const KeyMappingList &keyMappingList);

    void mergeBankList(const BankList &bankList);
    void mergeProgramList(const ProgramList &programList);
    void mergeKeyMappingList(const KeyMappingList &keyMappingList);

    /// Includes special Instrument below MidiInstrumentBase.
    InstrumentList getAllInstruments() const override;
    /// Omit special system Instruments below MidiInstrumentBase.
    /**
     * See generatePresentationList().
     */
    InstrumentList getPresentationInstruments() const override;

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
    void setDirection(DeviceDirection dir) { m_direction = dir; notifyDeviceModified(); }
    bool isOutput() const  override { return (m_direction == Play); }
    bool isInput() const  override { return (m_direction == Record); }

    enum VariationType {
        NoVariations,
        VariationFromLSB,
        VariationFromMSB
    };

    VariationType getVariationType() const { return m_variationType; }
    void setVariationType(VariationType v) { m_variationType = v; notifyDeviceModified(); }

    // Controllers - for mapping Controller names to values for use in
    // the InstrumentParameterBoxes (IPBs) and Control rulers.
    //
    ControlList::const_iterator beginControllers() const
        { return m_controlList.begin(); }
    ControlList::const_iterator endControllers() const
        { return m_controlList.end(); }

    // implemented from Controllable interface
    //
    const ControlList &getControlParameters() const override { return m_controlList; }

    // Only those on the IPB list
    //
    ControlList getIPBControlParameters() const;

    // Access ControlParameters (read/write)
    //
    ControlParameter *getControlParameter(int index);
    ControlParameter *getControlParameter(
            const std::string &type, MidiByte controllerNumber);
    const ControlParameter *getControlParameterConst(
            const std::string &type, MidiByte controllerNumber) const override;

    // Modify ControlParameters
    //
    void addControlParameter(const ControlParameter &con,
                             bool propagateToInstruments);
    void addControlParameter(const ControlParameter &con, int index,
                             bool propagateToInstruments);
    bool removeControlParameter(int index);
    bool modifyControlParameter(const ControlParameter &con, int index);

    void replaceControlParameters(const ControlList &);
    void refreshControlParameters();

    // Check to see if the passed ControlParameter is unique in
    // our ControlParameter list.
    //
    bool isUniqueControlParameter(const ControlParameter &con) const;

    const ControlParameter *
        findControlParameter(const std::string& type,
                             MidiByte controllerNumber) const;

    /// The CC or other controller has a knob on the MIPP.
    /**
     * ??? Only CCs can have knobs.  It's misleading to allow a complete
     *     ControlParameter here.  Perhaps remove this routine.  Certainly
     *     move it to private as it is only called internally (like most
     *     of the routines in this class).
     */
    static bool isVisibleControlParameter(const ControlParameter &con);
    /// The CC has a knob on the MIPP.
    bool isVisibleControlParameter(MidiByte controlNumber) const;

    // Generate some default controllers for the MidiDevice
    //
    void generateDefaultControllers();

    std::string toXmlString() const override;

    static bool isPercussionNumber(int channel)
    { return channel == 9; }

    /// See m_userConnection.
    void setUserConnection(const std::string& connection)
            { m_userConnection = connection; notifyDeviceModified(); }
    std::string getUserConnection() const
            { return m_userConnection; }

protected:
    void createInstruments(InstrumentId);
    void renameInstruments() override;
    //void conformInstrumentControllers(void);

    void generatePresentationList();

    // Push the default IPB controllers to the device's Instruments.
    //
    void deviceToInstrControllerPush();

    // Add a new control to all of the device's Instruments.
    //
    void addControlToInstrument(const ControlParameter &con) const;

    // Remove a control from all of the device's Instruments.
    //
    void removeControlFromInstrument(const ControlParameter &controlParameter) const;

    /// The connection the user has asked for.
    /**
     * This can only be changed by the user.  It is used when attempting
     * to make a connection.  It is stored in the .rg file.
     */
    std::string m_userConnection;

    ProgramList    m_programList;
    BankList       m_bankList;
    ControlList    m_controlList;
    KeyMappingList m_keyMappingList;
    // ??? This should be easy to change to an object which would simplify
    //     copying of MidiDevice.
    MidiMetronome *m_metronome;

    // used when we're presenting the instruments
    InstrumentList  m_presentationInstrumentList;

    // Is this device Play or Record?
    //
    DeviceDirection m_direction;

    // Should we present LSB or MSB of bank info as a Variation number?
    //
    VariationType m_variationType;

    // Librarian contact details
    //
    std::pair<std::string, std::string> m_librarian; // name. email

    // The channel allocator.
    // ??? Lifespan is same as class.  Make this an object.
    AllocateChannels  *m_allocator;

private:
    // not used
    MidiDevice &operator=(const MidiDevice &);
};

}

#endif // RG_MIDIDEVICE_H
