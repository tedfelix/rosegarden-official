/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_WAVEXPORTER_H
#define RG_WAVEXPORTER_H

typedef float sample_t;
#include "RingBuffer.h"

#include <memory>

class QString;


namespace Rosegarden
{


class AudioWriteStream;


/// Export playback to a wav file.
/**
 * This only exports audio generated by JackDriver.  That includes audio
 * and synth-plugin tracks.  It cannot export audio from synths running
 * externally (e.g. qsynth) or from external physical synths.
 *
 * jack_capture can record a mixdown of all JACK audio to an audio file.
 * See "Creating a Mixdown" on the wiki:
 *   https://www.rosegardenmusic.com/wiki/doc:creating_a_mixdown
 *
 * We should add the ability to export audio like jack_capture does.
 * We would need to create an "export_audio" port and connect all
 * ports that are connected to system out to this "export_audio" port.
 * Then we could record from "export_audio" to an audio file.
 * See JackDriver::jackProcessRecord().
 */
class WAVExporter
{
public:
    explicit WAVExporter(const QString& fileName);
    /*
     * Call this after the ctor to determine whether the file was
     * successfully created.
     */
    bool isOK() const  { return static_cast<bool>(m_audioWriteStream); }

    /// called by the audio thread on start playback
    void start();

    /// called by the audio thread on stop playback
    void stop();

    /// called by the audio thread to provide channel data
    void addSamples(sample_t *left, sample_t *right, size_t numSamples);

    /// called by the gui thread to update the file data
    void update();

    /// Export is complete, or this object is not OK.
    /**
     * Called by the gui thread to request completion status.
     */
    bool isComplete() const;

private:

    // Output File
    std::shared_ptr<AudioWriteStream> m_audioWriteStream;

    // Processing state.
    bool m_running{false};
    bool m_stopRequested{false};

    // Lock-free buffers written by the audio thread and read by the GUI thread.
    std::unique_ptr<RingBuffer<sample_t>> m_leftChannelBuffer;
    std::unique_ptr<RingBuffer<sample_t>> m_rightChannelBuffer;

};


}

#endif /* ifndef RG_WAVEXPORTER_H */
