// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <kapp.h>
#include <kconfig.h>

#include "notationhlayout.h"
#include "notationstaff.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "NotationTypes.h"
#include "BaseProperties.h"
#include "notepixmapfactory.h"
#include "notationproperties.h"
#include "notationsets.h"
#include "widgets.h"
#include "Quantizer.h"
#include "Profiler.h"
#include "Composition.h"
#include "SegmentNotationHelper.h"

#include <cmath> // for fabs()
#include <set>

using Rosegarden::Note;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Event;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Indication;
using Rosegarden::Text;
using Rosegarden::Segment;
using Rosegarden::Composition;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::TimeSignature;
using Rosegarden::timeT;
using Rosegarden::Quantizer;
using Rosegarden::Staff;

using Rosegarden::Accidental;
using namespace Rosegarden::Accidentals;

using namespace Rosegarden::BaseProperties;

std::vector<int> NotationHLayout::m_availableSpacings;
std::vector<int> NotationHLayout::m_availableProportions;


NotationHLayout::NotationHLayout(Composition *c, NotePixmapFactory *npf,
				   const NotationProperties &properties,
				   QObject* parent, const char* name) :
    ProgressReporter(parent, name),
    Rosegarden::HorizontalLayoutEngine(c),
    m_totalWidth(0.),
    m_pageMode(false),
    m_pageWidth(0.),
    m_spacing(100),
    m_proportion(40),
    m_npf(npf),
    m_notationQuantizer(c->getNotationQuantizer()),
    m_properties(properties),
    m_timePerProgressIncrement(0),
    m_staffCount(0)
{
//    NOTATION_DEBUG << "NotationHLayout::NotationHLayout()" << endl;
}

NotationHLayout::~NotationHLayout()
{
    // empty
}

std::vector<int> 
NotationHLayout::getAvailableSpacings()
{
    if (m_availableSpacings.size() == 0) {
        m_availableSpacings.push_back(30);
        m_availableSpacings.push_back(60);
        m_availableSpacings.push_back(85);
        m_availableSpacings.push_back(100);
        m_availableSpacings.push_back(130);
        m_availableSpacings.push_back(170);
        m_availableSpacings.push_back(220);
    }
    return m_availableSpacings;
}

std::vector<int> 
NotationHLayout::getAvailableProportions()
{
    if (m_availableProportions.size() == 0) {
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
NotationHLayout::getBarData(Staff &staff)
{
    BarDataMap::iterator i = m_barData.find(&staff);
    if (i == m_barData.end()) {
	m_barData[&staff] = BarDataList();
    }

    return m_barData[&staff];
}


const NotationHLayout::BarDataList &
NotationHLayout::getBarData(Staff &staff) const
{
    return ((NotationHLayout *)this)->getBarData(staff);
}


NotationElementList::iterator
NotationHLayout::getStartOfQuantizedSlice(const NotationElementList *notes,
					  timeT t)
    const
{
    NotationElementList::iterator i = notes->findTime(t);
    NotationElementList::iterator j(i);

    while (true) {
	if (i == notes->begin()) return i;
	--j;
	if ((*j)->getViewAbsoluteTime() < t) return i;
	i = j;
    }
}


void
NotationHLayout::scanStaff(Staff &staff, timeT startTime, timeT endTime)
{
    throwIfCancelled();
    Rosegarden::Profiler profiler("NotationHLayout::scanStaff");

    Segment &segment(staff.getSegment());
    bool isFullScan = (startTime == endTime);
    int startBarOfStaff = getComposition()->getBarNumber(segment.getStartTime());

    if (isFullScan) {
	clearBarList(staff);
	startTime = segment.getStartTime();
	endTime   = segment.getEndMarkerTime();
    } else {
	startTime = getComposition()->getBarStartForTime(startTime);
	endTime   = getComposition()->getBarEndForTime(endTime);
    }

    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));

    int startBarNo = getComposition()->getBarNumber(startTime);
    int endBarNo = getComposition()->getBarNumber(endTime);
/*
    if (endBarNo > startBarNo &&
	getComposition()->getBarStart(endBarNo) == segment.getEndMarkerTime()) {
	--endBarNo;
    }
*/
    std::string name =
	segment.getComposition()->
	getTrackById(segment.getTrack())->getLabel();
    m_staffNameWidths[&staff] =
	m_npf->getNoteBodyWidth() * 2 +
	m_npf->getTextWidth(Rosegarden::Text(name,Rosegarden::Text::StaffName));

    NOTATION_DEBUG << "NotationHLayout::scanStaff: full scan " << isFullScan << ", times " << startTime << "->" << endTime << ", bars " << startBarNo << "->" << endBarNo << ", staff name \"" << segment.getLabel() << "\", width " << m_staffNameWidths[&staff] << endl;

    SegmentNotationHelper helper(segment);
    if (isFullScan) {
	helper.setNotationProperties();
    } else {
	helper.setNotationProperties(startTime, endTime);
    }

    Rosegarden::Key key = segment.getKeyAtTime(startTime);
    Clef clef = segment.getClefAtTime(startTime);
    TimeSignature timeSignature =
	segment.getComposition()->getTimeSignatureAt(startTime);
    bool barCorrect = true;

    for (int barNo = startBarNo; barNo <= endBarNo; ++barNo) {

	std::pair<timeT, timeT> barTimes =
	    getComposition()->getBarRange(barNo);

	if (barTimes.first >= segment.getEndMarkerTime()) break;

        NotationElementList::iterator from = 
	    getStartOfQuantizedSlice(notes, barTimes.first);

        NotationElementList::iterator to =
	    getStartOfQuantizedSlice(notes, barTimes.second);

	if (barTimes.second >= segment.getEndMarkerTime()) {
	    to = notes->end();
	}

	bool newTimeSig = false;
	timeSignature = getComposition()->getTimeSignatureInBar
	    (barNo, newTimeSig);
	if (barNo == startBarOfStaff) newTimeSig = true;

	float fixedWidth = 0.0;
	if (newTimeSig && !timeSignature.isHidden()) {
	    fixedWidth += getFixedItemSpacing() * 2 +
		m_npf->getTimeSigWidth(timeSignature);
	}

	setBarBasicData(staff, barNo, from, barCorrect, timeSignature, newTimeSig);
	barList.find(barNo)->second.layoutData.needsLayout = true;

	ChunkList chunks;
	float lyricWidth = 0;

	typedef std::set<long> GroupIdSet;
	GroupIdSet groupIds;

	NOTATION_DEBUG << "NotationHLayout::scanStaff: bar " << barNo << ", from " << barTimes.first << ", to " << barTimes.second << " (end " << segment.getEndMarkerTime() << "); from is at " << (from == notes->end() ? -1 : (*from)->getViewAbsoluteTime()) << ", to is at " << (to == notes->end() ? -1 : (*to)->getViewAbsoluteTime()) << endl;

	timeT actualBarEnd = barTimes.first;

	AccidentalTable accTable(key, clef);

        for (NotationElementList::iterator itr = from; itr != to; ++itr) {
        
            NotationElement *el = static_cast<NotationElement*>((*itr));

	    if (el->event()->has(BEAMED_GROUP_ID)) {
		long groupId = el->event()->get<Int>(BEAMED_GROUP_ID);
		if (groupIds.find(groupId) == groupIds.end()) {
		    NotationGroup group(*staff.getViewElementList(),
					m_notationQuantizer,
					m_properties, clef, key);
		    group.applyStemProperties();
		    groupIds.insert(groupId);
		}
	    }

            if (el->event()->isa(Clef::EventType)) {

//		NOTATION_DEBUG << "Found clef" << endl;
		chunks.push_back(Chunk(el->event()->getSubOrdering(),
				       getLayoutWidth(*el)));
		
                clef = Clef(*el->event());

                // Probably not strictly the right thing to do
                // here, but I hope it'll do well enough in practice
                accTable = AccidentalTable(key, clef);

            } else if (el->event()->isa(Rosegarden::Key::EventType)) {

//		NOTATION_DEBUG << "Found key" << endl;
		chunks.push_back(Chunk(el->event()->getSubOrdering(),
				       getLayoutWidth(*el)));

                key = Rosegarden::Key(*el->event());

                accTable = AccidentalTable(key, clef);

	    } else if (el->event()->isa(Text::EventType)) {

		// the only text events of interest are lyrics, which
		// contribute to a fixed area following the next chord
		
		if (el->event()->has(Text::TextTypePropertyName) &&
		    el->event()->get<String>(Text::TextTypePropertyName) ==
		    Text::Lyric) {
		    lyricWidth = m_npf->getTextWidth(Text(*el->event()));
		}
		chunks.push_back(Chunk(el->event()->getSubOrdering(), 0));

	    } else if (el->isNote()) {
		
		scanChord(notes, itr, clef, key, accTable, lyricWidth, chunks, to);

	    } else if (el->isRest()) {

		chunks.push_back(Chunk(el->getViewDuration(),
				       el->event()->getSubOrdering(),
				       0,
				       getLayoutWidth(*el)));

	    } else if (el->event()->isa(Indication::EventType)) {

//		NOTATION_DEBUG << "Found indication" << endl;

		chunks.push_back(Chunk(el->event()->getSubOrdering(), 0));

	    } else {
		
		NOTATION_DEBUG << "Found something I don't know about (type is " << el->event()->getType() << ")" << endl;
		chunks.push_back(Chunk(el->event()->getSubOrdering(),
				       getLayoutWidth(*el)));
	    }

	    actualBarEnd = el->getViewAbsoluteTime() + el->getViewDuration();
//!!!            el->event()->setMaybe<Int>(m_properties.MIN_WIDTH, mw);
	}

	if (actualBarEnd == barTimes.first) actualBarEnd = barTimes.second;
	barCorrect = (actualBarEnd == barTimes.second);

	setBarSizeData(staff, barNo, chunks, fixedWidth,
		       actualBarEnd - barTimes.first);

	if ((endTime > startTime) && (barNo % 20 == 0)) {
	    emit setProgress((barTimes.second - startTime) * 95 /
			     (endTime - startTime));
	    kapp->processEvents(50);
	}

        throwIfCancelled();
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
NotationHLayout::clearBarList(Staff &staff)
{
    BarDataList &bdl = m_barData[&staff];
    bdl.clear();
}

void
NotationHLayout::setBarBasicData(Staff &staff,
				 int barNo,
				 NotationElementList::iterator start,
				 bool correct,
				 Rosegarden::TimeSignature timeSig,
				 bool newTimeSig)
{
    NOTATION_DEBUG << "setBarBasicData for " << barNo << endl;

    BarDataList &bdl(m_barData[&staff]);

    BarDataList::iterator i(bdl.find(barNo));
    if (i == bdl.end()) {
	NotationElementList::iterator endi = staff.getViewElementList()->end();
	bdl.insert(BarDataPair(barNo, BarData(endi, true,
					      Rosegarden::TimeSignature(), false)));
	i = bdl.find(barNo);
    }

    i->second.basicData.start = start;
    i->second.basicData.correct = correct;
    i->second.basicData.timeSignature = timeSig;
    i->second.basicData.newTimeSig = newTimeSig;
}

void
NotationHLayout::setBarSizeData(Staff &staff,
				 int barNo,
				 const ChunkList &chunks,
				 float fixedWidth,
				 Rosegarden::timeT actualDuration)
{
    NOTATION_DEBUG << "setBarSizeData for " << barNo << endl;

    BarDataList &bdl(m_barData[&staff]);

    BarDataList::iterator i(bdl.find(barNo));
    if (i == bdl.end()) {
	NotationElementList::iterator endi = staff.getViewElementList()->end();
	bdl.insert(BarDataPair(barNo, BarData(endi, true,
					      Rosegarden::TimeSignature(), false)));
	i = bdl.find(barNo);
    }

    i->second.chunks = chunks;
    i->second.sizeData.actualDuration = actualDuration;
    i->second.sizeData.idealWidth = 0.0;
    i->second.sizeData.reconciledWidth = 0.0;
    i->second.sizeData.fixedWidth = fixedWidth;
}

 
void
NotationHLayout::scanChord(NotationElementList *notes,
			    NotationElementList::iterator &itr,
			    const Rosegarden::Clef &clef,
			    const Rosegarden::Key &key,
			    AccidentalTable &accTable,
			    float &lyricWidth,
			    ChunkList &chunks,
			    NotationElementList::iterator &to)
{
    NotationChord chord(*notes, itr, m_notationQuantizer, m_properties);
    AccidentalTable newAccTable(accTable);
    Accidental someAccidental = NoAccidental;
    bool barEndsInChord = false;
    bool grace = false;
/*
    NOTATION_DEBUG << "NotationHLayout::scanChord: "
		   << chord.size() << "-voice chord at "
		   << (*itr)->event()->getAbsoluteTime()
		   << " unquantized, "
		   << (*itr)->getViewAbsoluteTime()
		   << " quantized" << endl;

    NOTATION_DEBUG << "Contents:" << endl;
    
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
	    if (i == chord.getFinalElement()) break;
	    continue;
	}

	if (el->isGrace()) grace = true;
	
	long pitch = 64;
	if (!el->event()->get<Int>(PITCH, pitch)) {
	    NOTATION_DEBUG <<
		"WARNING: NotationHLayout::scanChord: couldn't get pitch for element, using default pitch of " << pitch << endl;
	}

	Accidental explicitAccidental = NoAccidental;
	(void)el->event()->get<String>(ACCIDENTAL, explicitAccidental);
	
	Rosegarden::Pitch p(pitch, explicitAccidental);
	int h = p.getHeightOnStaff(clef, key);
	Accidental acc = p.getDisplayAccidental(key);

	el->event()->setMaybe<Int>(NotationProperties::HEIGHT_ON_STAFF, h);
	el->event()->setMaybe<String>(m_properties.CALCULATED_ACCIDENTAL, acc);

	// update display acc for note according to the accTable
	// (accidentals in force when the last chord ended) and update
	// newAccTable with accidentals from this note.  (We don't
	// update accTable yet because there may be other notes in
	// this chord that need accTable to be the same as it is for
	// this one)
                    
	Accidental dacc = accTable.getDisplayAccidental(acc, h);
	el->event()->setMaybe<String>(m_properties.DISPLAY_ACCIDENTAL, dacc);
	if (someAccidental == NoAccidental) someAccidental = dacc;

	newAccTable.update(acc, h);
	if (i == to) barEndsInChord = true;

	if (i == chord.getFinalElement()) break;
    }

    accTable.copyFrom(newAccTable);
    
    float extraWidth = 0;

    if (someAccidental != NoAccidental) {
	extraWidth += m_npf->getAccidentalWidth(someAccidental);
    }

    if (chord.hasNoteHeadShifted()) {
	extraWidth = std::max(extraWidth, float(m_npf->getNoteBodyWidth()));
    }

    if (grace) {
//	fixedWidth += m_npf->getNoteBodyWidth();
	chunks.push_back(Chunk(-1, extraWidth + m_npf->getNoteBodyWidth()));
	return;
    }

    NotationElementList::iterator myLongest = chord.getLongestElement();
    if (myLongest == notes->end()) {
	NOTATION_DEBUG << "WARNING: NotationHLayout::scanChord: No longest element in chord!" << endl;
    }

    timeT d = (*myLongest)->getViewDuration();

    chunks.push_back(Chunk(d, 0, extraWidth,
			   std::max(getLayoutWidth(**myLongest),
				    lyricWidth)));

    itr = chord.getFinalElement();
    if (barEndsInChord) { to = itr; ++to; }
    return;
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

    columns.clear();

    for (BarDataMap::iterator mi = m_barData.begin();
	 mi != m_barData.end(); ++mi) {

	BarDataList &bdl = mi->second;
	BarDataList::iterator bdli = bdl.find(barNo);
	if (bdli != bdl.end()) {
	    haveSomething = true;
	    ChunkList &cl(bdli->second.chunks);
	    timeT aggregateTime = 0;
	    for (ChunkList::iterator cli = cl.begin(); cli != cl.end(); ++cli) {
		columns[ChunkLocation(aggregateTime, cli->subordering)].
		    push_back(&(*cli));
		aggregateTime += cli->duration;
	    }
	}
    }

    if (!haveSomething) return;

    // now modify chunks in-place

    // What we want to do here is idle along the whole set of chunk
    // lists, inspecting all the chunks that occur at each moment in
    // turn and choosing a "rate" from the "slowest" of these
    // (i.e. most space per time)

    float x = 0.0;
    timeT prevTime = 0;
    double prevRate = 0.0;
    float maxStretchy = 0.0;

    for (ColumnMap::iterator i = columns.begin(); i != columns.end(); ++i) {

	timeT time = i->first.time;
	ChunkRefList &list = i->second;

	double minRate = -1.0;
	float totalFixed = 0.0;
	maxStretchy = 0.0;

	for (ChunkRefList::iterator j = list.begin(); j != list.end(); ++j) {
	    if ((*j)->stretchy > 0.0) {
		double rate = (*j)->duration / (*j)->stretchy; // time per px
		if (minRate < 0.0 || rate < minRate) minRate = rate;
	    }
	    maxStretchy = std::max(maxStretchy, (*j)->stretchy);
	    totalFixed = std::max(totalFixed, (*j)->fixed);
	}

	// we have the rate from this point, but we want to assign
	// these elements an x coord based on the rate and distance
	// from the previous point, plus fixed space for this point
	// if it's a note (otherwise fixed space goes afterwards)

	if (i->first.subordering == 0) x += totalFixed;
	if (prevRate > 0.0) x += (time - prevTime) / prevRate;
	
	for (ChunkRefList::iterator j = list.begin(); j != list.end(); ++j) {
	    NOTATION_DEBUG << "Setting x for time " << time << " to " << x << " in chunk at " << *j << endl;
	    (*j)->x = x;
	}
	
	if (i->first.subordering != 0) x += totalFixed;

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
		bdli->second.sizeData.fixedWidth + x + getBarMargin();

	    bdli->second.sizeData.reconciledWidth =
		bdli->second.sizeData.idealWidth;
	}
    }
}

Staff *
NotationHLayout::getStaffWithWidestBar(int barNo)
{
    float maxWidth = -1;
    Staff *widest = 0;

    for (BarDataMap::iterator mi = m_barData.begin();
	 mi != m_barData.end(); ++mi) {

	BarDataList &list = mi->second;
	BarDataList::iterator li = list.find(barNo);
	if (li != list.end()) {

	    NOTATION_DEBUG << "getStaffWithWidestBar: idealWidth is " << li->second.sizeData.idealWidth << endl;

	    if (li->second.sizeData.idealWidth == 0.0) {
		NOTATION_DEBUG << "getStaffWithWidestBar(" << barNo << "): found idealWidth of zero, presquishing" << endl;
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

void
NotationHLayout::reconcileBarsLinear()
{
    Rosegarden::Profiler profiler("NotationHLayout::reconcileBarsLinear");

    // Ensure that concurrent bars on all staffs have the same width,
    // which for now we make the maximum width required for this bar
    // on any staff.

    int barNo = getFirstVisibleBar();

    m_totalWidth = 0.0;
    for (StaffIntMap::iterator i = m_staffNameWidths.begin();
	 i != m_staffNameWidths.end(); ++i) {
	if (i->second > m_totalWidth) m_totalWidth = double(i->second);
    }

    for (;;) {

	Staff *widest = getStaffWithWidestBar(barNo);

	if (!widest) {
	    // have we reached the end of the piece?
	    if (barNo >= getLastVisibleBar()) { // yes
		break;
	    } else {
		m_totalWidth += m_spacing / 3;
		NOTATION_DEBUG << "Setting bar position for degenerate bar "
			       << barNo << " to " << m_totalWidth << endl;

		m_barPositions[barNo] = m_totalWidth;
		++barNo;
		continue;
	    }
	}

	float maxWidth = m_barData[widest].find(barNo)->second.sizeData.idealWidth;
	if (m_pageWidth > 0.1 && maxWidth > m_pageWidth) {
	    maxWidth = m_pageWidth;
	}

	NOTATION_DEBUG << "Setting bar position for bar " << barNo
			     << " to " << m_totalWidth << endl;

	m_barPositions[barNo] = m_totalWidth;
	m_totalWidth += maxWidth;

	// Now apply width to this bar on all staffs

	for (BarDataMap::iterator i = m_barData.begin();
	     i != m_barData.end(); ++i) {

	    BarDataList &list = i->second;
	    BarDataList::iterator bdli = list.find(barNo);
	    if (bdli != list.end()) {

		BarData::SizeData &bd(bdli->second.sizeData);

		NOTATION_DEBUG << "Changing width from " << bd.reconciledWidth << " to " << maxWidth << endl;

		double diff = maxWidth - bd.reconciledWidth;
		if (diff < -0.1 || diff > 0.1) {
		    NOTATION_DEBUG << "(So needsLayout becomes true)" << endl;
		    bdli->second.layoutData.needsLayout = true;
		}
		bd.reconciledWidth = maxWidth;
	    }
	}

	++barNo;
    }

    NOTATION_DEBUG << "Setting bar position for bar " << barNo
		   << " to " << m_totalWidth << endl;

    m_barPositions[barNo] = m_totalWidth;
}	


void
NotationHLayout::reconcileBarsPage()
{
    Rosegarden::Profiler profiler("NotationHLayout::reconcileBarsPage");

    int barNo = getFirstVisibleBar();
    int barNoThisRow = 0;
    
    // pair of the recommended number of bars with those bars'
    // original total width, for each row
    std::vector<std::pair<int, double> > rowData;

    double stretchFactor = 10.0;
    double maxStaffNameWidth = 0.0;

    for (StaffIntMap::iterator i = m_staffNameWidths.begin();
	 i != m_staffNameWidths.end(); ++i) {
	if (i->second > maxStaffNameWidth) {
	    maxStaffNameWidth = double(i->second);
	}
    }

    double pageWidthSoFar = maxStaffNameWidth;
    m_totalWidth = maxStaffNameWidth + getPreBarMargin();

    NOTATION_DEBUG << "NotationHLayout::reconcileBarsPage: pageWidthSoFar is " << pageWidthSoFar << endl;

    for (;;) {
	
	Staff *widest = getStaffWithWidestBar(barNo);
	double maxWidth = m_spacing / 3;

	if (!widest) {
	    // have we reached the end of the piece?
	    if (barNo >= getLastVisibleBar()) break; // yes
	} else {
	    maxWidth =
		m_barData[widest].find(barNo)->second.sizeData.idealWidth;
	}

	// Work on the assumption that this bar is the last in the
	// row.  How would that make things look?

	double nextPageWidth = pageWidthSoFar + maxWidth;
	double nextStretchFactor = m_pageWidth / nextPageWidth;

	NOTATION_DEBUG << "barNo is " << barNo << ", maxWidth " << maxWidth << ", nextPageWidth " << nextPageWidth << ", nextStretchFactor " << nextStretchFactor << ", m_pageWidth " << m_pageWidth << endl;

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
	    pageWidthSoFar = maxWidth;
	    stretchFactor = m_pageWidth / maxWidth;
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

	pageWidthSoFar = (row > 0 ? 0 : maxStaffNameWidth + getPreBarMargin());
	stretchFactor = m_pageWidth / rowData[row].second;

	for (; barNoThisRow <= finalBarThisRow; ++barNoThisRow, ++barNo) {

	    bool finalRow = (row == rowData.size()-1);

	    Staff *widest = getStaffWithWidestBar(barNo);
	    if (finalRow && (stretchFactor > 1.0)) stretchFactor = 1.0;
	    double maxWidth = 0.0;

	    if (!widest) {
		// have we reached the end of the piece? (shouldn't happen)
		if (barNo >= getLastVisibleBar()) break; // yes
		else maxWidth = stretchFactor * (m_spacing / 3);
	    } else {
		maxWidth =
		    (stretchFactor *
		     m_barData[widest].find(barNo)->
		     second.sizeData.idealWidth);
	    }

	    if (barNoThisRow == finalBarThisRow) {
		if (!finalRow ||
		    (maxWidth > (m_pageWidth - pageWidthSoFar))) {
		    maxWidth = m_pageWidth - pageWidthSoFar;
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


// and for once I swear things will still be good tomorrow

NotationHLayout::AccidentalTable::AccidentalTable(Rosegarden::Key key, Clef clef) :
    m_key(key), m_clef(clef)
{
    std::vector<int> heights(key.getAccidentalHeights(clef));
    unsigned int i;

    for (i = 0; i < 7; ++i) m_accidentals[i] = NoAccidental;
    for (i = 0; i < heights.size(); ++i) {
        m_accidentals[Rosegarden::Key::canonicalHeight(heights[i])] =
            (key.isSharp() ? Sharp : Flat);
    }
}

NotationHLayout::AccidentalTable::AccidentalTable(const AccidentalTable &t) :
    m_key(t.m_key), m_clef(t.m_clef)
{
    copyFrom(t);
}

NotationHLayout::AccidentalTable &
NotationHLayout::AccidentalTable::operator=(const AccidentalTable &t)
{
    if (&t != this) {
	m_key = t.m_key;
	m_clef = t.m_clef;
	copyFrom(t);
    }
    return *this;
}

Accidental
NotationHLayout::AccidentalTable::getDisplayAccidental(Accidental accidental,
                                                       int height) const
{
    height = Rosegarden::Key::canonicalHeight(height);

    if (accidental == NoAccidental) {
        accidental = m_key.getAccidentalAtHeight(height, m_clef);
    }

//    NOTATION_DEBUG << "accidental = " << accidental << ", stored accidental at height " << height << " is " << (*this)[height] << endl;

    if (m_accidentals[height] != NoAccidental) {

        if (accidental == m_accidentals[height]) {
            return NoAccidental;
        } else if (accidental == NoAccidental || accidental == Natural) {
            return Natural;
        } else {
            //!!! aargh.  What we really want to do now is have two
            //accidentals shown: first a natural, then the one
            //required for the note.  But there's no scope for that in
            //our accidental structure (RG2.1 is superior here)
            return accidental;
        }
    } else {
        return accidental;
    }
}

void
NotationHLayout::AccidentalTable::update(Accidental accidental, int height)
{
    height = Rosegarden::Key::canonicalHeight(height);

    if (accidental == NoAccidental) {
        accidental = m_key.getAccidentalAtHeight(height, m_clef);
    }

//    NOTATION_DEBUG << "updating height" << height << " from " << (*this)[height] << " to " << accidental << endl;


    //!!! again, we can't properly deal with the difficult case where
    //we already have an accidental at height but it's not the same
    //accidental

    m_accidentals[height] = accidental;
}

void
NotationHLayout::AccidentalTable::copyFrom(const AccidentalTable &t)
{
    for (int i = 0; i < 7; ++i) {
	m_accidentals[i] = t.m_accidentals[i];
    }
}

void
NotationHLayout::finishLayout(timeT startTime, timeT endTime)
{
    Rosegarden::Profiler profiler("NotationHLayout::finishLayout");
    m_barPositions.clear();

    bool isFullLayout = (startTime == endTime);
/*!!!
    if (isFullLayout) {
	startTime = 0;
	endTime = getComposition()->getDuration();
    }
    int startBar = getComposition()->getBarNumber(startTime);
    int   endBar = getComposition()->getBarNumber(endTime);
    for (int barNo = startBar; barNo <= endBar; ++barNo) preSquishBar(barNo);
*/
    if (m_pageMode && (m_pageWidth > 0.1)) reconcileBarsPage();
    else reconcileBarsLinear();

    int staffNo = 0;
      
    for (BarDataMap::iterator i(m_barData.begin());
	 i != m_barData.end(); ++i) {
	
        emit setProgress(100 * staffNo / m_barData.size());
        kapp->processEvents(50);

        throwIfCancelled();

        timeT timeCovered = endTime - startTime;
	    
        if (isFullLayout) {
            NotationElementList *notes = i->first->getViewElementList();
            if (notes->begin() != notes->end()) {
                NotationElementList::iterator j(notes->end());
                timeCovered =
                    (*--j)->getViewAbsoluteTime() -
                    (*notes->begin())->getViewAbsoluteTime();
            }
        }

        m_timePerProgressIncrement = timeCovered / (100 / m_barData.size());

	layout(i, startTime, endTime);
	++staffNo;
    }
}

void
NotationHLayout::layout(BarDataMap::iterator i, timeT startTime, timeT endTime)
{
    Rosegarden::Profiler profiler("NotationHLayout::layout");

    Staff &staff = *(i->first);
    NotationElementList *notes = staff.getViewElementList();
    BarDataList &barList(getBarData(staff));
    NotationStaff &notationStaff = dynamic_cast<NotationStaff &>(staff);

    bool isFullLayout = (startTime == endTime);

    // these two are for partial layouts:
//    bool haveSimpleOffset = false;
//    double simpleOffset = 0;

    NOTATION_DEBUG << "NotationHLayout::layout: full layout " << isFullLayout << ", times " << startTime << "->" << endTime << endl;

    double x = 0, barX = 0;
    TieMap tieMap;

    timeT lastIncrement =
	(isFullLayout && (notes->begin() != notes->end())) ?
	(*notes->begin())->getViewAbsoluteTime() : startTime;

    Rosegarden::Key key = notationStaff.getSegment().getKeyAtTime(lastIncrement);;
    Clef clef = notationStaff.getSegment().getClefAtTime(lastIncrement);
    TimeSignature timeSignature;

    int startBar = getComposition()->getBarNumber(startTime);

    for (BarPositionList::iterator bpi = m_barPositions.begin();
	 bpi != m_barPositions.end(); ++bpi) {

	int barNo = bpi->first;
	if (!isFullLayout && barNo < startBar) continue;

	NOTATION_DEBUG << "NotationHLayout::looking for bar "
			     << bpi->first << endl;
	BarDataList::iterator bdi = barList.find(barNo);
	if (bdi == barList.end()) continue;
	barX = bpi->second;

        NotationElementList::iterator from = bdi->second.basicData.start;
        NotationElementList::iterator to;

        NOTATION_DEBUG << "NotationHLayout::layout(): starting bar " << barNo << ", x = " << barX << ", width = " << bdi->second.sizeData.idealWidth << ", time = " << (from == notes->end() ? -1 : (*from)->getViewAbsoluteTime()) << endl;

        BarDataList::iterator nbdi(bdi);
        if (++nbdi == barList.end()) {
            to = notes->end();
        } else {
            to = nbdi->second.basicData.start;
        }

        if (from == notes->end()) {
            NOTATION_DEBUG << "Start is end" << endl;
        }
        if (from == to) {
            NOTATION_DEBUG << "Start is to" << endl;
        }

	if (!bdi->second.layoutData.needsLayout) {

            NOTATION_DEBUG << "NotationHLayout::layout(): bar " << barNo << " has needsLayout false" << endl;

	    double offset = barX - bdi->second.layoutData.x;

	    if (offset > -0.1 && offset < 0.1) {
		NOTATION_DEBUG << "NotationHLayout::layout(): no offset, ignoring" << endl;
		continue;
	    }

            bdi->second.layoutData.x += offset;
	    
            if (bdi->second.basicData.newTimeSig)
                bdi->second.layoutData.timeSigX += (int)offset;
	    
            for (NotationElementList::iterator it = from;
		 it != to && it != notes->end(); ++it) {

                NotationElement* nel = static_cast<NotationElement*>(*it);
                nel->setLayoutX((*it)->getLayoutX() + offset);
		double airX, airWidth;
		nel->getLayoutAirspace(airX, airWidth);
		nel->setLayoutAirspace(airX + offset, airWidth);
	    }

	    continue;
	}
		

#ifdef NOT_DEFINED
        if (!isFullLayout &&
	    !bdi->second.layoutData.needsLayout &&
	    (from == notes->end() ||
	     (*from)->getViewAbsoluteTime() > endTime)) {

	    //!!! don't seem to be getting here when we should
	    NOTATION_DEBUG << "Shifting elements only" << endl;

            // Find how far to move everything if necessary
            if (!haveSimpleOffset) {
                simpleOffset = barX - bdi->second.layoutData.x;
                haveSimpleOffset = true;
            }

            // Move all elements

            bdi->second.layoutData.x += simpleOffset;
	    
            if (bdi->second.basicData.newTimeSig)
                bdi->second.layoutData.timeSigX += (int) simpleOffset;

            for (NotationElementList::iterator it = from;
		 it != notes->end() && it != to; ++it) {
                NotationElement* nel = static_cast<NotationElement*>(*it);
                nel->setLayoutX((*it)->getLayoutX() + simpleOffset);
		double airX, airWidth;
		nel->getLayoutAirspace(airX, airWidth);
		nel->setLayoutAirspace(airX + simpleOffset, airWidth);
            }

            // And skip the real layout work
//!!!            bdi->second.layoutData.needsLayout = false;
            continue;
        }

        if (!bdi->second.layoutData.needsLayout) {
            NOTATION_DEBUG << "NotationHLayout::layout(): bar " << barNo << " has needsLayout false" << endl;
            continue;
        }
#endif

	bdi->second.layoutData.x = barX;
//	x = barX + getPostBarMargin();

	bool timeSigToPlace = false;
	if (bdi->second.basicData.newTimeSig) {
	    timeSignature = bdi->second.basicData.timeSignature;
	    timeSigToPlace = true;
	}

        if (timeSigToPlace) {
	    NOTATION_DEBUG << "NotationHLayout::layout(): there's a time sig in this bar" << endl;
	}

	NotationElement *lastDynamicText = 0;
	int count = 0;

	ChunkList &chunks = bdi->second.chunks;
	ChunkList::iterator chunkitr = chunks.begin();
	double reconcileRatio = 1.0;
	if (bdi->second.sizeData.idealWidth > 0.0) {
	    reconcileRatio =
		bdi->second.sizeData.reconciledWidth /
		bdi->second.sizeData.idealWidth;
	}

	double offset = getPostBarMargin();

        for (NotationElementList::iterator it = from; it != to; ++it) {

            NotationElement *el = static_cast<NotationElement*>(*it);

	    double delta = 0;
	    if (chunkitr != chunks.end()) {
//		NOTATION_DEBUG << "barX is " << barX << ", reconcileRatio is " << reconcileRatio << " (" << bdi->second.sizeData.reconciledWidth << "/" << bdi->second.sizeData.idealWidth << ") , chunk's x is " << (*chunkitr).x << ", chunk is at " << &(*chunkitr) << endl;
		x = barX + offset + reconcileRatio * (*chunkitr).x;
//		NOTATION_DEBUG << " x is " << x << endl;
		ChunkList::iterator chunkscooter(chunkitr);
		if (++chunkscooter != chunks.end()) {
		    delta = (*chunkscooter).x - (*chunkitr).x;
		} else {
		    delta = bdi->second.sizeData.reconciledWidth -
			bdi->second.sizeData.fixedWidth - (*chunkitr).x;
		}
		delta *= reconcileRatio;
		++chunkitr;
	    } else {
		x = barX + bdi->second.sizeData.reconciledWidth - getPreBarMargin();
		delta = 0;
	    }

	    if (timeSigToPlace &&
		!el->event()->isa(Rosegarden::Clef::EventType) &&
		!el->event()->isa(Rosegarden::Key::EventType)) {
		NOTATION_DEBUG << "Placing timesig at " << x << endl;
		bdi->second.layoutData.timeSigX = (int)x;
		double shift = getFixedItemSpacing()*2 +
		    m_npf->getTimeSigWidth(timeSignature);
		offset += shift;
		x += shift;
		NOTATION_DEBUG << "and moving next elt to " << x << endl;
		timeSigToPlace = false;
	    }

            NOTATION_DEBUG << "NotationHLayout::layout(): setting element's x to " << x << endl;

	    el->setLayoutX(x);
	    el->setLayoutAirspace(x, int(delta));

	    if (el->isNote()) {

		// This modifies "it" and "tieMap"
		positionChord(staff, it, bdi, timeSignature, clef, key, tieMap, to);

	    } else if (el->isRest()) {

		// nothing to do

	    } else if (el->event()->isa(Clef::EventType)) {
		
		clef = Clef(*el->event());

	    } else if (el->event()->isa(Rosegarden::Key::EventType)) {

		key = Rosegarden::Key(*el->event());

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
			(Rosegarden::Text(*lastDynamicText->event())) +
			m_npf->getNoteBodyWidth() / 4;
		}

		el->setLayoutX(ix);
		el->setLayoutAirspace(ix, delta - (ix - x));

	    } else {    

		// nothing else
	    }

	    if (it != to && el->event()->has(BEAMED_GROUP_ID)) {

		//!!! Gosh.  We need some clever logic to establish
		// whether one group is happening while another has
		// not yet ended -- perhaps we decide one has ended
		// if we see another, and then re-open the case of
		// the first if we meet another note that claims to
		// be in it.  Then we need to hint to both of the
		// groups that they should choose appropriate stem
		// directions -- we could just use HEIGHT_ON_STAFF
		// of their first notes to determine this, as if that
		// doesn't work, nothing will

		long groupId = el->event()->get<Int>(BEAMED_GROUP_ID);
//		NOTATION_DEBUG << "group id: " << groupId << endl;
		if (m_groupsExtant.find(groupId) == m_groupsExtant.end()) {
//		    NOTATION_DEBUG << "(new group)" << endl;
		    m_groupsExtant[groupId] =
			new NotationGroup(*staff.getViewElementList(),
					  m_notationQuantizer,
					  m_properties, clef, key);
		}
		m_groupsExtant[groupId]->sample(it);
	    }

	    x += delta;
	    
	    if (m_timePerProgressIncrement > 0 && (++count == 100)) {
		count = 0;
		timeT sinceIncrement = el->getViewAbsoluteTime() - lastIncrement;
		if (sinceIncrement > m_timePerProgressIncrement) {
		    emit incrementProgress
			(sinceIncrement / m_timePerProgressIncrement);
		    lastIncrement +=
			(sinceIncrement / m_timePerProgressIncrement)
			* m_timePerProgressIncrement;
		    throwIfCancelled();
		}
	    }
        }

	if (timeSigToPlace) {
	    // no other events in this bar, so we never managed to place it
	    NOTATION_DEBUG << "Placing timesig at " << x << endl;
	    bdi->second.layoutData.timeSigX = (int)x;
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


timeT
NotationHLayout::getSpacingDuration(Staff &staff,
				    const NotationElementList::iterator &i)
{
    SegmentNotationHelper helper(staff.getSegment());
    timeT t((*i)->getViewAbsoluteTime());
    timeT d((*i)->getViewDuration());

    NotationElementList::iterator j(i), e(staff.getViewElementList()->end());
    while (j != e && ((*j)->getViewAbsoluteTime() == t ||
		      (*j)->getViewDuration() == 0)) {
	++j;
    }
    if (j == e) return d;
    else return (*j)->getViewAbsoluteTime() - (*i)->getViewAbsoluteTime();
}

timeT
NotationHLayout::getSpacingDuration(Staff &staff,
				    const NotationChord &chord)
{
    SegmentNotationHelper helper(staff.getSegment());

    NotationElementList::iterator i = chord.getShortestElement();
    timeT d((*i)->getViewDuration());

    NotationElementList::iterator j(i), e(staff.getViewElementList()->end());
    while (j != e && (chord.contains(j) || (*j)->getViewDuration() == 0)) ++j;

    if (j != e) {
	d = (*j)->getViewAbsoluteTime() - (*i)->getViewAbsoluteTime();
    }

    return d;
}


void
NotationHLayout::positionChord(Staff &staff,
                               NotationElementList::iterator &itr,
				const BarDataList::iterator &, //!!!unreqd?
				const TimeSignature &, //!!! unreqd?
			       const Clef &clef, const Rosegarden::Key &key,
			       TieMap &tieMap, 
			       NotationElementList::iterator &to)
{
    NotationChord chord(*staff.getViewElementList(), itr, m_notationQuantizer,
			m_properties, clef, key);
    double baseX, delta;
    (static_cast<NotationElement *>(*itr))->getLayoutAirspace(baseX, delta);

    bool barEndsInChord = false;

    // To work out how much space to allot a note (or chord), start
    // with the amount alloted to the whole bar, subtract that
    // reserved for fixed-width items, and take the same proportion of
    // the remainder as our duration is of the whole bar's duration.
    // (We use the actual duration of the bar, not the nominal time-
    // signature duration.)

    // In case this chord has various durations in it, we choose an
    // effective duration based on the absolute time of the first
    // following event not in the chord (see getSpacingDuration)

//    int noteWidth = m_npf->getNoteBodyWidth();

    // Find out whether the chord contains any accidentals, and if so,
    // make space, and also shift the notes' positions right somewhat.
    // (notepixmapfactory quite reasonably places the hot spot at the
    // start of the note head, not at the start of the whole pixmap.)
    // Also use this loop to check for beamed-group information.

    unsigned int i;
//    int accWidth = 0;
//    long groupId = -1;

    for (i = 0; i < chord.size(); ++i) {

	if (chord[i] == to) barEndsInChord = true;

	static_cast<NotationElement*>(*chord[i])->setLayoutX(baseX);
	static_cast<NotationElement*>(*chord[i])->setLayoutAirspace(baseX, delta);

	NotationElement *note = static_cast<NotationElement*>(*(chord[i]));
	if (!note->isNote()) continue;
/*!!!
	Accidental acc = NoAccidental;
	if (note->event()->get<String>(m_properties.DISPLAY_ACCIDENTAL, acc) &&
	    acc != NoAccidental) {
            accWidth = std::max(accWidth, m_npf->getAccidentalWidth(acc));
	}
	if (groupId != -2) {
	    long myGroupId = -1;
	    if (note->event()->get<Int>(BEAMED_GROUP_ID, myGroupId) &&
		(groupId == -1 || myGroupId == groupId)) {
		groupId = myGroupId;
	    } else {

		// With a bit of luck, this should never happen any more
		// (as Chord has been reworked so as to only accept note
		// heads that are in the same group -- this situation
		// should now result in more than one Chord)

		groupId = -2; // not all note-heads think they're in the group
	    }
	}
*/
    }
/*!!!
    baseX += accWidth;

    // Cope with the presence of shifted note-heads

    bool shifted = chord.hasNoteHeadShifted();

    if (shifted) {
	// For this to work reliably with notes inside beamed groups,
	// we depend on the group's applyStemProperties() method
	// having already been called (as it is in scanStaff above).
	if (!chord.hasStemUp()) baseX += noteWidth;
    }
*/
    // Check for any ties going back, and if so work out how long it
    // must have been and assign accordingly.

    for (i = 0; i < chord.size(); ++i) {

	NotationElement *note = static_cast<NotationElement*>(*(chord[i]));
	if (!note->isNote()) continue;

	bool tiedForwards = false;
	bool tiedBack = false;

	note->event()->get<Bool>(TIED_FORWARD,  tiedForwards);
	note->event()->get<Bool>(TIED_BACKWARD, tiedBack);

	int pitch = note->event()->get<Int>(PITCH);

	if (tiedBack) {
	    TieMap::iterator ti(tieMap.find(pitch));

	    if (ti != tieMap.end()) {
		NotationElementList::iterator otherItr(ti->second);
		
		if ((*otherItr)->getViewAbsoluteTime() +
		    (*otherItr)->getViewDuration() ==
		    note->getViewAbsoluteTime()) {

		    NOTATION_DEBUG << "Second note in tie at " << note->getViewAbsoluteTime() << ": found first note, it matches" << endl;
		    
		    (*otherItr)->event()->setMaybe<Int>
			(m_properties.TIE_LENGTH,
			 (int)(baseX - (*otherItr)->getLayoutX()));
		    
		} else {
		    NOTATION_DEBUG << "Second note in tie at " << note->getViewAbsoluteTime() << ": found first note but it ends at " << ((*otherItr)->getViewAbsoluteTime() + (*otherItr)->getViewDuration()) << endl;
		    
		    tieMap.erase(pitch);
		}
	    }		
	}

	if (tiedForwards) {
	    note->event()->setMaybe<Int>(m_properties.TIE_LENGTH, 0);
	    tieMap[pitch] = chord[i];
	} else {
	    note->event()->unset(m_properties.TIE_LENGTH);
	}
    }

    // Now set the positions of all the notes in the chord.  We don't
    // need to shift the positions of shifted notes, because that will
    // be taken into account when making their pixmaps later (in
    // NotationStaff::makeNoteSprite / NotePixmapFactory::makeNotePixmap).
/*!!!
    bool barEndsInChord = false;
    for (i = 0; i < chord.size(); ++i) {
	NotationElementList::iterator subItr = chord[i];
	if (subItr == to) barEndsInChord = true;
	(*subItr)->setLayoutX(baseX);
	if (groupId < 0) (*chord[i])->event()->unset(m_properties.BEAMED);
	else (*chord[i])->event()->set<Int>(BEAMED_GROUP_ID, groupId);
    }
*/
    itr = chord.getFinalElement();
    if (barEndsInChord) { to = itr; ++to; }
}


float
NotationHLayout::getLayoutWidth(Rosegarden::ViewElement &ve) const
{
    NotationElement& e = static_cast<NotationElement&>(ve);

    if (e.isNote() || e.isRest()) {

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

	double multipliers[][8] = {
	    { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 },
	    { 0.4, 0.6, 0.8, 1.0, 1.4, 2.0, 2.7, 4.0 },
	    { 0.2, 0.2, 0.5, 1.0, 1.5, 3.0, 4.5, 7.0 },
	    { 0.2, 0.2, 0.5, 1.0, 1.5, 3.0, 4.5, 7.0 },
	    { 0.2, 0.2, 0.5, 1.0, 1.5, 3.0, 4.5, 7.0 },
	    { 0.2, 0.2, 0.5, 1.0, 1.5, 3.0, 4.5, 7.0 }
	};

	int proportionIndex = m_proportion / 20;
	if (proportionIndex < 0) proportionIndex = 0;
	if (proportionIndex > 5) proportionIndex = 5;
	if (noteType < 0) noteType = 0;
	if (noteType > 7) noteType = 7;
	double multiplier = multipliers[proportionIndex][noteType];
	if (noteType < 7 && dots > 0) {
	    multiplier += multipliers[proportionIndex][noteType+1];
	    multiplier /= 2;
	}
	double gap = m_npf->getNoteBodyWidth(noteType) * multiplier;
	gap = gap * m_spacing / 100.0;
	return bw + gap;

/*!!!
	//!!! This is where we plug in different spacings...
	switch (noteType) {
	case Note::Hemidemisemiquaver:
	case Note::Demisemiquaver:
	case Note::Semiquaver:         gap = bw / 3;     break;
	case Note::Quaver:             gap = bw * 2 / 3; break;
	case Note::Crotchet:           gap = bw * 3 / 2; break;
	case Note::Minim:              gap = bw * 3;     break;
	case Note::Semibreve:          gap = bw * 9 / 2; break;
	case Note::Breve:              gap = bw * 7;     break;
	}
*/
//	return bw + m_spacing * gap / 100;

    } else {

	double w = getFixedItemSpacing();

	if (e.event()->isa(Clef::EventType)) {

	    w += m_npf->getClefWidth(Clef(*e.event()));

	} else if (e.event()->isa(Rosegarden::Key::EventType)) {

	    w += m_npf->getKeyWidth(Rosegarden::Key(*e.event()));

	} else if (e.event()->isa(Indication::EventType) ||
		   e.event()->isa(Text::EventType)) {

	    w = 0;

	} else {
	    NOTATION_DEBUG << "NotationHLayout::getMinWidth(): no case for event type " << e.event()->getType() << endl;
	    w += 24;
	}

	return w + m_spacing * w / 300;
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
    return (int)((m_npf->getNoteBodyWidth() / 5) * m_spacing / 100.0);
}

void
NotationHLayout::reset()
{
    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	clearBarList(*i->first);
    }

    m_barData.clear();
    m_barPositions.clear();
    m_totalWidth = 0;
}

void
NotationHLayout::resetStaff(Staff &staff, timeT startTime, timeT endTime)
{
    if (startTime == endTime) {
        getBarData(staff).clear();
        m_totalWidth = 0;
    }
}

int
NotationHLayout::getFirstVisibleBar()
{
    int bar = 0;
    bool haveBar = false;
    for (BarDataMap::iterator i(m_barData.begin()); i != m_barData.end(); ++i) {
	if (i->second.begin() == i->second.end()) continue;
	int barHere = i->second.begin()->first;
	if (barHere < bar || !haveBar) {
	    bar = barHere;
	    haveBar = true;
	}
    }

//    NOTATION_DEBUG << "NotationHLayout::getFirstVisibleBar: returning " << bar << endl;

    return bar;
}

int
NotationHLayout::getFirstVisibleBarOnStaff(Staff &staff)
{
    BarDataList &bdl(getBarData(staff));

    int bar = 0;
    if (bdl.begin() != bdl.end()) bar = bdl.begin()->first;

//    NOTATION_DEBUG << "NotationHLayout::getFirstVisibleBarOnStaff: returning " << bar << endl;

    return bar;
}

int
NotationHLayout::getLastVisibleBar()
{
    int bar = 0;
    bool haveBar = false;
    for (BarDataMap::iterator i = m_barData.begin();
	 i != m_barData.end(); ++i) {
	if (i->second.begin() == i->second.end()) continue;
	int barHere = getLastVisibleBarOnStaff(*i->first);
	if (barHere > bar || !haveBar) {
	    bar = barHere;
	    haveBar = true;
	}
    }

//    NOTATION_DEBUG << "NotationHLayout::getLastVisibleBar: returning " << bar << endl;

    return bar;
}

int
NotationHLayout::getLastVisibleBarOnStaff(Staff &staff)
{
    BarDataList &bdl(getBarData(staff));
    int bar = 0;

    if (bdl.begin() != bdl.end()) {
	BarDataList::iterator i(bdl.end());
	bar = ((--i)->first) + 1; // last visible bar_line_
    }

//    NOTATION_DEBUG << "NotationHLayout::getLastVisibleBarOnStaff: returning " << bar << endl;

    return bar;
}

double
NotationHLayout::getBarPosition(int bar)
{
    double position = 0.0;

    BarPositionList::iterator i = m_barPositions.find(bar);

    if (i != m_barPositions.end()) {

	position = i->second;

    } else {

	i = m_barPositions.begin();
	if (i != m_barPositions.end()) {
	    if (bar < i->first) position = i->second;
	    else {
		i = m_barPositions.end();
		--i;
		if (bar > i->first) position = i->second;
	    }
	}
    }

//    NOTATION_DEBUG << "NotationHLayout::getBarPosition: returning " << position << " for bar " << bar << endl;

    return position;
}

bool
NotationHLayout::isBarCorrectOnStaff(Staff &staff, int i)
{
    BarDataList &bdl(getBarData(staff));
    ++i;

    BarDataList::iterator bdli(bdl.find(i));
    if (bdli != bdl.end()) return bdli->second.basicData.correct;
    else return true;
}

bool NotationHLayout::getTimeSignaturePosition(Staff &staff,
					       int i,
					       TimeSignature &timeSig,
					       double &timeSigX)
{
    BarDataList &bdl(getBarData(staff));

    BarDataList::iterator bdli(bdl.find(i));
    if (bdli != bdl.end()) {
	timeSig = bdli->second.basicData.timeSignature;
	timeSigX = (double)(bdli->second.layoutData.timeSigX);
	return bdli->second.basicData.newTimeSig;
    } else return 0;
}

