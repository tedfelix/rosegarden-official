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

#include <cmath>
#include "NotationVLayout.h"
#include "misc/Debug.h"
#include "misc/Strings.h"

#include "base/Composition.h"
#include "base/Event.h"
#include "base/LayoutEngine.h"
#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "base/NotationQuantizer.h"
#include "base/Profiler.h"
#include "base/ViewSegment.h"
#include "gui/general/ProgressReporter.h"
#include "gui/editors/guitar/Chord.h"
#include "misc/ConfigGroups.h"
#include "NotationChord.h"
#include "NotationElement.h"
#include "NotationProperties.h"
#include "NotationStaff.h"
#include "NotePixmapFactory.h"
#include <QMessageBox>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QWidget>


namespace Rosegarden
{

using namespace BaseProperties;


NotationVLayout::NotationVLayout(Composition *c, NotePixmapFactory *npf,
                                 const NotationProperties &properties,
                                 QObject* parent) :
        ProgressReporter(parent),
        m_composition(c),
        m_npf(npf),
        m_notationQuantizer(c->getNotationQuantizer()),
        m_properties(properties)
{
    // Get display settings
    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    m_showRepeated =  settings.value("showrepeated", true).toBool();
    m_distributeVerses =  settings.value("distributeverses", true).toBool();
    settings.endGroup();
}

NotationVLayout::~NotationVLayout()
{
    // empty
}

NotationVLayout::SlurList &
NotationVLayout::getSlurList(ViewSegment &staff)
{
    SlurListMap::iterator i = m_slurs.find(&staff);
    if (i == m_slurs.end()) {
        m_slurs[&staff] = SlurList();
    }

    return m_slurs[&staff];
}

void
NotationVLayout::reset()
{
    m_slurs.clear();
}

void
NotationVLayout::scanViewSegment(ViewSegment &staffBase, timeT, timeT, bool)
{
    // We actually always perform a full scan, for reasons of sheer laziness

    Profiler profiler("NotationVLayout::scanViewSegment");

    NotationStaff &staff = dynamic_cast<NotationStaff &>(staffBase);
    NotationElementList *notes = staff.getViewElementList();

    getSlurList(staff).clear();

    NotationElementList::iterator from = notes->begin();
    NotationElementList::iterator to = notes->end();
    NotationElementList::iterator i;

    for (i = from; i != to; ++i) {

        NotationElement *el = static_cast<NotationElement*>(*i);

        // Displaced Y will only be used for certain events -- in
        // particular not for notes, whose y-coord is obviously kind
        // of meaningful.
        double displacedY = 0.0;
        long dyRaw = 0;
        el->event()->get<Int>(DISPLACED_Y, dyRaw);
        displacedY = double(dyRaw * m_npf->getLineSpacing()) / 1000.0;

        el->setLayoutY(staff.getLayoutYForHeight( -9) + displacedY);

        if (el->isRest()) {

            // rests for notes longer than the minim have hotspots
            // aligned with the line above the middle line; the rest
            // are aligned with the middle line

            long noteType;
            bool hasNoteType = el->event()->get<Int>(NOTE_TYPE, noteType);
            if (hasNoteType && noteType > Note::Minim) {
                el->setLayoutY(staff.getLayoutYForHeight(6) + displacedY);
            } else {
                el->setLayoutY(staff.getLayoutYForHeight(4) + displacedY);
            }

            // Fix for bug 1090767 Rests outside staves have wrong glyphs
            // by William <rosegarden4c AT orthoset.com>
            // We use a "rest-outside-stave" glyph for any minim/semibreve/breve
            // rest that has been displaced vertically e.g. by fine-positioning
            // outside the stave. For small vertical displacements that keep
            // the rest inside the stave, we use the "rest-inside-stave" glyph
            // and also discretise the displacement into multiples of the
            // stave-line spacing. The outside-stave glyphs match the character
            // numbers 1D13A, 1D13B and 1D13C in the Unicode 4.0 standard.

            if (hasNoteType && (displacedY > 0.1 || displacedY < -0.1)) {

                // a fiddly check for transition from inside to outside:

                int min = -1, max = 1;

                switch (noteType) {
                case Note::Breve:
                    min = -1;
                    max = 2;
                    break;
                case Note::Semibreve:
                    min = -1;
                    max = 3;
                    break;
                case Note::Minim:
                    min = -2;
                    max = 2;
                    break;
                case Note::Crotchet:
                    min = -1;
                    max = 3;
                    break;
                case Note::Quaver:
                    min = -2;
                    max = 3;
                    break;
                case Note::Semiquaver:
                    min = -3;
                    max = 3;
                    break;
                case Note::Demisemiquaver:
                    min = -3;
                    max = 4;
                    break;
                case Note::Hemidemisemiquaver:
                    min = -4;
                    max = 4;
                    break;
                }

                bool outside = false;

                if (noteType == Note::Breve) {
                    if (nearbyint(displacedY) < min * m_npf->getLineSpacing() ||
                            nearbyint(displacedY) > max * m_npf->getLineSpacing()) {
                        outside = true;
                    }
                } else {
                    if ((int)displacedY < min * m_npf->getLineSpacing() ||
                            (int)displacedY > max * m_npf->getLineSpacing()) {
                        outside = true;
                    }
                }

                el->event()->setMaybe<Bool>(m_properties.REST_OUTSIDE_STAVE,
                                            outside);

                if (!outside) {
                    displacedY = (double)m_npf->getLineSpacing() *
                                 (int(nearbyint((double)displacedY /
                                                m_npf->getLineSpacing())));
                    if (noteType > Note::Minim)
                        el->setLayoutY(staff.getLayoutYForHeight(6) + displacedY);
                    else
                        el->setLayoutY(staff.getLayoutYForHeight(4) + displacedY);
                }

                //		if (displacedY != 0.0)
                //		    NOTATION_DEBUG << "REST_OUTSIDE_STAVE AFTER "
                //			       << " : displacedY : " << displacedY
                //			       << " line-spacing : " << m_npf->getLineSpacing()
                //			       << " time : " << (el->getViewAbsoluteTime())
                //			       << endl;
            } else {
                el->event()->setMaybe<Bool>(m_properties.REST_OUTSIDE_STAVE,
                                            false);
            }

        } else if (el->isNote()) {

            NotationChord chord(*notes, i, m_notationQuantizer, m_properties);
            if (chord.size() == 0)
                continue;

            std::vector<int> h;
            for (unsigned int j = 0; j < chord.size(); ++j) {
                long height = 0;
                if (!(*chord[j])->event()->get<Int>
                    (m_properties.HEIGHT_ON_STAFF, height)) {
                    std::cerr << QString("ERROR: Event in chord at %1 has no HEIGHT_ON_STAFF property!\nThis is a bug (the program would previously have crashed by now)").arg((*chord[j])->getViewAbsoluteTime()) << std::endl;
                    (*chord[j])->event()->dump(std::cerr);
                }
                h.push_back(height);
            }
            bool stemmed = chord.hasStem();
            bool stemUp = chord.hasStemUp();
            bool hasNoteHeadShifted = chord.hasNoteHeadShifted();

            unsigned int flaggedNote = (stemUp ? chord.size() - 1 : 0);

            bool hasShifted = chord.hasNoteHeadShifted();

            double y0 = -1E50;          // A very unlikely Y layout value

            for (unsigned int j = 0; j < chord.size(); ++j) {

                el = static_cast<NotationElement*>(*chord[j]);
                el->setLayoutY(staff.getLayoutYForHeight(h[j]));

                // Look for collision
                const double eps = 0.001;
                Event *eel = el->event();
                double y = el->getLayoutY();
                if (eel->has("pitch")) {
                    el->setIsColliding(fabs(y - y0) < eps);
                    y0 = y;
                }


                // These calculations and assignments are pretty much final
                // if the chord is not in a beamed group, but if it is then
                // they will be reworked by NotationGroup::applyBeam, which
                // is called from NotationHLayout::layout, which is called
                // after this.  Any inaccuracies here for beamed groups
                // should be stamped out there.

                //                el->event()->setMaybe<Bool>(STEM_UP, stemUp);
                el->event()->setMaybe<Bool>(m_properties.VIEW_LOCAL_STEM_UP, stemUp);

                bool primary =
                    ((stemmed && stemUp) ? (j == 0) : (j == chord.size() - 1));
                el->event()->setMaybe<Bool>
                (m_properties.CHORD_PRIMARY_NOTE, primary);

                if (primary) {
                    el->event()->setMaybe<Int>
                    (m_properties.CHORD_MARK_COUNT, chord.getMarkCountForChord());
                }

                bool shifted = chord.isNoteHeadShifted(chord[j]);
                el->event()->setMaybe<Bool>
                (m_properties.NOTE_HEAD_SHIFTED, shifted);

                el->event()->setMaybe<Bool>
                (m_properties.NOTE_DOT_SHIFTED, false);
                if (hasShifted && stemUp) {
                    long dots = 0;
                    (void)el->event()->get
                    <Int>(NOTE_DOTS, dots);
                    if (dots > 0) {
                        el->event()->setMaybe<Bool>
                        (m_properties.NOTE_DOT_SHIFTED, true);
                    }
                }

                el->event()->setMaybe<Bool>
                (m_properties.NEEDS_EXTRA_SHIFT_SPACE,
                 hasNoteHeadShifted && !stemUp);

                el->event()->setMaybe<Bool>
                (m_properties.DRAW_FLAG, j == flaggedNote);

                int stemLength = -1;
                if (j != flaggedNote) {
                    stemLength = staff.getLayoutYForHeight(h[flaggedNote]) -
                                 staff.getLayoutYForHeight(h[j]);
                    if (stemLength < 0)
                        stemLength = -stemLength;
                    //                    NOTATION_DEBUG << "Setting stem length to "
                    //                                         << stemLength << endl;
                } else {
                    int minStemLength = stemLength;
                    if (h[j] < -2 && stemUp) {
                        //!!! needs tuning, & applying for beamed stems too
                        minStemLength = staff.getLayoutYForHeight(h[j]) -
                                        staff.getLayoutYForHeight(4);
                    } else if (h[j] > 10 && !stemUp) {
                        minStemLength = staff.getLayoutYForHeight(4) -
                                        staff.getLayoutYForHeight(h[j]);
                    }
                    stemLength = std::max(minStemLength, stemLength);
                }

                el->event()->setMaybe<Int>
                    (m_properties.UNBEAMED_STEM_LENGTH, stemLength);
            }


            // #938545 (Broken notation: Duplicated note can float
            // outside stave) -- Need to cope with the case where a
            // note that's not a member of a chord (different stem
            // direction &c) falls between notes that are members.
            // Not optimal, as we can end up scanning the chord
            // multiple times (we'll return to it after scanning the
            // contained note).  [We can't just iterate over all
            // elements within the chord (as we can in hlayout)
            // because we need them in height order.]

            i = chord.getFirstElementNotInChord();
            if (i == notes->end())
                i = chord.getFinalElement();
            else
                --i;

        } else {

            if (el->event()->isa(Clef::EventType)) {

                // clef pixmaps have the hotspot placed to coincide
                // with the pitch of the clef -- so the alto clef
                // should be "on" the middle line, the treble clef
                // "on" the line below the middle, etc

                el->setLayoutY(staff.getLayoutYForHeight
                               (Clef(*el->event()).getAxisHeight()));

            } else if (el->event()->isa(Rosegarden::Symbol::EventType)) {
                //!!! Presently all symbols are above the staff only, at a fixed
                // height.  I've never seen a segno or coda anywhere else.
                // Breath marks might benefit from the ability to draw them up
                // or draw them down, or even draw them at an arbitrary Y, but
                // let's not get into any of that just now.
                //
                // The figure 19 is totally arbitrary, based on comparing the
                // results against a "D. C. al Fine" text, to get these to
                // display at approximately the same height.  Please feel free
                // to refine this first stab.
                el->setLayoutY(staff.getLayoutYForHeight(19 + displacedY));

            } else if (el->event()->isa(Rosegarden::Key::EventType)) {

                el->setLayoutY(staff.getLayoutYForHeight(12));

            } else if (el->event()->isa(Text::EventType)) {

                std::string type = Text::UnspecifiedType;
                el->event()->get<String>(Text::TextTypePropertyName, type);

                if (type == Text::Dynamic ||
                        type == Text::LocalDirection ||
                        type == Text::UnspecifiedType) {
                    // below the staff
                    el->setLayoutY(staff.getLayoutYForHeight(-7) + displacedY);
                } else if (type == Text::Lyric) {
                    long verse = 0;
                    if (!m_distributeVerses) {
                        // verses even further below the staff
                        el->event()->get<Int>(Text::LyricVersePropertyName, verse);
                    }
                    el->setLayoutY(staff.getLayoutYForHeight(-10 - 3 * verse) + displacedY);
                } else if (type == Text::Annotation) {
                    // annotations way below the staff
                    el->setLayoutY(staff.getLayoutYForHeight(-13) + displacedY);
                } else {
                    // every other text well above the staff
                    el->setLayoutY(staff.getLayoutYForHeight(22) + displacedY);
                }

            } else if (el->event()->isa(Indication::EventType)) {

                try {
                    std::string indicationType =
                        el->event()->get
                        <String>(Indication::IndicationTypePropertyName);

                    if (indicationType == Indication::Slur ||
                            indicationType == Indication::PhrasingSlur) {
                        getSlurList(staff).push_back(i);
                    }

                    if (indicationType == Indication::OttavaUp ||
                            indicationType == Indication::QuindicesimaUp) {
                        el->setLayoutY(staff.getLayoutYForHeight(15) + displacedY);
                    } else {
                        el->setLayoutY(staff.getLayoutYForHeight( -9) + displacedY);
                    }

                    if (indicationType == Indication::TrillLine)
                        // just draw them way above the staff for now
                        el->setLayoutY(staff.getLayoutYForHeight(15) + displacedY);
                } catch (...) {
                    el->setLayoutY(staff.getLayoutYForHeight( -9) + displacedY);
                }

            } else if (el->event()->isa(Guitar::Chord::EventType)) {

                el->setLayoutY(staff.getLayoutYForHeight(22) + displacedY);
            }
        }
    }
}

void
NotationVLayout::finishLayout(timeT, timeT, bool)
{
    Profiler profiler("NotationHLayout::finishLayout");

    for (SlurListMap::iterator mi = m_slurs.begin();
         mi != m_slurs.end(); ++mi) {

        for (SlurList::iterator si = mi->second.begin();
             si != mi->second.end(); ++si) {

            NotationElementList::iterator i = *si;
            NotationStaff &staff = dynamic_cast<NotationStaff &>(*(mi->first));

            positionSlur(staff, i);
        }
    }
}

void
NotationVLayout::positionSlur(NotationStaff &staff,
                              NotationElementList::iterator i)
{
    NotationRules rules;

    bool phrasing = ((*i)->event()->get
                     <String>(Indication::IndicationTypePropertyName)
                     == Indication::PhrasingSlur);

    NotationElementList::iterator scooter = i;

    timeT slurDuration = (*i)->event()->getDuration();
    if (slurDuration == 0 && (*i)->event()->has("indicationduration")) {
        slurDuration = (*i)->event()->get
                       <Int>("indicationduration"); // obs property
    }
    timeT endTime = (*i)->getViewAbsoluteTime() + slurDuration;

    bool haveStart = false;

    int startTopHeight = 4, endTopHeight = 4,
        startBottomHeight = 4, endBottomHeight = 4,
        maxTopHeight = 4, minBottomHeight = 4,
        maxCount = 0, minCount = 0;

    int startX = (int)(*i)->getLayoutX(), endX = startX + 10;
    bool startStemUp = false, endStemUp = false;
    long startMarks = 0;
    // long endMarks = 0;
    // bool startTied = false;
    // bool endTied = false;
    bool beamAbove = false, beamBelow = false;
    bool dynamic = false;

    std::vector<Event *> stemUpNotes, stemDownNotes;

    // Scan the notes spanned by the slur, recording the top and
    // bottom heights of the first and last chords, plus the presence
    // of any troublesome beams and high or low notes in the body.

    while (scooter != staff.getViewElementList()->end()) {

        if ((*scooter)->getViewAbsoluteTime() >= endTime) break;
        Event *event = (*scooter)->event();

        if (event->isa(Note::EventType)) {

            long h = 0;
            if (!event->get<Int>(m_properties.HEIGHT_ON_STAFF, h)) {
				QMessageBox::warning
					( dynamic_cast<QWidget *>(parent()), 
					"", 
	 				tr("Spanned note at %1 has no HEIGHT_ON_STAFF property!\nThis is a bug (the program would previously have crashed by now)").arg((*scooter)->getViewAbsoluteTime())
					);
                event->dump(std::cerr);
            }

            bool stemUp = rules.isStemUp(h);
            event->get
            <Bool>(m_properties.VIEW_LOCAL_STEM_UP, stemUp);

            bool beamed = false;
            event->get
            <Bool>(m_properties.BEAMED, beamed);

            bool primary = false;

            if (event->get
                    <Bool>
                    (m_properties.CHORD_PRIMARY_NOTE, primary) && primary) {

                NotationChord chord(*(staff.getViewElementList()), scooter,
                                    m_notationQuantizer, m_properties);

                if (beamed) {
                    if (stemUp)
                        beamAbove = true;
                    else
                        beamBelow = true;
                }

                if (!haveStart) {

                    startBottomHeight = chord.getLowestNoteHeight();
                    startTopHeight = chord.getHighestNoteHeight();
                    minBottomHeight = startBottomHeight;
                    maxTopHeight = startTopHeight;

                    startX = (int)(*scooter)->getLayoutX();
                    startStemUp = stemUp;
                    startMarks = chord.getMarkCountForChord();

                    // bool tied = false;
                    // if ((event->get
                    //         <Bool>(TIED_FORWARD, tied) && tied) ||
                    //         (event->get<Bool>(TIED_BACKWARD, tied) && tied)) {
                    //     startTied = true;
                    // }

                    haveStart = true;

                } else {
                    if (chord.getLowestNoteHeight() < minBottomHeight) {
                        minBottomHeight = chord.getLowestNoteHeight();
                        ++minCount;
                    }
                    if (chord.getHighestNoteHeight() > maxTopHeight) {
                        maxTopHeight = chord.getHighestNoteHeight();
                        ++maxCount;
                    }
                }

                endBottomHeight = chord.getLowestNoteHeight();
                endTopHeight = chord.getHighestNoteHeight();
                endX = (int)(*scooter)->getLayoutX();
                endStemUp = stemUp;
                // endMarks = chord.getMarkCountForChord();

                // bool tied = false;
                // if ((event->get
                //         <Bool>(TIED_FORWARD, tied) && tied) ||
                //         (event->get<Bool>(TIED_BACKWARD, tied) && tied)) {
                //     endTied = true;
                // }
            }

            if (!beamed) {
                if (stemUp)
                    stemUpNotes.push_back(event);
                else
                    stemDownNotes.push_back(event);
            }

        } else if (event->isa(Indication::EventType)) {

            try {
                std::string indicationType =
                    event->get
                    <String>(Indication::IndicationTypePropertyName);

                if (indicationType == Indication::Crescendo ||
                        indicationType == Indication::Decrescendo)
                    dynamic = true;
            } catch (...) { }
        }

        ++scooter;
    }

    bool above = true;

    if ((*i)->event()->has(NotationProperties::SLUR_ABOVE) &&
            (*i)->event()->isPersistent<Bool>(NotationProperties::SLUR_ABOVE)) {

        (*i)->event()->get
        <Bool>(NotationProperties::SLUR_ABOVE, above);

    } else if (phrasing) {

        int score = 0; // for "above"

        if (dynamic)
            score += 2;

        if (startStemUp == endStemUp) {
            if (startStemUp)
                score -= 2;
            else
                score += 2;
        } else if (beamBelow != beamAbove) {
            if (beamAbove)
                score -= 2;
            else
                score += 2;
        }

        if (maxTopHeight < 6)
            score += 1;
        else if (minBottomHeight > 2)
            score -= 1;

        if (stemUpNotes.size() != stemDownNotes.size()) {
            if (stemUpNotes.size() < stemDownNotes.size())
                score += 1;
            else
                score -= 1;
        }

        above = (score >= 0);

    } else {

        if (startStemUp == endStemUp) {
            above = !startStemUp;
        } else if (beamBelow) {
            above = true;
        } else if (beamAbove) {
            above = false;
        } else if (stemUpNotes.size() != stemDownNotes.size()) {
            above = (stemUpNotes.size() < stemDownNotes.size());
        } else {
            above = ((startTopHeight - 4) + (endTopHeight - 4) +
                     (4 - startBottomHeight) + (4 - endBottomHeight) <= 8);
        }
    }

    // now choose the actual y-coord of the slur based on the side
    // we've decided to put it on

    int startHeight, endHeight;
    int startOffset = 2, endOffset = 2;

    if (above) {

        if (!startStemUp)
            startOffset += startMarks * 2;
        else
            startOffset += 5;

        if (!endStemUp)
            endOffset += startMarks * 2;
        else
            endOffset += 5;

        startHeight = startTopHeight + startOffset;
        endHeight = endTopHeight + endOffset;

        bool maxRelevant = ((maxTopHeight != endTopHeight) || (maxCount > 1));
        if (maxRelevant) {
            int midHeight = (startHeight + endHeight) / 2;
            if (maxTopHeight > midHeight - 1) {
                startHeight += maxTopHeight - midHeight + 1;
                endHeight += maxTopHeight - midHeight + 1;
            }
        }

    } else {

        if (startStemUp)
            startOffset += startMarks * 2;
        else
            startOffset += 5;

        if (endStemUp)
            endOffset += startMarks * 2;
        else
            endOffset += 5;

        startHeight = startBottomHeight - startOffset;
        endHeight = endBottomHeight - endOffset;

        bool minRelevant = ((minBottomHeight != endBottomHeight) || (minCount > 1));
        if (minRelevant) {
            int midHeight = (startHeight + endHeight) / 2;
            if (minBottomHeight < midHeight + 1) {
                startHeight -= midHeight - minBottomHeight + 1;
                endHeight -= midHeight - minBottomHeight + 1;
            }
        }
    }

    int y0 = staff.getLayoutYForHeight(startHeight),
             y1 = staff.getLayoutYForHeight(endHeight);

    int dy = y1 - y0;
    int length = endX - startX;
    int diff = staff.getLayoutYForHeight(0) - staff.getLayoutYForHeight(3);
    if (length < diff*10)
        diff /= 2;
    if (length > diff*3)
        length -= diff / 2;
    startX += diff;

    (*i)->event()->setMaybe<Bool>(NotationProperties::SLUR_ABOVE, above);
    (*i)->event()->setMaybe<Int>(m_properties.SLUR_Y_DELTA, dy);
    (*i)->event()->setMaybe<Int>(m_properties.SLUR_LENGTH, length);

    double displacedX = 0.0, displacedY = 0.0;

    long dxRaw = 0;
    (*i)->event()->get<Int>(DISPLACED_X, dxRaw);
    displacedX = double(dxRaw * m_npf->getNoteBodyWidth()) / 1000.0;

    long dyRaw = 0;
    (*i)->event()->get<Int>(DISPLACED_Y, dyRaw);
    displacedY = double(dyRaw * m_npf->getLineSpacing()) / 1000.0;

    (*i)->setLayoutX(startX + displacedX);
    (*i)->setLayoutY(y0 + displacedY);
}

}
