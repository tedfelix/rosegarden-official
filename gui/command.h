// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <kcommand.h>
#include <set>

#include "Segment.h"

/**
 * A set of simple abstract base-classes for commands, to be mixed
 * in (via multiple inheritance) with KCommand.
 *
 * Although you can execute any KCommand in Rosegarden's command
 * history, the various GUI views will only be able to refresh
 * correctly if all commands also subclass from the appropriate one of
 * these bases.  (Please don't attempt to do the refresh yourself --
 * it's much easier to support multiple views using this technique!)
 */


/**
 * SegmentCommand is a base for commands that affect the "envelope"
 * of one or more entire segments, and that may or may not also
 * affect their contents -- for example, resizing or splitting
 * segments.
 */

class SegmentCommand
{
public:
    typedef std::set<Rosegarden::Segment *> SegmentSet;
    
    /** 
     * Obtain the set of affected Segments.  Note that Segments
     * in this set may have been removed from the Composition or
     * even destroyed by the time this command has completed.
     */
    virtual void getSegments(SegmentSet &) = 0;

protected:
    SegmentCommand() { }
};


/**
 * TimeAndTempoChangeCommand is a base for commands that affect
 * only non-Segment data in a Composition, such as time signature
 * and tempo data.
 */

class TimeAndTempoChangeCommand
{
protected:
    TimeAndTempoChangeCommand() { }
};


/**
 * IntraSegmentCommand is a base for commands that affect a
 * (section of) a single Segment.  If that section is contiguous,
 * then you may wish to derive from BasicCommand or
 * BasicSelectionCommand (see basiccommand.h), which are more
 * specialised subclasses of IntraSegmentCommand.
 */

class IntraSegmentCommand
{
public:
    /**
     * Return the segment in which the changes happened.
     */
    virtual Rosegarden::Segment &getSegment() = 0;

protected:
    IntraSegmentCommand() { }
};


#endif
