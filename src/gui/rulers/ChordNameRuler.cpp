/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ChordNameRuler.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AnalysisTypes.h"
#include "base/Composition.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/Instrument.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/PropertyName.h"
#include "base/NotationQuantizer.h"
#include "base/RefreshStatus.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/general/GUIPalette.h"

#include <QPaintEvent>
#include <QFont>
#include <QFontMetrics>
#include <QObject>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QToolTip>
#include <QWidget>


namespace Rosegarden
{

ChordNameRuler::ChordNameRuler(RulerScale *rulerScale,
                               RosegardenDocument *doc,
                               double xorigin,
                               int height,
                               QWidget *parent) :
        QWidget(parent),
        m_xorigin(xorigin),
        m_height(height),
        m_currentXOffset(0),
        m_width( -1),
        m_ready(false),
        m_rulerScale(rulerScale),
        m_composition(&doc->getComposition()),
        m_regetSegmentsOnChange(true),
        m_currentSegment(0),
        m_studio(0),
        m_chordSegment(0),
        m_fontMetrics(m_boldFont),
        TEXT_FORMAL_X("TextFormalX"),
        TEXT_ACTUAL_X("TextActualX")
{
    m_font.setPointSize(11);
    m_font.setPixelSize(12);
    m_boldFont.setPointSize(11);
    m_boldFont.setPixelSize(12);
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);
//    setBackgroundColor(GUIPalette::getColour(GUIPalette::ChordNameRulerBackground));

    m_compositionRefreshStatusId = m_composition->getNewRefreshStatusId();

    QObject::connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
                     this, SLOT(update()));

    this->setToolTip(tr("<qt><p>Chord name ruler.  This ruler analyzes your harmonies and attempts to guess what chords your composition contains.  These chords cannot be printed or manipulated, and this is only a reference for your information.</p><p>Turn it on and off with the <b>View -> Rulers</b> menu.</p></qt>"));
}

ChordNameRuler::ChordNameRuler(RulerScale *rulerScale,
                               RosegardenDocument *doc,
                               std::vector<Segment *> &segments,
                               double xorigin,
                               int height,
                               QWidget *parent) :
        QWidget(parent),
        m_xorigin(xorigin),
        m_height(height),
        m_currentXOffset(0),
        m_width( -1),
        m_ready(false),
        m_rulerScale(rulerScale),
        m_composition(&doc->getComposition()),
        m_regetSegmentsOnChange(false),
        m_currentSegment(0),
        m_studio(0),
        m_chordSegment(0),
        m_fontMetrics(m_boldFont),
        TEXT_FORMAL_X("TextFormalX"),
        TEXT_ACTUAL_X("TextActualX")
{
    m_font.setPointSize(11);
    m_font.setPixelSize(12);
    m_boldFont.setPointSize(11);
    m_boldFont.setPixelSize(12);
    m_boldFont.setBold(true);
    m_fontMetrics = QFontMetrics(m_boldFont);
//    setBackgroundColor(GUIPalette::getColour(GUIPalette::ChordNameRulerBackground));

    m_compositionRefreshStatusId = m_composition->getNewRefreshStatusId();

    QObject::connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
                     this, SLOT(update()));

    for (std::vector<Segment *>::iterator i = segments.begin();
            i != segments.end(); ++i) {
        m_segments.insert(SegmentRefreshMap::value_type
                          (*i, (*i)->getNewRefreshStatusId()));
    }
}

ChordNameRuler::~ChordNameRuler()
{
    delete m_chordSegment;
}

void
ChordNameRuler::setReady()
{
    m_ready = true;
    update();
}

void
ChordNameRuler::setCurrentSegment(Segment *segment)
{
    m_currentSegment = segment;
}

void
ChordNameRuler::setStudio(Studio *studio)
{
    m_studio = studio;
}

void
ChordNameRuler::slotScrollHoriz(int x)
{
    int w = width();
    // int h = height();
    int dx = x - ( -m_currentXOffset);
    m_currentXOffset = -x;

    if (dx == 0)
        return ;

    if (dx > w*7 / 8 || dx < -w*7 / 8) {
        update();
        return ;
    }

    //@@@ These are probably not working like the bitBlts that were commented
    // out in one of the other rulers.  That (the marks ruler, I think, or the
    // loops ruler) works fine without them, and this one is mangled, so what
    // happens if we comment these out?
    //
    // On the surface of it, it apparently fixes the grotesque drawing problem.
//    
//    if (dx > 0) { // moving right, so the existing stuff moves left
//        bitBlt(this, 0, 0, this, dx, 0, w - dx, h);
//        repaint(w - dx, 0, dx, h);
//    } else {      // moving left, so the existing stuff moves right
//        bitBlt(this, -dx, 0, this, 0, 0, w + dx, h);
//        repaint(0, 0, -dx, h);
//    }
    update();
}

QSize
ChordNameRuler::sizeHint() const
{
    double width =
        m_rulerScale->getBarPosition(m_rulerScale->getLastVisibleBar()) +
        m_rulerScale->getBarWidth(m_rulerScale->getLastVisibleBar());

    NOTATION_DEBUG << "Returning chord-label ruler width as " << width << endl;

    QSize res(std::max(int(width), m_width), m_height);

    return res;
}

QSize
ChordNameRuler::minimumSizeHint() const
{
    double firstBarWidth = m_rulerScale->getBarWidth(0);
    QSize res = QSize(int(firstBarWidth), m_height);
    return res;
}

void
ChordNameRuler::recalculate(timeT from, timeT to)
{
    if (!m_ready)
        return ;

    Profiler profiler("ChordNameRuler::recalculate");
    NOTATION_DEBUG << "ChordNameRuler[" << this << "]::recalculate" << endl;

    bool regetSegments = false;

    enum RecalcLevel { RecalcNone, RecalcVisible, RecalcWhole };
    RecalcLevel level = RecalcNone;

    if (m_segments.empty()) {

        regetSegments = true;

    } else if (m_regetSegmentsOnChange) {

        RefreshStatus &rs =
            m_composition->getRefreshStatus(m_compositionRefreshStatusId);

        if (rs.needsRefresh()) {
            rs.setNeedsRefresh(false);
            regetSegments = true;
        }
    }

    if (regetSegments) {

        SegmentSelection ss;

        for (Composition::iterator ci = m_composition->begin();
                ci != m_composition->end(); ++ci) {

            if (m_studio) {

                TrackId ti = (*ci)->getTrack();

                Instrument *instr = m_studio->getInstrumentById
                                    (m_composition->getTrackById(ti)->getInstrument());

                if (instr &&
                        instr->getInstrumentType() == Instrument::Midi &&
                        instr->isPercussion()) {
                    continue;
                }
            }

            ss.insert(*ci);
        }

        std::vector<SegmentRefreshMap::iterator> eraseThese;

        for (SegmentRefreshMap::iterator si = m_segments.begin();
                si != m_segments.end(); ++si) {
            if (ss.find(si->first) == ss.end()) {
                eraseThese.push_back(si);
                level = RecalcWhole;
                NOTATION_DEBUG << "Segment deleted, updating (now have " << m_segments.size() << " segments)" << endl;
            }
        }

        for (std::vector<SegmentRefreshMap::iterator>::iterator ei = eraseThese.begin();
                ei != eraseThese.end(); ++ei) {
            m_segments.erase(*ei);
        }


        for (SegmentSelection::iterator si = ss.begin();
                si != ss.end(); ++si) {

            if (m_segments.find(*si) == m_segments.end()) {
                m_segments.insert(SegmentRefreshMap::value_type
                                  (*si, (*si)->getNewRefreshStatusId()));
                level = RecalcWhole;
                NOTATION_DEBUG << "Segment created, adding (now have " << m_segments.size() << " segments)" << endl;
            }
        }

        if (m_currentSegment &&
                ss.find(m_currentSegment) == ss.end()) {
            m_currentSegment = 0;
            level = RecalcWhole;
        }
    }

    if (!m_chordSegment)
        m_chordSegment = new Segment();
    if (m_segments.empty())
        return ;

    SegmentRefreshStatus overallStatus;
    overallStatus.setNeedsRefresh(false);

    for (SegmentRefreshMap::iterator i = m_segments.begin();
            i != m_segments.end(); ++i) {
        SegmentRefreshStatus &status =
            i->first->getRefreshStatus(i->second);
        if (status.needsRefresh()) {
            overallStatus.push(status.from(), status.to());
        }
    }

    // We now have the overall area affected by these changes, across
    // all segments.  If it's entirely within our displayed area, just
    // recalculate the displayed area; if it overlaps, calculate the
    // union of the two areas; if it's entirely without, calculate
    // nothing.

    if (level == RecalcNone) {
        if (from == to) {
            NOTATION_DEBUG << "ChordNameRuler::recalculate: from==to, recalculating all" << endl;
            level = RecalcWhole;
        } else if (overallStatus.from() == overallStatus.to()) {
            NOTATION_DEBUG << "ChordNameRuler::recalculate: overallStatus.from==overallStatus.to, ignoring" << endl;
            level = RecalcNone;
        } else if (overallStatus.from() >= from && overallStatus.to() <= to) {
            NOTATION_DEBUG << "ChordNameRuler::recalculate: change is " << overallStatus.from() << "->" << overallStatus.to() << ", I show " << from << "->" << to << ", recalculating visible area" << endl;
            level = RecalcVisible;
        } else if (overallStatus.from() >= to || overallStatus.to() <= from) {
            NOTATION_DEBUG << "ChordNameRuler::recalculate: change is " << overallStatus.from() << "->" << overallStatus.to() << ", I show " << from << "->" << to << ", ignoring" << endl;
            level = RecalcNone;
        } else {
            NOTATION_DEBUG << "ChordNameRuler::recalculate: change is " << overallStatus.from() << "->" << overallStatus.to() << ", I show " << from << "->" << to << ", recalculating whole" << endl;
            level = RecalcWhole;
        }
    }

    if (level == RecalcNone)
        return ;

    for (SegmentRefreshMap::iterator i = m_segments.begin();
            i != m_segments.end(); ++i) {
        i->first->getRefreshStatus(i->second).setNeedsRefresh(false);
    }

    if (!m_currentSegment) { //!!! arbitrary, must do better
        //!!! need a segment starting at zero or so with a clef and key in it!
        m_currentSegment = m_segments.begin()->first;
    }

    /*!!!
     
    	for (Composition::iterator ci = m_composition->begin();
    	     ci != m_composition->end(); ++ci) {
     
    	    if ((*ci)->getEndMarkerTime() >= from &&
    		((*ci)->getStartTime() <= from ||
    		 (clefKeySegment &&
    		  (*ci)->getStartTime() < clefKeySegment->getStartTime()))) {
     
    		clefKeySegment = *ci;
    	    }
    	}
     
    	if (!clefKeySegment) return;
        }
    */

    if (level == RecalcWhole) {

        m_chordSegment->clear();

        timeT clefKeyTime = m_currentSegment->getStartTime();
        //(from < m_currentSegment->getStartTime() ?
        //	        m_currentSegment->getStartTime() : from);

        Clef clef = m_currentSegment->getClefAtTime(clefKeyTime);
        m_chordSegment->insert(clef.getAsEvent( -1));

        ::Rosegarden::Key key = m_currentSegment->getKeyAtTime(clefKeyTime);
        m_chordSegment->insert(key.getAsEvent( -1));

        from = 0;
        to = 0;

    } else {
        Segment::iterator i = m_chordSegment->findTime(from);
        Segment::iterator j = m_chordSegment->findTime(to);
        m_chordSegment->erase(i, j);
    }

    SegmentSelection selection;
    for (SegmentRefreshMap::iterator si = m_segments.begin(); si != m_segments.end();
            ++si) {
        selection.insert(si->first);
    }

    CompositionTimeSliceAdapter adapter(m_composition, &selection, from, to);
    AnalysisHelper helper;
    helper.labelChords(adapter, *m_chordSegment, m_composition->getNotationQuantizer());
}

void
ChordNameRuler::paintEvent(QPaintEvent* e)
{
    if (!m_composition || !m_ready)
        return ;

    NOTATION_DEBUG << "*** Chord Name Ruler: paintEvent" << endl;

    Profiler profiler1("ChordNameRuler::paintEvent (whole)");

    QPainter paint(this);

    // In a stylesheet world...  Yadda yadda.  Fix the stupid background to
    // rescue it from QWidget Hack Black. (Ahhh.  Yes, after being thwarted for
    // a month or something, I'm really enjoying getting this solved.)
    QBrush bg = QBrush(GUIPalette::getColour(GUIPalette::ChordNameRulerBackground));
    paint.fillRect(e->rect(), bg);

    paint.setPen(GUIPalette::getColour(GUIPalette::ChordNameRulerForeground));

    paint.setClipRegion(e->region());
    paint.setClipRect(e->rect().normalized());

    QRect clipRect = paint.clipRegion().boundingRect();

    timeT from = m_rulerScale->getTimeForX
                 (clipRect.x() - m_currentXOffset - m_xorigin - 50);
    timeT to = m_rulerScale->getTimeForX
               (clipRect.x() + clipRect.width() - m_currentXOffset - m_xorigin + 50);

    recalculate(from, to);

    if (!m_chordSegment)
        return ;

    Profiler profiler2("ChordNameRuler::paintEvent (paint)");

    QRect boundsForHeight = m_fontMetrics.boundingRect("^j|lM");
    int fontHeight = boundsForHeight.height();
    int textY = (height() - 6) / 2 + fontHeight / 2;

    double prevX = 0;
    timeT keyAt = from - 1;
    std::string keyText;

    NOTATION_DEBUG << "*** Chord Name Ruler: paint " << from << " -> " << to << endl;

    for (Segment::iterator i = m_chordSegment->findTime(from);
            i != m_chordSegment->findTime(to); ++i) {

        NOTATION_DEBUG << "type " << (*i)->getType() << " at " << (*i)->getAbsoluteTime()
        << endl;

        if (!(*i)->isa(Text::EventType) ||
                !(*i)->has(Text::TextPropertyName) ||
                !(*i)->has(Text::TextTypePropertyName))
            continue;

        std::string text((*i)->get
                         <String>(Text::TextPropertyName));

        if ((*i)->get
                <String>(Text::TextTypePropertyName) == Text::KeyName) {
            timeT myTime = (*i)->getAbsoluteTime();
            if (myTime == keyAt && text == keyText)
                continue;
            else {
                keyAt = myTime;
                keyText = text;
            }
        }

        double x = m_rulerScale->getXForTime((*i)->getAbsoluteTime());
        (*i)->set
        <Int>(TEXT_FORMAL_X, (long)x);

        QRect textBounds = m_fontMetrics.boundingRect(strtoqstr(text));
        int width = textBounds.width();

        x -= width / 2;
        if (prevX >= x - 3)
            x = prevX + 3;
        (*i)->set
        <Int>(TEXT_ACTUAL_X, long(x));
        prevX = x + width;
    }

    for (Segment::iterator i = m_chordSegment->findTime(from);
            i != m_chordSegment->findTime(to); ++i) {

        if (!(*i)->isa(Text::EventType))
            continue;
        std::string text((*i)->get
                         <String>(Text::TextPropertyName));
        std::string type((*i)->get
                         <String>(Text::TextTypePropertyName));

        if (!(*i)->has(TEXT_FORMAL_X))
            continue;

        long formalX = (*i)->get
                       <Int>(TEXT_FORMAL_X);
        long actualX = (*i)->get
                       <Int>(TEXT_ACTUAL_X);

        formalX += m_currentXOffset + long(m_xorigin);
        actualX += m_currentXOffset + long(m_xorigin);

        paint.drawLine(formalX, height() - 4, formalX, height());

        if (type == Text::KeyName) {
            paint.setFont(m_boldFont);
        } else {
            paint.setFont(m_font);
        }

        NOTATION_DEBUG << "ChordNameRuler drawing text " << text << endl;

        paint.drawText(actualX, textY, strtoqstr(text));
    }
}

}
#include "ChordNameRuler.moc"
