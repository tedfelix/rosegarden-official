/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef NOTATION_COMMANDS_H
#define NOTATION_COMMANDS_H

#include "Segment.h"
#include "SegmentNotationHelper.h"
#include "BaseProperties.h"
#include <kcommand.h>
#include <klocale.h>

class EventSelection;

/**
 * Command that handles undo on a single Segment by brute-force,
 * saving the affected area of the Segment before the execute() and
 * then restoring it on unexecute().  Subclass this with your own
 * implementation.
 *
 * This class implements execute() and unexecute(); in order to create
 * a working subclass, you should normally put the code you would
 * otherwise have put in execute() into modifySegment().
 */

class BasicCommand : public KCommand
{
public:
    virtual ~BasicCommand();

    virtual void execute();
    virtual void unexecute();

    Rosegarden::Segment &getSegment() { return m_segment; }
    const Rosegarden::Segment &getSegment() const { return m_segment; }

    Rosegarden::timeT getBeginTime() { return m_savedEvents.getStartIndex(); }
    Rosegarden::timeT getEndTime() { return m_endTime; }
    virtual Rosegarden::timeT getRelayoutEndTime() { return getEndTime(); }

protected:
    BasicCommand(const QString &name,
		 Rosegarden::Segment &segment,
		 Rosegarden::timeT begin, Rosegarden::timeT end);

    virtual void modifySegment(Rosegarden::SegmentNotationHelper &) = 0;

    virtual void beginExecute();
    virtual void finishExecute();

private:
    void deleteSavedEvents();
    Rosegarden::Segment &m_segment;
    Rosegarden::Segment m_savedEvents;
    Rosegarden::timeT m_endTime;
};


/**
 * Subclass of BasicCommand that manages the brute-force undo and redo
 * extends based on a given selection.
 */

class BasicSelectionCommand : public BasicCommand
{
public:
    virtual ~BasicSelectionCommand();

protected:
    BasicSelectionCommand(const QString &name, EventSelection &selection);
};


// Insertion commands

class NoteInsertionCommand : public BasicCommand
{
public:
    NoteInsertionCommand(Rosegarden::Segment &segment,
			 Rosegarden::timeT time,
			 Rosegarden::timeT endTime,
			 Rosegarden::Note note,
			 int pitch,
			 Rosegarden::Accidental accidental);
    virtual ~NoteInsertionCommand();

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

    Rosegarden::Note m_note;
    int m_pitch;
    Rosegarden::Accidental m_accidental;
    Rosegarden::Event *m_lastInsertedEvent;
};

class RestInsertionCommand : public NoteInsertionCommand
{
public:
    RestInsertionCommand(Rosegarden::Segment &segment,
			 Rosegarden::timeT time,
			 Rosegarden::timeT endTime,
			 Rosegarden::Note note);
    virtual ~RestInsertionCommand();

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);
};

class ClefInsertionCommand : public BasicCommand
{
public:
    ClefInsertionCommand(Rosegarden::Segment &segment,
			 Rosegarden::timeT time,
			 Rosegarden::Clef clef);
    virtual ~ClefInsertionCommand();

    virtual Rosegarden::timeT getRelayoutEndTime();

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

    Rosegarden::Clef m_clef;
    Rosegarden::Event *m_lastInsertedEvent;
};


/* Not ideal, but we can't pass in the exact Event as the thing to erase
   because we're just about to erase it, so replay wouldn't work (as the
   event restored to the segment by unexecute() will be only a duplicate,
   not the same event) */

class EraseCommand : public BasicCommand
{
public:
    EraseCommand(Rosegarden::Segment &segment,
		 Rosegarden::timeT time,
		 std::string eventType,
		 int pitch,
		 bool collapseRest);
    virtual ~EraseCommand();

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

    std::string m_eventType;
    int m_pitch;
    bool m_collapseRest;
    Rosegarden::timeT m_relayoutEndTime;

    std::string makeName(std::string);
};


// Group menu commands


class GroupMenuBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuBeamCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }
    virtual ~GroupMenuBeamCommand() { }

    static QString name() { return i18n("Beam Group"); }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.makeBeamedGroup(getBeginTime(), getEndTime(),
			       Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
    }
};


class GroupMenuAutoBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuAutoBeamCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }
    virtual ~GroupMenuAutoBeamCommand() { }

    static QString name() { return i18n("Auto-Beam"); }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.autoBeam(getBeginTime(), getEndTime(),
			Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
    }
};


class GroupMenuBreakCommand : public BasicSelectionCommand
{
public:
    GroupMenuBreakCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }
    virtual ~GroupMenuBreakCommand() { }

    static QString name() { return i18n("Break Groups"); }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.unbeam(getBeginTime(), getEndTime());
    }
};



// Transforms menu commands


class TransformsMenuNormalizeRestsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuNormalizeRestsCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }
    virtual ~TransformsMenuNormalizeRestsCommand() { }

    static QString name() { return i18n("Normalize Rests"); }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.normalizeRests(getBeginTime(), getEndTime());
    }
};


class TransformsMenuCollapseRestsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuCollapseRestsCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }
    virtual ~TransformsMenuCollapseRestsCommand() { }

    static QString name() { return i18n("Collapse Rests Aggressively"); }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.collapseRestsAggressively(getBeginTime(), getEndTime());
    }
};



#endif
