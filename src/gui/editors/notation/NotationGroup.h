
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

#ifndef RG_NOTATIONGROUP_H
#define RG_NOTATIONGROUP_H

#include "base/Sets.h"
#include <utility>
#include "base/Event.h"
#include "NotationElement.h"




namespace Rosegarden
{

class Quantizer;
class NotationStaff;
class NotationProperties;
class Key;
class Clef;


/// Several sorts of "Beamed Group"

class NotationGroup : public AbstractSet<NotationElement,
                                         NotationElementList>
{
public:
    typedef NotationElementList::iterator NELIterator;

    enum Type { Beamed, Tupled };

    /// Group contents will be sampled from elements surrounding elementInGroup
    NotationGroup(NotationElementList &nel, NELIterator elementInGroup,
                  const Quantizer *,
                  std::pair<timeT, timeT> barRange,
                  const NotationProperties &properties,
                  const Clef &clef, const Key &key);

    /// Caller intends to call sample() for each item in the group, _in order_
    NotationGroup(NotationElementList &nel,
                  const Quantizer *,
                  const NotationProperties &properties,
                  const Clef &clef, const Key &key);

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

    virtual bool sample(const NELIterator &i, bool goingForwards);

protected:
    virtual bool test(const NELIterator &i);
    virtual void initialiseFinish(void);

private:
    struct Beam
    {                           // if a beam has a line equation y = mx + c,
	float gradient;         // -- then this is m*100 (i.e. a percentage)
        int   startY;           // -- and this is c
        bool  aboveNotes;
        bool  necessary;
    };

    Beam calculateBeam(NotationStaff &);

    int height(const NELIterator&) const;

    bool haveInternalRest() const;

    //--------------- Data members ---------------------------------

    std::pair<timeT, timeT> m_barRange;
    const Clef &m_clef;
    const Key &m_key;
    int m_weightAbove, m_weightBelow;
    bool m_userSamples;
    long m_groupNo;
    Type m_type;

    const NotationProperties &m_properties;
};


}

#endif
