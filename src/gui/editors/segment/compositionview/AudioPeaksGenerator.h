
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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
#include <inttypes.h>

class QEvent;


namespace Rosegarden
{

class Segment;
class CompositionModelImpl;
class Composition;
class AudioPeaksThread;


/// Sends a request to the AudioPeaksThread to generate audio peaks (m_values).
class AudioPeaksGenerator : public QObject
{
    Q_OBJECT

public:
    AudioPeaksGenerator(AudioPeaksThread &thread,
                        const Composition &composition,
                        const Segment *segment,
                        const QRect &displayExtent,
                        CompositionModelImpl *parent);
    ~AudioPeaksGenerator();

    // ??? rename: generateAsync()
    void update();
    void cancel();

    QRect getDisplayExtent() const { return m_rect; }
    void setDisplayExtent(const QRect &rect) { m_rect = rect; }

    const Segment *getSegment() const { return m_segment; }

    // ??? rename: getPeaks()
    const std::vector<float> &getComputedValues(unsigned int &channels) const
    { channels = m_channels; return m_values; }

signals:
    void audioPeaksComplete(AudioPeaksGenerator *);

protected:
    virtual bool event(QEvent*);

    AudioPeaksThread &m_thread;

    const Composition& m_composition;
    const Segment*     m_segment;
    QRect                          m_rect;
    bool                           m_showMinima;
    unsigned int                   m_channels;
    // ??? rename: m_peaks
    std::vector<float>             m_values;

    intptr_t m_token;
};


}

#endif
