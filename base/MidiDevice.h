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


#ifndef _MIDIDEVICE_H_
#define _MIDIDEVICE_H_

namespace Rosegarden
{

typedef std::vector<std::string> StringList;
typedef std::vector<MidiProgram*> ProgramList;
typedef std::vector<MidiBank*> BankList;

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

    virtual void addInstrument(Instrument*);

    void removeMetronome();
    void setMetronome(InstrumentId instrument,
		      const MidiMetronome &metronome);
    const MidiMetronome* getMetronome() const { return m_metronome; }

    // Get a program list for a certain bank (disregarding bank name)
    //
    StringList getProgramList(const MidiBank &bank);
    std::string getProgramName(const MidiProgram &program);

    // Get a list of all banks
    //
    //!!! lose in favour of returning bank object ptrs
    StringList getBankList();

    // Add either
    //
    void addProgram(MidiProgram *program);
    void addBank(MidiBank *bank);

    // Clear down both banks and programs
    //
    void clearBankList();
    void clearProgramList();

    // Retrieve by different criteria
    //
    //!!! lose
    const MidiBank* getBankByIndex(int index) const;

    //!!! lose -- what we really need is getBankName or something
    const MidiBank* getBankByMsbLsb(bool percussion, MidiByte msb, MidiByte lsb) const;

    std::vector<const MidiBank *> getBanks(bool percussion) const;
    std::vector<const MidiBank *> getBanksByMSB(bool percussion, MidiByte msb) const;
    std::vector<const MidiBank *> getBanksByLSB(bool percussion, MidiByte lsb) const;
    std::vector<MidiByte> getDistinctMSBs(bool percussion, int lsb = -1) const;
    std::vector<MidiByte> getDistinctLSBs(bool percussion, int msb = -1) const;
    std::string getBankName(const MidiBank &bank) const;

    const MidiProgram* getProgramByIndex(int index) const;
    const MidiProgram* getProgram(const MidiBank &bank, int index) const;

    virtual std::string toXmlString();

    virtual InstrumentList getAllInstruments() const;
    virtual InstrumentList getPresentationInstruments() const;

    // Return a copy of banks and programs
    //
    std::vector<MidiBank> getBanks() const;
    std::vector<MidiProgram> getPrograms() const;

    void replaceBankList(const std::vector<Rosegarden::MidiBank> &bank);
    void replaceProgramList(const std::vector<Rosegarden::MidiProgram> &program);

    void mergeBankList(const std::vector<Rosegarden::MidiBank> &bank);
    void mergeProgramList(const std::vector<Rosegarden::MidiProgram> &program);

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

protected:
    void generatePresentationList();

    ProgramList   *m_programList;
    BankList      *m_bankList;
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
