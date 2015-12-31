
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

#ifndef RG_NOTATIONCHORD_H
#define RG_NOTATIONCHORD_H

#include "base/NotationTypes.h"
#include "base/Sets.h"
#include "NotationElement.h"

class Iterator;


namespace Rosegarden
{

class Quantizer;
class NotationProperties;


class NotationChord : public GenericChord<NotationElement,
                                          NotationElementList,
                                          true>
{
public:
    NotationChord(NotationElementList &c,
                  NotationElementList::iterator i,
                  const Quantizer *quantizer,
                  const NotationProperties &properties,
                  const Clef &clef = Clef::DefaultClef,
                  const Key &key = Key::DefaultKey);

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

    void applyAccidentalShiftProperties();

    virtual int getMaxAccidentalShift(bool &extra) const;
    virtual int getAccidentalShift(const NotationElementList::iterator &itr,
                                   bool &extra) const;

protected:
    const NotationProperties &m_properties;
    Clef m_clef;
    Key m_key;


    int getHeight(const Iterator&) const;
};



}

#endif
