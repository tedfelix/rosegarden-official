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

#define RG_MODULE_STRING "[Instrument]"

#include "Instrument.h"
#include "InstrumentStaticSignals.h"
#include "sound/Midi.h"
#include "MidiDevice.h"
#include "base/AllocateChannels.h"
#include "base/AudioPluginInstance.h"
#include "base/AudioLevel.h"
#include "gui/studio/StudioControl.h"
#include "sound/ControlBlock.h"

#include <cassert>

#include <algorithm>  // std::find()
#include <sstream>
#include <cstdio>
#include <iostream>

#include <QString>
#include <QCoreApplication>
#include <QtGlobal>

namespace Rosegarden
{

const unsigned int PluginContainer::PLUGIN_COUNT = 5;

PluginContainer::PluginContainer(bool havePlugins)
{
    if (havePlugins) {
        // Add a number of plugin place holders (unassigned)
        for (unsigned int i = 0; i < PLUGIN_COUNT; i++)
            addPlugin(new AudioPluginInstance(i));
    }
}

PluginContainer::~PluginContainer()
{
    clearPlugins();
}

void
PluginContainer::addPlugin(AudioPluginInstance *instance)
{
    m_audioPlugins.push_back(instance);
}

bool
PluginContainer::removePlugin(unsigned int position)
{
    PluginInstanceIterator it = m_audioPlugins.begin();

    for (; it != m_audioPlugins.end(); ++it)
    {
        if ((*it)->getPosition() == position)
        {
            delete (*it);
            m_audioPlugins.erase(it);
            return true;
        }

    }

    return false;
}

void
PluginContainer::clearPlugins()
{
    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); ++it)
        delete (*it);

    m_audioPlugins.erase(m_audioPlugins.begin(), m_audioPlugins.end());
}

void 
PluginContainer::emptyPlugins()
{
    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); ++it)
    {
        (*it)->setAssigned(false);
        (*it)->setBypass(false);
        (*it)->clearPorts();
    }
}


// Get an instance for an index
//
AudioPluginInstance*
PluginContainer::getPlugin(unsigned int position)
{
    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); ++it)
    {
        if ((*it)->getPosition() == position)
            return *it;
    }

    return 0;
}


const unsigned int Instrument::SYNTH_PLUGIN_POSITION = 999;


Instrument::Instrument(InstrumentId id,
                       InstrumentType it,
                       const std::string &name,
                       Device *device) :
    PluginContainer(it == Audio || it == SoftSynth),
    m_id(id),
    m_name(name),
    m_alias(""),
    m_type(it),
    m_channel(0),
    //m_input_channel(-1),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_volume(100),
    m_fixed(false),
    m_level(0.0),
    m_recordLevel(0.0),
    m_device(device),
    m_sendBankSelect(false),
    m_sendProgramChange(false),
    m_sendPan(false),
    m_sendVolume(false),
    m_mappedId(0),
    m_audioInput(1000),
    m_audioInputChannel(0),
    m_audioOutput(0)
{
    Q_ASSERT(m_id >= AudioInstrumentBase);//!DEVPUSH

    if (it == Audio || it == SoftSynth)
    {
        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 2 (stereo).
        //
        m_channel = 2;

        m_pan = 100; // audio pan ranges from -100 to 100 but
                     // we store within an unsigned char as 
                     // 0 to 200. 
    }

    if (it == SoftSynth) {
        addPlugin(new AudioPluginInstance(SYNTH_PLUGIN_POSITION));
    }
}

Instrument::Instrument(InstrumentId id,
                       InstrumentType it,
                       const std::string &name,
                       MidiByte channel,
                       Device *device):
    PluginContainer(it == Audio || it == SoftSynth),
    m_id(id),
    m_name(name),
    m_alias(""),
    m_type(it),
    m_channel(channel),
    //m_input_channel(-1),
    m_transpose(MidiMidValue),
    m_pan(MidiMidValue),
    m_volume(100),
    m_fixed(false), // However, this ctor isn't used for MIDI.
    m_level(0.0),
    m_recordLevel(0.0),
    m_device(device),
    m_sendBankSelect(false),
    m_sendProgramChange(false),
    m_sendPan(false),
    m_sendVolume(false),
    m_mappedId(0),
    m_audioInput(1000),
    m_audioInputChannel(0),
    m_audioOutput(0)
{
    Q_ASSERT(m_id >= AudioInstrumentBase);//!DEVPUSH

    // Add a number of plugin place holders (unassigned)
    //
    if (it == Audio || it == SoftSynth)
    {
        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 2 (stereo).
        //
        m_channel = 2;

        m_pan = 100; // audio pan ranges from -100 to 100 but
                     // we store within an unsigned char as 

    } else {
/*
 *
 * Let's try getting rid of this default behavior, and replacing it with a
 * change to the factory autoload instead, because this just doesn't work out
 * very well, and it's fiddly trying to sort the overall behavior into something
 * less quirky (dmm)
 *
        // Also defined in Midi.h but we don't use that - not here
        // in the clean inner sanctum.
        //
        const MidiByte MIDI_PERCUSSION_CHANNEL = 9;
        const MidiByte MIDI_EXTENDED_PERCUSSION_CHANNEL = 10;

        if (m_channel == MIDI_PERCUSSION_CHANNEL ||
            m_channel == MIDI_EXTENDED_PERCUSSION_CHANNEL) {
            setPercussion(true);
        }
*/
    }

    if (it == SoftSynth) {
        addPlugin(new AudioPluginInstance(SYNTH_PLUGIN_POSITION));
    }
}

Instrument::Instrument(const Instrument &ins):
    QObject(),
    XmlExportable(),
    PluginContainer(ins.getType() == Audio || ins.getType() == SoftSynth),
    m_id(ins.getId()),
    m_name(ins.getName()),
    m_alias(ins.getAlias()),
    m_type(ins.getType()),
    m_channel(ins.getNaturalChannel()),
    //m_input_channel(ins.getMidiInputChannel()),
    m_program(ins.getProgram()),
    m_transpose(ins.getMidiTranspose()),
    m_pan(ins.getPan()),
    m_volume(ins.getVolume()),
    m_fixed(ins.m_fixed),
    m_level(ins.getLevel()),
    m_recordLevel(ins.getRecordLevel()),
    m_device(ins.getDevice()),
    m_sendBankSelect(ins.sendsBankSelect()),
    m_sendProgramChange(ins.sendsProgramChange()),
    m_sendPan(ins.sendsPan()),
    m_sendVolume(ins.sendsVolume()),
    m_mappedId(ins.getMappedId()),
    m_audioInput(ins.m_audioInput),
    m_audioInputChannel(ins.m_audioInputChannel),
    m_audioOutput(ins.m_audioOutput)
{
    if (ins.getType() == Audio || ins.getType() == SoftSynth)
    {
        // In an audio instrument we use the m_channel attribute to
        // hold the number of audio channels this Instrument uses -
        // not the MIDI channel number.  Default is 2 (stereo).
        //
        m_channel = 2;
    }

    if (ins.getType() == SoftSynth) {
        addPlugin(new AudioPluginInstance(SYNTH_PLUGIN_POSITION));
    }
    
    StaticControllerConstIterator cIt = ins.m_staticControllers.begin();
    for (; cIt != ins.m_staticControllers.end(); ++cIt) {
        m_staticControllers.push_back(*cIt);
    }
}

#if 0
// ??? Never used.  See comments in header for more.
Instrument &
Instrument::operator=(const Instrument &ins)
{
    if (&ins == this) return *this;

    m_id = ins.getId();
    m_name = ins.getName();
    m_alias = ins.getAlias();
    m_type = ins.getType();
    m_channel = ins.getNaturalChannel();
    //m_input_channel = ins.getMidiInputChannel();
    m_program = ins.getProgram();
    m_transpose = ins.getMidiTranspose();
    m_pan = ins.getPan();
    m_volume = ins.getVolume();
    m_fixed  = false;  // ??? This is not an assignment operator.  It's a partialCopy().
    m_level = ins.getLevel();
    m_recordLevel = ins.getRecordLevel();
    m_device = ins.getDevice();
    m_sendBankSelect = ins.sendsBankSelect();
    m_sendProgramChange = ins.sendsProgramChange();
    m_sendPan = ins.sendsPan();
    m_sendVolume = ins.sendsVolume();
    m_mappedId = ins.getMappedId();
    m_audioInput = ins.m_audioInput;
    m_audioInputChannel = ins.m_audioInputChannel;
    m_audioOutput = ins.m_audioOutput;

    StaticControllerConstIterator cIt = ins.m_staticControllers.begin();
    for (; cIt != ins.m_staticControllers.end(); ++cIt) {
        m_staticControllers.push_back(*cIt);
    }

    return *this;
}
#endif

Instrument::~Instrument()
{
    clearStaticControllers();
}

std::string
Instrument::getPresentationName() const
{
    return m_name;
}

QString
Instrument::getLocalizedPresentationName() const
{
    // This is hacky, but m_name is always set externally, and I don't feel like
    // tracking down all the different places where this could occur.  Instead,
    // we at least limit the spread of hackery to one centralized function to
    // cut the name string apart and translate the relevant part of it.


    // take everything left of the # - 1 to get the "General MIDI Device"
    // out of a string like "General MIDI Device #16"
    QString iname = QString::fromStdString(m_name);
    QString inameL = iname.left(iname.indexOf("#") - 1);
    QString inameR = iname.right(iname.length() - inameL.length());

    // translate the left piece (we'll leave the #1..#n as an untranslatable
    // Rosegarden-specific concept unless people are really bothered by it)
    return QString("%1 %2").arg(QObject::tr(inameL.toLocal8Bit())).arg(inameR);
}

unsigned int
Instrument::getPresentationNumber() const
{
    // Again, m_name is always set externally.  Instruments are numbered
    // sequentially in a way that makes determining the correct channel
    // association tricky, so we pick it back apart from the string to return
    // what channel a given instrument should be set to.
    QString iname = QString::fromStdString(m_name);
    QString number = iname.mid(iname.indexOf("#") + 1, iname.length());
    // for "10[D]" take the left 2 chars:
    if (number.length() > 2) number = number.left(2);
    return number.toUInt();
}

std::string
Instrument::getAlias() const
{
    // return the alias if available, else return the "presentation name" rather
    // than an empty string
    if (m_alias.size()) return m_alias;
    else return m_name;
}

void
Instrument::sendChannelSetup()
{
    // MIDI only
    if (m_type != Midi)
        return;

    if (hasFixedChannel()) {
        StudioControl::sendChannelSetup(this, m_channel);
    }
}

void
Instrument::
setProgram(const MidiProgram &program)
{
    m_program = program;
    emit changedChannelSetup();
    ControlBlock::getInstance()->instrumentChangedProgram(getId());
    if (hasFixedChannel()) {
        StudioControl::sendChannelSetup(this, m_channel);
    }
}

bool
Instrument::isProgramValid() const
{
    MidiDevice *md = dynamic_cast<MidiDevice *>(m_device);
    if (!md)
        return false;

    // Check the bank against the list of valid banks.

    BankList validBanks = md->getBanks(isPercussion());

    bool bankValid =
            (std::find(validBanks.begin(),
                       validBanks.end(),
                       m_program.getBank()) != validBanks.end());

    if (!bankValid)
        return false;

    // Check the program change against the list of program changes
    // for this bank.

    ProgramList programList = md->getPrograms(m_program.getBank());

    bool programChangeValid =
            (std::find(programList.begin(),
                       programList.end(),
                       m_program) != programList.end());

    if (!programChangeValid)
        return false;

    return true;
}

void
Instrument::setProgramChange(MidiByte program)
{
    setProgram(MidiProgram(m_program.getBank(), program));
}

MidiByte
Instrument::getProgramChange() const
{
    return m_program.getProgram();
}

void
Instrument::setMSB(MidiByte msb)
{
    setProgram(MidiProgram(MidiBank(m_program.getBank().isPercussion(),
                                    msb,
                                    m_program.getBank().getLSB()),
                           m_program.getProgram()));
}

MidiByte
Instrument::getMSB() const
{
    return m_program.getBank().getMSB();
}

void
Instrument::setLSB(MidiByte lsb)
{
    setProgram(MidiProgram(MidiBank(m_program.getBank().isPercussion(),
                                    m_program.getBank().getMSB(),
                                    lsb),
                           m_program.getProgram()));
}

MidiByte
Instrument::getLSB() const
{
    return m_program.getBank().getLSB();
}

void
Instrument::pickFirstProgram(bool percussion)
{
    MidiDevice *md = dynamic_cast<MidiDevice *>(m_device);
    if (!md)
        return;

    BankList banks = md->getBanks(percussion);
    if (banks.empty())
        return;

    // Get the programs for the first bank.
    ProgramList programs = md->getPrograms(banks.front());
    if (programs.empty())
        return;

    // setProgram() will notify the rest of the system of this change.
    m_sendBankSelect = true;
    m_sendProgramChange = true;
    setProgram(programs.front());
}

void
Instrument::setPercussion(bool percussion)
{
    setProgram(MidiProgram(MidiBank(percussion,
                                    m_program.getBank().getMSB(),
                                    m_program.getBank().getLSB()),
                           m_program.getProgram()));
}

bool
Instrument::isPercussion() const
{
    return m_program.getBank().isPercussion();
}

void
Instrument::setAudioInputToBuss(BussId buss, int channel)
{
    m_audioInput = buss;
    m_audioInputChannel = channel;
}

void
Instrument::setAudioInputToRecord(int recordIn, int channel)
{
    m_audioInput = recordIn + 1000;
    m_audioInputChannel = channel;
}

int
Instrument::getAudioInput(bool &isBuss, int &channel) const
{
    channel = m_audioInputChannel;

    if (m_audioInput >= 1000) {
        isBuss = false;
        return m_audioInput - 1000;
    } else {
        isBuss = true;
        return m_audioInput;
    }
}


// Implementation of the virtual method to output this class
// as XML.  We don't send out the name as it's redundant in
// the file - that is driven from the sequencer.
//
//
std::string
Instrument::toXmlString()
{

    std::stringstream instrument;

    // We don't send system Instruments out this way -
    // only user Instruments.
    //
    if (m_id < AudioInstrumentBase)
    {
        return instrument.str();
    } 

    instrument << "        <instrument id=\"" << m_id;
    instrument << "\" channel=\"" << (int)m_channel;
    instrument << "\" fixed=\""   << (m_fixed ? "true" : "false");
    instrument << "\" type=\"";

    if (m_type == Midi)
    {
        instrument << "midi\">" << std::endl;

        instrument << "            <bank send=\""
                   << (m_sendBankSelect ? "true" : "false") << "\" percussion=\""
                   << (isPercussion() ? "true" : "false") << "\" msb=\""
                   << (int)getMSB()
                   << "\" lsb=\"" << (int)getLSB() << "\"/>" << std::endl;

        instrument << "            <program id=\""
                   << (int)getProgramChange() << "\" send=\""
                   << (m_sendProgramChange ? "true" : "false") << "\"/>"
                   << std::endl;
    
        instrument << "            <pan value=\""
                   << (int)m_pan << "\"/>" << std::endl;

        instrument << "            <volume value=\""
                   << (int)m_volume << "\"/>" << std::endl;

        for (StaticControllerConstIterator it = m_staticControllers.begin();
             it != m_staticControllers.end(); ++it)
        {
            instrument << "            <controlchange type=\"" << int(it->first)
                       << "\" value=\"" << int(it->second) << "\"/>" << std::endl;
        }

    }
    else // Audio or SoftSynth
    {

        if (m_type == Audio) {
            instrument << "audio\">" << std::endl;
        } else {
            instrument << "softsynth\">" << std::endl;
        }

        instrument << "            <pan value=\""
                   << (int)m_pan << "\"/>" << std::endl;

        instrument << "            <level value=\""
                   << m_level << "\"/>" << std::endl;

        instrument << "            <recordLevel value=\""
                   << m_recordLevel << "\"/>" << std::endl;

        bool aibuss;
        int channel;
        int ai = getAudioInput(aibuss, channel);

        instrument << "            <audioInput value=\""
                   << ai << "\" type=\""
                   << (aibuss ? "buss" : "record")
                   << "\" channel=\"" << channel
                   << "\"/>" << std::endl;

        instrument << "            <audioOutput value=\""
                   << m_audioOutput << "\"/>" << std::endl;

        instrument << "            <alias value=\""
                   << m_alias << "\"/>" << std::endl;

        PluginInstanceIterator it = m_audioPlugins.begin();
        for (; it != m_audioPlugins.end(); ++it)
        {
            instrument << (*it)->toXmlString();
        }
    }
        
    instrument << "        </instrument>" << std::endl
               << std::endl;

    return instrument.str();

}


// Return a program name given a bank select (and whether
// we send it or not)
//
std::string
Instrument::getProgramName() const
{
    if (m_sendProgramChange == false)
        return std::string("");

    MidiProgram program(m_program);

    if (!m_sendBankSelect)
        program = MidiProgram(MidiBank(isPercussion(), 0, 0), program.getProgram());

    return ((dynamic_cast<MidiDevice*>(m_device))->getProgramName(program));
}

void
Instrument::setControllerValue(MidiByte controller, MidiByte value)
{
    // two special cases
    if (controller == MIDI_CONTROLLER_PAN) {
        setPan(value);
    } else if (controller == MIDI_CONTROLLER_VOLUME) {
        setVolume(value);
    }

    for (StaticControllerIterator it = m_staticControllers.begin();
         it != m_staticControllers.end(); ++it)
    {
        if (it->first == controller)
        {
            it->second = value;
            emit changedChannelSetup();
            if (hasFixedChannel()) {
                StudioControl::sendController(this, m_channel,
                                              controller, value);
            }
            return;
        }
    }

    m_staticControllers.push_back(std::pair<MidiByte, MidiByte>(controller, value));

    emit changedChannelSetup();
}

MidiByte
Instrument::getControllerValue(MidiByte controller) const
{
    for (StaticControllerConstIterator it = m_staticControllers.begin();
         it != m_staticControllers.end(); ++it)
    {
        if (it->first == controller) {
            return it->second;
        }
    }

    throw std::string("<no controller of that value>");
}

void
Instrument::removeStaticController(MidiByte controller)
{
    for (StaticControllerIterator it = m_staticControllers.begin();
         it != m_staticControllers.end(); ++it)
    {
        if (it->first == controller) {
            m_staticControllers.erase(it);
            return;
        }
    }
}

const MidiKeyMapping *
Instrument::getKeyMapping() const
{
    MidiDevice *md = dynamic_cast<MidiDevice*>(m_device);
    if (!md) return 0;

    const MidiKeyMapping *mkm = md->getKeyMappingForProgram(m_program);
    if (mkm) return mkm;

    if (isPercussion()) { // if any key mapping is available, use it
        const KeyMappingList &kml = md->getKeyMappings();
        if (kml.begin() != kml.end()) {
            return &(*kml.begin());
        }
    }

    return 0;
}    

// Set a fixed channel.  For MIDI instruments, conform allocator
// accordingly. 
void
Instrument::
setFixedChannel(void)
{
    if (m_fixed) { return; }

    AllocateChannels *allocator = getDevice()->getAllocator();
    if (allocator) {
        allocator->reserveFixedChannel(m_channel);
        m_fixed = true;
        emit channelBecomesFixed();
        StudioControl::sendChannelSetup(this, m_channel);
        ControlBlock::getInstance()->instrumentChangedFixity(getId());
    }
}

// Release this instrument's fixed channel, if any.
// @author Tom Breton (Tehom) 
void
Instrument::
releaseFixedChannel(void)
{
    if (!m_fixed) { return; }
    
    AllocateChannels *allocator = getDevice()->getAllocator();
    if (allocator) {
        allocator->releaseFixedChannel(m_channel);
    }

    m_fixed = false;
    emit channelBecomesUnfixed();
    ControlBlock::getInstance()->instrumentChangedFixity(getId());
}

QSharedPointer<InstrumentStaticSignals>
Instrument::getStaticSignals()
{
    // Keep the static instance here so there is no way to get
    // to it other than to call getStaticSignals().
    static QSharedPointer<InstrumentStaticSignals> instrumentStaticSignals;

    // Delayed creation to avoid static construction order fiasco.
    if (!instrumentStaticSignals)
        instrumentStaticSignals =
                QSharedPointer<InstrumentStaticSignals>(new InstrumentStaticSignals);

    // If you want to avoid the complementary static destruction order
    // fiasco, hold on to a copy of this in a member QSharedPointer.  This
    // is only really an issue if your object also has static lifespan.
    // Still, it's a good habit.

    return instrumentStaticSignals;
}

void
Instrument::changed()
{
    emit getStaticSignals()->changed(this);
}


/***** Buss *****/

Buss::Buss(BussId id) :
    PluginContainer(true),
    m_id(id),
    m_level(0.0),
    m_pan(100),
    m_mappedId(0)
{
}

Buss::~Buss()
{
}

std::string
Buss::toXmlString()
{
    std::stringstream buss;

    buss << "    <buss id=\"" << m_id << "\">" << std::endl;
    buss << "       <pan value=\"" << (int)m_pan << "\"/>" << std::endl;
    buss << "       <level value=\"" << m_level << "\"/>" << std::endl;

    PluginInstanceIterator it = m_audioPlugins.begin();
    for (; it != m_audioPlugins.end(); ++it) {
        buss << (*it)->toXmlString();
    }

    buss << "    </buss>" << std::endl;

    return buss.str();
}

std::string
Buss::getName() const
{
    char buffer[20];
    sprintf(buffer, "Submaster %d", m_id);
    return buffer;
}

std::string
Buss::getPresentationName() const
{
    return getName();
}

std::string
Buss::getAlias() const
{
    return getName();
}

/***** RecordIn *****/

RecordIn::RecordIn() :
    m_mappedId(0)
{
}

RecordIn::~RecordIn()
{
}

std::string
RecordIn::toXmlString()
{
    // We don't actually save these, as they have nothing persistent
    // in them.  The studio just remembers how many there should be.
    return "";
}


}

#include "Instrument.moc"
