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
			 Rosegarden::Accidental accidental,
			 bool autoBeam);
    virtual ~NoteInsertionCommand();

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

    Rosegarden::timeT m_insertionTime;
    Rosegarden::Note m_note;
    int m_pitch;
    Rosegarden::Accidental m_accidental;
    bool m_autoBeam;

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

class MultiKeyInsertionCommand : public KMacroCommand
{
public:
    MultiKeyInsertionCommand(Rosegarden::Composition &composition,
			     Rosegarden::timeT time,
			     Rosegarden::Key key,
			     bool shouldConvert,
			     bool shouldTranspose);
    virtual ~MultiKeyInsertionCommand();

    static QString name(Rosegarden::Key *key = 0) {
	if (key) {
	    return QString("Change all to &Key ") + key->getName().c_str() + "...";
	} else {
	    return "Add &Key Change...";
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
	BasicSelectionCommand(name(), selection) { }

#ifdef RGKDE3
    virtual QString name() { return "&Beam Group"; }
#else
    static QString name() { return "&Beam Group"; }
#endif

protected:
    virtual void modifySegment();
};


class GroupMenuAutoBeamCommand : public BasicSelectionCommand
{
public:
    GroupMenuAutoBeamCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

#ifdef RGKDE3
    virtual QString name() { return "&Auto-Beam"; }
#else
    static QString name() { return "&Auto-Beam"; }
#endif

protected:
    virtual void modifySegment();
};


class GroupMenuTupletCommand : public BasicCommand
{
public:
    GroupMenuTupletCommand(Rosegarden::Segment &segment,
			   Rosegarden::timeT startTime,
			   Rosegarden::timeT unit,
			   int untupled = 3, int tupled = 2);

#ifdef RGKDE3
    virtual QString name(bool simple = true) {
	if (simple) return "&Simple Tuplet";
	else return "Tu&plet...";
    }
#else
    static QString name(bool simple = true) {
	if (simple) return "&Simple Tuplet";
	else return "Tu&plet...";
    }
#endif

protected:
    virtual void modifySegment();

private:
    Rosegarden::timeT m_unit;
    int m_untupled;
    int m_tupled;
};


class GroupMenuBreakCommand : public BasicSelectionCommand
{
public:
    GroupMenuBreakCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection) { }

#ifdef RGKDE3
    virtual QString name() { return "&Unbeam"; }
#else
    static QString name() { return "&Unbeam"; }
#endif

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

#ifdef RGKDE3
    virtual QString name(std::string indicationType);
#else
    static QString name(std::string indicationType);
#endif

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
					Rosegarden::timeT beginTime,
					Rosegarden::timeT endTime) :
	BasicCommand(name(), s, beginTime, endTime) { }

    TransformsMenuNormalizeRestsCommand(Rosegarden::EventSelection &selection);

#ifdef RGKDE3
    virtual QString name() { return "&Normalize Rests"; }
#else
    static QString name() { return "&Normalize Rests"; }
#endif

protected:
    virtual void modifySegment();
};


class TransformsMenuCollapseRestsCommand : public BasicCommand
{
public:
    TransformsMenuCollapseRestsCommand(Rosegarden::Segment &s,
				       Rosegarden::timeT beginTime,
				       Rosegarden::timeT endTime) :
	BasicCommand(name(), s, beginTime, endTime) { }

    TransformsMenuCollapseRestsCommand(Rosegarden::EventSelection &selection);

#ifdef RGKDE3
    virtual QString name() { return "&Collapse Rests"; }
#else
    static QString name() { return "&Collapse Rests"; }
#endif

protected:
    virtual void modifySegment();
};


class TransformsMenuCollapseNotesCommand : public BasicSelectionCommand
{
public:
    TransformsMenuCollapseNotesCommand(Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(name(), selection, true),
	m_selection(&selection) { }

#ifdef RGKDE3
    virtual QString name() { return "&Collapse Equal-Pitch Notes"; }
#else
    static QString name() { return "&Collapse Equal-Pitch Notes"; }
#endif

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

#ifdef RGKDE3
    virtual QString name(bool up) {
	return up ? "Stems &Up" : "Stems &Down";
    }
#else
    static QString name(bool up) {
	return up ? "Stems &Up" : "Stems &Down";
    }
#endif

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

#ifdef RGKDE3
    virtual QString name() {
	return "&Restore Computed Stems";
    }
#else
    static QString name() {
	return "&Restore Computed Stems";
    }
#endif

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

#ifdef RGKDE3
    virtual QString name() {
	return "&Transpose...";
    }
#else
    static QString name() {
	return "&Transpose...";
    }
#endif

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

#ifdef RGKDE3
    virtual QString name(bool up) {
	return up ? "&Up a Semitone" : "&Down a Semitone";
    }
#else
    static QString name(bool up) {
	return up ? "&Up a Semitone" : "&Down a Semitone";
    }
#endif

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

#ifdef RGKDE3
    virtual QString name(bool up) {
	return up ? "Up an &Octave" : "Down an Octa&ve";
    }
#else
    static QString name(bool up) {
	return up ? "Up an &Octave" : "Down an Octa&ve";
    }
#endif

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

#ifdef RGKDE3
    virtual QString name(Rosegarden::Mark mark);
#else
    static QString name(Rosegarden::Mark mark);
#endif

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

#ifdef RGKDE3
    virtual QString name() {
	return "Add Te&xt Mark...";
    }
#else
    static QString name() {
	return "Add Te&xt Mark...";
    }
#endif

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

#ifdef RGKDE3
    virtual QString name() {
	return "&Remove All Marks";
    }
#else
    static QString name() {
	return "&Remove All Marks";
    }
#endif

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};



#endif
