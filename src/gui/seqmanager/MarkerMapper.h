/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MARKERMAPPER_H
#define RG_MARKERMAPPER_H

#include "SpecialSegmentMapper.h"


namespace Rosegarden
{
class RosegardenDocument;

// @class mapper for markers.  We don't store a copy of the marker
// list here, we get it from composition when we need it.
// @author Tom Breton (Tehom)
class MarkerMapper : public SpecialSegmentMapper
{
public:
    explicit MarkerMapper(RosegardenDocument *doc);

protected:
    // override from MappedEventBuffer
    int calculateSize() override;

    // override from MappedEventBuffer
    void fillBuffer() override;

    bool shouldPlay(MappedEvent *evt, RealTime /*startTime*/) override;
};

}

#endif /* ifndef RG_MARKERMAPPER_H */
