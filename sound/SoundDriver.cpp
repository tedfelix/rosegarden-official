// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden-4 v0.2
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


#include "SoundDriver.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"


namespace Rosegarden
{

PlayableAudioFile::PlayableAudioFile(InstrumentId instrumentId,
                                     AudioFile *audioFile,
                                     const RealTime &startTime,
                                     const RealTime &startIndex,
                                     const RealTime &duration):
        m_startTime(startTime),
        m_startIndex(startIndex),
        m_duration(duration),
        m_status(IDLE),
        m_file(0),
        m_audioFile(audioFile),
        m_instrumentId(instrumentId)
{
    m_file = new std::ifstream(m_audioFile->getFilename().c_str(),
                               std::ios::in | std::ios::binary);

    if (!*m_file)
        throw(std::string("PlayableAudioFile - can't open file"));

    // scan to the beginning of the data chunk
    scanTo(RealTime(0, 0));
}

PlayableAudioFile::~PlayableAudioFile()
{
    if (m_file)
    {
        m_file->close();
        delete m_file;
    }
}
 
bool
PlayableAudioFile::scanTo(const RealTime &time)
{
    if (m_audioFile)
    {
        return m_audioFile->scanTo(m_file, time);
    }
    return false;
}


// Get some sample frames using this object's file handle
//
std::string
PlayableAudioFile::getSampleFrames(unsigned int frames)
{
    if (m_audioFile)
    {
        return m_audioFile->getSampleFrames(m_file, frames);
    }
    return std::string("");
}

// Get a sample file slice using this object's file handle
//
std::string
PlayableAudioFile::getSampleFrameSlice(const RealTime &time)
{
    if (m_audioFile)
    {
        return m_audioFile->getSampleFrameSlice(m_file, time);
    }
    return std::string("");
}

// How many channels in the base AudioFile?
//
unsigned int
PlayableAudioFile::getChannels()
{
    if (m_audioFile)
    {
        return m_audioFile->getChannels();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getBytesPerSample()
{
    if (m_audioFile)
    {
        return m_audioFile->getBytesPerFrame();
    }
    return 0;
}

unsigned int
PlayableAudioFile::getSampleRate()
{
    if (m_audioFile)
    {
        return m_audioFile->getSampleRate();
    }
    return 0;
}


// How many bits per sample in the base AudioFile?
//
unsigned int
PlayableAudioFile::getBitsPerSample()
{
    if (m_audioFile)
    {
        return m_audioFile->getBitsPerSample();
    }
    return 0;
}


// ---------- SoundDriver -----------
//

SoundDriver::SoundDriver(MappedStudio *studio, const std::string &name):
    m_name(name),
    m_driverStatus(NO_DRIVER),
    m_playStartPosition(0, 0),
    m_startPlayback(false),
    m_playing(false),
    m_recordStatus(ASYNCHRONOUS_MIDI),
    m_midiRunningId(MidiInstrumentBase),
    m_audioRunningId(AudioInstrumentBase),
    m_deviceRunningId(0),
    m_audioMonitoringInstrument(Rosegarden::AudioInstrumentBase),
    m_audioPlayLatency(0, 0),
    m_audioRecordLatency(0, 0),
    m_studio(studio)
{
}


SoundDriver::~SoundDriver()
{
    std::cout << "SoundDriver::~SoundDriver" << std::endl;
}

MappedInstrument*
SoundDriver::getMappedInstrument(InstrumentId id)
{
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getId() == id)
            return (*it);
    }

    return 0;
}

void
SoundDriver::queueAudio(PlayableAudioFile *audioFile)
{
    // Push to the back of the thread queue and then we must
    // process this across to the proper audio queue when
    // it's safe to do so - use the method below
    //
    m_audioPlayThreadQueue.push_back(audioFile);
}

// Move the pending thread queue across to the real queue
// at a safe point in time (when another thread isn't
// accessing the vector.
//
void
SoundDriver::pushPlayableAudioQueue()
{
    std::vector<PlayableAudioFile*>::iterator it;

    for (it = m_audioPlayThreadQueue.begin();
         it != m_audioPlayThreadQueue.end();
         it++)

    {
        m_audioPlayQueue.push_back(*it);
    }

    m_audioPlayThreadQueue.erase(m_audioPlayThreadQueue.begin(),
                                 m_audioPlayThreadQueue.end());
}

void
SoundDriver::setMappedInstrument(MappedInstrument *mI)
{
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

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

// m_deviceRunningId should carry the id of the last allocated device
//
unsigned int
SoundDriver::getDevices()
{
    return m_deviceRunningId + 1;
}

MappedDevice
SoundDriver::getMappedDevice(DeviceId id)
{
    MappedDevice retDevice;
    std::vector<Rosegarden::MappedInstrument*>::iterator it;

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++)
    {
        if ((*it)->getDevice() == id)
            retDevice.push_back(*it);
    }

    return retDevice;
}


// Clear down the audio play queue
//
void
SoundDriver::clearAudioPlayQueue()
{
    std::vector<PlayableAudioFile*>::iterator it;

    for (it = m_audioPlayQueue.begin(); it != m_audioPlayQueue.end(); it++)
        delete (*it);

    m_audioPlayQueue.erase(m_audioPlayQueue.begin(), m_audioPlayQueue.end());
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


bool
SoundDriver::queueAudio(InstrumentId instrumentId,
                        AudioFileId audioFileId,
                        const RealTime &absoluteTime,
                        const RealTime &audioStartMarker,
                        const RealTime &duration,
                        const RealTime &playLatency)
{
    AudioFile* audioFile = getAudioFile(audioFileId);

    if (audioFile == 0)
        return false;

    std::cout << "queueAudio() - queuing Audio event at time "
              << absoluteTime + playLatency << std::endl;

    // register the AudioFile in the playback queue
    //
    PlayableAudioFile *newAF =
                         new PlayableAudioFile(instrumentId,
                                               audioFile,
                                               absoluteTime + playLatency,
                                               audioStartMarker - absoluteTime,
                                               duration);

    queueAudio(newAF);

    return true;
}



// Close down any playing audio files - we can manipulate the
// live play stack as we're only changing state.
//
void
SoundDriver::cancelAudioFile(InstrumentId instrumentId, AudioFileId audioFileId)
{
    std::vector<PlayableAudioFile*>::iterator it;

    for (it = m_audioPlayQueue.begin();
         it != m_audioPlayQueue.end();
         it++)
    {
        if((*it)->getInstrument() == instrumentId &&
           (*it)->getAudioFile()->getId() == audioFileId)
        {
            (*it)->setStatus(PlayableAudioFile::DEFUNCT);
        }
    }
}


}

