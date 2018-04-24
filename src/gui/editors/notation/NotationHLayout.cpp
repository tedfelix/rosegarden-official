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

#define RG_MODULE_STRING "[NotationHLayout]"
#define RG_NO_DEBUG_PRINT 1

#include "NotationHLayout.h"

#include "base/Composition.h"
#include "base/LayoutEngine.h"
#include "base/NotationQuantizer.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/ViewElement.h"
#include "base/ViewSegment.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/guitar/Chord.h"
#include "gui/editors/notation/ClefKeyContext.h"
#include "gui/editors/notation/NotationChord.h"
#include "gui/editors/notation/NotationElement.h"
#include "gui/editors/notation/NotationGroup.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NotationScene.h"
#include "gui/editors/notation/NotationStaff.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"

#include <QApplication>
#include <QSettings>
#include <QObject>

#include <cmath>
#include <limits>

namespace Rosegarden
{

using namespace BaseProperties;


NotationHLayout::NotationHLayout(Composition *c, NotePixmapFactory *npf,
                                 const NotationProperties &properties,
                                 QObject* parent) :
    HorizontalLayoutEngine(c),
    m_totalWidth(0.),
    m_pageMode(false),
    m_pageWidth(0.),
    m_spacing(100),
    m_proportion(60),
    m_keySigCancelMode(1),
    m_hideRedundance(true),
    m_npf(npf),
    m_notationQuantizer(c->getNotationQuantizer()),
    m_properties(properties),
    m_timePerProgressIncrement(0),
    m_staffCount(0),
    m_scene(static_cast<NotationScene *>(parent))
{
    //    RG_DEBUG << "NotationHLayout()";

    QSettings settings;
    settings.beginGroup(NotationOptionsConfigGroup);
    m_keySigCancelMode = settings.value("keysigcancelmode", 1).toInt() ;
    m_hideRedundance = settings.value("hideredundantclefkey", "true").toBool();
    settings.endGroup();

    settings.beginGroup(NotationViewConfigGroup);
    m_showRepeated =  settings.value("showrepeated", true).toBool();
    m_distributeVerses =  settings.value("distributeverses", true).toBool();
    settings.endGroup();
}

NotationHLayout::~NotationHLayout()
{
    // empty
}

std::vector<int>
NotationHLayout::getAvailableSpacings()
{
    if (m_availableSpacings.empty()) {
        m_availableSpacings.push_back(30);
        m_availableSpacings.push_back(60);
        m_availableSpacings.push_back(85);
        m_availableSpacings.push_back(100);
        m_availableSpacings.push_back(130);
        m_availableSpacings.push_back(170);
        m_availableSpacings.push_back(220);
        m_availableSpacings.push_back(250);
        m_availableSpacings.push_back(300);
        m_availableSpacings.push_back(350);
        m_availableSpacings.push_back(400);
    }
    return m_availableSpacings;
}

std::vector<int>
NotationHLayout::getAvailableProportions()
{
    if (m_availableProportions.empty()) {
        m_availableProportions.push_back(0);
        m_availableProportions.push_back(20);
        m_availableProportions.push_back(40);
        m_availableProportions.push_back(60);
        m_availableProportions.push_back(80);
        m_availableProportions.push_back(100);
    }
    return m_availableProportions;
}

NotationHLayout::BarDataList &
NotationHLayout::getBarData(ViewSegment &staff)
{
    BarDataMap::iterator i = m_barData.find(&staff);
    if (i == m_barData.end()) {
        m_barData[&staff] = BarDataList();
    }

    return m_barData[&staff];
}

const NotationHLayout::BarDataList &
NotationHLayout::getBarData(ViewSegment &staff) const
{
    return ((NotationHLayout *)this)->getBarData(staff);
}

NotationElementList::iterator
NotationHLayout::getStartOfQuantizedSlice(NotationElementList *notes,
                                          timeT t) const
{
    NotationElementList::iterator i = notes->findTime(t);
    NotationElementList::iterator j(i);

    while (true) {
        if (i == notes->begin())
            return i;
        --j;
        if ((*j)->getViewAbsoluteTime() < t)
            return i;
        i = j;
    }
}

NotePixmapFactory *
NotationHLayout::getNotePixmapFactory(ViewSegment &staff)
{
    NotationStaff *ns = dynamic_cast<NotationStaff *>(&staff);
    if (ns) return &ns->getNotePixmapFactory(false);
    else return 0;
}

NotePixmapFactory *
NotationHLayout::getGraceNotePixmapFactory(ViewSegment &staff)
{
    NotationStaff *ns = dynamic_cast<NotationStaff *>(&staff);
    if (ns) return &ns->getNotePixmapFactory(true);
    else return 0;
}

void
NotationHLayout::scanViewSegment(ViewSegment &staff, timeT startTime,
                                 timeT endTime, bool full)
{
    //throwIfCancelled();
    Profiler profiler("NotationHLayout::scanViewSegment");

    Segment &segment(staff.getSegment());
    timeT segStartTime = segment.getStartTime();
    timeT segEndTime = segment.getEndMarkerTime();

    int startBarOfViewSegment = getComposition()->getBarNumber(segment.getStartTime());

    if (full) {
        clearBarList(staff);
        startTime = segStartTime;
        endTime = segEndTime;
    } else {
        // Time must be limited to values inside segment to avoid the extension
        // of the staff toward the start of the composition (experienced with
        // linked segments) with various effects as the display of an
        // unnecessary time signature at a wrong place.
        startTime = getComposition()->getBarStartForTime(startTime);
        endTime = getComposition()->getBarEndForTime(endTime);
        if (segStartTime > startTime) startTime = segStartTime;
        if (segEndTime < endTime) endTime = segEndTime;
    }

    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));

    NotePixmapFactory *npf = getNotePixmapFactory(staff);

    int startBarNo = getComposition()->getBarNumber(startTime);
    int endBarNo = getComposition()->getBarNumber(endTime);
    /*
        if (endBarNo > startBarNo &&
        getComposition()->getBarStart(endBarNo) == segment.getEndMarkerTime()) {
        --endBarNo;
        }
    */
    TrackId trackId = segment.getTrack();
    std::string name =
        segment.getComposition()->getTrackById(trackId)->getLabel();
    m_staffNameWidths[&staff] =
        npf->getNoteBodyWidth() * 2 +
        npf->getTextWidth(Text(name, Text::StaffName));

    RG_DEBUG << "scanViewSegment: full scan " << full << ", times " << startTime << "->" << endTime << ", bars " << startBarNo << "->" << endBarNo << ", staff name \"" << segment.getLabel() << "\", width " << m_staffNameWidths[&staff];

    SegmentNotationHelper helper(segment);
    if (full) {
        helper.setNotationProperties();
    } else {
        helper.setNotationProperties(startTime, endTime);
    }

    ::Rosegarden::Key key = segment.getKeyAtTime(startTime);
    Clef clef = segment.getClefAtTime(startTime);
    TimeSignature timeSignature =
        getComposition()->getTimeSignatureAt(startTime);
    float timeSigWidth = npf->getNoteBodyWidth() +
                            npf->getTimeSigWidth(timeSignature);
    bool barCorrect = true;

    int ottavaShift = 0;
    timeT ottavaEnd = segEndTime;

    if (full) {

        RG_DEBUG << "full scan: setting haveOttava false";

        m_haveOttavaSomewhere[&staff] = false;

    } else if (m_haveOttavaSomewhere[&staff]) {

        RG_DEBUG << "not full scan but ottava is listed";

        Segment::iterator i = segment.findTime(startTime);
        while (1) {
            if ((*i)->isa(Indication::EventType)) {
                try {
                    Indication indication(**i);
                    if (indication.isOttavaType()) {
                        ottavaShift = indication.getOttavaShift();
                        ottavaEnd = (*i)->getAbsoluteTime() +
                                    indication.getIndicationDuration();
                        break;
                    }
                } catch (...) { }
            }
            if (i == segment.begin())
                break;
            --i;
        }
    }

    RG_DEBUG << "ottava shift at start:" << ottavaShift << ", ottavaEnd " << ottavaEnd;

    QSettings settings;
    settings.beginGroup(NotationOptionsConfigGroup);

    int accOctaveMode = settings.value("accidentaloctavemode", 1).toInt() ;
    AccidentalTable::OctaveType octaveType =
        (accOctaveMode == 0 ? AccidentalTable::OctavesIndependent :
         accOctaveMode == 1 ? AccidentalTable::OctavesCautionary :
         AccidentalTable::OctavesEquivalent);

    int accBarMode = settings.value("accidentalbarmode", 0).toInt() ;
    AccidentalTable::BarResetType barResetType =
        (accBarMode == 0 ? AccidentalTable::BarResetNone :
         accBarMode == 1 ? AccidentalTable::BarResetCautionary :
         AccidentalTable::BarResetExplicit);

    bool showInvisibles = qStrToBool( settings.value("showinvisibles", "true" ) ) ;
    settings.endGroup();

    if (barResetType != AccidentalTable::BarResetNone) {
        //!!! very crude and expensive way of making sure we see the
        // accidentals from previous bar:
        if (startBarNo > segment.getComposition()->getBarNumber(segment.getStartTime())) {
            --startBarNo;
        }
    }

    AccidentalTable accTable(key, clef, octaveType, barResetType);

    for (int barNo = startBarNo; barNo <= endBarNo; ++barNo) {

        std::pair<timeT, timeT> barTimes =
            getComposition()->getBarRange(barNo);

        if (barTimes.first >= segment.getEndMarkerTime()) {
            // clear data if we have any old stuff
            BarDataList::iterator i(barList.find(barNo));
            if (i != barList.end()) {
                barList.erase(i);
            }
            continue; // so as to erase any further bars next time around
        }

        NotationElementList::iterator from =
            getStartOfQuantizedSlice(notes, barTimes.first);

        RG_DEBUG << "getStartOfQuantizedSlice returned " <<
        (from != notes->end() ? (*from)->getViewAbsoluteTime() : -1)
        << " from " << barTimes.first;

        NotationElementList::iterator to =
            getStartOfQuantizedSlice(notes, barTimes.second);

        if (barTimes.second >= segment.getEndMarkerTime()) {
            to = notes->end();
        }

        bool newTimeSig = false;
        timeSignature = getComposition()->getTimeSignatureInBar
                        (barNo, newTimeSig);
        RG_DEBUG << "bar " << barNo << ", startBarOfViewSegment " << startBarOfViewSegment
                       << ", newTimeSig " << newTimeSig;

        // When bar is the first one in a segment, the delay computed here
        // is the difference between event position in bar and event position
        // in segment.
        timeT segDelay = (barNo == startBarOfViewSegment) ?
            segStartTime - getComposition()->getBarStart(barNo) : 0;

        setBarBasicData(staff, barNo, from, barCorrect, timeSignature,
                        newTimeSig, segDelay, trackId);
        BarDataList::iterator bdli(barList.find(barNo));
        bdli->second.layoutData.needsLayout = true;

        ChunkList &chunks = bdli->second.chunks;
        chunks.clear();

        float lyricWidth = 0;
        // int graceCount = 0;

        typedef std::set
            <long> GroupIdSet;
        GroupIdSet groupIds;

        RG_DEBUG << "scanViewSegment: bar " << barNo << ", from " << barTimes.first << ", to " << barTimes.second << " (end " << segment.getEndMarkerTime() << "); from is at " << (from == notes->end() ? -1 : (*from)->getViewAbsoluteTime()) << ", to is at " << (to == notes->end() ? -1 : (*to)->getViewAbsoluteTime());

        timeT actualBarEnd = barTimes.first;

        accTable.newBar();

        for (NotationElementList::iterator itr = from; itr != to; ++itr) {

            NotationElement *el = static_cast<NotationElement*>((*itr));
            RG_DEBUG << "element is a " << el->event()->getType();

            if (ottavaShift != 0) {
                if (el->event()->getAbsoluteTime() >= ottavaEnd) {
                    RG_DEBUG << "reached end of ottava";
                    ottavaShift = 0;
                }
            }
            
            // Clefs and key signatures must always be memorized here (even
            // when they are invisible) as the way other elements are displayed
            // may depend from them.
            Key oldKey;
            if (el->event()->isa(Clef::EventType)) {
                clef = Clef(*el->event());
                accTable.newClef(clef);
            } else if (el->event()->isa(::Rosegarden::Key::EventType)) {
                oldKey = key;
                key = ::Rosegarden::Key(*el->event());
                accTable = AccidentalTable
                           (key, clef, octaveType, barResetType);
            }

            bool invisible = false;
            if (el->event()->get<Bool>(INVISIBLE, invisible) && invisible) {
                if (!showInvisibles)
                    continue;
            }
            if (m_hideRedundance &&
                m_scene->isEventRedundant(el->event(), segment)) continue;

            if (el->event()->has(BEAMED_GROUP_ID)) {
                RG_DEBUG << "element is beamed";
                long groupId = el->event()->get<Int>(BEAMED_GROUP_ID);
                if (groupIds.find(groupId) == groupIds.end()) {
                    RG_DEBUG << "it's a new beamed group, applying stem properties";
                    NotationGroup group(*staff.getViewElementList(),
                                        itr,
                                        m_notationQuantizer,
                                        barTimes,
                                        m_properties,
                                        clef, key);
                    group.applyStemProperties();
                    groupIds.insert(groupId);
                }
            }

            if (el->event()->isa(Clef::EventType)) {

                //              RG_DEBUG << "Found clef";
                chunks.push_back(Chunk(el->event()->getSubOrdering(),
                                       getLayoutWidth(*el, npf, key)));

            } else if (el->event()->isa(::Rosegarden::Key::EventType)) {

                //              RG_DEBUG << "Found key";
                chunks.push_back(Chunk(el->event()->getSubOrdering(),
                                       getLayoutWidth(*el, npf, oldKey)));

            } else if (el->event()->isa(Text::EventType)) {

                bool isLyric = el->event()->has(Text::TextTypePropertyName) &&
                    el->event()->get<String>(Text::TextTypePropertyName) ==
                    Text::Lyric;
                bool verseOk = el->event()->has(Text::LyricVersePropertyName) &&
                    el->event()->get<Int>(Text::LyricVersePropertyName) ==
                    segment.getVerseWrapped();

                // the only text events of interest are lyrics, which
                // contribute to a fixed area following the next chord

                if (isLyric && (!m_distributeVerses || (m_distributeVerses && verseOk))) {
                    lyricWidth = std::max
                        (lyricWidth, float(npf->getTextWidth(Text(*el->event()))));
                    RG_DEBUG << "Setting lyric width to " << lyricWidth
                                   << " for text " << el->event()->get<String>(Text::TextPropertyName);
                }

                chunks.push_back(Chunk(el->event()->getSubOrdering(), 0));

            } else if (el->isNote()) {

                NotePixmapFactory *cnpf = npf;
                if (el->isGrace()) cnpf = getGraceNotePixmapFactory(staff);

                scanChord(notes, itr, clef, key, accTable,
                          lyricWidth, chunks, cnpf, ottavaShift, to);

            } else if (el->isRest()) {

                chunks.push_back(Chunk(el->getViewDuration(),
                                       el->event()->getSubOrdering(),
                                       0,
                                       getLayoutWidth(*el, npf, key)));

            } else if (el->event()->isa(Indication::EventType)) {

                //              RG_DEBUG << "Found indication";

                chunks.push_back(Chunk(el->event()->getSubOrdering(), 0));

                try {
                    Indication indication(*el->event());
                    if (indication.isOttavaType()) {
                        ottavaShift = indication.getOttavaShift();
                        ottavaEnd = el->event()->getAbsoluteTime() +
                                    indication.getIndicationDuration();
                        m_haveOttavaSomewhere[&staff] = true;
                    }
                } catch (...) {
                    RG_DEBUG << "Bad indication!";
                }

            } else {

//                RG_DEBUG << "Found something I don't know about (type is " << el->event()->getType() << ")";
                chunks.push_back(Chunk(el->event()->getSubOrdering(),
                                       getLayoutWidth(*el, npf, key)));
            }

            // If the last event in the bar is a controller or pitch bend,
            // ignore it when calculating actualBarEnd.  This fixes a very old
            // bug whereby inserting a controller into an empty bar would turn
            // the barline red.
            if (!(el->event()->isa(Controller::EventType)) && !(el->event()->isa(PitchBend::EventType))) {
                actualBarEnd = el->getViewAbsoluteTime() + el->getViewDuration();
            }
        }

//        RG_DEBUG << "barTimes.first: " << barTimes.first << " .second: " << barTimes.second << " actualBarEnd: " << actualBarEnd;
        if (actualBarEnd == barTimes.first) actualBarEnd = barTimes.second;
        barCorrect = (actualBarEnd == barTimes.second);
        setBarSizeData(staff, barNo, 0.0,
                       timeSigWidth, actualBarEnd - barTimes.first);

        //if ((endTime > startTime) && (barNo % 20 == 0)) {
        //    emit setValue((barTimes.second - startTime) * 95 /
        //                     (endTime - startTime));
        //}

        //throwIfCancelled();
    }
    /*
        BarDataList::iterator ei(barList.end());
        while (ei != barList.begin() && (--ei)->first > endBarNo) {
        barList.erase(ei);
        ei = barList.end();
        }
    */
}

void
NotationHLayout::clearBarList(ViewSegment &staff)
{
    BarDataList &bdl = m_barData[&staff];
    bdl.clear();
}

void
NotationHLayout::setBarBasicData(ViewSegment &staff,
                                 int barNo,
                                 NotationElementList::iterator start,
                                 bool correct,
                                 TimeSignature timeSig,
                                 bool newTimeSig,
                                 timeT segDelay,
                                 TrackId trackId
                                )
{
    //    RG_DEBUG << "setBarBasicData for " << barNo;

    BarDataList &bdl(m_barData[&staff]);

    BarDataList::iterator i(bdl.find(barNo));
    if (i == bdl.end()) {
        NotationElementList::iterator endi = staff.getViewElementList()->end();
        bdl.insert(BarDataPair(barNo, BarData(endi, true,
                                              TimeSignature(), false)));
        i = bdl.find(barNo);
    }

    i->second.basicData.start = start;
    i->second.basicData.correct = correct;
    i->second.basicData.timeSignature = timeSig;
    i->second.basicData.newTimeSig = newTimeSig;
    i->second.basicData.delayInBar = segDelay;
    i->second.basicData.trackId = trackId;
}

void
NotationHLayout::setBarSizeData(ViewSegment &staff,
                                int barNo,
                                float fixedWidth,
                                float timeSigFixedWidth,
                                timeT actualDuration)
{
    //    RG_DEBUG << "setBarSizeData for " << barNo;

    BarDataList &bdl(m_barData[&staff]);

    BarDataList::iterator i(bdl.find(barNo));
    if (i == bdl.end()) {
        NotationElementList::iterator endi = staff.getViewElementList()->end();
        bdl.insert(BarDataPair(barNo, BarData(endi, true,
                                              TimeSignature(), false)));
        i = bdl.find(barNo);
    }

    i->second.sizeData.actualDuration = actualDuration;
    i->second.sizeData.idealWidth = 0.0;
    i->second.sizeData.reconciledWidth = 0.0;
    i->second.sizeData.clefKeyWidth = 0;
    i->second.sizeData.fixedWidth = fixedWidth;
    i->second.sizeData.timeSigFixedWidth = timeSigFixedWidth;
}

void
NotationHLayout::scanChord(NotationElementList *notes,
                           NotationElementList::iterator &itr,
                           const Clef &clef,
                           const ::Rosegarden::Key &key,
                           AccidentalTable &accTable,
                           float &lyricWidth,
                           ChunkList &chunks,
                           NotePixmapFactory *npf,
                           int ottavaShift,
                           NotationElementList::iterator &to)
{
    NotationChord chord(*notes, itr, m_notationQuantizer, m_properties);
    Accidental someAccidental = Accidentals::NoAccidental;
    bool someCautionary = false;
    bool barEndsInChord = false;
    bool grace = false;

//    RG_DEBUG << "NotationHLayout::scanChord: "
//              << chord.size() << "-voice chord at "
//              << (*itr)->event()->getAbsoluteTime()
//              << " unquantized, "
//              << (*itr)->getViewAbsoluteTime()
//              << " quantized";

//    RG_DEBUG << "Contents:";

    /*
        for (NotationElementList::iterator i = chord.getInitialElement();
         i != notes->end(); ++i) {
        (*i)->event()->dump(std::cerr);
        if (i == chord.getFinalElement()) break;
        }
    */
    // We don't need to get the chord's notes in pitch order here,
    // but we do need to ensure we see any random non-note events
    // that may crop up in the middle of it.

    for (NotationElementList::iterator i = chord.getInitialElement();
         i != notes->end(); ++i) {

        NotationElement *el = static_cast<NotationElement*>(*i);
        if (el->isRest()) {
            el->event()->setMaybe<Bool>(m_properties.REST_TOO_SHORT, true);
            if (i == chord.getFinalElement())
                break;
            continue;
        }

        if (el->isGrace()) {
            grace = true;
        }

        long pitch = 64;
        if (!el->event()->get<Int>(PITCH, pitch)) {
            RG_DEBUG <<
            "WARNING: NotationHLayout::scanChord: couldn't get pitch for element, using default pitch of " << pitch;
        }

        Accidental explicitAccidental = Accidentals::NoAccidental;
        (void)el->event()->get<String>(ACCIDENTAL, explicitAccidental);

        Pitch p(pitch, explicitAccidental);
        int h = p.getHeightOnStaff(clef, key);
        Accidental acc = p.getDisplayAccidental(key);

        h -= 7 * ottavaShift;

        el->event()->setMaybe<Int>(NotationProperties::OTTAVA_SHIFT, ottavaShift);
        el->event()->setMaybe<Int>(NotationProperties::HEIGHT_ON_STAFF, h);
        el->event()->setMaybe<String>(m_properties.CALCULATED_ACCIDENTAL, acc);

        // update display acc for note according to the accTable
        // (accidentals in force when the last chord ended) and tell
        // accTable about accidentals from this note.

        bool cautionary = false;
        if (el->event()->has(m_properties.USE_CAUTIONARY_ACCIDENTAL)) {
            cautionary = el->event()->get<Bool>(m_properties.USE_CAUTIONARY_ACCIDENTAL);
        }
        Accidental dacc = accTable.processDisplayAccidental(acc, h, cautionary);
        el->event()->setMaybe<String>(m_properties.DISPLAY_ACCIDENTAL, dacc);
        el->event()->setMaybe<Bool>(m_properties.DISPLAY_ACCIDENTAL_IS_CAUTIONARY,
                                    cautionary);
        if (cautionary) {
            someCautionary = true;
        }

        if (someAccidental == Accidentals::NoAccidental)
            someAccidental = dacc;

        if (i == to)
            barEndsInChord = true;

        if (i == chord.getFinalElement())
            break;
    }

    // tell accTable the chord has ended, so to bring its accidentals
    // into force for future chords
    accTable.update();

    chord.applyAccidentalShiftProperties();

    float extraWidth = 0;

    if (someAccidental != Accidentals::NoAccidental) {
        bool extraShift = false;
        int shift = chord.getMaxAccidentalShift(extraShift);
        int e = npf->getAccidentalWidth(someAccidental, shift, extraShift);
        if (someAccidental != Accidentals::Sharp) {
            e = std::max(e, npf->getAccidentalWidth(Accidentals::Sharp, shift, extraShift));
        }
        if (someCautionary) {
            e += npf->getNoteBodyWidth();
        }
        extraWidth += e;
    }

    float layoutExtra = 0;
    if (chord.hasNoteHeadShifted()) {
        if (chord.hasStemUp()) {
            layoutExtra += npf->getNoteBodyWidth();
        } else {
            extraWidth = std::max(extraWidth, float(npf->getNoteBodyWidth()));
        }
    }
/*!!!
    if (grace) {
        std::cerr << "Grace note: subordering " << chord.getSubOrdering();
        chunks.push_back(Chunk(-10 + graceCount,
                               extraWidth + npf->getNoteBodyWidth()));
        if (graceCount < 9) ++graceCount;
        return;
    } else {
        std::cerr << "Non-grace note (grace count was " << graceCount << ")";
        graceCount = 0;
    }
*/
    NotationElementList::iterator myLongest = chord.getLongestElement();
    if (myLongest == notes->end()) {
        RG_DEBUG << "WARNING: NotationHLayout::scanChord: No longest element in chord!";
    }

    timeT d = (*myLongest)->getViewDuration();

    RG_DEBUG << "Lyric width is " << lyricWidth;

    if (grace) {
        chunks.push_back(Chunk(d, chord.getSubOrdering(),
                               extraWidth + layoutExtra
                               + getLayoutWidth(**myLongest, npf, key)
                               - npf->getNoteBodyWidth(), // tighten up
                               0));
    } else {
        chunks.push_back(Chunk(d, 0, extraWidth,
                               std::max(layoutExtra +
                                        getLayoutWidth(**myLongest, npf, key),
                                        lyricWidth)));
        lyricWidth = 0;
    }

    itr = chord.getFinalElement();
    if (barEndsInChord) {
        to = itr;
        ++to;
    }
}

struct ChunkLocation {
    timeT time;
    short subordering;
    ChunkLocation(timeT t, short s) : time(t), subordering(s) { }
};

bool operator<(const ChunkLocation &l0, const ChunkLocation &l1) {
    return ((l0.time < l1.time) ||
            ((l0.time == l1.time) && (l0.subordering < l1.subordering)));
}



void
NotationHLayout::preSquishBar(int barNo)
{
    typedef std::vector<Chunk *> ChunkRefList;
    typedef std::map<ChunkLocation, ChunkRefList> ColumnMap;
    static ColumnMap columns;
    bool haveSomething = false;

    typedef std::vector<BarData *> BarDataVector;
    typedef std::map<TrackTimeSig, BarDataVector> TimeSigMap;
    static TimeSigMap timeSigMap;

    columns.clear();
    timeSigMap.clear();

    for (BarDataMap::iterator mi = m_barData.begin();
            mi != m_barData.end(); ++mi) {

        BarDataList &bdl = mi->second;
        BarDataList::iterator bdli = bdl.find(barNo);

        if (bdli != bdl.end()) {

            haveSomething = true;
            ChunkList &cl(bdli->second.chunks);

            // Delay between start of bar and start of segment have to be
            // added to event durations to avoid wrong position of notes
            // when a segment is not precisely beginning at a start of a bar.
            // This fixes the "anacrusis problem".

            timeT aggregateTime = bdli->second.basicData.delayInBar;
            for (ChunkList::iterator cli = cl.begin(); cli != cl.end(); ++cli) {

                // Subordering is typically zero for notes, positive
                // for rests and negative for other stuff.  We want to
                // handle notes and rests together, but not the others.

                int subordering = cli->subordering;
                if (subordering > 0)
                    subordering = 0;

                columns[ChunkLocation(aggregateTime, subordering)].push_back(&(*cli));

                aggregateTime += cli->duration;
            }

            // Sometimes, two time signatures may be displayed in the same bar.
            // Following code should keep only one of them.
            //    - If the two time signatures are different, don't do anything
            //    - Else keep the first one

            // Elimination of redundant time signature in the same bar - Step 1:
            // Remember the time bar, keyed with time signature and track Id in
            // timeSigMap
            if (bdli->second.basicData.newTimeSig
                    && !bdli->second.basicData.timeSignature.isHidden()) {
                TrackTimeSig tts
                    = TrackTimeSig(bdli->second.basicData.trackId,
                                   bdli->second.basicData.timeSignature);
                timeSigMap[tts].push_back(&(bdli->second));
            }
        }
    }

    if (!haveSomething)
        return ;

    // Elimination of redundant time signature in the same bar - Step 2:
    // Scan every memorized bar and when several time signatures (i.e. several
    // bars with the same keys in TimeSigMap) are found keep only one whose
    // segment exists the soonest after the start of the bar.

    // Walk through the "track and time signature" keys
    for (TimeSigMap::iterator
            i = timeSigMap.begin(); i != timeSigMap.end(); ++i) {

        // If only one bar keep its time signature and update fixedWidth
        if (i->second.size() == 1) {
            BarData *dataPtr = *(i->second.begin());
            dataPtr->sizeData.fixedWidth = dataPtr->sizeData.timeSigFixedWidth;
            continue;
        }

        // else walk through the bars and find the first segment to exist
        // (i.e. with the smallest basicData.delayInBar value)
        timeT delay = std::numeric_limits<timeT>::max();
        BarData * dataPtr = 0;
        for (BarDataVector::iterator
            j = i->second.begin(); j != i->second.end(); ++j) {

            // Hide all the time signatures
            (*j)->basicData.timeSignature.setHidden(true);  

            // Remember the smallest delayInBar and the associated barData
            if ((*j)->basicData.delayInBar < delay) {
                delay = (*j)->basicData.delayInBar;
                dataPtr = *j;
            }
        }

        // Set visible again the time sig of the selected bar and update
        // the bar fixed width
        dataPtr->basicData.timeSignature.setHidden(false);
        dataPtr->sizeData.fixedWidth += dataPtr->sizeData.timeSigFixedWidth;
    }

    // now modify chunks in-place

    // What we want to do here is idle along the whole set of chunk
    // lists, inspecting all the chunks that occur at each moment in
    // turn and choosing a "rate" from the "slowest" of these
    // (i.e. most space per time)

    float x = 0.0;
    timeT prevTime = 0;
    double prevRate = 0.0;
    float maxStretchy = 0.0;

    RG_DEBUG << "preSquishBar(" << barNo << "): have "
    << columns.size() << " columns";

    for (ColumnMap::iterator i = columns.begin(); i != columns.end(); ++i) {

        timeT time = i->first.time;
        ChunkRefList &list = i->second;

        RG_DEBUG << "preSquishBar: "
        << "column at " << time << " : " << i->first.subordering;


        double minRate = -1.0;
        float totalFixed = 0.0;
        maxStretchy = 0.0;

        for (ChunkRefList::iterator j = list.begin(); j != list.end(); ++j) {
            if ((*j)->stretchy > 0.0) {
                double rate = (*j)->duration / (*j)->stretchy; // time per px
                RG_DEBUG << "preSquishBar: rate " << rate;
                if (minRate < 0.0 || rate < minRate)
                    minRate = rate;
            } else {
                RG_DEBUG << "preSquishBar: not stretchy";
            }

            maxStretchy = std::max(maxStretchy, (*j)->stretchy);
            totalFixed = std::max(totalFixed, (*j)->fixed);
        }

        RG_DEBUG << "preSquishBar: minRate " << minRate << ", maxStretchy " << maxStretchy << ", totalFixed " << totalFixed;

        // we have the rate from this point, but we want to assign
        // these elements an x coord based on the rate and distance
        // from the previous point, plus fixed space for this point
        // if it's a note (otherwise fixed space goes afterwards)

        if (i->first.subordering == 0)
            x += totalFixed;
        if (prevRate > 0.0)
            x += (time - prevTime) / prevRate;

        for (ChunkRefList::iterator j = list.begin(); j != list.end(); ++j) {
            RG_DEBUG << "Setting x for time " << time << " to " << x << " in chunk at " << *j;
            (*j)->x = x;
        }

        if (i->first.subordering != 0)
            x += totalFixed;

        prevTime = time;
        prevRate = minRate;
    }

    x += maxStretchy;

    for (BarDataMap::iterator mi = m_barData.begin();
            mi != m_barData.end(); ++mi) {

        BarDataList &bdl = mi->second;
        BarDataList::iterator bdli = bdl.find(barNo);
        if (bdli != bdl.end()) {

            bdli->second.sizeData.idealWidth =
                bdli->second.sizeData.fixedWidth + x;

            if (!bdli->second.basicData.timeSignature.hasHiddenBars()) {
                bdli->second.sizeData.idealWidth += getBarMargin();
            } else if (bdli->second.basicData.newTimeSig) {
                bdli->second.sizeData.idealWidth += getPostBarMargin();
            }

            bdli->second.sizeData.reconciledWidth =
                bdli->second.sizeData.idealWidth;

            bdli->second.layoutData.needsLayout = true;
        }
    }
}

ViewSegment *
NotationHLayout::getViewSegmentWithWidestBar(int barNo)
{
    float maxWidth = -1;
    ViewSegment *widest = 0;

    for (BarDataMap::iterator mi = m_barData.begin();
            mi != m_barData.end(); ++mi) {

        BarDataList &list = mi->second;
        BarDataList::iterator li = list.find(barNo);
        if (li != list.end()) {

            RG_DEBUG << "getViewSegmentWithWidestBar: idealWidth is " << li->second.sizeData.idealWidth;

            if (li->second.sizeData.idealWidth == 0.0) {
                RG_DEBUG << "getViewSegmentWithWidestBar(" << barNo << "): found idealWidth of zero, presquishing";
                preSquishBar(barNo);
            }

            if (li->second.sizeData.idealWidth > maxWidth) {
                maxWidth = li->second.sizeData.idealWidth;
                widest = mi->first;
            }
        }
    }

    return widest;
}

int
NotationHLayout::getMaxRepeatedClefAndKeyWidth(int barNo)
{
    int max = 0;

    timeT barStart = 0;

    for (BarDataMap::iterator mi = m_barData.begin();
            mi != m_barData.end(); ++mi) {

        ViewSegment *staff = mi->first;
        if (mi == m_barData.begin()) {
            barStart = staff->getSegment().getComposition()->getBarStart(barNo);
        }

        timeT t;
        int w = 0;

        Clef clef = staff->getSegment().getClefAtTime(barStart, t);
        if (t < barStart)
            w += m_npf->getClefWidth(clef);

        ::Rosegarden::Key key = staff->getSegment().getKeyAtTime(barStart, t);
        if (t < barStart)
            w += m_npf->getKeyWidth(key);

        if (w > max)
            max = w;
    }

    RG_DEBUG << "getMaxRepeatedClefAndKeyWidth(" << barNo << "): " << max
   ;

    if (max > 0)
        return max + getFixedItemSpacing() * 2;
    else
        return 0;
}

void
NotationHLayout::reconcileBarsLinear()
{
    Profiler profiler("NotationHLayout::reconcileBarsLinear");

    // Ensure that concurrent bars on all staffs have the same width,
    // which for now we make the maximum width required for this bar
    // on any staff.  These days getViewSegmentWithWidestBar actually does
    // most of the work in its call to preSquishBar, but this function
    // still sets the bar line positions etc.

    int barNo = getFirstVisibleBar();

    m_totalWidth = 0.0;
    for (ViewSegmentIntMap::iterator i = m_staffNameWidths.begin();
            i != m_staffNameWidths.end(); ++i) {
        if (i->second > m_totalWidth)
            m_totalWidth = double(i->second);
    }

    for (;;) {

        ViewSegment *widest = getViewSegmentWithWidestBar(barNo);

        if (!widest) {
            // have we reached the end of the piece?
            if (barNo >= getLastVisibleBar()) { // yes
                break;
            } else {
                m_totalWidth += m_spacing / 3;
                RG_DEBUG << "Setting bar position for degenerate bar "
                << barNo << " to " << m_totalWidth;

                m_barPositions[barNo] = m_totalWidth;
                ++barNo;
                continue;
            }
        }

        float maxWidth = m_barData[widest].find(barNo)->second.sizeData.idealWidth;
        if (m_pageWidth > 0.1 && maxWidth > m_pageWidth) {
            maxWidth = m_pageWidth;
        }

        RG_DEBUG << "Setting bar position for bar " << barNo
        << " to " << m_totalWidth;

        m_barPositions[barNo] = m_totalWidth;
        m_totalWidth += maxWidth;

        // Now apply width to this bar on all staffs

        for (BarDataMap::iterator i = m_barData.begin();
                i != m_barData.end(); ++i) {

            BarDataList &list = i->second;
            BarDataList::iterator bdli = list.find(barNo);
            if (bdli != list.end()) {

                BarData::SizeData &bd(bdli->second.sizeData);

                RG_DEBUG << "Changing width from " << bd.reconciledWidth << " to " << maxWidth;

                double diff = maxWidth - bd.reconciledWidth;
                if (diff < -0.1 || diff > 0.1) {
                    RG_DEBUG << "(So needsLayout becomes true)";
                    bdli->second.layoutData.needsLayout = true;
                }
                bd.reconciledWidth = maxWidth;
            }
        }

        ++barNo;
    }

    RG_DEBUG << "Setting bar position for bar " << barNo
    << " to " << m_totalWidth;

    m_barPositions[barNo] = m_totalWidth;
}

void
NotationHLayout::reconcileBarsPage()
{
    Profiler profiler("NotationHLayout::reconcileBarsPage");

    int barNo = getFirstVisibleBar();
    int barNoThisRow = 0;

    // pair of the recommended number of bars with those bars'
    // original total width, for each row
    std::vector<std::pair<int, double> > rowData;

    double stretchFactor = 10.0;
    double maxViewSegmentNameWidth = 0.0;

    for (ViewSegmentIntMap::iterator i = m_staffNameWidths.begin();
            i != m_staffNameWidths.end(); ++i) {
        if (i->second > maxViewSegmentNameWidth) {
            maxViewSegmentNameWidth = double(i->second);
        }
    }

    double pageWidthSoFar = maxViewSegmentNameWidth;
    m_totalWidth = maxViewSegmentNameWidth + getPreBarMargin();

    RG_DEBUG << "reconcileBarsPage: pageWidthSoFar is " << pageWidthSoFar;

    for (;;) {

        ViewSegment *widest = getViewSegmentWithWidestBar(barNo);
        double maxWidth = m_spacing / 3;

        if (!widest) {
            // have we reached the end of the piece?
            if (barNo >= getLastVisibleBar())
                break; // yes
        } else {
            maxWidth =
                m_barData[widest].find(barNo)->second.sizeData.idealWidth;
        }

        // Work on the assumption that this bar is the last in the
        // row.  How would that make things look?

        double nextPageWidth = pageWidthSoFar + maxWidth;
        double nextStretchFactor = m_pageWidth / nextPageWidth;

        RG_DEBUG << "barNo is " << barNo << ", maxWidth " << maxWidth << ", nextPageWidth " << nextPageWidth << ", nextStretchFactor " << nextStretchFactor << ", m_pageWidth " << m_pageWidth;

        // We have to have at least one bar per row

        bool tooFar = false;

        if (barNoThisRow >= 1) {

            // If this stretch factor is "worse" than the previous
            // one, we've come too far and have too many bars

            if (fabs(1.0 - nextStretchFactor) > fabs(1.0 - stretchFactor)) {
                tooFar = true;
            }

            // If the next stretch factor is less than 1 and would
            // make this bar on any of the staffs narrower than it can
            // afford to be, then we've got too many bars
            //!!! rework this -- we have no concept of "too narrow"
            // any more but we can declare we don't want it any
            // narrower than e.g. 90% or something based on the spacing
            /*!!!
                    if (!tooFar && (nextStretchFactor < 1.0)) {

                        for (BarDataMap::iterator i = m_barData.begin();
                             i != m_barData.end(); ++i) {

                            BarDataList &list = i->second;
                            BarDataList::iterator bdli = list.find(barNo);
                            if (bdli != list.end()) {
                                BarData::SizeData &bd(bdli->second.sizeData);
                                if ((nextStretchFactor * bd.idealWidth) <
                                    (double)(bd.fixedWidth + bd.baseWidth)) {
                                    tooFar = true;
                                    break;
                                }
                            }
                        }
                    }
            */
        }

        if (tooFar) {
            rowData.push_back(std::pair<int, double>(barNoThisRow,
                              pageWidthSoFar));
            barNoThisRow = 1;

            // When we start a new row, we always need to allow for the
            // repeated clef and key at the start of it.
            int maxClefKeyWidth = getMaxRepeatedClefAndKeyWidth(barNo);

            for (BarDataMap::iterator i = m_barData.begin();
                    i != m_barData.end(); ++i) {

                BarDataList &list = i->second;
                BarDataList::iterator bdli = list.find(barNo);

                if (bdli != list.end()) {
                    bdli->second.sizeData.clefKeyWidth = maxClefKeyWidth;
                }
            }

            pageWidthSoFar = maxWidth + maxClefKeyWidth;
            stretchFactor = m_pageWidth / pageWidthSoFar;
        } else {
            ++barNoThisRow;
            pageWidthSoFar = nextPageWidth;
            stretchFactor = nextStretchFactor;
        }

        ++barNo;
    }

    if (barNoThisRow > 0) {
        rowData.push_back(std::pair<int, double>(barNoThisRow,
                          pageWidthSoFar));
    }

    // Now we need to actually apply the widths

    barNo = getFirstVisibleBar();

    for (unsigned int row = 0; row < rowData.size(); ++row) {

        barNoThisRow = barNo;
        int finalBarThisRow = barNo + rowData[row].first - 1;

        pageWidthSoFar = (row > 0 ? 0 : maxViewSegmentNameWidth + getPreBarMargin());
        stretchFactor = m_pageWidth / rowData[row].second;

        for (; barNoThisRow <= finalBarThisRow; ++barNoThisRow, ++barNo) {

            bool finalRow = (row == rowData.size() - 1);

            ViewSegment *widest = getViewSegmentWithWidestBar(barNo);
            if (finalRow && (stretchFactor > 1.0))
                stretchFactor = 1.0;
            double maxWidth = 0.0;

            if (!widest) {
                // have we reached the end of the piece? (shouldn't happen)
                if (barNo >= getLastVisibleBar())
                    break; // yes
                else
                    maxWidth = stretchFactor * (m_spacing / 3);
            } else {
                BarData &bd = m_barData[widest].find(barNo)->second;
                maxWidth = (stretchFactor * bd.sizeData.idealWidth) +
                           bd.sizeData.clefKeyWidth;
                RG_DEBUG << "setting maxWidth to " << (stretchFactor * bd.sizeData.idealWidth) << " + " << bd.sizeData.clefKeyWidth << " = " << maxWidth;
            }

            if (barNoThisRow == finalBarThisRow) {
                if (!finalRow ||
                        (maxWidth > (m_pageWidth - pageWidthSoFar))) {
                    maxWidth = m_pageWidth - pageWidthSoFar;
                    RG_DEBUG << "reset maxWidth to " << m_pageWidth << " - " << pageWidthSoFar << " = " << maxWidth;
                }
            }

            m_barPositions[barNo] = m_totalWidth;
            m_totalWidth += maxWidth;

            for (BarDataMap::iterator i = m_barData.begin();
                    i != m_barData.end(); ++i) {

                BarDataList &list = i->second;
                BarDataList::iterator bdli = list.find(barNo);
                if (bdli != list.end()) {
                    BarData::SizeData &bd(bdli->second.sizeData);
                    double diff = maxWidth - bd.reconciledWidth;
                    if (diff < -0.1 || diff > 0.1) {
                        bdli->second.layoutData.needsLayout = true;
                    }
                    bd.reconciledWidth = maxWidth;
                }
            }

            pageWidthSoFar += maxWidth;
        }
    }

    m_barPositions[barNo] = m_totalWidth;
}

void
NotationHLayout::finishLayout(timeT startTime, timeT endTime, bool full)
{
    Profiler profiler("NotationHLayout::finishLayout");
    m_barPositions.clear();

    if (m_pageMode && (m_pageWidth > 0.1)) reconcileBarsPage();
    else reconcileBarsLinear();

    int staffNo = 0;

    for (BarDataMap::iterator i(m_barData.begin());
         i != m_barData.end(); ++i) {

        //emit setValue(100 * staffNo / m_barData.size());

        //throwIfCancelled();

        timeT timeCovered = endTime - startTime;

        if (full) {
            NotationElementList *notes = i->first->getViewElementList();
            if (notes->begin() != notes->end()) {
                NotationElementList::iterator j(notes->end());
                timeCovered =
                    (*--j)->getViewAbsoluteTime() -
                    (*notes->begin())->getViewAbsoluteTime();
            }
        }

        // Don't crash if more than 100 segments
        int k = 100 / m_barData.size();
        if (k < 1) k = 1;

        m_timePerProgressIncrement = timeCovered / k;

        layout(i, startTime, endTime, full);
        ++staffNo;
    }
}

void
NotationHLayout::layout(BarDataMap::iterator i, timeT startTime, timeT endTime,
                        bool full)
{
    Profiler profiler("NotationHLayout::layout");

    ViewSegment &staff = *(i->first);
    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));
    NotationStaff &notationStaff = dynamic_cast<NotationStaff &>(staff);

    // these two are for partial layouts:
    //    bool haveSimpleOffset = false;
    //    double simpleOffset = 0;

    RG_DEBUG << "layout: full layout " << full << ", times " << startTime << "->" << endTime;

    double x = 0, barX = 0;
    TieMap tieMap;

    timeT lastIncrement =
        (full && (notes->begin() != notes->end())) ?
        (*notes->begin())->getViewAbsoluteTime() : startTime;

    Segment &segment = notationStaff.getSegment();
    ::Rosegarden::Key key = segment.getKeyAtTime(lastIncrement);
    Clef clef = segment.getClefAtTime(lastIncrement);
    TimeSignature timeSignature;

    int startBar = getComposition()->getBarNumber(startTime);

    QSettings settings;
    settings.beginGroup(NotationOptionsConfigGroup);

    bool showInvisibles = qStrToBool( settings.value("showinvisibles", "true" ) ) ;
    settings.endGroup();

    for (BarPositionList::iterator bpi = m_barPositions.begin();
            bpi != m_barPositions.end(); ++bpi) {

        int barNo = bpi->first;
        if (!full && barNo < startBar) continue;

        RG_DEBUG << "looking for bar "
                       << bpi->first;
        BarDataList::iterator bdi = barList.find(barNo);
        if (bdi == barList.end()) continue;
        barX = bpi->second;

        NotationElementList::iterator from = bdi->second.basicData.start;
        NotationElementList::iterator to;

        RG_DEBUG << "layout(): starting bar " << barNo << ", x = " << barX << ", width = " << bdi->second.sizeData.idealWidth << ", time = " << (from == notes->end() ? -1 : (*from)->getViewAbsoluteTime());

        BarDataList::iterator nbdi(bdi);
        if (++nbdi == barList.end()) {
            to = notes->end();
        } else {
            to = nbdi->second.basicData.start;
        }

        if (from == notes->end()) {
            RG_DEBUG << "Start is end";
        }
        if (from == to) {
            RG_DEBUG << "Start is to";
        }

        if (!bdi->second.layoutData.needsLayout) {

            double offset = barX - bdi->second.layoutData.x;

            RG_DEBUG << "layout(): bar " << barNo << " has needsLayout false and offset of " << offset;

            if (offset > -0.1 && offset < 0.1) {
                RG_DEBUG << "layout(): no offset, ignoring";
                continue;
            }

            bdi->second.layoutData.x += offset;

            if (bdi->second.basicData.newTimeSig)
                bdi->second.layoutData.timeSigX += (int)offset;

            for (NotationElementList::iterator it = from;
                    it != to && it != notes->end(); ++it) {

                NotationElement* nel = static_cast<NotationElement*>(*it);
                RG_DEBUG << "layout(): shifting element's x to " << ((*it)->getLayoutX() + offset) << " (was " << (*it)->getLayoutX() << ")";
                nel->setLayoutX((*it)->getLayoutX() + offset);
                double airX, airWidth;
                nel->getLayoutAirspace(airX, airWidth);
                nel->setLayoutAirspace(airX + offset, airWidth);
            }

            continue;
        }

        bdi->second.layoutData.x = barX;
        //      x = barX + getPostBarMargin();

        bool timeSigToPlace = false;
        if (bdi->second.basicData.newTimeSig) {
            timeSignature = bdi->second.basicData.timeSignature;
            timeSigToPlace = !bdi->second.basicData.timeSignature.isHidden();
        }
        if (timeSigToPlace) {
            RG_DEBUG << "layout(): there's a time sig in this bar";
        }

        bool repeatClefAndKey = false;
        if (bdi->second.sizeData.clefKeyWidth > 0) {
            repeatClefAndKey = true;
        }
        if (repeatClefAndKey) {
            RG_DEBUG << "layout(): need to repeat clef & key in this bar";
        }

        double barInset = notationStaff.getBarInset(barNo, repeatClefAndKey);

        NotationElement *lastDynamicText = 0;
        int fretboardCount = 0;
        int count = 0;

        double offset = 0.0;
        double reconciledWidth = bdi->second.sizeData.reconciledWidth;

        if (repeatClefAndKey) {
            offset = bdi->second.sizeData.clefKeyWidth;
            reconciledWidth -= offset;
        }

        if (bdi->second.basicData.newTimeSig ||
            !bdi->second.basicData.timeSignature.hasHiddenBars()) {
            offset += getPostBarMargin();
        }

        ChunkList &chunks = bdi->second.chunks;
        ChunkList::iterator chunkitr = chunks.begin();
        double reconcileRatio = 1.0;
        if (bdi->second.sizeData.idealWidth > 0.0) {
            reconcileRatio = reconciledWidth / bdi->second.sizeData.idealWidth;
        }

        RG_DEBUG << "have " << chunks.size() << " chunks, reconciledWidth " << bdi->second.sizeData.reconciledWidth << ", idealWidth " << bdi->second.sizeData.idealWidth << ", ratio " << reconcileRatio;

        double delta = 0;
        float sigx = 0.f;

        for (NotationElementList::iterator it = from;
             it != to && it != notes->end();
             ++it) {

            NotationElement *el = static_cast<NotationElement*>(*it);
            delta = 0;
            float fixed = 0;

            if (el->event()->isa(Note::EventType)) {
                long pitch = 0;
                el->event()->get<Int>(PITCH, pitch);
                RG_DEBUG << "element is a " << el->event()->getType() << " (pitch " << pitch << ")";
            } else {
                RG_DEBUG << "element is a " << el->event()->getType();
            }

            bool invisible = false;
            if (el->event()->get<Bool>(INVISIBLE, invisible) && invisible) {
                if (!showInvisibles)
                    continue;
            }
            if (m_hideRedundance &&
                m_scene->isEventRedundant(el->event(), segment)) continue;

//            float sigx = 0;

            if (chunkitr != chunks.end()) {
                RG_DEBUG << "new chunk: addr " << &(*chunkitr) << " duration=" << (*chunkitr).duration << " subordering=" << (*chunkitr).subordering << " fixed=" << (*chunkitr).fixed << " stretchy=" << (*chunkitr).stretchy << " x=" << (*chunkitr).x;
                x = barX + offset + reconcileRatio * (*chunkitr).x;
                fixed = (*chunkitr).fixed;
//                sigx = barX + offset - fixed;
//                sigx = x - fixed;
                RG_DEBUG << "adjusted x is " << x << ", fixed is " << fixed;

                if (timeSigToPlace) {
                    if (el->event()->isa(Clef::EventType) ||
                        el->event()->isa(Rosegarden::Key::EventType)) {
                        sigx = x + (*chunkitr).fixed + (*chunkitr).stretchy;
                    }
                }

                ChunkList::iterator chunkscooter(chunkitr);
                if (++chunkscooter != chunks.end()) {
                    delta = (*chunkscooter).x - (*chunkitr).x;
                } else {
                    delta = reconciledWidth -
                            bdi->second.sizeData.fixedWidth - (*chunkitr).x;
                }
                delta *= reconcileRatio;

                ++chunkitr;
            } else {
                x = barX + reconciledWidth - getPreBarMargin();
//                sigx = x;
                delta = 0;
            }

            if (timeSigToPlace &&
                !el->event()->isa(Clef::EventType) &&
                !el->event()->isa(::Rosegarden::Key::EventType)) {

                if (sigx == 0.f) {
                    sigx = barX + offset;
                }

//                RG_DEBUG << "Placing timesig at " << (x - fixed);
//                bdi->second.layoutData.timeSigX = (int)(x - fixed);
                RG_DEBUG << "Placing timesig at " << sigx << " (would previously have been " << int(x-fixed) << "?)";
                bdi->second.layoutData.timeSigX = (int)sigx;
                double shift = getFixedItemSpacing() +
                               m_npf->getTimeSigWidth(timeSignature);
                offset += shift;
                x += shift;
                RG_DEBUG << "and moving next elt to " << x;
                timeSigToPlace = false;
            }

            if (barInset >= 1.0) {
                if (el->event()->isa(Clef::EventType) ||
                        el->event()->isa(::Rosegarden::Key::EventType)) {
                    RG_DEBUG << "Pulling clef/key back by " << getPreBarMargin();
                    x -= getPostBarMargin() * 2 / 3;
                } else {
                    barInset = 0.0;
                }
            }

            RG_DEBUG << "layout(): setting element's x to " << x << " (was " << el->getLayoutX() << ")";

            double displacedX = 0.0;
            long dxRaw = 0;
            el->event()->get<Int>(DISPLACED_X, dxRaw);
            displacedX = double(dxRaw * m_npf->getNoteBodyWidth()) / 1000.0;

            el->setLayoutX(x + displacedX);
            el->setLayoutAirspace(x, int(delta));

            // #704958 (multiple tuplet spanners created when entering
            // triplet chord) -- only do this here for non-notes,
            // notes get it from positionChord
            if (!el->isNote()) {
                sampleGroupElement(staff, clef, key, it);
            }

            if (el->isNote()) {

                // This modifies "it" and "tieMap"
                positionChord(staff, it, clef, key, tieMap, to);

            } else if (el->isRest()) {

                // nothing to do

            } else if (el->event()->isa(Clef::EventType)) {

                clef = Clef(*el->event());

            } else if (el->event()->isa(::Rosegarden::Key::EventType)) {

                key = ::Rosegarden::Key(*el->event());

            } else if (el->event()->isa(Text::EventType)) {

                // if it's a dynamic, make a note of it in case a
                // hairpin immediately follows it

                if (el->event()->has(Text::TextTypePropertyName) &&
                        el->event()->get<String>(Text::TextTypePropertyName) ==
                        Text::Dynamic) {
                    lastDynamicText = el;
                }

            } else if (el->event()->isa(Indication::EventType)) {

                std::string type;
                double ix = x;

                // Check for a dynamic text at the same time as the
                // indication and if found, move the indication to the
                // right to make room.  We know the text should have
                // preceded the indication in the staff because it has
                // a smaller subordering

                if (el->event()->get<String>
                        (Indication::IndicationTypePropertyName, type) &&
                        (type == Indication::Crescendo ||
                         type == Indication::Decrescendo) &&
                        lastDynamicText &&
                        lastDynamicText->getViewAbsoluteTime() ==
                        el->getViewAbsoluteTime()) {

                    ix = x + m_npf->getTextWidth
                         (Text(*lastDynamicText->event())) +
                         m_npf->getNoteBodyWidth() / 4;
                }

                el->setLayoutX(ix + displacedX);
                el->setLayoutAirspace(ix, delta - (ix - x));

            } else if (el->event()->isa(Guitar::Chord::EventType)) {

                int guitarChordWidth = m_npf->getLineSpacing() * 6;
                el->setLayoutX(x - (guitarChordWidth / 2)
                               + fretboardCount * (guitarChordWidth +
                                                   m_npf->getNoteBodyWidth()/2)
                               + displacedX);
                ++fretboardCount;

            } else {

                // nothing else
            }

            if (m_timePerProgressIncrement > 0 && (++count == 100)) {
                count = 0;
                timeT sinceIncrement = el->getViewAbsoluteTime() - lastIncrement;
                if (sinceIncrement > m_timePerProgressIncrement) {
                    //emit setValue(sinceIncrement / m_timePerProgressIncrement);
                    lastIncrement +=
                        (sinceIncrement / m_timePerProgressIncrement)
                        * m_timePerProgressIncrement;
                    //throwIfCancelled();
                }
            }
        }

        if (timeSigToPlace) {
            // no other events in this bar, so we never managed to place it
            x = barX + offset;
            RG_DEBUG << "Placing timesig reluctantly at " << x;
            bdi->second.layoutData.timeSigX = (int)(x);
            timeSigToPlace = false;
        }

        for (NotationGroupMap::iterator mi = m_groupsExtant.begin();
                mi != m_groupsExtant.end(); ++mi) {
            mi->second->applyBeam(notationStaff);
            mi->second->applyTuplingLine(notationStaff);
            delete mi->second;
        }
        m_groupsExtant.clear();

        bdi->second.layoutData.needsLayout = false;
    }
}

void
NotationHLayout::sampleGroupElement(ViewSegment &staff,
                                    const Clef &clef,
                                    const ::Rosegarden::Key &key,
                                    const NotationElementList::iterator &itr)
{
    NotationElement *el = static_cast<NotationElement *>(*itr);

    if (el->event()->has(BEAMED_GROUP_ID)) {

        //!!! Gosh.  We need some clever logic to establish whether
        // one group is happening while another has not yet ended --
        // perhaps we decide one has ended if we see another, and then
        // re-open the case of the first if we meet another note that
        // claims to be in it.  Then we need to hint to both of the
        // groups that they should choose appropriate stem directions
        // -- we could just use HEIGHT_ON_STAFF of their first notes
        // to determine this, as if that doesn't work, nothing will

        long groupId = el->event()->get<Int>(BEAMED_GROUP_ID);
        RG_DEBUG << "group id: " << groupId;
        if (m_groupsExtant.find(groupId) == m_groupsExtant.end()) {
            RG_DEBUG << "(new group)";
            m_groupsExtant[groupId] =
                new NotationGroup(*staff.getViewElementList(),
                                  m_notationQuantizer,
                                  m_properties, clef, key);
        }
        m_groupsExtant[groupId]->sample(itr, true);
    }
}

timeT
NotationHLayout::getSpacingDuration(ViewSegment &staff,
                                    const NotationElementList::iterator &i)
{
    SegmentNotationHelper helper(staff.getSegment());
    timeT t((*i)->getViewAbsoluteTime());
    timeT d((*i)->getViewDuration());

    if (d > 0 && (*i)->event()->getDuration() == 0) return d; // grace note

    NotationElementList::iterator j(i), e(staff.getViewElementList()->end());
    while (j != e && ((*j)->getViewAbsoluteTime() == t ||
                      (*j)->getViewDuration() == 0)) {
        ++j;
    }
    if (j == e) {
        return d;
    } else {
        return (*j)->getViewAbsoluteTime() - (*i)->getViewAbsoluteTime();
    }
}

timeT
NotationHLayout::getSpacingDuration(ViewSegment &staff,
                                    const NotationChord &chord)
{
    SegmentNotationHelper helper(staff.getSegment());

    NotationElementList::iterator i = chord.getShortestElement();
    timeT d((*i)->getViewDuration());

    if (d > 0 && (*i)->event()->getDuration() == 0) return d; // grace note

    NotationElementList::iterator j(i), e(staff.getViewElementList()->end());
    while (j != e && (chord.contains(j) || (*j)->getViewDuration() == 0))
        ++j;

    if (j != e) {
        d = (*j)->getViewAbsoluteTime() - (*i)->getViewAbsoluteTime();
    }

    return d;
}

void
NotationHLayout::positionChord(ViewSegment &staff,
                               NotationElementList::iterator &itr,
                               const Clef &clef, const ::Rosegarden::Key &key,
                               TieMap &tieMap,
                               NotationElementList::iterator &to)
{
    NotationChord chord(*staff.getViewElementList(), itr, m_notationQuantizer,
                        m_properties, clef, key);
    double baseX, delta;
    (static_cast<NotationElement *>(*itr))->getLayoutAirspace(baseX, delta);

    bool barEndsInChord = false;

    RG_DEBUG << "positionChord: x = " << baseX;

    // #938545 (Broken notation: Duplicated note can float outside
    // stave) -- We need to iterate over all elements in the chord
    // range here, not just the ordered set of notes actually in the
    // chord.  They all have the same x-coord, so there's no
    // particular complication here.

    for (NotationElementList::iterator citr = chord.getInitialElement();
         citr != staff.getViewElementList()->end(); ++citr) {

        if (citr == to)
            barEndsInChord = true;

        // #704958 (multiple tuplet spanners created when entering
        // triplet chord) -- layout() updates the beamed group data
        // for non-notes, but we have to do it for notes so as to
        // ensure every note in the chord is accounted for
        sampleGroupElement(staff, clef, key, citr);

        NotationElement *elt = static_cast<NotationElement*>(*citr);

        double displacedX = 0.0;
        long dxRaw = 0;
        elt->event()->get<Int>(DISPLACED_X, dxRaw);
        displacedX = double(dxRaw * m_npf->getNoteBodyWidth()) / 1000.0;

        elt->setLayoutX(baseX + displacedX);
        elt->setLayoutAirspace(baseX, delta);

        RG_DEBUG << "positionChord: assigned x to elt at " << elt->getViewAbsoluteTime();

        if (citr == chord.getFinalElement())
            break;
    }

    // Check for any ties going back, and if so work out how long they
    // must have been and assign accordingly.

    for (NotationElementList::iterator citr = chord.getInitialElement();
            citr != staff.getViewElementList()->end(); ++citr) {

        NotationElement *note = static_cast<NotationElement*>(*citr);
        if (!note->isNote()) {
            if (citr == chord.getFinalElement())
                break;
            continue;
        }

        // Make a copy of the next event if available.
        NotationElement *nextNote = note;
        if (++citr != staff.getViewElementList()->end()) {
            nextNote = static_cast<NotationElement*>(*citr);
        }
        --citr;

        bool tiedForwards = false;
        bool tiedBack = false;

        note->event()->get<Bool>(TIED_FORWARD, tiedForwards);
        note->event()->get<Bool>(TIED_BACKWARD, tiedBack);

        if (!note->event()->has(PITCH))
            continue;
        int pitch = note->event()->get<Int>(PITCH);

        if (tiedBack) {
            TieMap::iterator ti(tieMap.find(pitch));

            if (ti != tieMap.end()) {
                NotationElementList::iterator otherItr(ti->second);

                if ((*otherItr)->getViewAbsoluteTime() +
                    (*otherItr)->getViewDuration() ==
                    note->getViewAbsoluteTime()) {

                    RG_DEBUG << "Second note in tie at " << note->getViewAbsoluteTime() << ": found first note, it matches";

                    (*otherItr)->event()->setMaybe<Int>
                    (m_properties.TIE_LENGTH,
                     (int)(baseX - (*otherItr)->getLayoutX()));

                } else {
                    RG_DEBUG << "Second note in tie at " << note->getViewAbsoluteTime() << ": found first note but it ends at " << ((*otherItr)->getViewAbsoluteTime() + (*otherItr)->getViewDuration());

                    tieMap.erase(pitch);
                }
            }
        }

        if (tiedForwards) {
//          note->event()->setMaybe<Int>(m_properties.TIE_LENGTH, 0);

            // Don't modify tie info here, unless the next note has become
            // something other than a note.
            if (!nextNote->isNote()) {
                note->event()->setMaybe<Int>(m_properties.TIE_LENGTH, 0);
            }

            tieMap[pitch] = citr;
        } else {
            note->event()->unset(m_properties.TIE_LENGTH);
        }

        if (citr == chord.getFinalElement())
            break;
    }

    itr = chord.getFinalElement();
    if (barEndsInChord) {
        to = itr;
        ++to;
    }
}

float
NotationHLayout::getLayoutWidth(ViewElement &ve,
                                NotePixmapFactory */* npf */,
                                const ::Rosegarden::Key &previousKey) const
{
    NotationElement& e = static_cast<NotationElement&>(ve);

    if ((e.isNote() || e.isRest()) && e.event()->has(NOTE_TYPE)) {

        long noteType = e.event()->get<Int>(NOTE_TYPE);
        long dots = 0;
        (void)e.event()->get<Int>(NOTE_DOTS, dots);

        double bw = 0;

        if (e.isNote()) {
            bw = m_npf->getNoteBodyWidth(noteType)
                + m_npf->getDotWidth() * dots;
        } else {
            bw = m_npf->getRestWidth(Note(noteType, dots));
        }

        double multiplier = double(Note(noteType, dots).getDuration()) /
                            double(Note(Note::Quaver).getDuration());
        multiplier -= 1.0;
        multiplier *= m_proportion / 100.0;
        multiplier += 1.0;

        double gap = m_npf->getNoteBodyWidth(noteType) * multiplier;

        RG_DEBUG << "note type " << noteType << ", isNote " << e.isNote() << ", dots " << dots << ", multiplier " << multiplier << ", gap " << gap << ", result " << (bw + gap * m_spacing / 100.0);

        gap = gap * m_spacing / 100.0;
        return bw + gap;

    } else {

        double w = getFixedItemSpacing();

        if (e.event()->isa(Clef::EventType)) {

            w += m_npf->getClefWidth(Clef(*e.event()));

        } else if (e.event()->isa(::Rosegarden::Key::EventType)) {

            ::Rosegarden::Key key(*e.event());

            ::Rosegarden::Key cancelKey = previousKey;

            if (m_keySigCancelMode == 0) { // only when entering C maj / A min

                if (key.getAccidentalCount() != 0)
                    cancelKey = ::Rosegarden::Key();

            } else if (m_keySigCancelMode == 1) { // only when reducing acc count

                if (key.getAccidentalCount() &&
                    !(key.isSharp() == cancelKey.isSharp() &&
                      key.getAccidentalCount() < cancelKey.getAccidentalCount()
                     )
                   ) {
                    cancelKey = ::Rosegarden::Key();
                }
            }

            w += m_npf->getKeyWidth(key, cancelKey);

        } else if (e.event()->isa(Indication::EventType) ||
                   e.event()->isa(Text::EventType)) {

            w = 0;

        } else {
            //      RG_DEBUG << "getLayoutWidth(): no case for event type " << e.event()->getType();
            //      w += 24;
            w = 0;
        }

        return w;
    }
}

int NotationHLayout::getBarMargin() const
{
    return (int)(m_npf->getBarMargin() * m_spacing / 100.0);
}

int NotationHLayout::getPreBarMargin() const
{
    return getBarMargin() / 3;
}

int NotationHLayout::getPostBarMargin() const
{
    return getBarMargin() - getPreBarMargin();
}

int NotationHLayout::getFixedItemSpacing() const
{
    return (int)((m_npf->getNoteBodyWidth() * 2.0 / 3.0) * m_spacing / 100.0);
}

void
NotationHLayout::reset()
{
    for (BarDataMap::iterator i = m_barData.begin(); i != m_barData.end(); ++i) {
        clearBarList(*i->first);
    }

    m_barData.clear();
    m_barPositions.clear();
    m_totalWidth = 0;
}

int
NotationHLayout::getFirstVisibleBar() const
{
    int bar = 0;
    bool haveBar = false;
    for (BarDataMap::const_iterator i = m_barData.begin(); i != m_barData.end(); ++i) {
        if (i->second.begin() == i->second.end())
            continue;
        int barHere = i->second.begin()->first;
        if (barHere < bar || !haveBar) {
            bar = barHere;
            haveBar = true;
        }
    }

    //    RG_DEBUG << "getFirstVisibleBar: returning " << bar;

    return bar;
}

int
NotationHLayout::getFirstVisibleBarOnViewSegment(ViewSegment &staff) const
{
    const BarDataList &bdl(getBarData(staff));

    int bar = 0;
    if (bdl.begin() != bdl.end()) bar = bdl.begin()->first;

    //    RG_DEBUG << "getFirstVisibleBarOnViewSegment: returning " << bar;
    return bar;
}

int
NotationHLayout::getLastVisibleBar() const
{
    int bar = 0;
    bool haveBar = false;
    for (BarDataMap::const_iterator i = m_barData.begin();
         i != m_barData.end(); ++i) {
        if (i->second.begin() == i->second.end())
            continue;
        int barHere = getLastVisibleBarOnViewSegment(*i->first);
        if (barHere > bar || !haveBar) {
            bar = barHere;
            haveBar = true;
        }
    }

    //    RG_DEBUG << "getLastVisibleBar: returning " << bar;

    return bar;
}

int
NotationHLayout::getLastVisibleBarOnViewSegment(ViewSegment &staff) const
{
    const BarDataList &bdl(getBarData(staff));
    int bar = 0;

    if (bdl.begin() != bdl.end()) {
        BarDataList::const_iterator i = bdl.end();
        bar = ((--i)->first) + 1; // last visible bar_line_
    }

    //    RG_DEBUG << "getLastVisibleBarOnViewSegment: returning " << bar;

    return bar;
}

double
NotationHLayout::getBarPosition(int bar) const
{
    double position = 0.0;

    BarPositionList::const_iterator i = m_barPositions.find(bar);

    if (i != m_barPositions.end()) {

        position = i->second;

    } else {

        i = m_barPositions.begin();
        if (i != m_barPositions.end()) {
            if (bar < i->first)
                position = i->second;
            else {
                i = m_barPositions.end();
                --i;
                if (bar > i->first)
                    position = i->second;
            }
        }
    }

    //    RG_DEBUG << "getBarPosition: returning " << position << " for bar " << bar;

    return position;
}

bool
NotationHLayout::isBarCorrectOnViewSegment(ViewSegment &staff, int i) const
{
    const BarDataList &bdl(getBarData(staff));
    ++i;

    BarDataList::const_iterator bdli(bdl.find(i));
    if (bdli != bdl.end()) return bdli->second.basicData.correct;
    else return true;
}

bool NotationHLayout::getTimeSignaturePosition(ViewSegment &staff,
                                               int barNo,
                                               TimeSignature &timeSig,
                                               double &timeSigX) const
{
    const BarDataList &bdl(getBarData(staff));

    BarDataList::const_iterator bdli(bdl.find(barNo));
    if (bdli != bdl.end()) {
        timeSig = bdli->second.basicData.timeSignature;
        timeSigX = (double)(bdli->second.layoutData.timeSigX);
        return bdli->second.basicData.newTimeSig;
    } else
        return 0;
}

timeT
NotationHLayout::getTimeForX(double x) const
{
    return RulerScale::getTimeForX(x);
}

double
NotationHLayout::getXForTime(timeT t) const
{
    return RulerScale::getXForTime(t);
}

double
NotationHLayout::getXForTimeByEvent(timeT time) const
{
    //    RG_DEBUG << "getXForTime(" << time << ")";

    for (BarDataMap::const_iterator i = m_barData.begin(); i != m_barData.end(); ++i) {

        ViewSegment *staff = i->first;

        if (staff->getSegment().getStartTime() <= time &&
                staff->getSegment().getEndMarkerTime() > time) {

            ViewElementList::iterator vli =
                staff->getViewElementList()->findNearestTime(time);

            bool found = false;
            double x = 0.0, dx = 0.0;
            timeT t = 0, dt = 0;

            while (!found) {
                if (vli == staff->getViewElementList()->end())
                    break;
                NotationElement *element = static_cast<NotationElement *>(*vli);
                if (element->getItem()) {
                    x = element->getLayoutX();
                    double temp;
                    element->getLayoutAirspace(temp, dx);
                    t = element->event()->getNotationAbsoluteTime();
                    dt = element->event()->getNotationDuration();
                    found = true;
                    break;
                }
                ++vli;
            }

            if (found) {
                if (time > t) {

                    while (vli != staff->getViewElementList()->end() &&
                            ((*vli)->event()->getNotationAbsoluteTime() < time ||
                             !((static_cast<NotationElement *>(*vli))->getItem())))
                        ++vli;

                    if (vli != staff->getViewElementList()->end()) {
                        NotationElement *element = static_cast<NotationElement *>(*vli);
                        dx = element->getLayoutX() - x;
                        dt = element->event()->getNotationAbsoluteTime() - t;
                    }

                    if (dt > 0 && dx > 0) {
                        return x + dx * (time - t) / dt;
                    }
                }

                return x - 3;
            }
        }
    }

    return RulerScale::getXForTime(time);
}

std::vector<int> NotationHLayout::m_availableSpacings;
std::vector<int> NotationHLayout::m_availableProportions;

/// YG: Only for debug
void
NotationHLayout::BarData::dump(std::string indent)
{
    RG_DEBUG << indent
             << "basic(start=<x>"
             << " correct=" << basicData.correct
             << " timeSig=" << basicData.timeSignature.getNumerator()
                     << "/" << basicData.timeSignature.getDenominator()
             << " newTimeSig=" << basicData.newTimeSig
             << " delayInBar=" << basicData.delayInBar
             << " trackId=" << basicData.trackId << ")";
    RG_DEBUG << indent
             << "size(ideal=" << sizeData.idealWidth
             << "reconcile=" << sizeData.reconciledWidth
             << "fixed=" << sizeData.fixedWidth
             << "timeSigFixed=" << sizeData.timeSigFixedWidth
             << "clefKey=" << sizeData.clefKeyWidth
             << "duration=" << sizeData.actualDuration << ")";
    RG_DEBUG << indent
             << "layout(needs=" << layoutData.needsLayout
             << " x=" << layoutData.x
             << " timeSigX=" << layoutData.timeSigX << ")";

    ChunkList::iterator i;
    for (i=chunks.begin(); i!=chunks.end(); ++i) {
        RG_DEBUG << indent
                  << "   Chunk duration=" << (*i).duration
                  << " subord=" << (*i).subordering
                  << " fixed=" << (*i).fixed
                  << " stretchy=" << (*i).stretchy
                  << " x=" << (*i).x;
    }
}

/// YG: Only for debug
void
NotationHLayout::dumpBarDataMap()
{
    BarDataMap::iterator i;
    for (i=m_barData.begin(); i!=m_barData.end(); ++i) {
        ViewSegment *vs = (*i).first;
        BarDataList bdl = (*i).second;

        RG_DEBUG << "------- ViewSegment=" << vs
                  << " seg=" << &vs->getSegment() << "\n";
        BarDataList::iterator j;
        for (j=bdl.begin(); j!=bdl.end(); ++j) {
            RG_DEBUG << "       ------- BarData (" << (*j).first << ")\n";
            (*j).second.dump("       ");
        }
    }
}

}
