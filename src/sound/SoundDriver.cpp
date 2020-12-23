/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SoundDriver]"

#include <stdlib.h>

#include "misc/Debug.h"
#include "SoundDriver.h"
#include "WAVAudioFile.h"
#include "MappedStudio.h"
#include "AudioPlayQueue.h"
#include "PlayableAudioFile.h"
#include "sequencer/RosegardenSequencer.h"

#include <unistd.h>  // for usleep()
#include <sys/time.h>
#include <pthread.h> // for mutex

//#define DEBUG_SOUND_DRIVER 1

namespace Rosegarden
{


SoundDriver::SoundDriver(MappedStudio *studio, const QString &name) :
        m_name(name),
        m_driverStatus(NO_DRIVER),
        m_sequencer(RosegardenSequencer::getInstance()),
        m_playStartPosition(0, 0),
        m_playing(false),
        m_recordStatus(RECORD_OFF),
        m_midiClockInterval(0, 0),
        m_audioQueue(nullptr),
        m_smallFileSize(0),
        m_audioRecFileFormat(RIFFAudioFile::FLOAT),
        m_studio(studio)
{
    m_audioQueue = new AudioPlayQueue();
}

SoundDriver::~SoundDriver()
{
    RG_DEBUG << "SoundDriver::~SoundDriver (exiting)";
    delete m_audioQueue;
}

void
SoundDriver::initialiseAudioQueue(const std::vector<MappedEvent> &events)
{
    AudioPlayQueue *newQueue = new AudioPlayQueue();

    for (std::vector<MappedEvent>::const_iterator i = events.begin();
            i != events.end(); ++i) {

        // Check for existence of file - if the sequencer has died
        // and been restarted then we're not always loaded up with
        // the audio file references we should have.  In the future
        // we could make this just get the gui to reload our files
        // when (or before) this fails.
        //
        AudioFile *audioFile = getAudioFile(i->getAudioID());

        if (audioFile) {
            MappedAudioFader *fader =
                dynamic_cast<MappedAudioFader*>
                (m_studio->getAudioFader(i->getInstrument()));

            if (!fader) {
                RG_DEBUG << "WARNING: SoundDriver::initialiseAudioQueue: no fader for audio instrument " << i->getInstrument();
                continue;
            }

            int channels = fader->getPropertyList(
                                        MappedAudioFader::Channels)[0].toInt();

            //#define DEBUG_PLAYING_AUDIO
#ifdef DEBUG_PLAYING_AUDIO

            RG_DEBUG << "Creating playable audio file: id " << audioFile->getId() << ", event time " << i->getEventTime() << ", time now " << getSequencerTime() << ", start marker " << i->getAudioStartMarker() << ", duration " << i->getDuration() << ", instrument " << i->getInstrument() << " channels " << channels;
#endif

            RealTime bufferLength = getAudioReadBufferLength();
            size_t bufferFrames = (size_t)RealTime::realTime2Frame
                                  (bufferLength, getSampleRate());

            PlayableAudioFile *paf = nullptr;

            try {
                paf = new PlayableAudioFile(i->getInstrument(),
                                            audioFile,
                                            i->getEventTime(),
                                            i->getAudioStartMarker(),
                                            i->getDuration(),
                                            bufferFrames,
                                            size_t(m_smallFileSize) * 1024,
                                            channels,
                                            int(getSampleRate()));
            } catch (...) {
                continue;
            }

            paf->setRuntimeSegmentId(i->getRuntimeSegmentId());

            if (i->isAutoFading()) {
                paf->setAutoFade(true);
                paf->setFadeInTime(i->getFadeInTime());
                paf->setFadeOutTime(i->getFadeInTime());

                //#define DEBUG_AUTOFADING
#ifdef DEBUG_AUTOFADING

                RG_DEBUG << "SoundDriver::initialiseAudioQueue - "
                << "PlayableAudioFile is AUTOFADING - "
                << "in = " << i->getFadeInTime()
                << ", out = " << i->getFadeOutTime();
#endif

            }
#ifdef DEBUG_AUTOFADING
            else {
                RG_DEBUG << "PlayableAudioFile has no AUTOFADE";
            }
#endif

            newQueue->addScheduled(paf);
        } else {
            RG_DEBUG << "SoundDriver::initialiseAudioQueue - "
            << "can't find audio file reference for id " << i->getAudioID();

            RG_DEBUG << "SoundDriver::initialiseAudioQueue - "
            << "try reloading the current Rosegarden file";
        }
    }

    RG_DEBUG << "SoundDriver::initialiseAudioQueue -- new queue has "
    << newQueue->size() << " files";

    if (newQueue->empty()) {
        if (m_audioQueue->empty()) {
            delete newQueue;
            return ;
        }
    }

    AudioPlayQueue *oldQueue = m_audioQueue;
    m_audioQueue = newQueue;
    if (oldQueue)
        m_audioQueueScavenger.claim(oldQueue);
}

const AudioPlayQueue *
SoundDriver::getAudioQueue() const
{
    return m_audioQueue;
}


void
SoundDriver::setMappedInstrument(MappedInstrument *mI)
{
    std::vector<MappedInstrument*>::iterator it;

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); ++it) {
        if ((*it)->getId() == mI->getId()) {
            (*it)->setType(mI->getType());
            delete mI;
            return ;
        }
    }

    // else create a new one
    m_instruments.push_back(mI);

    //RG_DEBUG << "setMappedInstrument(): type = " << mI->getType() << " : " << "id = " << mI->getId();

}

/*!DEVPUSH
unsigned int
SoundDriver::getDevices()
{
    return (unsigned int)m_devices.size();
}

MappedDevice
SoundDriver::getMappedDevice(DeviceId id)
{
    MappedDevice retDevice;
    std::vector<MappedInstrument*>::iterator it;

    std::vector<MappedDevice*>::iterator dIt = m_devices.begin();
    for (; dIt != m_devices.end(); dIt++) {
        if ((*dIt)->getId() == id)
            retDevice = **dIt;
    }

    // If we match then change existing entry
    for (it = m_instruments.begin(); it != m_instruments.end(); it++) {
        if ((*it)->getDevice() == id)
            retDevice.push_back(*it);
    }

#ifdef DEBUG_SOUND_DRIVER
    RG_DEBUG << "SoundDriver::getMappedDevice(" << id << ") - "
    << "name = \"" << retDevice.getName()
    << "\" type = " << retDevice.getType()
    << " direction = " << retDevice.getDirection()
    << " connection = \"" << retDevice.getConnection() << "\""
    << " recording = " << retDevice.isRecording();
#endif

    return retDevice;
}
*/


bool
SoundDriver::addAudioFile(const QString &fileName, unsigned int id)
{
    AudioFile *ins = nullptr;

    try {
        ins = new WAVAudioFile(id, qstrtostr(fileName), fileName);
        ins->open();
        m_audioFiles.push_back(ins);

        //RG_DEBUG << "Sequencer::addAudioFile() = \"" << fileName << "\"";

        return true;

    } catch (const SoundFile::BadSoundFileException &e) {
        RG_DEBUG << "SoundDriver::addAudioFile: Failed to add audio file " << fileName << ": " << e.getMessage();
        delete ins;
        return false;
    }
}

bool
SoundDriver::removeAudioFile(unsigned int id)
{
    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); ++it) {
        if ((*it)->getId() == id) {
            RG_DEBUG << "Sequencer::removeAudioFile() = \"" <<
                (*it)->getFilename() << "\"";

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
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); ++it) {
        if ((*it)->getId() == id)
            return *it;
    }

    return nullptr;
}

void
SoundDriver::clearAudioFiles()
{
    //RG_DEBUG << "SoundDriver::clearAudioFiles() - clearing down audio files";

    std::vector<AudioFile*>::iterator it;
    for (it = m_audioFiles.begin(); it != m_audioFiles.end(); ++it)
        delete(*it);

    m_audioFiles.erase(m_audioFiles.begin(), m_audioFiles.end());
}

void
SoundDriver::sleep(const RealTime &rt)
{
    // The usleep man page says it's deprecated and we should use
    // nanosleep.  And that's what we did.  But it seems quite a few
    // people don't have nanosleep, so we're reverting to usleep.

    unsigned long usec = rt.sec * 1000000 + rt.usec();
    usleep(usec);
}


}

