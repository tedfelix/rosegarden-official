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

#ifndef NOTATION_COMMANDS_H
#define NOTATION_COMMANDS_H

#include "BaseProperties.h"
#include <kcommand.h>
#include <klocale.h>

class EventSelection;

#include "basiccommand.h"

/**
 * Subclass of BasicCommand that manages the brute-force undo and redo
 * extends based on a given selection.
 */

class BasicSelectionCommand : public BasicCommand
{
public:
    virtual ~BasicSelectionCommand();

protected:
    BasicSelectionCommand(const QString &name, EventSelection &selection,
			  bool bruteForceRedoRequired = false);
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

    //--------------- Data members ---------------------------------

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

    //--------------- Data members ---------------------------------

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
		 Rosegarden::Event *event,
		 bool collapseRest);
    virtual ~EraseCommand();

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

    //--------------- Data members ---------------------------------

    bool m_collapseRest;

    Rosegarden::Event *m_event; // only used on 1st execute (cf bruteForceRedo)
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

    static QString name() { return "&Beam Group"; }

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

    static QString name() { return "&Auto-Beam"; }

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

    static QString name() { return "&Unbeam"; }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.unbeam(getBeginTime(), getEndTime());
    }
};


class GroupMenuAddIndicationCommand : public BasicCommand
{
public:
    GroupMenuAddIndicationCommand(std::string indicationType,
				  EventSelection &selection);
    virtual ~GroupMenuAddIndicationCommand();

    Rosegarden::Event *getLastInsertedEvent() {
	return m_lastInsertedEvent;
    }
    virtual Rosegarden::timeT getRelayoutEndTime() {
	return getBeginTime() + m_indicationDuration;
    }

    static QString name(std::string indicationType);

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

    //--------------- Data members ---------------------------------

    std::string m_indicationType;
    Rosegarden::timeT m_indicationDuration;
    Rosegarden::Event *m_lastInsertedEvent;
};
    


// Transforms menu commands


class TransformsMenuNormalizeRestsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuNormalizeRestsCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }
    virtual ~TransformsMenuNormalizeRestsCommand() { }

    static QString name() { return "&Normalize Rests"; }

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

    static QString name() { return "&Collapse Rests Aggressively"; }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper) {
	helper.collapseRestsAggressively(getBeginTime(), getEndTime());
    }
};

class TransformsMenuChangeStemsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuChangeStemsCommand(bool up, EventSelection &selection) :
	BasicSelectionCommand(name(up), selection, true),
	m_selection(&selection), m_up(up) { }
    virtual ~TransformsMenuChangeStemsCommand() { }

    static QString name(bool up) {
	return up ? "Stems &Up" : "Stems &Down";
    }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

private:
    //--------------- Data members ---------------------------------

    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    bool m_up;
};

class TransformsMenuRestoreStemsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuRestoreStemsCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }
    virtual ~TransformsMenuRestoreStemsCommand() { }

    static QString name() {
	return "&Restore Computed Stems";
    }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};

class TransformsMenuTransposeOneStepCommand : public BasicSelectionCommand
{
public:
    TransformsMenuTransposeOneStepCommand(bool up, EventSelection &selection) :
	BasicSelectionCommand(name(up), selection, true),
	m_selection(&selection), m_up(up) { }
    virtual ~TransformsMenuTransposeOneStepCommand() { }

    static QString name(bool up) {
	return up ? "&Up a Semitone" : "&Down a Semitone";
    }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

private:
    //--------------- Data members ---------------------------------

    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    bool m_up;
};

class TransformsMenuAddMarkCommand : public BasicSelectionCommand
{
public:
    TransformsMenuAddMarkCommand(Rosegarden::Mark mark,
				 EventSelection &selection) :
	BasicSelectionCommand(name(mark), selection, true),
	m_selection(&selection), m_mark(mark) { }
    virtual ~TransformsMenuAddMarkCommand() { }

    static QString name(Rosegarden::Mark mark);

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

private:
    //--------------- Data members ---------------------------------

    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::Mark m_mark;
};

class TransformsMenuRemoveMarksCommand : public BasicSelectionCommand
{
public:
    TransformsMenuRemoveMarksCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }
    virtual ~TransformsMenuRemoveMarksCommand() { }

    static QString name() {
	return "&Remove Marks";
    }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

private:
    //--------------- Data members ---------------------------------

    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


class TransformsMenuLabelChordsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuLabelChordsCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }
    virtual ~TransformsMenuLabelChordsCommand() { }

    static QString name() {
	return "Label &Chords";
    }

protected:
    virtual void modifySegment(Rosegarden::SegmentNotationHelper &helper);

private:
    //--------------- Data members ---------------------------------

    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};

#endif
