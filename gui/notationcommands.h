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

class NotationView;
class EventSelection;

// Implementation of Command that handles undo on a single Segment by
// brute-force (saving the affected area of the Segment and then
// restoring it if necessary).  This class implements execute() and
// unexecute(); you should normally put the code you would otherwise
// have put in execute() into modifySegment() instead, although you
// can choose to override execute() so long as you call beginExecute()
// at the start of your implementation and finishExecute() at the end.

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

    NotationView *getView() { return m_view; }

protected:
    BasicCommand(const QString &name, NotationView *view,
		 Rosegarden::Segment &segment,
		 Rosegarden::timeT begin, Rosegarden::timeT end);

    virtual void modifySegment(Rosegarden::SegmentNotationHelper &) = 0;

    virtual void beginExecute();
    virtual void finishExecute();

private:
    void deleteSavedEvents();
    NotationView *m_view;
    Rosegarden::Segment &m_segment;
    Rosegarden::Segment m_savedEvents;
    Rosegarden::timeT m_endTime;
};

// Implementation of Command that handles undo and redo by
// brute-force, based on the extents of an EventSelection

class BasicSelectionCommand : public BasicCommand
{
public:
    virtual ~BasicSelectionCommand();

protected:
    BasicSelectionCommand(const QString &name, NotationView *view,
			  EventSelection &selection);
};


class GroupMenuBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuBeamCommand(NotationView *view, EventSelection &selection) :
	BasicSelectionCommand(name(), view, selection) { }
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
    GroupMenuAutoBeamCommand(NotationView *view, EventSelection &selection) :
	BasicSelectionCommand(name(), view, selection) { }
    virtual ~GroupMenuAutoBeamCommand() { }

    static QString name() { return i18n("Auto-Beam Group"); }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.autoBeam(getBeginTime(), getEndTime(),
			Rosegarden::BaseProperties::GROUP_TYPE_BEAMED);
    }
};


class GroupMenuBreakCommand : public BasicSelectionCommand
{
public:
    GroupMenuBreakCommand(NotationView *view, EventSelection &selection) :
	BasicSelectionCommand(name(), view, selection) { }
    virtual ~GroupMenuBreakCommand() { }

    static QString name() { return i18n("Break Group"); }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.unbeam(getBeginTime(), getEndTime());
    }
};


class TransformsMenuNormalizeRestsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuNormalizeRestsCommand(NotationView *view,
					EventSelection &selection) :
	BasicSelectionCommand(name(), view, selection) { }
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
    TransformsMenuCollapseRestsCommand(NotationView *view,
					EventSelection &selection) :
	BasicSelectionCommand(name(), view, selection) { }
    virtual ~TransformsMenuCollapseRestsCommand() { }

    static QString name() { return i18n("Collapse Rests Aggressively"); }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.collapseRestsAggressively(getBeginTime(), getEndTime());
    }
};



#endif
