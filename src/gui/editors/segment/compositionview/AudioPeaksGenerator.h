
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOPEAKSGENERATOR_H
#define RG_AUDIOPEAKSGENERATOR_H

#include <QObject>
#include <QRect>

#include <vector>

class QEvent;


namespace Rosegarden
{

class Segment;
class CompositionModelImpl;
class Composition;
class AudioPeaksThread;

/// Generates peaks for an audio Segment.
/**
 * Used by CompositionModelImpl to generate audio previews for an audio
 * Segment.
 *
 * Uses AudioPeaksThread to generate the audio peaks (m_peaks).
 */
class AudioPeaksGenerator : public QObject
{
    Q_OBJECT

public:
    AudioPeaksGenerator(AudioPeaksThread &thread,
                        const Composition &composition,
                        const Segment *segment,
                        const QRect &segmentRect,
                        CompositionModelImpl *parent);
    ~AudioPeaksGenerator();

    void setSegmentRect(const QRect &rect)  { m_rect = rect; }

    /// Generate audio peaks for the Segment.
    /**
     * The audioPeaksComplete() signal will be emitted on completion.
     */
    void generateAsync();

    /// Stop a peak generation in progress.
    void cancel();

    const std::vector<float> &getPeaks(unsigned int &channels) const
    {
        channels = m_channels;
        return m_peaks;
    }

    const Segment *getSegment() const  { return m_segment; }

signals:
    /// Emitted once the asynchronous generation of peaks is complete.
    void audioPeaksComplete(AudioPeaksGenerator *);

protected:
    // QObject override.
    bool event(QEvent *) override;

private:
    // ??? Instead of requiring that this comes in via the ctor, why not
    //     get it directly from RosegardenDocument?  Or, even better, since
    //     this is the main user, it should own the instance and allow
    //     RosegardenDocument and CompositionView to get it from here.
    AudioPeaksThread &m_thread;

    const Composition &m_composition;

    const Segment *m_segment;
    QRect m_rect;
    bool m_showMinima;

    /// Token from AudioPeaksThread to identify the work being done.
    int m_token;

    unsigned int m_channels;
    std::vector<float> m_peaks;
};


}

#endif
