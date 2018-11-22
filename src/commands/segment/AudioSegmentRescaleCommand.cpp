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


#include "AudioSegmentRescaleCommand.h"

#include "misc/AppendLabel.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/Event.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "document/RosegardenDocument.h"
#include "sound/AudioFileTimeStretcher.h"
#include "sound/AudioFileManager.h"
#include <QString>


namespace Rosegarden
{

AudioSegmentRescaleCommand::AudioSegmentRescaleCommand(RosegardenDocument *doc,
                                                       Segment *s,
						       float ratio) :
    NamedCommand(getGlobalName()),
    m_afm(&doc->getAudioFileManager()),
    m_stretcher(new AudioFileTimeStretcher(m_afm)),
    m_segment(s),
    m_newSegment(nullptr),
    m_timesGiven(false),
    m_startTime(0),
    m_endMarkerTime(0),
    m_fid(-1),
    m_ratio(ratio),
    m_detached(false)
{
    // nothing
}

AudioSegmentRescaleCommand::AudioSegmentRescaleCommand(RosegardenDocument *doc,
                                                       Segment *s,
						       float ratio,
                                                       timeT st,
                                                       timeT emt) :
    NamedCommand(getGlobalName()),
    m_afm(&doc->getAudioFileManager()),
    m_stretcher(new AudioFileTimeStretcher(m_afm)),
    m_segment(s),
    m_newSegment(nullptr),
    m_timesGiven(true),
    m_startTime(st),
    m_endMarkerTime(emt),
    m_fid(-1),
    m_ratio(ratio),
    m_detached(false)
{
    // nothing
}

AudioSegmentRescaleCommand::~AudioSegmentRescaleCommand()
{
    delete m_stretcher;

    if (m_detached) {
        delete m_segment;
    } else {
        delete m_newSegment;
    }
}

void
AudioSegmentRescaleCommand::setProgressDialog(
        QPointer<QProgressDialog> progressDialog)
{
    if (m_stretcher)
        m_stretcher->setProgressDialog(progressDialog);
}

void
AudioSegmentRescaleCommand::execute()
{
    // Audio segments only.
    if (m_segment->getType() != Segment::Audio) {
        RG_WARNING << "WARNING: execute() called with a non-audio segment.";
        return;
    }

    // If we don't have the rescaled segment yet, create it.
    if (!m_newSegment) {

        // Rescale the audio file.

        AudioFileId sourceFileId = m_segment->getAudioFileId();
        float absoluteRatio = m_ratio;

        RG_DEBUG << "AudioSegmentRescaleCommand: segment file id " << sourceFileId << ", given ratio " << m_ratio;

        if (m_segment->getStretchRatio() != 1.f &&
            m_segment->getStretchRatio() != 0.f) {
            sourceFileId = m_segment->getUnstretchedFileId();
            absoluteRatio *= m_segment->getStretchRatio();
            RG_DEBUG << "AudioSegmentRescaleCommand: unstretched file id " << sourceFileId << ", prev ratio " << m_segment->getStretchRatio() << ", resulting ratio " << absoluteRatio;
        }

        if (!m_timesGiven) {
            m_endMarkerTime = m_segment->getStartTime() +
                (m_segment->getEndMarkerTime() - m_segment->getStartTime()) * m_ratio;
        }

        m_fid = m_stretcher->getStretchedAudioFile(sourceFileId,
                                                   absoluteRatio);
        // If the stretch failed, bail.
        if (m_fid < 0)
            return;

        // Audio file was rescaled successfully.  Create the new Segment.

        m_newSegment = m_segment->clone(false);

        std::string label = m_newSegment->getLabel();
        m_newSegment->setLabel(appendLabel(label, qstrtostr(tr("(rescaled)"))));

        m_newSegment->setAudioFileId(m_fid);
        m_newSegment->setUnstretchedFileId(sourceFileId);
        m_newSegment->setStretchRatio(absoluteRatio);
        m_newSegment->setAudioStartTime(m_segment->getAudioStartTime() *
                                        m_ratio);
        if (m_timesGiven) {
            m_newSegment->setStartTime(m_startTime);
            m_newSegment->setAudioEndTime(m_segment->getAudioEndTime() *
                                          m_ratio);
            m_newSegment->setEndMarkerTime(m_endMarkerTime);
        } else {
            m_newSegment->setEndMarkerTime(m_endMarkerTime);
            m_newSegment->setAudioEndTime(m_segment->getAudioEndTime() *
                                          m_ratio);
        }
    }

    m_segment->getComposition()->addSegment(m_newSegment);
    m_segment->getComposition()->detachSegment(m_segment);

//    m_newSegment->setEndMarkerTime
//    (startTime + rescale(m_segment->getEndMarkerTime() - startTime));

    m_detached = true;
}

void
AudioSegmentRescaleCommand::unexecute()
{
    if (m_newSegment) {
        m_newSegment->getComposition()->addSegment(m_segment);
        m_newSegment->getComposition()->detachSegment(m_newSegment);
        m_detached = false;
    }
}

}
