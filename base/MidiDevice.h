// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
    MidiDevice();
    MidiDevice(DeviceId id, const std::string &name);
    virtual ~MidiDevice();

    virtual void createInstruments();
    virtual void addInstrument(Instrument*);

    void removeMetronome();
    void setMetronome(InstrumentId instrument,
                      MidiByte msb, MidiByte lsb, MidiByte program,
                      MidiByte pitch,
                      const std::string &name);
    MidiMetronome* getMetronome() const { return m_metronome; }

    // Get a program list for a certain bank
    //
    StringList getProgramList(MidiByte msb, MidiByte lsb);
    std::string getProgramName(MidiByte msb, MidiByte lsb, MidiByte program);

    // Get a list of all banks
    //
    StringList getBankList();

    // Add either
    //
    void addProgram(MidiProgram *program);
    void addBank(MidiBank *bank);

    // Clear down both banks and programs
    //
    void clearBankList();
    void clearProgramList();

    // Retreive some stuff
    //
    MidiBank* getBankByIndex(int index);
    MidiProgram* getProgramByIndex(int index);

    virtual std::string toXmlString();

    virtual InstrumentList& getAllInstruments();
    virtual InstrumentList& getPresentationInstruments();

    // Return a copy of banks and programs
    //
    std::vector<MidiBank> getBanks() const;
    std::vector<MidiProgram> getPrograms() const;

    void replaceBankList(const std::vector<Rosegarden::MidiBank> &bank);
    void replaceProgramList(const std::vector<Rosegarden::MidiProgram> &program);

private:
    void generatePresentationList();

    // Brief (probably incorrect) synopsis of bank select 
    // messages for XG:
    //
    // MSB = 0 for Normal Voices
    // MSB = 64 for SFX
    // MSG = 126 for SFX Drum
    // MSG = 127 for Drum kits
    //
    // and then fiddle with LSB for individual banks
    //
    // For Soundblaster use MSG = 0 and then LSB to
    // required bank.
    //
    // We set all bank select messages in the Studio section
    // of the rosegarden file and at the gui map names only.
    //
    //

    // We store voice names that depend on bank select state
    // - we can create our own and save them out
    //
    ProgramList   *m_programList;
    BankList      *m_bankList;
    MidiMetronome *m_metronome;

    // used when we're presenting the instruments
    InstrumentList m_presentationInstrumentList;

};

}

#endif // _MIDIDEVICE_H_
