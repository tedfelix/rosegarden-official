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
#include "gui/widgets/ProgressDialog.h"
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
    m_newSegment(0),
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
    m_newSegment(0),
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
AudioSegmentRescaleCommand::connectProgressDialog(ProgressDialog *dlg)
{
    QObject::connect(m_stretcher, SIGNAL(setValue(int)),
					 dlg, SLOT(setValue(int)));
					//dlg->progressBar(), SLOT(setValue(int)));
    // Removed since ProgressDialog::cancelClicked() does not exist.
    //QObject::connect(dlg, SIGNAL(cancelClicked()),
    //                 m_stretcher, SLOT(slotStopTimestretch()));
}
 
void
AudioSegmentRescaleCommand::disconnectProgressDialog(ProgressDialog *dlg)
{
    QObject::disconnect(m_stretcher, SIGNAL(setValue(int)),
						dlg, SLOT(setValue(int)));
						//dlg->progressBar(), SLOT(setValue(int)));
    // Removed since ProgressDialog::cancelClicked() does not exist.
    //QObject::disconnect(dlg, SIGNAL(cancelClicked()),
    //                    m_stretcher, SLOT(slotStopTimestretch()));
}

void
AudioSegmentRescaleCommand::execute()
{
    /* timeT startTime = m_segment->getStartTime(); */

    if (m_segment->getType() != Segment::Audio) {
        return;
    }

    bool failed = false;

    if (!m_newSegment) {

        m_newSegment = m_segment->clone(false);

        std::string label = m_newSegment->getLabel();
        m_newSegment->setLabel(appendLabel(label, qstrtostr(tr("(rescaled)"))));

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

        try {
            m_fid = m_stretcher->getStretchedAudioFile(sourceFileId,
                                                       absoluteRatio);
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
        } catch (SoundFile::BadSoundFileException e) {
            RG_WARNING << "AudioSegmentRescaleCommand: ERROR: BadSoundFileException: "
                      << e.getMessage();
            delete m_newSegment;
            m_newSegment = 0;
            m_fid = -1;
            failed = true;
        } catch (AudioFileManager::BadAudioPathException e) {
            RG_WARNING << "AudioSegmentRescaleCommand: ERROR: BadAudioPathException: "
                      << e.getMessage();
            delete m_newSegment;
            m_newSegment = 0;
            m_fid = -1;
            failed = true;
        } catch (AudioFileTimeStretcher::CancelledException e) {
            RG_WARNING << "AudioSegmentRescaleCommand: ERROR: Rescale cancelled";
            delete m_newSegment;
            m_newSegment = 0;
            m_fid = -1;
            failed = true;
        }
    }

    if (failed) return;

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
