// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

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

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>

#include <kstddirs.h>

#include "MappedEvent.h"
#include "BaseProperties.h"
#include "MidiTypes.h"
 
namespace Rosegarden
{

MappedEvent::MappedEvent(InstrumentId id,
                         const Event &e,
                         const RealTime &eventTime,
                         const RealTime &duration):
    m_trackId(0),
    m_instrument(id),
    m_type(MidiNote),
    m_data1(0),
    m_data2(0),
    m_eventTime(eventTime),
    m_duration(duration),
    m_audioStartMarker(0, 0),
    m_dataBlockId(0),
    m_isPersistent(false)
{
    try {

	// For each event type, we set the properties in a particular
	// order: first the type, then whichever of data1 and data2 fits
	// less well with its default value.  This way if one throws an
	// exception for no data, we still have a good event with the
	// defaults set.

	if (e.isa(Note::EventType))
            {
                m_type = MidiNote;
                long v = MidiMaxValue;
                e.get<Int>(BaseProperties::VELOCITY, v);
                m_data2 = v;
                m_data1 = e.get<Int>(BaseProperties::PITCH);
            }
	else if (e.isa(PitchBend::EventType))
            {
                m_type = MidiPitchBend;
                PitchBend pb(e);
                m_data1 = pb.getMSB();
                m_data2 = pb.getLSB();
            }
	else if (e.isa(Controller::EventType))
            {
                m_type = MidiController;
                Controller c(e);
                m_data1 = c.getNumber();
                m_data2 = c.getValue();
            }
	else if (e.isa(ProgramChange::EventType))
            {
                m_type = MidiProgramChange;
                ProgramChange pc(e);
                m_data1 = pc.getProgram();
            }
	else if (e.isa(KeyPressure::EventType))
            {
                m_type = MidiKeyPressure;
                KeyPressure kp(e);
                m_data1 = kp.getPitch();
                m_data2 = kp.getPressure();
            }
	else if (e.isa(ChannelPressure::EventType))
            {
                m_type = MidiChannelPressure;
                ChannelPressure cp(e);
                m_data1 = cp.getPressure();
            }
	else if (e.isa(SystemExclusive::EventType))
            {
                m_type = MidiSystemExclusive;
                SystemExclusive s(e);
                std::string dataBlock = s.getRawData();
                DataBlockRepository::getInstance()->registerDataBlockForEvent(dataBlock, this);
            }
	else 
            {
                m_type = InvalidMappedEvent;
            }
    } catch (MIDIValueOutOfRange r) {
	std::cerr << "MIDI value out of range in MappedEvent ctor"
		  << std::endl;
    } catch (Event::NoData d) {
	std::cerr << "Caught Event::NoData in MappedEvent ctor, message is:"
		  << std::endl << d.getMessage() << std::endl;
    } catch (Event::BadType b) {
	std::cerr << "Caught Event::BadType in MappedEvent ctor, message is:"
		  << std::endl << b.getMessage() << std::endl;
    } catch (SystemExclusive::BadEncoding e) {
	std::cerr << "Caught bad SysEx encoding in MappedEvent ctor"
		  << std::endl;
    }
}

bool
operator<(const MappedEvent &a, const MappedEvent &b)
{
    return a.getEventTime() < b.getEventTime();
}

MappedEvent&
MappedEvent::operator=(const MappedEvent &mE)
{
    if (&mE == this) return *this;

    m_trackId = mE.getTrackId();
    m_instrument = mE.getInstrument();
    m_type = mE.getType();
    m_data1 = mE.getData1();
    m_data2 = mE.getData2();
    m_eventTime = mE.getEventTime();
    m_duration = mE.getDuration();
    m_audioStartMarker = mE.getAudioStartMarker();
    m_dataBlockId = mE.getDataBlockId();

    return *this;
}

const size_t MappedEvent::streamedSize = 12 * sizeof(unsigned int);

QDataStream&
operator<<(QDataStream &dS, MappedEvent *mE)
{
    dS << (unsigned int)mE->getTrackId();
    dS << (unsigned int)mE->getInstrument();
    dS << (unsigned int)mE->getType();
    dS << (unsigned int)mE->getData1();
    dS << (unsigned int)mE->getData2();
    dS << (unsigned int)mE->getEventTime().sec;
    dS << (unsigned int)mE->getEventTime().usec;
    dS << (unsigned int)mE->getDuration().sec;
    dS << (unsigned int)mE->getDuration().usec;
    dS << (unsigned int)mE->getAudioStartMarker().sec;
    dS << (unsigned int)mE->getAudioStartMarker().usec;
    dS << (unsigned long)mE->getDataBlockId();

    return dS;
}

QDataStream&
operator<<(QDataStream &dS, const MappedEvent &mE)
{
    dS << (unsigned int)mE.getTrackId();
    dS << (unsigned int)mE.getInstrument();
    dS << (unsigned int)mE.getType();
    dS << (unsigned int)mE.getData1();
    dS << (unsigned int)mE.getData2();
    dS << (unsigned int)mE.getEventTime().sec;
    dS << (unsigned int)mE.getEventTime().usec;
    dS << (unsigned int)mE.getDuration().sec;
    dS << (unsigned int)mE.getDuration().usec;
    dS << (unsigned int)mE.getAudioStartMarker().sec;
    dS << (unsigned int)mE.getAudioStartMarker().usec;
    dS << (unsigned long)mE.getDataBlockId();

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent *mE)
{
    unsigned int trackId = 0, instrument = 0, type = 0, data1 = 0, data2 = 0;
    long eventTimeSec = 0, eventTimeUsec = 0, durationSec = 0, durationUsec = 0,
        audioSec = 0, audioUsec = 0;
    std::string dataBlock;
    unsigned long dataBlockId = 0;

    dS >> trackId;
    dS >> instrument;
    dS >> type;
    dS >> data1;
    dS >> data2;
    dS >> eventTimeSec;
    dS >> eventTimeUsec;
    dS >> durationSec;
    dS >> durationUsec;
    dS >> audioSec;
    dS >> audioUsec;
    dS >> dataBlockId;

    mE->setTrackId((TrackId)trackId);
    mE->setInstrument((InstrumentId)instrument);
    mE->setType((MappedEvent::MappedEventType)type);
    mE->setData1((MidiByte)data1);
    mE->setData2((MidiByte)data2);
    mE->setEventTime(RealTime(eventTimeSec, eventTimeUsec));
    mE->setDuration(RealTime(durationSec, durationUsec));
    mE->setAudioStartMarker(RealTime(audioSec, audioUsec));
    mE->setDataBlockId(dataBlockId);

    return dS;
}

QDataStream&
operator>>(QDataStream &dS, MappedEvent &mE)
{
    unsigned int trackId = 0, instrument = 0, type = 0, data1 = 0, data2 = 0;
    long eventTimeSec = 0, eventTimeUsec = 0, durationSec = 0, durationUsec = 0,
        audioSec = 0, audioUsec = 0;
    std::string dataBlock;
    unsigned long dataBlockId = 0;
         
    dS >> trackId;
    dS >> instrument;
    dS >> type;
    dS >> data1;
    dS >> data2;
    dS >> eventTimeSec;
    dS >> eventTimeUsec;
    dS >> durationSec;
    dS >> durationUsec;
    dS >> audioSec;
    dS >> audioUsec;
    dS >> dataBlockId;

    mE.setTrackId((TrackId)trackId);
    mE.setInstrument((InstrumentId)instrument);
    mE.setType((MappedEvent::MappedEventType)type);
    mE.setData1((MidiByte)data1);
    mE.setData2((MidiByte)data2);
    mE.setEventTime(RealTime(eventTimeSec, eventTimeUsec));
    mE.setDuration(RealTime(durationSec, durationUsec));
    mE.setAudioStartMarker(RealTime(audioSec, audioUsec));
    mE.setDataBlockId(dataBlockId);

    return dS;
}

void
MappedEvent::addDataByte(MidiByte byte)
{
    DataBlockRepository::getInstance()->addDataByteForEvent(byte, this);
}

void 
MappedEvent::addDataString(const std::string& data)
{
    DataBlockRepository::getInstance()->addDataStringForEvent(data, this);
}



//--------------------------------------------------

class DataBlockFile
{
public:
    DataBlockFile(DataBlockRepository::blockid id);
    ~DataBlockFile();
    
    QString getFileName() { return m_fileName; }

    void addDataByte(MidiByte);
    void addDataString(const std::string&);
    
    void clear() { m_cleared = true; }
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
    : m_fileName(KGlobal::dirs()->resourceDirs("tmp").first() + QString("/datablock_%1").arg(id)),
      m_file(m_fileName),
      m_cleared(false)
{
    std::cerr << "DataBlockFile " << m_fileName.latin1() << endl;
}

DataBlockFile::~DataBlockFile()
{
    if (m_cleared) QFile::remove(m_fileName);
}

bool DataBlockFile::exists()
{
    return QFile::exists(m_fileName);
}

void DataBlockFile::setData(const std::string& s)
{
    prepareToWrite();

    QDataStream stream(&m_file);
    stream.writeRawBytes(s.data(), s.length());
}

std::string DataBlockFile::getData()
{
    if (!exists()) return std::string();

    prepareToRead();

    QDataStream stream(&m_file);
    char* tmp = new char[m_file.size()];
    stream.readRawBytes(tmp, m_file.size());
    std::string res(tmp);

    delete[] tmp;

    return res;
}

void DataBlockFile::addDataByte(MidiByte byte)
{
    prepareToWrite();
    m_file.putch(byte);
}

void DataBlockFile::addDataString(const std::string& s)
{
    prepareToWrite();
    QDataStream stream(&m_file);
    stream.writeRawBytes(s.data(), s.length());
}

void DataBlockFile::prepareToWrite()
{    
    if (!m_file.isWritable()) {
        m_file.close();
        m_file.open(IO_WriteOnly | IO_Append);
    }
}

void DataBlockFile::prepareToRead()
{
    if (!m_file.isReadable()) {
        m_file.close();
        m_file.open(IO_ReadOnly);
    }
}



//--------------------------------------------------

DataBlockRepository* DataBlockRepository::getInstance()
{
    if (!m_instance) m_instance = new DataBlockRepository;
    return m_instance;
}

std::string DataBlockRepository::getDataBlock(DataBlockRepository::blockid id)
{
    DataBlockFile dataBlockFile(id);
    
    if (dataBlockFile.exists()) return dataBlockFile.getData();

    return std::string();
}


std::string DataBlockRepository::getDataBlockForEvent(MappedEvent* e)
{
    return getInstance()->getDataBlock(e->getDataBlockId());
}

void DataBlockRepository::setDataBlockForEvent(MappedEvent* e, const std::string& s)
{
    DataBlockFile dataBlockFile(e->getDataBlockId());
    dataBlockFile.setData(s);
}

bool DataBlockRepository::hasDataBlock(DataBlockRepository::blockid id)
{
    return DataBlockFile(id).exists();
}

DataBlockRepository::blockid DataBlockRepository::registerDataBlock(const std::string& s)
{
    blockid id = m_lastId++;

    DataBlockFile dataBlockFile(id);
    dataBlockFile.setData(s);

    return id;
}

void DataBlockRepository::unregisterDataBlock(DataBlockRepository::blockid id)
{
    DataBlockFile dataBlockFile(id);
    
    dataBlockFile.clear();
}

void DataBlockRepository::registerDataBlockForEvent(const std::string& s, MappedEvent* e)
{
    e->setDataBlockId(registerDataBlock(s));
}

void DataBlockRepository::unregisterDataBlockForEvent(MappedEvent* e)
{
    unregisterDataBlock(e->getDataBlockId());
}

    
DataBlockRepository::DataBlockRepository()
    : m_lastId(1)
{
    // Erase all 'datablock_*' files
    //
    QString tmpPath = KGlobal::dirs()->resourceDirs("tmp").first();

    QDir segmentsDir(tmpPath, "datablock_*");
    for (unsigned int i = 0; i < segmentsDir.count(); ++i) {
        QString segmentName = tmpPath + '/' + segmentsDir[i];
        QFile::remove(segmentName);
    }
}

void DataBlockRepository::addDataByteForEvent(MidiByte byte, MappedEvent* e)
{
    DataBlockFile dataBlockFile(e->getDataBlockId());
    dataBlockFile.addDataByte(byte);
    
}

void DataBlockRepository::addDataStringForEvent(const std::string& s, MappedEvent* e)
{
    DataBlockFile dataBlockFile(e->getDataBlockId());
    dataBlockFile.addDataString(s);
}

DataBlockRepository* DataBlockRepository::m_instance = 0;

}
