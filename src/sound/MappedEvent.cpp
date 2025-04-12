/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MappedEvent]"

#include "MappedEvent.h"
#include "base/BaseProperties.h"
#include "Midi.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h" // for Note::EventType
#include "misc/Debug.h"
#include "misc/TempDir.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtGlobal>

#include <cstdlib>

// #define DEBUG_MAPPEDEVENT 1

namespace Rosegarden
{


MappedEvent::MappedEvent(const Event &e)
{
    try {

        // For each event type, we set the properties in a particular
        // order: first the type, then whichever of data1 and data2 fits
        // less well with its default value.  This way if one throws an
        // exception for no data, we still have a good event with the
        // defaults set.

        if (e.isa(Note::EventType)) {
            m_type = MidiNote;
            long v = MidiMaxValue;
            e.get<Int>(BaseProperties::VELOCITY, v);
            m_data2 = v;
            m_data1 = e.get<Int>(BaseProperties::PITCH);
        } else if (e.isa(PitchBend::EventType)) {
            m_type = MidiPitchBend;
            m_data1 = e.get<Int>(PitchBend::MSB);
            m_data2 = e.get<Int>(PitchBend::LSB);
        } else if (e.isa(Controller::EventType)) {
            m_type = MidiController;
            m_data1 = e.get<Int>(Controller::NUMBER);
            m_data2 = e.get<Int>(Controller::VALUE);
        } else if (e.isa(RPN::EventType)) {
            m_type = MidiRPN;
            m_number = e.get<Int>(RPN::NUMBER);
            m_value = e.get<Int>(RPN::VALUE);
        } else if (e.isa(NRPN::EventType)) {
            m_type = MidiNRPN;
            m_number = e.get<Int>(NRPN::NUMBER);
            m_value = e.get<Int>(NRPN::VALUE);
        } else if (e.isa(ProgramChange::EventType)) {
            m_type = MidiProgramChange;
            m_data1 = e.get<Int>(ProgramChange::PROGRAM);
        } else if (e.isa(KeyPressure::EventType)) {
            m_type = MidiKeyPressure;
            m_data1 = e.get<Int>(KeyPressure::PITCH);
            m_data2 = e.get<Int>(KeyPressure::PRESSURE);
        } else if (e.isa(ChannelPressure::EventType)) {
            m_type = MidiChannelPressure;
            m_data1 = e.get<Int>(ChannelPressure::PRESSURE);
        } else if (e.isa(SystemExclusive::EventType)) {
            m_type = MidiSystemMessage;
            m_data1 = MIDI_SYSTEM_EXCLUSIVE;
            std::string dataBlock;
            e.get<String>(SystemExclusive::DATABLOCK, dataBlock);
            dataBlock = SystemExclusive::toRaw(dataBlock);
            DataBlockRepository::getInstance()->registerDataBlockForEvent(dataBlock, this);
        } else if (e.isa(Text::EventType)) {
            const Rosegarden::Text text(e);

            // Somewhat hacky: We know that annotations and LilyPond directives
            // aren't to be output, so we make their MappedEvents invalid.
            // InternalSegmentMapper will then discard those.
            if (text.getTextType() == Text::Annotation || text.getTextType() == Text::LilyPondDirective) {
                m_type = InvalidMappedEvent;
            } else {
                m_type = MappedEvent::Text;

                MidiByte midiTextType =
                    (text.getTextType() == Text::Lyric) ?
                    MIDI_LYRIC :
                    MIDI_TEXT_EVENT;
                setData1(midiTextType);

                std::string metaMessage = text.getText();
                setDataBlock(metaMessage);
            }
        } else if (e.isa(Key::EventType)) {
            const Rosegarden::Key key(e);
            m_type = KeySignature;
            int accidentals = key.getAccidentalCount();
            if (!key.isSharp()) {
                accidentals = -accidentals;
            }
            m_data1 = accidentals;
            m_data2 = key.isMinor();
        } else {
            m_type = InvalidMappedEvent;
        }

    } catch (const Event::NoData &d) {

#ifdef DEBUG_MAPPEDEVENT
        RG_WARNING << "Caught Event::NoData in MappedEvent ctor, message is:\n" << d.getMessage();
#endif

    } catch (const Event::BadType &b) {

#ifdef DEBUG_MAPPEDEVENT
        RG_WARNING << "Caught Event::BadType in MappedEvent ctor, message is:\n" << b.getMessage();
#endif

    } catch (const SystemExclusive::BadEncoding &e) {

#ifdef DEBUG_MAPPEDEVENT
        RG_WARNING << "Caught bad SysEx encoding in MappedEvent ctor";
#endif

    }
}

bool
operator<(const MappedEvent &a, const MappedEvent &b)
{
    return a.getEventTime() < b.getEventTime();
}

void
MappedEvent::setDataBlock(const std::string& rawData)
{
    DataBlockRepository::getInstance()->
        setDataBlockForEvent(this, rawData, true);
}

QDebug operator<<(QDebug dbg, const MappedEvent &mE)
{
    dbg << "MappedEvent" << "\n";

    dbg << "  Track ID:";

    if (mE.m_trackId != NoTrack)
        dbg << static_cast<int>(mE.m_trackId);
    else
        dbg << "NO_TRACK";

    dbg << "\n";

    dbg << "  Instrument ID:" << mE.m_instrument << "\n";

    QString type;

    // ToString(MappedEventType)?
    switch(mE.m_type)
    {
    case MappedEvent::InvalidMappedEvent:
        type = "InvalidMappedEvent";
        break;
    case MappedEvent::MidiNote:
        type = "MidiNote";
        break;
    case MappedEvent::MidiNoteOneShot:
        type = "MidiNoteOneShot";
        break;
    case MappedEvent::MidiProgramChange:
        type = "MidiProgramChange";
        break;
    case MappedEvent::MidiKeyPressure:
        type = "MidiKeyPressure";
        break;
    case MappedEvent::MidiChannelPressure:
        type = "MidiChannelPressure";
        break;
    case MappedEvent::MidiPitchBend:
        type = "MidiPitchBend";
        break;
    case MappedEvent::MidiController:
        type = "MidiController";
        break;
    case MappedEvent::MidiSystemMessage:
        type = "MidiSystemMessage";
        break;
    case MappedEvent::MidiRPN:
        type = "RPN";
        break;
    case MappedEvent::MidiNRPN:
        type = "NRPN";
        break;
    case MappedEvent::Audio:
        type = "Audio";
        break;
    case MappedEvent::AudioCancel:
        type = "AudioCancel";
        break;
    case MappedEvent::AudioLevel:
        type = "AudioLevel";
        break;
    case MappedEvent::AudioStopped:
        type = "AudioStopped";
        break;
    case MappedEvent::AudioGeneratePreview:
        type = "AudioGeneratePreview";
        break;
    case MappedEvent::SystemUpdateInstruments:
        type = "SystemUpdateInstruments";
        break;
    case MappedEvent::SystemJackTransport:
        type = "SystemJackTransport";
        break;
    case MappedEvent::SystemMMCTransport:
        type = "SystemMMCTransport";
        break;
    case MappedEvent::SystemMIDIClock:
        type = "SystemMIDIClock";
        break;
    case MappedEvent::SystemMetronomeDevice:
        type = "SystemMetronomeDevice";
        break;
    case MappedEvent::SystemAudioPortCounts:
        type = "SystemAudioPortCounts";
        break;
    case MappedEvent::SystemAudioPorts:
        type = "SystemAudioPorts";
        break;
    case MappedEvent::SystemFailure:
        type = "SystemFailure";
        break;
    case MappedEvent::TimeSignature:
        type = "TimeSignature";
        break;
    case MappedEvent::Tempo:
        type = "Tempo";
        break;
    case MappedEvent::Panic:
        type = "Panic";
        break;
    case MappedEvent::SystemMTCTransport:
        type = "SystemMTCTransport";
        break;
    case MappedEvent::SystemMIDISyncAuto:
        type = "SystemMIDISyncAuto";
        break;
    case MappedEvent::SystemAudioFileFormat:
        type = "SystemAudioFileFormat";
        break;
    case MappedEvent::Marker:
        type = "Marker";
        break;
    case MappedEvent::Text:
        type = "Text";
        break;
    case MappedEvent::KeySignature:
        type = "KeySignature";
        break;
    default:
        // ??? This is a bitmask, so this might happen with perfectly
        //     legitimate values.
        type = "*** Unexpected";
        break;
    }

    dbg << "  Type:" << type << "\n";

    dbg << "  Data 1:" << mE.m_data1 << "\n";
    dbg << "  Data 2:" << mE.m_data2 << "\n";
    dbg << "  Event Time:" << mE.m_eventTime << "\n";
    dbg << "  Duration:" << mE.m_duration << "\n";
    dbg << "  Audio Start Marker:" << mE.m_audioStartMarker << "\n";
    dbg << "  Runtime Segment ID:" << mE.m_runtimeSegmentId << "\n";
    dbg << "  Auto Fade:" << mE.m_autoFade << "\n";
    dbg << "  Fade In Time:" << mE.m_fadeInTime << "\n";
    dbg << "  Fade Out Time:" << mE.m_fadeOutTime << "\n";
    dbg << "  Recorded Channel:" << mE.m_recordedChannel << "\n";
    dbg << "  Recorded Device:" << mE.m_recordedDevice << "\n";
    dbg << "  Number:" << mE.m_number << "\n";
    dbg << "  Value:" << mE.m_value << "\n";

    return dbg;
}

//--------------------------------------------------

/// Actual data storage for DataBlockRepository.
/**
 * ??? Files?  Really?  Wouldn't memory make more sense and be a lot faster?
 */
class DataBlockFile
{
public:
    explicit DataBlockFile(DataBlockRepository::blockid id);
    ~DataBlockFile();

    QString getFileName() const
    {
        return m_fileName;
    }

    void addDataString(const std::string&);

    void clear()
    {
        m_cleared = true;
    }
    bool exists();
    void setData(const std::string&);
    std::string getData();

protected:
    void prepareToWrite();
    void prepareToRead();

    //--------------- Data members ---------------------------------
    QString m_fileName;
    QFile m_file;
    bool m_cleared;
};

DataBlockFile::DataBlockFile(DataBlockRepository::blockid id)
    : m_fileName(TempDir::path() + QString("/rosegarden_datablock_%1").arg(id)),
      m_file(m_fileName),
      m_cleared(false)
{
    //     std::cerr << "DataBlockFile " << m_fileName.toLatin1().data() << std::endl;
}

DataBlockFile::~DataBlockFile()
{
    if (m_cleared) {
//        std::cerr << "~DataBlockFile : removing " << m_fileName.toLatin1().data() << std::endl;
        QFile::remove
            (m_fileName);
    }

}

bool DataBlockFile::exists()
{
    return QFile::exists(m_fileName);
}

void DataBlockFile::setData(const std::string& s)
{
  //  std::cerr << "DataBlockFile::setData() : setting data to " << m_fileName << std::endl;
    prepareToWrite();

    QDataStream stream(&m_file);
    stream.writeRawData(s.data(), s.length());
}

std::string DataBlockFile::getData()
{
    if (!exists())
        return std::string();

    prepareToRead();

    QDataStream stream(&m_file);
 //   std::cerr << "DataBlockFile::getData() : file size = " << m_file.size() << std::endl;
    char* tmp = new char[m_file.size()];
    stream.readRawData(tmp, m_file.size());
    std::string res(tmp, m_file.size());
    delete[] tmp;

    return res;
}

void DataBlockFile::addDataString(const std::string& s)
{
    prepareToWrite();
    QDataStream stream(&m_file);
    stream.writeRawData(s.data(), s.length());
}

void DataBlockFile::prepareToWrite()
{
 //   std::cerr << "DataBlockFile[" << m_fileName << "]: prepareToWrite" << std::endl;
    if (!m_file.isWritable()) {
        m_file.close();
        m_file.open(QIODevice::WriteOnly | QIODevice::Append);
        Q_ASSERT(m_file.isWritable());
    }
}

void DataBlockFile::prepareToRead()
{
//    std::cerr << "DataBlockFile[" << m_fileName << "]: prepareToRead" << std::endl;
    if (!m_file.isReadable()) {
        m_file.close();
        m_file.open(QIODevice::ReadOnly);
        Q_ASSERT(m_file.isReadable());
    }
}



//--------------------------------------------------

DataBlockRepository* DataBlockRepository::getInstance()
{
    if (!m_instance)
        m_instance = new DataBlockRepository;
    return m_instance;
}

std::string DataBlockRepository::getDataBlock(DataBlockRepository::blockid id)
{
    DataBlockFile dataBlockFile(id);

    if (dataBlockFile.exists())
        return dataBlockFile.getData();

    return std::string();
}


std::string DataBlockRepository::getDataBlockForEvent(const MappedEvent* e)
{
    blockid id = e->getDataBlockId();
    if (id == 0) {
   //     std::cerr << "WARNING: DataBlockRepository::getDataBlockForEvent called on event with data block id 0" << std::endl;
        return "";
    }
    return getInstance()->getDataBlock(id);
}

void DataBlockRepository::setDataBlockForEvent(MappedEvent* e,
                                               const std::string& s,
                                               bool extend)
{
    blockid id = e->getDataBlockId();
    if (id == 0) {
#ifdef DEBUG_MAPPEDEVENT
        RG_DEBUG << "Creating new datablock for event";
#endif
        getInstance()->registerDataBlockForEvent(s, e);
    } else {
#ifdef DEBUG_MAPPEDEVENT
        RG_DEBUG << "Writing" << s.length()
                  << "chars to file for datablock" << id;
#endif
        DataBlockFile dataBlockFile(id);
        if (extend)
            { dataBlockFile.addDataString(s);}
        else
            { dataBlockFile.setData(s); }
    }
}

/* unused
bool DataBlockRepository::hasDataBlock(DataBlockRepository::blockid id)
{
    return DataBlockFile(id).exists();
}
*/

DataBlockRepository::blockid DataBlockRepository::registerDataBlock(const std::string& s)
{
    blockid id = 0;
    while (id == 0 || DataBlockFile(id).exists())
        id = (blockid)random();

 //   std::cerr << "DataBlockRepository::registerDataBlock: " << s.length() << " chars, id is " << id << std::endl;

    DataBlockFile dataBlockFile(id);
    dataBlockFile.setData(s);

    return id;
}

/* unused
void DataBlockRepository::unregisterDataBlock(DataBlockRepository::blockid id)
{
    DataBlockFile dataBlockFile(id);

    dataBlockFile.clear();
}
*/

void DataBlockRepository::registerDataBlockForEvent(const std::string& s, MappedEvent* e)
{
    e->setDataBlockId(registerDataBlock(s));
}

/* unused
void DataBlockRepository::unregisterDataBlockForEvent(MappedEvent* e)
{
    unregisterDataBlock(e->getDataBlockId());
}
*/

DataBlockRepository::DataBlockRepository()
{}

void DataBlockRepository::clear()
{
#ifdef DEBUG_MAPPEDEVENT
    RG_DEBUG << "DataBlockRepository::clear()";
#endif

    // Erase all 'datablock_*' files
    //
    QString tmpPath = TempDir::path();

    QDir segmentsDir(tmpPath, "rosegarden_datablock_*");

    if (segmentsDir.count() > 2000) {
        RG_DEBUG << "DataBlockRepository::clear(): A rather large number of rosegarden_datablock_*\n" <<
                     "  files (" << segmentsDir.count() << " of them) have been found in " << tmpPath.toStdString() << ".\n" <<
                     "  It may take a while to delete them all.  Working...";
    }

    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        QString segmentName = tmpPath + '/' + segmentsDir[i];
        QFile::remove
            (segmentName);
    }
}

// setDataBlockForEvent does what addDataStringForEvent used to do.


DataBlockRepository* DataBlockRepository::m_instance = nullptr;


}
