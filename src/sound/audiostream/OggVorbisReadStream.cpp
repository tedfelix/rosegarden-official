/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND

#include "OggVorbisReadStream.h"

#include "sound/RingBuffer.h"

#include <oggz/oggz.h>
#include <fishsound/fishsound.h>

#ifndef __GNUC__
#include <alloca.h>
#endif

namespace Rosegarden
{

static
AudioReadStreamBuilder<OggVorbisReadStream>
oggbuilder(
    QUrl("http://breakfastquay.com/rdf/rosegarden/fileio/OggVorbisReadStream"),
    QStringList() << "ogg" << "oga"
    );

class OggVorbisReadStream::D
{
public:
    D(OggVorbisReadStream *rs) :
        m_rs(rs),
        m_oggz(0),
        m_fishSound(0),
        m_buffer(0),
        m_finished(false) { }
    ~D() {
	if (m_fishSound) fish_sound_delete(m_fishSound);
	if (m_oggz) oggz_close(m_oggz);
        delete m_buffer;
    }

    OggVorbisReadStream *m_rs;
    FishSound *m_fishSound;
    OGGZ *m_oggz;
    RingBuffer<float> *m_buffer;
    bool m_finished;

    bool isFinished() const {
        return m_finished;
    }

    int getAvailableFrameCount() const {
        if (!m_buffer) return 0;
        return m_buffer->getReadSpace() / m_rs->getChannelCount();
    }

    void readNextBlock() {
        if (m_finished) return;
        if (oggz_read(m_oggz, 1024) <= 0) {
            m_finished = true;
        }
    }

    void sizeBuffer(int minFrames) {
        int samples = minFrames * m_rs->getChannelCount();
        if (!m_buffer) {
            m_buffer = new RingBuffer<float>(samples);
        } else if (m_buffer->getSize() < samples) {
            m_buffer = m_buffer->resized(samples);
        }
    }

    int acceptPacket(ogg_packet *p) {
        fish_sound_prepare_truncation(m_fishSound, p->granulepos, p->e_o_s);
        fish_sound_decode(m_fishSound, p->packet, p->bytes);
        return 0;
    }

    int acceptFrames(float **frames, long n) {

        if (m_rs->getChannelCount() == 0) {
            FishSoundInfo fsinfo;
            fish_sound_command(m_fishSound, FISH_SOUND_GET_INFO,
                               &fsinfo, sizeof(FishSoundInfo));
            m_rs->m_channelCount = fsinfo.channels;
            m_rs->m_sampleRate = fsinfo.samplerate;
        }

        sizeBuffer(getAvailableFrameCount() + n);
        int channels = m_rs->getChannelCount();
#ifdef __GNUC__
        float interleaved[n * channels];
#else
        float *interleaved = (float *)alloca(n * channels * sizeof(float));
#endif
	for (long i = 0; i < n; ++i) {
	    for (int c = 0; c < channels; ++c) {
                interleaved[i * channels + c] = frames[c][i];
            }
        }
        m_buffer->write(interleaved, n * channels);
        return 0;
    }

    static int acceptPacketStatic(OGGZ *, ogg_packet *packet, long, void *data) {
        D *d = (D *)data;
        return d->acceptPacket(packet);
    }

    static int acceptFramesStatic(FishSound *, float **frames, long n, void *data) {
        D *d = (D *)data;
        return d->acceptFrames(frames, n);
    }
};

OggVorbisReadStream::OggVorbisReadStream(QString path) :
    m_path(path),
    m_d(new D(this))
{
    m_channelCount = 0;
    m_sampleRate = 0;

    if (!(m_d->m_oggz = oggz_open(path.toLocal8Bit().data(), OGGZ_READ))) {
	m_error = QString("File \"%1\" is not an OGG file.").arg(path);
	return;
    }

    FishSoundInfo fsinfo;
    m_d->m_fishSound = fish_sound_new(FISH_SOUND_DECODE, &fsinfo);

    fish_sound_set_decoded_callback(m_d->m_fishSound, D::acceptFramesStatic, m_d);
    oggz_set_read_callback(m_d->m_oggz, -1, D::acceptPacketStatic, m_d);

    // initialise m_channelCount
    while (m_channelCount == 0 && !m_d->m_finished) {
        m_d->readNextBlock(); 
    }
}

OggVorbisReadStream::~OggVorbisReadStream()
{
    delete m_d;
}

size_t
OggVorbisReadStream::getFrames(size_t count, float *frames)
{
    if (!m_channelCount) return 0;
    if (count == 0) return 0;

    while (m_d->getAvailableFrameCount() < count) {
        if (m_d->isFinished()) break;
        m_d->readNextBlock();
    }

    return m_d->m_buffer->read(frames, count * m_channelCount);
}

}

#endif
#endif
