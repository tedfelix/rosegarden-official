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

namespace Rosegarden {
    class Clipboard;
}


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

class KeyInsertionCommand : public BasicCommand
{
public:
    KeyInsertionCommand(Rosegarden::Segment &segment,
			Rosegarden::timeT time,
			Rosegarden::Key key,
			bool shouldConvert,
			bool shouldTranspose);
    virtual ~KeyInsertionCommand();

    static QString name(Rosegarden::Key *key = 0) {
	if (key) {
	    return QString("Change to &Key ") + key->getName().c_str() + "...";
	} else {
	    return "Add &Key Change...";
	}
    }

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    Rosegarden::Key m_key;
    Rosegarden::Event *m_lastInsertedEvent;
    bool m_convert;
    bool m_transpose;
};


class EraseEventCommand : public BasicCommand
{
public:
    EraseEventCommand(Rosegarden::Segment &segment,
		      Rosegarden::Event *event,
		      bool collapseRest);
    virtual ~EraseEventCommand();

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

    bool m_collapseRest;

    Rosegarden::Event *m_event; // only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_relayoutEndTime;
    std::string makeName(std::string);
};


// Cut, copy, paste, erase selection

class CutNotationCommand : public CompoundCommand
{
public:
    CutNotationCommand(Rosegarden::EventSelection &selection,
			Rosegarden::Clipboard *clipboard);

    static QString name() { return "Cu&t"; }
};

class CopyNotationCommand : public KCommand // no refresh needed
{
public:
    CopyNotationCommand(Rosegarden::EventSelection &selection,
			Rosegarden::Clipboard *clipboard);
    virtual ~CopyNotationCommand();

    static QString name() { return "&Copy"; }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Clipboard *m_sourceClipboard;
    Rosegarden::Clipboard *m_targetClipboard;
};

class PasteNotationCommand : public BasicCommand
{
public:
    PasteNotationCommand(Rosegarden::Segment &segment,
			 Rosegarden::Clipboard *clipboard,
			 Rosegarden::timeT pasteTime);

    static QString name() { return "&Paste"; }

    /// Determine whether this paste will succeed (without executing it yet)
    bool isPossible();

    virtual Rosegarden::timeT getRelayoutEndTime();
    
protected:
    virtual void modifySegment();
    Rosegarden::timeT getEffectiveEndTime(Rosegarden::Segment &,
					  Rosegarden::Clipboard *,
					  Rosegarden::timeT);
    Rosegarden::timeT m_relayoutEndTime;
    Rosegarden::Clipboard *m_clipboard;
};

class EraseNotationCommand : public BasicSelectionCommand
{
public:
    EraseNotationCommand(Rosegarden::EventSelection &selection);

    static QString name() { return "&Erase"; }

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_relayoutEndTime;
};



// Group menu commands


class GroupMenuBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuBeamCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

    static QString name() { return "&Beam Group"; }

protected:
    virtual void modifySegment();
};


class GroupMenuAutoBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuAutoBeamCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

    static QString name() { return "&Auto-Beam"; }

protected:
    virtual void modifySegment();
};


class GroupMenuBreakCommand : public BasicSelectionCommand
{
public:
    GroupMenuBreakCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

    static QString name() { return "&Unbeam"; }

protected:
    virtual void modifySegment();
};


class GroupMenuAddIndicationCommand : public BasicCommand
{
public:
    GroupMenuAddIndicationCommand(std::string indicationType,
				  Rosegarden::EventSelection &selection);
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
    TransformsMenuNormalizeRestsCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

    static QString name() { return "&Normalize Rests"; }

protected:
    virtual void modifySegment();
};


class TransformsMenuCollapseRestsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuCollapseRestsCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

    static QString name() { return "&Collapse Rests"; }

protected:
    virtual void modifySegment();
};


class TransformsMenuCollapseNotesCommand : public BasicSelectionCommand
{
public:
    TransformsMenuCollapseNotesCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }

    static QString name() { return "&Collapse Equal-Pitch Notes"; }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};
    
  
class TransformsMenuChangeStemsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuChangeStemsCommand(bool up, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(up), selection, true),
	m_selection(&selection), m_up(up) { }

    static QString name(bool up) {
	return up ? "Stems &Up" : "Stems &Down";
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    bool m_up;
};


class TransformsMenuRestoreStemsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuRestoreStemsCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }

    static QString name() {
	return "&Restore Computed Stems";
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


class TransformsMenuTransposeCommand : public BasicSelectionCommand
{
public:
    TransformsMenuTransposeCommand(int semitones, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection), m_semitones(semitones) { }

    static QString name() {
	return "&Transpose...";
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_semitones;
};


class TransformsMenuTransposeOneStepCommand : public BasicSelectionCommand
{
public:
    TransformsMenuTransposeOneStepCommand(bool up, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(up), selection, true),
	m_selection(&selection), m_up(up) { }

    static QString name(bool up) {
	return up ? "&Up a Semitone" : "&Down a Semitone";
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    bool m_up;
};


class TransformsMenuTransposeOctaveCommand : public BasicSelectionCommand
{
public:
    TransformsMenuTransposeOctaveCommand(bool up, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(up), selection, true),
	m_selection(&selection), m_up(up) { }

    static QString name(bool up) {
	return up ? "Up an &Octave" : "Down an Octa&ve";
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    bool m_up;
};


class TransformsMenuAddMarkCommand : public BasicSelectionCommand
{
public:
    TransformsMenuAddMarkCommand(Rosegarden::Mark mark,
				 Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(mark), selection, true),
	m_selection(&selection), m_mark(mark) { }

    static QString name(Rosegarden::Mark mark);

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::Mark m_mark;
};


class TransformsMenuAddTextMarkCommand : public BasicSelectionCommand
{
public:
    TransformsMenuAddTextMarkCommand(std::string text,
				     Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection), m_text(text) { }

    static QString name() {
	return "Add Te&xt Mark...";
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    std::string m_text;
};


class TransformsMenuRemoveMarksCommand : public BasicSelectionCommand
{
public:
    TransformsMenuRemoveMarksCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }

    static QString name() {
	return "&Remove All Marks";
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


#endif
