/*
    Rosegarden-4 v0.1
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


#ifndef _SEGMENTCOMMANDS_H_
#define _SEGMENTCOMMANDS_H_

#include "klocale.h"

#include "command.h"
#include "Segment.h"
#include "Composition.h"
#include "NotationTypes.h"
#include "rosegardenguidoc.h"


class SegmentEraseCommand : public SegmentCommand
{
public:
    SegmentEraseCommand(Rosegarden::Segment *segment);
    virtual ~SegmentEraseCommand();

    virtual void execute();
    virtual void unexecute();
    
    virtual void getSegments(std::set<Rosegarden::Segment *> &);
    
private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
};


class SegmentInsertCommand : public SegmentCommand
{
public:
    SegmentInsertCommand(Rosegarden::Composition *composition,
                         Rosegarden::TrackId track,
                         Rosegarden::timeT startTime,
                         Rosegarden::timeT duration);
    virtual ~SegmentInsertCommand();

    virtual void execute();
    virtual void unexecute();

    virtual void getSegments(SegmentSet &);
    
private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment     *m_segment;
    int                      m_track;
    Rosegarden::timeT        m_startTime;
    Rosegarden::timeT        m_duration;
};


/**
 * SegmentRecordCommand pretends to insert a Segment that is actually
 * already in the Composition (the currently-being-recorded one).  It's
 * used at the end of recording, to ensure that GUI updates happen
 * correctly, and it provides the ability to undo recording.  (The
 * unexecute does remove the segment, it doesn't just pretend to.)
 */
class SegmentRecordCommand : public SegmentCommand
{
public:
    SegmentRecordCommand(Rosegarden::Segment *segment);
    virtual ~SegmentRecordCommand();

    virtual void execute();
    virtual void unexecute();

    virtual void getSegments(SegmentSet &);

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::Segment *m_segment;
};


/**
 * SegmentReconfigureCommand is a general-purpose command for
 * moving, resizing or changing the track of one or more segments
 */
class SegmentReconfigureCommand : public SegmentCommand
{
public:
    SegmentReconfigureCommand(QString name);
    virtual ~SegmentReconfigureCommand();

    void addSegment(Rosegarden::Segment *segment,
                    Rosegarden::timeT startTime,
                    Rosegarden::timeT duration,
                    Rosegarden::TrackId track);

    void execute();
    void unexecute();

    virtual void getSegments(SegmentSet &);

private:
    struct SegmentRec {
        Rosegarden::Segment *segment;
        Rosegarden::timeT startTime;
        Rosegarden::timeT duration;
        Rosegarden::TrackId track;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;
    SegmentRecSet m_records;
    void swap();
};


class SegmentSplitCommand : public SegmentCommand
{
public:
    SegmentSplitCommand(Rosegarden::Segment *segment,
                        Rosegarden::timeT splitTime);
    virtual ~SegmentSplitCommand();

    virtual void execute();
    virtual void unexecute();

    virtual void getSegments(SegmentSet &);

private:
    Rosegarden::Segment *m_segment;
    Rosegarden::Segment *m_newSegment;
    Rosegarden::timeT m_splitTime;
};


class SegmentChangeQuantizationCommand : public SegmentCommand
{
public:
    /// Set quantization on segments.  If sq is null, switch quantization off.
    SegmentChangeQuantizationCommand(Rosegarden::StandardQuantization *sq);
    virtual ~SegmentChangeQuantizationCommand();

    void addSegment(Rosegarden::Segment *s);

    virtual void execute();
    virtual void unexecute();

    virtual void getSegments(SegmentSet &);

    static QString name(Rosegarden::StandardQuantization *sq);

private:
    struct SegmentRec {
        Rosegarden::Segment *segment;
        Rosegarden::Quantizer *oldQuantizer;
        bool wasQuantized;
    };
    typedef std::vector<SegmentRec> SegmentRecSet;
    SegmentRecSet m_records;

    Rosegarden::StandardQuantization *m_quantization;
};


class AddTimeSignatureCommand : public TimeAndTempoChangeCommand
{
public:
    AddTimeSignatureCommand(Rosegarden::Composition *composition,
                            Rosegarden::timeT time,
                            Rosegarden::TimeSignature timeSig) :
        TimeAndTempoChangeCommand(name()),
        m_composition(composition),
        m_time(time),
        m_timeSignature(timeSig),
        m_oldTimeSignature(0) { }
    virtual ~AddTimeSignatureCommand();

    static QString name() {
        return i18n("Add &Time Signature Change...");
    }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::timeT m_time;
    Rosegarden::TimeSignature m_timeSignature;
    Rosegarden::TimeSignature *m_oldTimeSignature; // for undo
    int m_timeSigIndex; // for undo
};    

class AddTempoChangeCommand : public TimeAndTempoChangeCommand
{
public:
    AddTempoChangeCommand(Rosegarden::Composition *composition,
                          Rosegarden::timeT time,
                          double tempo):
        TimeAndTempoChangeCommand(name()),
        m_composition(composition),
        m_time(time),
        m_tempo(tempo) {}

    static QString name()
    {
        return i18n("Add Tempo Change...");
    }
    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    Rosegarden::timeT m_time;
    double m_tempo;
};


class AddTracksCommand : public Command
{
public:
    AddTracksCommand(Rosegarden::Composition *composition,
                     unsigned int nbTracks): 
        Command(name()),
        m_composition(composition),
        m_nbNewTracks(nbTracks) {}

    static QString name()
    {
        return i18n("Add Tracks...");
    }
    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Composition *m_composition;
    unsigned int m_nbNewTracks;
};



#endif  // _SEGMENTCOMMANDS_H_
