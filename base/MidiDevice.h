// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <string>
#include <vector>

#include "Device.h"
#include "Instrument.h"
#include "MidiProgram.h"
#include "ControlParameter.h"

#ifndef _MIDIDEVICE_H_
#define _MIDIDEVICE_H_

namespace Rosegarden
{

typedef std::vector<std::string> StringList;
typedef std::vector<MidiByte> MidiByteList;
typedef std::vector<ControlParameter> ControlList;

class MidiDevice : public Device
{
public:
    typedef enum
    {
	Play = 0,
	Record = 1
    } DeviceDirection;

    typedef enum
    {
	NoVariations,
	VariationFromLSB,
	VariationFromMSB
    } VariationType;

    MidiDevice();
    MidiDevice(DeviceId id,
               const std::string &name,
               DeviceDirection dir);
    MidiDevice(DeviceId id,
               const std::string &name,
	       const std::string &label,
               DeviceDirection dir);
    virtual ~MidiDevice();

    // Copy constructor
    MidiDevice(const MidiDevice &);

    // Assignment
    MidiDevice &operator=(const MidiDevice &);

    // Instrument must be on heap; I take ownership of it
    virtual void addInstrument(Instrument*);

    void removeMetronome();
    void setMetronome(const MidiMetronome &);
    const MidiMetronome* getMetronome() const { return m_metronome; }

    void addProgram(const MidiProgram &program);
    void addBank(const MidiBank &bank);

    void clearBankList();
    void clearProgramList();
    void clearControlList();

    const BankList &getBanks() const { return m_bankList; }
    BankList getBanks(bool percussion) const;
    BankList getBanksByMSB(bool percussion, MidiByte msb) const; 
    BankList getBanksByLSB(bool percussion, MidiByte lsb) const;

    MidiByteList getDistinctMSBs(bool percussion, int lsb = -1) const;
    MidiByteList getDistinctLSBs(bool percussion, int msb = -1) const;

    const ProgramList &getPrograms() const { return m_programList; }
    ProgramList getPrograms(const MidiBank &bank) const;

    std::string getBankName(const MidiBank &bank) const;
    std::string getProgramName(const MidiProgram &program) const;

    void replaceBankList(const BankList &bank);
    void replaceProgramList(const ProgramList &program);

    void mergeBankList(const BankList &bank);
    void mergeProgramList(const ProgramList &program);

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

    const ControlList &getControlParameters() const { return m_controlList; }

    // Access ControlParameters (read/write)
    //
    ControlParameter *getControlParameter(int index);
    ControlParameter *getControlParameter(Rosegarden::MidiByte controllerNumber);

    // Modify ControlParameters
    //
    void addControlParameter(const ControlParameter &con);
    void addControlParameter(const ControlParameter &con, int index);
    bool removeControlParameter(int index);
    bool modifyControlParameter(const ControlParameter &con, int index);

    void replaceControlParameters(const ControlList &);

    // Check to see if the passed ControlParameter is unique in
    // our ControlParameter list.
    //
    bool isUniqueControlParameter(const ControlParameter &con) const;

    virtual std::string toXmlString();

protected:
    void generatePresentationList();
    void generateDefaultMetronome();

    ProgramList    m_programList;
    BankList       m_bankList;
    ControlList    m_controlList;
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
};

}

#endif // _MIDIDEVICE_H_
