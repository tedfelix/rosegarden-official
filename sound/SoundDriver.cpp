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

#include <stdlib.h>

#include "SoundDriver.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"

#include <sys/time.h>
#include <pthread.h> // for mutex

//#define DEBUG_PLAYABLE_CONSTRUCTION 1
//#define DEBUG_PLAYABLE 1

namespace Rosegarden
{

// ---------- SoundDriver -----------
//

static pthread_mutex_t _audioQueueLock = PTHREAD_MUTEX_INITIALIZER;


SoundDriver::SoundDriver(MappedStudio *studio, const std::string &name):
    m_name(name),
    m_driverStatus(NO_DRIVER),
    m_playStartPosition(0, 0),
    m_startPlayback(false),
    m_playing(false),
    m_midiRecordDevice(0),
    m_recordStatus(ASYNCHRONOUS_MIDI),
    m_midiRunningId(MidiInstrumentBase),
    m_audioRunningId(AudioInstrumentBase),
    m_audioMonitoringInstrument(AudioInstrumentBase),
    m_audioPlayLatency(0, 0),
    m_audioRecordLatency(0, 0),
    m_studio(studio),
    m_sequencerDataBlock(0),
    m_externalTransport(0),
    m_mmcEnabled(false),
    m_mmcMaster(false),
    m_mmcId(0),           // default MMC id of 0
    m_midiClockEnabled(false),
    m_midiClockInterval(0, 0),
    m_midiClockSendTime(RealTime::zeroTime),
    m_midiSongPositionPointer(0)
{
    // nothing else
}


SoundDriver::~SoundDriver()
{
    std::cout << "SoundDriver::~SoundDriver (exiting)" << std::endl;
}

MappedInstrument*
SoundDriver::getMappedInstrument(InstrumentId id)
{
    std::vector<MappedInstrument*>::const_iterator it;

    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == id)
            return (*it);
    }

    return 0;
}

PlayableAudioFileList
SoundDriver::getAudioPlayQueue()
{
    PlayableAudioFileList rq;
    std::list<PlayableAudioFile *>::const_iterator it;

    pthread_mutex_lock(&_audioQueueLock);

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
	rq.push_back(*it);
    }

    pthread_mutex_unlock(&_audioQueueLock);

    return rq;
}

// Generates a list of queued PlayableAudioFiles that aren't marked
// as defunct (and hence likely to be garbage collected by another
// thread).
//
PlayableAudioFileList
SoundDriver::getAudioPlayQueueNotDefunct()
{
    PlayableAudioFileList rq;
    std::list<PlayableAudioFile *>::const_iterator it;

    pthread_mutex_lock(&_audioQueueLock);

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
#ifdef DEBUG_PLAYABLE
	std::cout << "SoundDriver::getAudioPlayQueueNotDefunct: id "
		  << (*it)->getRuntimeSegmentId() << ", status " << (*it)->getStatus() << " (defunct is " << PlayableAudioFile::DEFUNCT << "), initialised " << (*it)->isInitialised() << std::endl;
#endif

        if ((*it)->getStatus() != PlayableAudioFile::DEFUNCT) {
            rq.push_back(*it);
        }
    }

    pthread_mutex_unlock(&_audioQueueLock);

    return rq;
}

// Generates a list of queued PlayableAudioFiles on a particular
// instrument that aren't marked as defunct (and hence likely to be
// garbage collected by another thread).
//
PlayableAudioFileList
SoundDriver::getAudioPlayQueuePerInstrument(InstrumentId instrument)
{
    PlayableAudioFileList rq;
    std::list<PlayableAudioFile *>::const_iterator it;

    pthread_mutex_lock(&_audioQueueLock);

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ++it)
    {
#ifdef DEBUG_PLAYABLE
	std::cout << "SoundDriver::getAudioPlayQueuePerInstrument(" << instrument << "): id "
		  << (*it)->getRuntimeSegmentId() << ", instrument " << (*it)->getInstrument() << ", status " << (*it)->getStatus() << " (defunct is " << PlayableAudioFile::DEFUNCT << "), initialised " << (*it)->isInitialised() << std::endl;
#endif

        if ((*it)->getStatus() != PlayableAudioFile::DEFUNCT &&
	    (*it)->getInstrument() == instrument) {
            rq.push_back(*it);
        }
    }

    pthread_mutex_unlock(&_audioQueueLock);

    return rq;
}


void
SoundDriver::queueAudio(PlayableAudioFile *audioFile)
{
#ifdef DEBUG_PLAYABLE
    std::cout << "SoundDriver::queueAudio called, queueing file " << audioFile << std::endl;
#endif

    pthread_mutex_lock(&_audioQueueLock);

    m_audioPlayQueue.push_back(audioFile);

    pthread_mutex_unlock(&_audioQueueLock);
}

void
SoundDriver::setMappedInstrument(MappedInstrument *mI)
{
    std::vector<MappedInstrument*>::iterator it;

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == mI->getId())
        {
            (*it)->setChannel(mI->getChannel());
            (*it)->setType(mI->getType());
            delete mI;
            return;
        }
    }

    // else create a new one
    m_instruments.push_back(mI);

    std::cout << "SoundDriver: setMappedInstrument() : "
              << "type = " << mI->getType() << " : "
              << "channel = " << (int)(mI->getChannel()) << " : "
              << "id = " << mI->getId() << std::endl;

}

unsigned int
SoundDriver::getDevices()
{
    return m_devices.size();
}

MappedDevice
SoundDriver::getMappedDevice(DeviceId id)
{
    MappedDevice retDevice;
    std::vector<MappedInstrument*>::iterator it;

    std::vector<MappedDevice*>::iterator dIt = m_devices.begin();
    for (; dIt != m_devices.end(); dIt++)
    {
        if ((*dIt)->getId() == id) retDevice = **dIt;
    }

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getDevice() == id)
            retDevice.push_back(*it);
    }

    std::cout << "SoundDriver::getMappedDevice(" << id << ") - "
              << "name = \"" << retDevice.getName() 
              << "\" type = " << retDevice.getType()
              << " direction = " << retDevice.getDirection()
              << " connection = \"" << retDevice.getConnection()
              << "\"" << std::endl;

    return retDevice;
}


// Clear down the audio play queue
//
void
SoundDriver::clearAudioPlayQueue()
{
    std::list<PlayableAudioFile *>::iterator it;

    pthread_mutex_lock(&_audioQueueLock);

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); it++)
        delete (*it);

    m_audioPlayQueue.erase(m_audioPlayQueue.begin(), m_audioPlayQueue.end());

    pthread_mutex_unlock(&_audioQueueLock);
}


void
SoundDriver::clearDefunctFromAudioPlayQueue()
{
    std::list<PlayableAudioFile *>::iterator it;

    pthread_mutex_lock(&_audioQueueLock);

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); ) {

        if ((*it)->getStatus() == PlayableAudioFile::DEFUNCT) {
#ifdef DEBUG_PLAYABLE
            std::cout << "SoundDriver::clearDefunctFromAudioPlayQueue - "
                      << "clearing down " << *it << std::endl;
#endif
	    std::list<PlayableAudioFile *>::iterator ni = it;
	    ++ni;
            delete *it;
	    m_audioPlayQueue.erase(it);
	    it = ni;

        } else {
	    ++it;
	}
    }

    pthread_mutex_unlock(&_audioQueueLock);
}

bool
SoundDriver::addAudioFile(const std::string &fileName, unsigned int id)
{
    AudioFile *ins = new WAVAudioFile(id, fileName, fileName);
    try
    {
        ins->open();
    }
    catch(std::string s)
    {
        return false;
    }

    m_audioFiles.push_back(ins);

    std::cout << "Sequencer::addAudioFile() = \"" << fileName << "\"" << std::endl;

    return true;
}

bool
SoundDriver::removeAudioFile(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getId() == id)
        {
            std::cout << "Sequencer::removeAudioFile() = \"" <<
                          (*it)->getFilename() << "\"" << std::endl;

            delete (*it);
            m_audioFiles.erase(it);
            return true;
        }
    }

    return false;
}

AudioFile*
SoundDriver::getAudioFile(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
    {
        if ((*it)->getId() == id)
            return *it;
    }

    return 0;
}

void
SoundDriver::clearAudioFiles()
{
    std::cout << "SoundDriver::clearAudioFiles() - clearing down audio files"
              << std::endl;

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); it++)
        delete(*it);

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());
}


// Close down any playing audio files - we can manipulate the
// live play stack as we're only changing state.  Switch on
// segment id first and then drop to instrument/audio file.
//
void
SoundDriver::cancelAudioFile(MappedEvent *mE)
{
    std::list<PlayableAudioFile *>::iterator it;

    pthread_mutex_lock(&_audioQueueLock);

    for (it = m_audioPlayQueue.begin();
         it != m_audioPlayQueue.end();
         it++)
    {
        if (mE->getRuntimeSegmentId() == -1)
        {
            if ((*it)->getInstrument() == mE->getInstrument() &&
		(int)(*it)->getAudioFile()->getId() == mE->getAudioID())
                (*it)->setStatus(PlayableAudioFile::DEFUNCT);
        }
        else
        {
            if ((*it)->getRuntimeSegmentId() == mE->getRuntimeSegmentId() &&
		(*it)->getStartTime() == mE->getEventTime())
                (*it)->setStatus(PlayableAudioFile::DEFUNCT);
	    else {
		std::cerr << "audio file mismatch: ids "
			  << (*it)->getRuntimeSegmentId() << " vs "
			  << mE->getRuntimeSegmentId() << ", times "
			  << (*it)->getStartTime() << " vs "
			  << mE->getEventTime() << std::endl;
	    }
        }
    }

    pthread_mutex_unlock(&_audioQueueLock);
}

void
SoundDriver::sleep(const RealTime &rt)
{
    struct timespec reg;
    reg.tv_sec = rt.sec;
    reg.tv_nsec = rt.nsec;
    nanosleep(&reg, 0);
}


}

