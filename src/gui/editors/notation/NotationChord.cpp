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


#include "NotationChord.h"

#include "base/Sets.h"
#include "base/Event.h"
#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "base/Quantizer.h"
#include "misc/Debug.h"
#include "NotationProperties.h"
#include "NoteStyleFactory.h"

namespace Rosegarden
{

template <>
Event *
AbstractSet<NotationElement, NotationElementList>::getAsEvent(const NotationElementList::iterator &i)
{
    return (*i)->event();
}

NotationChord::NotationChord(NotationElementList &c,
                             NotationElementList::iterator i,
                             const Quantizer *quantizer,
                             const NotationProperties &properties,
                             const Clef &clef,
                             const ::Rosegarden::Key &key) :
        GenericChord < NotationElement,
        NotationElementList, true > (c, i, quantizer,
                                     NotationProperties::STEM_UP),
        m_properties(properties),
        m_clef(clef),
        m_key(key)
{
    initialise();
}

int
NotationChord::getHeight(const Iterator &i) const
{
    //!!! We use HEIGHT_ON_STAFF in preference to the passed clef/key,
    //but what if the clef/key changed since HEIGHT_ON_STAFF was
    //written?  Who updates the properties then?  Check this.

    long h = 0;
    if (getAsEvent(i)->get
            <Int>(NotationProperties::HEIGHT_ON_STAFF, h)) {
        return h;
    }

    try {
        Pitch pitch(*getAsEvent(i));
        h = pitch.getHeightOnStaff(m_clef, m_key);
    } catch (...) {
        // no pitch!
    }

    // set non-persistent, not setMaybe, as we know the property is absent:
    getAsEvent(i)->set
    <Int>(NotationProperties::HEIGHT_ON_STAFF, h, false);
    return h;
}

bool
NotationChord::hasStem() const
{
    // true if any of the notes is stemmed

    Iterator i(getInitialNote());
    for (;;) {
        long note;
        if (!getAsEvent(i)->get
                <Int>(BaseProperties::NOTE_TYPE, note)) return true;
        if (NoteStyleFactory::getStyleForEvent(getAsEvent(i))->hasStem(note))
            return true;
        if (i == getFinalNote())
            return false;
        ++i;
    }
    return false;
}

bool
NotationChord::hasStemUp() const
{
    NotationRules rules;

    // believe anything found in any of the notes, if in a persistent
    // property or a property apparently set by the beaming algorithm

    Iterator i(getInitialNote());

    for (;;) {
        Event *e = getAsEvent(i);
        /*!!!
        	if (e->has(m_properties.VIEW_LOCAL_STEM_UP)) {
        	    return e->get<Bool>(m_properties.VIEW_LOCAL_STEM_UP);
        	}
        */
        if (e->has(NotationProperties::STEM_UP)) {
            return e->get
                   <Bool>(NotationProperties::STEM_UP);
        }

        if (e->has(NotationProperties::BEAM_ABOVE)) {
            if (e->has(NotationProperties::BEAMED) &&
                    e->get
                    <Bool>(NotationProperties::BEAMED)) {
                return e->get
                       <Bool>(NotationProperties::BEAM_ABOVE);
            }
            else {
                return !e->get
                       <Bool>(NotationProperties::BEAM_ABOVE);
            }
        }

        if (i == getFinalNote())
            break;
        ++i;
    }

    return rules.isStemUp(getHighestNoteHeight(),getLowestNoteHeight());
}

bool
NotationChord::hasNoteHeadShifted() const
{
    int ph = 10000;

    for (unsigned int i = 0; i < size(); ++i) {
        int h = getHeight((*this)[i]);
        if (h == ph + 1)
            return true;
        ph = h;
    }

    return false;
}

bool
NotationChord::isNoteHeadShifted(const Iterator &itr) const
{
    unsigned int i;
    for (i = 0; i < size(); ++i) {
        if ((*this)[i] == itr)
            break;
    }

    if (i == size()) {
        RG_WARNING << "NotationChord::isNoteHeadShifted: Warning: Unable to find note head " << getAsEvent(itr);
        return false;
    }

    int h = getHeight((*this)[i]);

    if (hasStemUp()) {
        if ((i > 0) && (h == getHeight((*this)[i - 1]) + 1)) {
            return (!isNoteHeadShifted((*this)[i - 1]));
        }
    } else {
        if ((i < size() - 1) && (h == getHeight((*this)[i + 1]) - 1)) {
            return (!isNoteHeadShifted((*this)[i + 1]));
        }
    }

    return false;
}

void
NotationChord::applyAccidentalShiftProperties()
{
    // Some rules:
    //
    // The top accidental always gets the minimum shift (i.e. normally
    // right next to the note head or stem).
    //
    // The bottom accidental gets the next least: the same, if the
    // interval is more than a sixth, or the next shift out otherwise.
    //
    // We then progress up from the bottom accidental upwards.
    //
    // These rules aren't really enough, but they might do for now!

    //!!! Uh-oh... we have a catch-22 here.  We can't determine the
    // proper minimum shift until we know which way the stem goes,
    // because if we have a shifted note head and the stem goes down,
    // we need to shift one place further than otherwise.  But we
    // don't know for sure which way the stem goes until we've
    // calculated the beam, and we don't do that until after we've
    // worked out the x-coordinates based on (among other things) the
    // accidental shift.

    int minShift = 0;
    bool extra = false;

    if (!hasStemUp() && hasNoteHeadShifted()) {
        minShift = 1; // lazy
        extra = true;
    }

    int lastShift = minShift;
    int lastHeight = 0, maxHeight = 999;
    int lastWidth = 1;

    for (iterator i = end(); i != begin(); ) {

        --i;
        Event *e = getAsEvent(*i);

        Accidental acc;
        if (e->get
                <String>(m_properties.DISPLAY_ACCIDENTAL, acc) &&
                acc != Accidentals::NoAccidental) {
            e->setMaybe<Int>(m_properties.ACCIDENTAL_SHIFT, minShift);
            e->setMaybe<Bool>(m_properties.ACCIDENTAL_EXTRA_SHIFT, extra);
            maxHeight = lastHeight = getHeight(*i);
            break;
        }
    }

    if (maxHeight == 999) {
        return ;
    }

    for (iterator i = begin(); i != end(); ++i) {

        Event *e = getAsEvent(*i);
        int height = getHeight(*i);

        if (height == maxHeight && e->has(m_properties.ACCIDENTAL_SHIFT)) {
            // finished -- we've come around to the highest one again
            break;
        }

        Accidental acc;

        if (e->get
                <String>(m_properties.DISPLAY_ACCIDENTAL, acc) &&
                acc != Accidentals::NoAccidental) {

            int shift = lastShift;

            if (height < lastHeight) { // lastHeight was the first, up top
                if (lastHeight - height < 6) {
                    shift = lastShift + lastWidth;
                }
            } else {
                if (height - lastHeight >= 6) {
                    if (maxHeight - height >= 6) {
                        shift = minShift;
                    } else {
                        shift = minShift + 1;
                    }
                } else {
                    shift = lastShift + lastWidth;
                }
            }

            e->setMaybe<Int>(m_properties.ACCIDENTAL_SHIFT, shift);

            lastHeight = height;
            lastShift = shift;

            lastWidth = 1;
            bool c = false;
            if (e->get
                    <Bool>(m_properties.DISPLAY_ACCIDENTAL_IS_CAUTIONARY, c)
                    && c) {
                lastWidth = 2;
            }
        }
    }
}

int
NotationChord::getMaxAccidentalShift(bool &extra) const
{
    int maxShift = 0;

    for (const_iterator i = begin(); i != end(); ++i) {
        Event *e = getAsEvent(*i);
        if (e->has(m_properties.ACCIDENTAL_SHIFT)) {
            int shift = e->get
                        <Int>(m_properties.ACCIDENTAL_SHIFT);
            if (shift > maxShift) {
                maxShift = shift;
                e->get
                <Bool>(m_properties.ACCIDENTAL_EXTRA_SHIFT, extra);
            }
        }
    }

    return maxShift;
}

int
NotationChord::getAccidentalShift(const Iterator &i, bool &extra) const
{
    if (getAsEvent(i)->has(m_properties.ACCIDENTAL_SHIFT)) {
        int shift = getAsEvent(i)->get
                    <Int>(m_properties.ACCIDENTAL_SHIFT);
        getAsEvent(i)->get
        <Bool>(m_properties.ACCIDENTAL_EXTRA_SHIFT, extra);
        return shift;
    } else {
        return 0;
    }
}

}
