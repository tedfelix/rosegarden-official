// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
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


#include "rosestrings.h"
#include "basiccommand.h"
#include "notestyle.h"
#include <klocale.h>


// Insertion commands

class NoteInsertionCommand : public BasicCommand
{
public:
    NoteInsertionCommand(Rosegarden::Segment &segment,
			 Rosegarden::timeT time,
			 Rosegarden::timeT endTime,
			 Rosegarden::Note note,
			 int pitch,
			 Rosegarden::Accidental accidental,
			 bool autoBeam,
			 bool matrixType,
			 NoteStyleName noteStyle);
    virtual ~NoteInsertionCommand();

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    Rosegarden::timeT m_insertionTime;
    Rosegarden::Note m_note;
    int m_pitch;
    Rosegarden::Accidental m_accidental;
    bool m_autoBeam;
    bool m_matrixType;
    NoteStyleName m_noteStyle;

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
			 Rosegarden::Clef clef,
			 bool shouldChangeOctave = false,
			 bool shouldTranspose = false);
    virtual ~ClefInsertionCommand();

    static QString getGlobalName(Rosegarden::Clef *clef = 0);
    virtual Rosegarden::timeT getRelayoutEndTime();

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    Rosegarden::Clef m_clef;
    bool m_shouldChangeOctave;
    bool m_shouldTranspose;

    Rosegarden::Event *m_lastInsertedEvent;
};

class TextInsertionCommand : public BasicCommand
{
public:
    TextInsertionCommand(Rosegarden::Segment &segment,
			 Rosegarden::timeT time,
			 Rosegarden::Text text);
    virtual ~TextInsertionCommand();

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    Rosegarden::Text m_text;
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

    static QString getGlobalName(Rosegarden::Key *key = 0) {
	if (key) {
	    return i18n("Change to &Key %1...").arg(strtoqstr(key->getName()));
	} else {
	    return i18n("Add &Key Change...");
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

class MultiKeyInsertionCommand : public KMacroCommand
{
public:
    MultiKeyInsertionCommand(Rosegarden::Composition &composition,
			     Rosegarden::timeT time,
			     Rosegarden::Key key,
			     bool shouldConvert,
			     bool shouldTranspose);
    virtual ~MultiKeyInsertionCommand();

    static QString getGlobalName(Rosegarden::Key *key = 0) {
	if (key) {
	    return i18n("Change all to &Key %1...").arg(strtoqstr(key->getName()));
	} else {
	    return i18n("Add &Key Change...");
	}
    }
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



// Group menu commands


class GroupMenuBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuBeamCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    static QString getGlobalName() { return i18n("&Beam Group"); }

protected:
    virtual void modifySegment();
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


class GroupMenuAutoBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuAutoBeamCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection) { }

    GroupMenuAutoBeamCommand(Rosegarden::Segment &segment) :
	BasicSelectionCommand(getGlobalName(), segment) { }

    static QString getGlobalName() { return i18n("&Auto-Beam"); }

protected:
    virtual void modifySegment();
};


class GroupMenuBreakCommand : public BasicSelectionCommand
{
public:
    GroupMenuBreakCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    static QString getGlobalName() { return i18n("&Unbeam"); }

protected:
    virtual void modifySegment();
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


class GroupMenuTupletCommand : public BasicCommand
{
public:
    GroupMenuTupletCommand(Rosegarden::Segment &segment,
			   Rosegarden::timeT startTime,
			   Rosegarden::timeT unit,
			   int untupled = 3, int tupled = 2);

    static QString getGlobalName(bool simple = true) {
	if (simple) return i18n("&Simple Tuplet");
	else return i18n("&Tuplet...");
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::timeT m_unit;
    int m_untupled;
    int m_tupled;
};


class GroupMenuUnTupletCommand : public BasicSelectionCommand
{
public:
    GroupMenuUnTupletCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    static QString getGlobalName() {
	return i18n("&Untuplet");
    }

protected:
    virtual void modifySegment();
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


class GroupMenuGraceCommand : public BasicCommand
{
public:
    GroupMenuGraceCommand(Rosegarden::EventSelection &selection);

    static QString getGlobalName() { return i18n("Make &Grace Notes"); }

protected:
    virtual void modifySegment();
    Rosegarden::timeT getEffectiveEndTime(Rosegarden::EventSelection &);
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};

class GroupMenuUnGraceCommand : public BasicSelectionCommand
{
public:
    GroupMenuUnGraceCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection) { }

    static QString getGlobalName() { return i18n("Ung&race"); }

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
	return getStartTime() + m_indicationDuration;
    }

    static QString getGlobalName(std::string indicationType);

protected:
    virtual void modifySegment();

    std::string m_indicationType;
    Rosegarden::timeT m_indicationDuration;
    Rosegarden::Event *m_lastInsertedEvent;
};
    


// Transforms menu commands


class TransformsMenuNormalizeRestsCommand : public BasicCommand
{
public:
    TransformsMenuNormalizeRestsCommand(Rosegarden::Segment &s,
					Rosegarden::timeT startTime,
					Rosegarden::timeT endTime) :
	BasicCommand(getGlobalName(), s, startTime, endTime) { }

    TransformsMenuNormalizeRestsCommand(Rosegarden::EventSelection &selection);

    static QString getGlobalName() { return i18n("&Normalize Rests"); }

protected:
    virtual void modifySegment();
};


class TransformsMenuCollapseRestsCommand : public BasicCommand
{
public:
    TransformsMenuCollapseRestsCommand(Rosegarden::Segment &s,
				       Rosegarden::timeT startTime,
				       Rosegarden::timeT endTime) :
	BasicCommand(getGlobalName(), s, startTime, endTime) { }

    TransformsMenuCollapseRestsCommand(Rosegarden::EventSelection &selection);

    static QString getGlobalName() { return i18n("&Collapse Rests"); }

protected:
    virtual void modifySegment();
};


class TransformsMenuCollapseNotesCommand : public BasicSelectionCommand
{
public:
    TransformsMenuCollapseNotesCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    static QString getGlobalName() { return i18n("Collapse &Equal-Pitch Notes"); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};
    

class TransformsMenuTieNotesCommand : public BasicSelectionCommand
{
public:
    TransformsMenuTieNotesCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    static QString getGlobalName() { return i18n("&Tie Equal-Pitch Notes"); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};
     

class TransformsMenuUntieNotesCommand : public BasicSelectionCommand
{
public:
    TransformsMenuUntieNotesCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    static QString getGlobalName() { return i18n("&Untie Notes"); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};
    

class TransformsMenuMakeNotesViableCommand : public BasicSelectionCommand
{
public:
    TransformsMenuMakeNotesViableCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    TransformsMenuMakeNotesViableCommand(Rosegarden::Segment &segment) :
	BasicSelectionCommand(getGlobalName(), segment, true),
	m_selection(0) { }

    static QString getGlobalName() { return i18n("Make Notes &Viable"); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};
     

class TransformsMenuDeCounterpointCommand : public BasicSelectionCommand
{
public:
    TransformsMenuDeCounterpointCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    TransformsMenuDeCounterpointCommand(Rosegarden::Segment &segment) :
	BasicSelectionCommand(getGlobalName(), segment, true),
	m_selection(0) { }

    static QString getGlobalName() { return i18n("De-&Counterpoint"); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};
  

class TransformsMenuChangeStemsCommand : public BasicSelectionCommand
{
public:
    TransformsMenuChangeStemsCommand(bool up, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(up), selection, true),
	m_selection(&selection), m_up(up) { }

    static QString getGlobalName(bool up) {
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
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    static QString getGlobalName() { return i18n("&Restore Computed Stems"); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


class TransformsMenuChangeStyleCommand : public BasicSelectionCommand
{
public:
    TransformsMenuChangeStyleCommand(NoteStyleName style,
				     Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(style), selection, true),
	m_selection(&selection), m_style(style) { }

    static QString getGlobalName() {
	return i18n("Change &Note Style");
    }

    static QString getGlobalName(NoteStyleName style);

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    NoteStyleName m_style;
};


class NotesMenuAddSlashesCommand : public BasicSelectionCommand
{
public:
    NotesMenuAddSlashesCommand(int number,
			       Rosegarden::EventSelection &selection) :
	BasicSelectionCommand("Slashes", selection, true),
	m_selection(&selection), m_number(number) { }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_number;
};    

class MarksMenuAddMarkCommand : public BasicSelectionCommand
{
public:
    MarksMenuAddMarkCommand(Rosegarden::Mark mark,
			    Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(mark), selection, true),
	m_selection(&selection), m_mark(mark) { }

    static QString getGlobalName(Rosegarden::Mark mark);

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::Mark m_mark;
};


class MarksMenuAddTextMarkCommand : public BasicSelectionCommand
{
public:
    MarksMenuAddTextMarkCommand(std::string text,
				Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection), m_text(text) { }

    static QString getGlobalName() { return i18n("Add Te&xt Mark..."); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    std::string m_text;
};


class MarksMenuRemoveMarksCommand : public BasicSelectionCommand
{
public:
    MarksMenuRemoveMarksCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection) { }

    static QString getGlobalName() { return i18n("&Remove All Marks"); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};

/*!!!
class TransformsMenuFixSmoothingCommand : public BasicSelectionCommand
{
public:
    TransformsMenuFixSmoothingCommand(Rosegarden::EventSelection &selection,
				      Rosegarden::Quantizer *quantizer) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection),
	m_quantizer(quantizer) { }
    
    static QString getGlobalName() { return i18n("Fi&x Smoothed Values"); }
    
protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::Quantizer *m_quantizer;
};
*/

class TransformsMenuInterpretCommand : public BasicSelectionCommand
{
public:
    // bit masks: pass an OR of these to the constructor
    static const int NoInterpretation;
    static const int GuessDirections;    // allegro, rit, pause &c: kinda bogus
    static const int ApplyTextDynamics;  // mp, ff
    static const int ApplyHairpins;      // self-evident
    static const int StressBeats;        // stress bar/beat boundaries
    static const int Articulate;         // slurs, marks, legato etc
    static const int AllInterpretations; // all of the above

    TransformsMenuInterpretCommand(Rosegarden::EventSelection &selection,
				   const Rosegarden::Quantizer *quantizer,
				   int interpretations) :
	BasicSelectionCommand(getGlobalName(), selection, true),
	m_selection(&selection),
	m_quantizer(quantizer),
	m_interpretations(interpretations) { }

    virtual ~TransformsMenuInterpretCommand();

    //!!! might be nice to get a name based on the interpretations
    // applied as well
    static QString getGlobalName() { return i18n("&Interpret..."); }
    
protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    const Rosegarden::Quantizer *m_quantizer;
    int m_interpretations;

    typedef std::map<Rosegarden::timeT,
		     Rosegarden::Indication *> IndicationMap;
    IndicationMap m_indications;

    void guessDirections();
    void applyTextDynamics();
    void applyHairpins();
    void stressBeats();
    void articulate(); // must be applied last

    // test if the event is within an indication of the given type, return
    // an iterator pointing to that indication if so
    IndicationMap::iterator findEnclosingIndication(Rosegarden::Event *,
						    std::string type);
    int getVelocityForDynamic(std::string dynamic);
};


#endif
