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

#include "basiccommand.h"


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
    virtual void modifySegment();

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
    virtual void modifySegment();
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
    virtual void modifySegment();

    Rosegarden::Clef m_clef;
    Rosegarden::Event *m_lastInsertedEvent;
};


class EraseCommand : public BasicCommand
{
public:
    EraseCommand(Rosegarden::Segment &segment,
		 Rosegarden::Event *event,
		 bool collapseRest);
    virtual ~EraseCommand();

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

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

    static QString name() { return "&Beam Group"; }

protected:
    virtual void modifySegment();
};


class GroupMenuAutoBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuAutoBeamCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

    static QString name() { return "&Auto-Beam"; }

protected:
    virtual void modifySegment();
};


class GroupMenuBreakCommand : public BasicSelectionCommand
{
public:
    GroupMenuBreakCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

    static QString name() { return "&Unbeam"; }

protected:
    virtual void modifySegment();
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
    virtual void modifySegment();

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

    static QString name() { return "&Normalize Rests"; }

protected:
    virtual void modifySegment();
};


class TransformsMenuCollapseRestsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuCollapseRestsCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

    static QString name() { return "&Collapse Rests"; }

protected:
    virtual void modifySegment();
};


class TransformsMenuCollapseNotesCommand : public BasicSelectionCommand
{
public:
    TransformsMenuCollapseNotesCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }

    static QString name() { return "&Collapse Equal-Pitch Notes"; }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};
    
  
class TransformsMenuChangeStemsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuChangeStemsCommand(bool up, EventSelection &selection) :
	BasicSelectionCommand(name(up), selection, true),
	m_selection(&selection), m_up(up) { }

    static QString name(bool up) {
	return up ? "Stems &Up" : "Stems &Down";
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    bool m_up;
};


class TransformsMenuRestoreStemsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuRestoreStemsCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }

    static QString name() {
	return "&Restore Computed Stems";
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


class TransformsMenuTransposeCommand : public BasicSelectionCommand
{
public:
    TransformsMenuTransposeCommand(int semitones, EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection), m_semitones(semitones) { }

    static QString name() {
	return "&Transpose...";
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_semitones;
};


class TransformsMenuTransposeOneStepCommand : public BasicSelectionCommand
{
public:
    TransformsMenuTransposeOneStepCommand(bool up, EventSelection &selection) :
	BasicSelectionCommand(name(up), selection, true),
	m_selection(&selection), m_up(up) { }

    static QString name(bool up) {
	return up ? "&Up a Semitone" : "&Down a Semitone";
    }

protected:
    virtual void modifySegment();

private:
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

    static QString name(Rosegarden::Mark mark);

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::Mark m_mark;
};


class TransformsMenuAddTextMarkCommand : public BasicSelectionCommand
{
public:
    TransformsMenuAddTextMarkCommand(std::string text,
				     EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection), m_text(text) { }

    static QString name() {
	return "Add Te&xt Mark...";
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    std::string m_text;
};


class TransformsMenuRemoveMarksCommand : public BasicSelectionCommand
{
public:
    TransformsMenuRemoveMarksCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }

    static QString name() {
	return "&Remove All Marks";
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};



class TransformsMenuLabelChordsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuLabelChordsCommand(EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }

    static QString name() {
	return "Label &Chords";
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


#endif
