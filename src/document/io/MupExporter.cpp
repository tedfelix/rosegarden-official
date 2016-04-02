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

#define RG_MODULE_STRING "[MupExporter]"

#include "MupExporter.h"

#include "misc/Debug.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/NotationQuantizer.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Sets.h"
#include "base/Track.h"
#include "gui/general/ProgressReporter.h"
#include <QObject>

using std::string;

namespace Rosegarden
{
using namespace BaseProperties;

MupExporter::MupExporter(QObject *parent,
                         Composition *composition,
                         string fileName) :
        ProgressReporter(parent),
        m_composition(composition),
        m_fileName(fileName)
{
    // nothing else
}

MupExporter::~MupExporter()
{
    // nothing
}

bool
MupExporter::write()
{
    Composition *c = m_composition;

    std::ofstream str(m_fileName.c_str(), std::ios::out);
    if (!str) {
        RG_WARNING << "MupExporter::write() - can't write file " << m_fileName;
        return false;
    }

    str << "score\n";
    str << "\tstaffs=" << c->getNbTracks() << "\n";

    int ts = c->getTimeSignatureCount();
    std::pair<timeT, TimeSignature> tspair;
    if (ts > 0)
        tspair = c->getTimeSignatureChange(0);
    str << "\ttime="
    << tspair.second.getNumerator() << "/"
    << tspair.second.getDenominator() << "\n";

    for (int barNo = -1; barNo < c->getNbBars(); ++barNo) {

        for (TrackId trackNo = c->getMinTrackId();
                trackNo <= c->getMaxTrackId(); ++trackNo) {

            if (barNo < 0) {
                writeClefAndKey(str, trackNo);
                continue;
            }

            if (barNo == 0 && trackNo == 0) {
                str << "\nmusic\n";
            }

            str << "\t" << trackNo + 1 << ":";

            Segment *s = 0;
            timeT barStart = c->getBarStart(barNo);
            timeT barEnd = c->getBarEnd(barNo);

            for (Composition::iterator ci = c->begin(); ci != c->end(); ++ci) {
                if ((*ci)->getTrack() == trackNo &&
                        (*ci)->getStartTime() < barEnd &&
                        (*ci)->getEndMarkerTime() > barStart) {
                    s = *ci;
                    break;
                }
            }

            TimeSignature timeSig(c->getTimeSignatureAt(barStart));

            if (!s) {
                // write empty bar
                writeInventedRests(str, timeSig, 0, barEnd - barStart);
                continue;
            }

            if (s->getStartTime() > barStart) {
                writeInventedRests(str, timeSig,
                                   0, s->getStartTime() - barStart);
            }

            // Mup insists that every bar has the correct duration, and won't
            // recover if one goes wrong.  Keep careful tabs on this: it means
            // that for example we have to round chord durations down where
            // the next chord starts too soon
            //!!! we _really_ can't cope with time sig changes yet!

            timeT writtenDuration = writeBar(str, c, s, barStart, barEnd,
                                             timeSig, trackNo);

            if (writtenDuration < timeSig.getBarDuration()) {
                RG_DEBUG << "writtenDuration: " << writtenDuration
                << ", bar duration " << timeSig.getBarDuration()
                << endl;
                writeInventedRests(str, timeSig, writtenDuration,
                                   timeSig.getBarDuration() - writtenDuration);

            } else if (writtenDuration > timeSig.getBarDuration()) {
                RG_WARNING << "WARNING: overfull bar in Mup export: duration " << writtenDuration
                << " into bar of duration " << timeSig.getBarDuration();
                //!!! warn user
            }

            str << "\n";
        }

        if (barNo >= 0)
            str << "bar" << std::endl;
    }

    str << "\n" << std::endl;
    str.close();
    return true;
}

timeT
MupExporter::writeBar(std::ofstream &str,
                      Composition *c,
                      Segment *s,
                      timeT barStart, timeT barEnd,
                      TimeSignature &timeSig,
                      TrackId trackNo)
{
    timeT writtenDuration = 0;
    SegmentNotationHelper helper(*s);
    helper.setNotationProperties();

    long currentGroupId = -1;
    string currentGroupType = "";
    long currentTupletCount = 3;
    bool first = true;
    bool openBeamWaiting = false;

    for (Segment::iterator si =
             SegmentNotationHelper(*s).findNotationAbsoluteTime(barStart);
            s->isBeforeEndMarker(si) &&
            (*si)->getNotationAbsoluteTime() < barEnd; ++si) {

        if ((*si)->isa(Note::EventType)) {

            Chord chord(*s, si, c->getNotationQuantizer());
            Event *e = *chord.getInitialNote();

            timeT absTime = e->getNotationAbsoluteTime();
            timeT duration = e->getNotationDuration();
            try {
                // tuplet compensation, etc
                Note::Type type = e->get<Int>(NOTE_TYPE);
                int dots = e->get
                           <Int>(NOTE_DOTS);
                duration = Note(type, dots).getDuration();
            } catch (Exception e) { // no properties
                RG_WARNING << "WARNING: MupExporter::writeBar: incomplete note properties: " << e.getMessage();
            }

            timeT toNext = duration;
            Segment::iterator nextElt = chord.getFinalElement();
            if (s->isBeforeEndMarker(++nextElt)) {
                toNext = (*nextElt)->getNotationAbsoluteTime() - absTime;
                if (toNext < duration)
                    duration = toNext;
            }

            bool enteringGroup = false;

            if (e->has(BEAMED_GROUP_ID) && e->has(BEAMED_GROUP_TYPE)) {

                long id = e->get
                          <Int>(BEAMED_GROUP_ID);
                string type = e->get
                              <String>(BEAMED_GROUP_TYPE);

                if (id != currentGroupId) {

                    // leave previous group first
                    if (currentGroupId >= 0) {
                        if (!openBeamWaiting)
                            str << " ebm";
                        openBeamWaiting = false;

                        if (currentGroupType == GROUP_TYPE_TUPLED) {
                            str << "; }" << currentTupletCount;
                        }
                    }

                    currentGroupId = id;
                    currentGroupType = type;
                    enteringGroup = true;
                }
            } else {

                if (currentGroupId >= 0) {
                    if (!openBeamWaiting)
                        str << " ebm";
                    openBeamWaiting = false;

                    if (currentGroupType == GROUP_TYPE_TUPLED) {
                        str << "; }" << currentTupletCount;
                    }

                    currentGroupId = -1;
                    currentGroupType = "";
                }
            }

            if (openBeamWaiting)
                str << " bm";
            if (!first)
                str << ";";
            str << " ";

            if (currentGroupType == GROUP_TYPE_TUPLED) {
                e->get
                <Int>(BEAMED_GROUP_UNTUPLED_COUNT, currentTupletCount);
                if (enteringGroup)
                    str << "{ ";
                //!!!		duration = helper.getCompensatedNotationDuration(e);

            }

            writeDuration(str, duration);

            if (toNext > duration && currentGroupType != GROUP_TYPE_TUPLED) {
                writeInventedRests
                (str, timeSig,
                 absTime + duration - barStart, toNext - duration);
            }

            writtenDuration += toNext;

            for (Chord::iterator chi = chord.begin();
                    chi != chord.end(); ++chi) {
                writePitch(str, trackNo, **chi);
            }

            openBeamWaiting = false;
            if (currentGroupType == GROUP_TYPE_BEAMED ||
                    currentGroupType == GROUP_TYPE_TUPLED) {
                if (enteringGroup)
                    openBeamWaiting = true;
            }

            si = chord.getFinalElement();

            first = false;

        } else if ((*si)->isa(Note::EventRestType)) {

            if (currentGroupId >= 0) {

                if (!openBeamWaiting)
                    str << " ebm";
                openBeamWaiting = false;

                if (currentGroupType == GROUP_TYPE_TUPLED) {
                    str << "; }" << currentTupletCount;
                }

                currentGroupId = -1;
                currentGroupType = "";
            }

            if (openBeamWaiting)
                str << " bm";
            if (!first)
                str << ";";
            str << " ";

            writeDuration(str, (*si)->getNotationDuration());
            writtenDuration += (*si)->getNotationDuration();
            str << "r";

            first = false;
            openBeamWaiting = false;

        } // ignore all other sorts of events for now
    }

    if (currentGroupId >= 0) {
        if (!openBeamWaiting)
            str << " ebm";
        openBeamWaiting = false;

        if (currentGroupType == GROUP_TYPE_TUPLED) {
            str << "; }" << currentTupletCount;
        }
    }

    if (openBeamWaiting)
        str << " bm";
    if (!first)
        str << ";";

    return writtenDuration;
}

void
MupExporter::writeClefAndKey(std::ofstream &str, TrackId trackNo)
{
    Composition *c = m_composition;

    for (Composition::iterator i = c->begin(); i != c->end(); ++i) {
        if ((*i)->getTrack() == trackNo) {

            Clef clef((*i)->getClefAtTime((*i)->getStartTime()));
            Rosegarden::Key key((*i)->getKeyAtTime((*i)->getStartTime()));


            str << "staff " << trackNo + 1 << "\n";

            if (clef.getClefType() == Clef::Treble) {
                str << "\tclef=treble\n";
            } else if (clef.getClefType() == Clef::Alto) {
                str << "\tclef=alto\n";
            } else if (clef.getClefType() == Clef::Tenor) {
                str << "\tclef=tenor\n";
            } else if (clef.getClefType() == Clef::Bass) {
                str << "\tclef=bass\n";
            }

            str << "\tkey=" << key.getAccidentalCount()
            << (key.isSharp() ? "#" : "&")
            << (key.isMinor() ? "minor" : "major") << std::endl;

            m_clefKeyMap[trackNo] = ClefKeyPair(clef, key);

            return ;
        }
    }
}

void
MupExporter::writeInventedRests(std::ofstream &str,
                                TimeSignature &timeSig,
                                timeT offset,
                                timeT duration)
{
    str << " ";
    DurationList dlist;
    timeSig.getDurationListForInterval(dlist, duration, offset);
    for (DurationList::iterator i = dlist.begin();
            i != dlist.end(); ++i) {
        writeDuration(str, *i);
        str << "r;";
    }
}

void
MupExporter::writePitch(std::ofstream &str, TrackId trackNo,
                        Event *event)
{
    long pitch = 0;
    if (!event->get
            <Int>(PITCH, pitch)) {
        str << "c"; // have to write something, or it won't parse
        return ;
    }

    Accidental accidental = Accidentals::NoAccidental;
    (void)event->get
    <String>(ACCIDENTAL, accidental);

    // mup octave: treble clef is in octave 4?

    ClefKeyPair ck;
    ClefKeyMap::iterator ckmi = m_clefKeyMap.find(trackNo);
    if (ckmi != m_clefKeyMap.end())
        ck = ckmi->second;

    Pitch p(pitch, accidental);
    Accidental acc(p.getDisplayAccidental(ck.second));
    char note(p.getNoteName(ck.second));
    int octave(p.getOctaveAccidental(-2, acc));

    // just to avoid assuming that the note names returned by Pitch are in
    // the same set as those expected by Mup -- in practice they are the same
    // letters but this changes the case
    str << "cdefgab"[Pitch::getIndexForNote(note)];

    if (acc == Accidentals::DoubleFlat)
        str << "&&";
    else if (acc == Accidentals::Flat)
        str << "&";
    else if (acc == Accidentals::Sharp)
        str << "#";
    else if (acc == Accidentals::DoubleSharp)
        str << "##";
    else if (acc == Accidentals::Natural)
        str << "n";

    str << octave + 1;
}

void
MupExporter::writeDuration(std::ofstream &str, timeT duration)
{
    Note note(Note::getNearestNote(duration, 2));
    int n = Note::Semibreve - note.getNoteType();
    if (n < 0)
        str << "1/" << (1 << ( -n));
    else
        str << (1 << n);
    for (int d = 0; d < note.getDots(); ++d)
        str << ".";
}

}
