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

#ifndef _NOTATION_SETS_H_
#define _NOTATION_SETS_H_

#include "Sets.h"

#include "NotationTypes.h"
#include "notationelement.h"
#include "notationproperties.h"
#include "notepixmapfactory.h"

class NotationStaff;


class NotationChord : public Rosegarden::GenericChord<NotationElement,
		                                      NotationElementList>
{
public:
    NotationChord(const NotationElementList &c,
		  NotationElementList::iterator i,
		  const Rosegarden::Quantizer *quantizer,
		  const NotationProperties &properties,
		  const Rosegarden::Clef &clef = Rosegarden::Clef::DefaultClef,
		  const Rosegarden::Key &key = Rosegarden::Key::DefaultKey);

    virtual ~NotationChord() { }

    virtual int getHighestNoteHeight() const {
	return getHeight(getHighestNote());
    }
    virtual int getLowestNoteHeight() const {
	return getHeight(getLowestNote());
    }

    virtual bool hasStem() const;
    virtual bool hasStemUp() const;

    virtual bool hasNoteHeadShifted() const;
    virtual bool isNoteHeadShifted(const NotationElementList::iterator &itr)
	const;

    virtual int getMaxAccidentalShift() const;
    virtual int getAccidentalShift(const NotationElementList::iterator &itr)
	const;

private:
    int getHeight(const Iterator&) const;

    const NotationProperties &m_properties;
};


/// Several sorts of "Beamed Group"

class NotationGroup : public Rosegarden::AbstractSet<NotationElement,
		                                     NotationElementList>
{
public:
    typedef NotationElementList::iterator NELIterator;

    enum Type { Beamed, Tupled, Grace };

    /// Group contents will be sampled from elements surrounding elementInGroup
    NotationGroup(const NotationElementList &nel, NELIterator elementInGroup,
		  const Rosegarden::Quantizer *,
		  std::pair<Rosegarden::timeT, Rosegarden::timeT> barRange,
		  const NotationProperties &properties,
                  const Rosegarden::Clef &clef, const Rosegarden::Key &key);

    /// Caller intends to call sample() for each item in the group, _in order_
    NotationGroup(const NotationElementList &nel,
		  const Rosegarden::Quantizer *,
		  const NotationProperties &properties,
                  const Rosegarden::Clef &clef, const Rosegarden::Key &key);

    virtual ~NotationGroup();

    Type getGroupType() const { return m_type; }

    /**
     * Writes the BEAMED, BEAM_ABOVE, and STEM_UP properties into the
     * notes in the group, as appropriate.  Does not require layout x
     * coordinates to have been set.
     */
    void applyStemProperties();

    /**
     * Writes beam data into each note in the group.  Notes' layout x
     * coordinates must already have been set.  Does not require
     * applyStemProperties to have already been called.
     */
    void applyBeam(NotationStaff &);

    /**
     * Writes tupling line data into each note in the group.  Notes'
     * layout x coordinates must already have been set.  Does nothing
     * if this is not a tupled group.
     */
    void applyTuplingLine(NotationStaff &);

    virtual bool contains(const NELIterator &) const;

    virtual bool sample(const NELIterator &i);

protected:
    virtual bool test(const NELIterator &i);

private:
    struct Beam
    {                           // if a beam has a line equation y = mx + c,
        int  gradient;          // -- then this is m*100 (i.e. a percentage)
        int  startY;            // -- and this is c
        bool aboveNotes;
        bool necessary;
    };

    Beam calculateBeam(NotationStaff &);

    int height(const NELIterator&) const;

    //--------------- Data members ---------------------------------

    std::pair<Rosegarden::timeT, Rosegarden::timeT> m_barRange;
    const Rosegarden::Clef &m_clef;
    const Rosegarden::Key &m_key;
    int m_weightAbove, m_weightBelow;
    bool m_userSamples;
    long m_groupNo;
    Type m_type;

    const NotationProperties &m_properties;
};

#endif
