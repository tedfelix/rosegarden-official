/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMPOSITIONEXPORTMANAGER_H
#define RG_COMPOSITIONEXPORTMANAGER_H

typedef float sample_t;
#include "RingBuffer.h"

namespace Rosegarden
{

class AudioWriteStream;

// Manage export of composition to wav file
class CompositionExportManager
{
public:
    explicit CompositionExportManager(const QString& fileName);

    /// called by the audio thread on start playback
    void start(int sampleRate);

    /// called by the audio thread on stop playback
    void stop();

    /// called by the aduio thread to provide channel data
    void addSamples(sample_t *left, sample_t *right, size_t numSamples);

    /// called by the gui thread to update the file data
    void update();

    /// called by the gui thread to request completion status
    bool isComplete() const;

 private:
    QString m_fileName;
    int m_sampleRate;
    bool m_start;
    bool m_stop;
    RingBuffer<sample_t>* m_leftChannelBuffer;
    RingBuffer<sample_t>* m_rightChannelBuffer;
    bool m_running;
    AudioWriteStream *m_ws;

};

}

#endif /* ifndef RG_COMPOSITIONEXPORTMANAGER_H */
