// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef EDIT_COMMANDS_H
#define EDIT_COMMANDS_H


#include "basiccommand.h"
#include "notestyle.h"

#include "Quantizer.h"
#include "TriggerSegment.h"
#include <klocale.h>

namespace Rosegarden {
    class Clipboard;
    class Composition;
    class EventSelection;
    class SegmentSelection;
    class Marker;

    // Patterns of properties
    //
    typedef enum 
    {
        FlatPattern,          // set selection to velocity 1.
        AlternatingPattern,   // alternate between velocity 1 and 2 on
                              // subsequent events.
        CrescendoPattern,     // increasing from velocity 1 to velocity 2.
        DecrescendoPattern,   // decreasing from velocity 1 to velocity 2.
        RingingPattern        // between velocity 1 and 2, dying away.
    } PropertyPattern;
}


/// Cut a selection

class CutCommand : public KMacroCommand
{
public:
    /// Make a CutCommand that cuts events from within a Segment
    CutCommand(Rosegarden::EventSelection &selection,
	       Rosegarden::Clipboard *clipboard);

    /// Make a CutCommand that cuts whole Segments
    CutCommand(Rosegarden::SegmentSelection &selection,
	       Rosegarden::Clipboard *clipboard);

    static QString getGlobalName() { return i18n("Cu&t"); }
};


/// Cut a selection and close the gap

class CutAndCloseCommand : public KMacroCommand
{
public:
    CutAndCloseCommand(Rosegarden::EventSelection &selection,
		       Rosegarden::Clipboard *clipboard);

    static QString getGlobalName() { return i18n("C&ut and Close"); }

protected:
    class CloseCommand : public KNamedCommand
    {
    public:
	CloseCommand(Rosegarden::Segment *segment,
		     Rosegarden::timeT fromTime,
		     Rosegarden::timeT toTime) :
	    KNamedCommand("Close"),
	    m_segment(segment),
	    m_gapEnd(fromTime),
	    m_gapStart(toTime) { }

	virtual void execute();
	virtual void unexecute();

    private:
	Rosegarden::Segment *m_segment;
	Rosegarden::timeT m_gapEnd;
	Rosegarden::timeT m_gapStart;
	int m_staticEvents;
    };
};    


/// Copy a selection

class CopyCommand : public KNamedCommand
{
public:
    /// Make a CopyCommand that copies events from within a Segment
    CopyCommand(Rosegarden::EventSelection &selection,
		Rosegarden::Clipboard *clipboard);

    /// Make a CopyCommand that copies whole Segments
    CopyCommand(Rosegarden::SegmentSelection &selection,
		Rosegarden::Clipboard *clipboard);

    virtual ~CopyCommand();

    static QString getGlobalName() { return i18n("&Copy"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Clipboard *m_sourceClipboard;
    Rosegarden::Clipboard *m_targetClipboard;
};


/// Paste one or more segments from the clipboard into the composition

class PasteSegmentsCommand : public KNamedCommand
{
public:
    PasteSegmentsCommand(Rosegarden::Composition *composition,
			 Rosegarden::Clipboard *clipboard,
			 Rosegarden::timeT pasteTime);

    virtual ~PasteSegmentsCommand();

    static QString getGlobalName() { return i18n("&Paste"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::Clipboard *m_clipboard;
    Rosegarden::timeT m_pasteTime;
    std::vector<Rosegarden::Segment *> m_addedSegments;
    bool m_detached;
};


/// Paste from a single-segment clipboard to a segment

class PasteEventsCommand : public BasicCommand
{
public:
    enum PasteType {
	Restricted,		// paste into existing gap
	Simple,			// erase existing events to make room
	OpenAndPaste,		// bump up existing events to make room
	NoteOverlay,		// overlay and tie notation-style
	MatrixOverlay		// overlay raw matrix-style
    };

    typedef std::map<PasteType, QString> PasteTypeMap;
    static PasteTypeMap getPasteTypes(); // type, descrip

    /**
     * Construct a Paste command from a clipboard that already contains
     * the events to be pasted.
     */
    PasteEventsCommand(Rosegarden::Segment &segment,
		       Rosegarden::Clipboard *clipboard,
		       Rosegarden::timeT pasteTime,
		       PasteType pasteType);

    /**
     * Construct a Paste command from a clipboard that will contain
     * the events to be pasted by the time the Paste command is
     * executed, but might not do so yet.  This is necessary if the
     * Paste command is to follow another clipboard-based command
     * in a KMacroCommand sequence.  pasteEndTime must supply the
     * latest time in the destination segment that may be modified
     * by the paste.
     */
    PasteEventsCommand(Rosegarden::Segment &segment,
		       Rosegarden::Clipboard *clipboard,
		       Rosegarden::timeT pasteTime,
		       Rosegarden::timeT pasteEndTime,
		       PasteType pasteType);

    static QString getGlobalName() { return i18n("&Paste"); }

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
    PasteType m_pasteType;
};


/// Erase a selection from within a segment

class EraseCommand : public BasicSelectionCommand
{
public:
    EraseCommand(Rosegarden::EventSelection &selection);

    static QString getGlobalName() { return i18n("&Erase"); }

    virtual Rosegarden::timeT getRelayoutEndTime();

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_relayoutEndTime;
};


/**
 * Replace an event with another one (likely to be used in
 * conjunction with EventEditDialog)
 */

class EventEditCommand : public BasicCommand
{
public:
    EventEditCommand(Rosegarden::Segment &segment,
		     Rosegarden::Event *eventToModify,
		     const Rosegarden::Event &newEvent);

    static QString getGlobalName() { return i18n("Edit E&vent"); }

protected:
    virtual void modifySegment();

private:
    Rosegarden::Event *m_oldEvent; // only used on 1st execute
    Rosegarden::Event m_newEvent; // only used on 1st execute
};


class EventQuantizeCommand : public QObject, public BasicCommand
{
    Q_OBJECT

public:
    /// Quantizer must be on heap (EventQuantizeCommand dtor will delete)
    EventQuantizeCommand(Rosegarden::Segment &segment,
			 Rosegarden::timeT startTime,
			 Rosegarden::timeT endTime,
			 Rosegarden::Quantizer *);
    
    /// Quantizer must be on heap (EventQuantizeCommand dtor will delete)
    EventQuantizeCommand(Rosegarden::EventSelection &selection,
			 Rosegarden::Quantizer *);

    /// Constructs own quantizer based on KConfig data in given group
    EventQuantizeCommand(Rosegarden::Segment &segment,
			 Rosegarden::timeT startTime,
			 Rosegarden::timeT endTime,
			 QString configGroup,
			 bool notationDefault);
    
    /// Constructs own quantizer based on KConfig data in given group
    EventQuantizeCommand(Rosegarden::EventSelection &selection,
			 QString configGroup,
			 bool notationDefault);

    ~EventQuantizeCommand();
    
    static QString getGlobalName(Rosegarden::Quantizer *quantizer = 0);
    void setProgressTotal(int total) { m_progressTotal = total; }

signals:
    void incrementProgress(int);

protected:
    virtual void modifySegment();

private:
    Rosegarden::Quantizer *m_quantizer; // I own this
    Rosegarden::EventSelection *m_selection;
    QString m_configGroup;
    int m_progressTotal;

    /// Sets to m_quantizer as well as returning value
    Rosegarden::Quantizer *makeQuantizer(QString, bool);
};

// Set the (numerical) property of a selection according given pattern.
//
class SelectionPropertyCommand : public BasicSelectionCommand
{
public:

    SelectionPropertyCommand(Rosegarden::EventSelection *selection,
                             const Rosegarden::PropertyName &property,
                             Rosegarden::PropertyPattern pattern,
                             int value1,
                             int value2);

    static QString getGlobalName() { return i18n("Set &Property"); }

    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;
    Rosegarden::PropertyName    m_property;
    Rosegarden::PropertyPattern m_pattern;
    int                         m_value1;
    int                         m_value2;

};

class EventUnquantizeCommand : public BasicCommand
{
public:
    /// Quantizer must be on heap (EventUnquantizeCommand dtor will delete)
    EventUnquantizeCommand(Rosegarden::Segment &segment,
			   Rosegarden::timeT startTime,
			   Rosegarden::timeT endTime,
			   Rosegarden::Quantizer *);
    
    /// Quantizer must be on heap (EventUnquantizeCommand dtor will delete)
    EventUnquantizeCommand(Rosegarden::EventSelection &selection,
			   Rosegarden::Quantizer *);

    ~EventUnquantizeCommand();
    
    static QString getGlobalName(Rosegarden::Quantizer *quantizer = 0);
    
protected:
    virtual void modifySegment();

private:
    Rosegarden::Quantizer *m_quantizer;
    Rosegarden::EventSelection *m_selection;
};


class SetLyricsCommand : public KNamedCommand
{
public:
    SetLyricsCommand(Rosegarden::Segment *segment, QString newLyricData);
    ~SetLyricsCommand();
    
    static QString getGlobalName() { return i18n("Edit L&yrics"); }

    virtual void execute();
    virtual void unexecute();

private:
    Rosegarden::Segment *m_segment;
    std::vector<Rosegarden::Event *> m_oldLyricEvents;
    QString m_newLyricData;
};


class TransposeCommand : public BasicSelectionCommand
{
public:
    TransposeCommand(int semitones, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(semitones), selection, true),
	m_selection(&selection), m_semitones(semitones) { }

    static QString getGlobalName(int semitones = 0) {
	switch (semitones) {
	case   1: return i18n("&Up a Semitone");
	case  -1: return i18n("&Down a Semitone");
	case  12: return i18n("Up an &Octave");
	case -12: return i18n("Down an Octa&ve");
	default:  return i18n("&Transpose...");
	}
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_semitones;
};


class RescaleCommand : public BasicCommand
{
public:
    RescaleCommand(Rosegarden::EventSelection &selection,
		   Rosegarden::timeT newDuration,
		   bool closeGap);

    static QString getGlobalName() { return i18n("Stretch or S&quash..."); }
    
protected:
    virtual void modifySegment();

private:
    Rosegarden::timeT rescale(Rosegarden::timeT);

    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_oldDuration;
    Rosegarden::timeT m_newDuration;
    bool m_closeGap;
};


class MoveCommand : public BasicCommand
{
public:
    MoveCommand(Rosegarden::Segment &segment,
		Rosegarden::timeT delta,
		bool useNotationTimings,
		Rosegarden::EventSelection &selection);
    
    static QString getGlobalName(Rosegarden::timeT delta = 0);

    Rosegarden::Event *getLastInsertedEvent() { return m_lastInsertedEvent; }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::timeT m_delta;
    bool m_useNotationTimings;
    Rosegarden::Event *m_lastInsertedEvent;
};

class MoveAcrossSegmentsCommand : public KMacroCommand
{
public:
    MoveAcrossSegmentsCommand(Rosegarden::Segment &firstSegment,
			      Rosegarden::Segment &secondSegment,
			      Rosegarden::timeT newStartTime,
			      bool notation,
			      Rosegarden::EventSelection &selection);
    virtual ~MoveAcrossSegmentsCommand();

    static QString getGlobalName();

private:
    Rosegarden::Clipboard *m_clipboard;
};
    

/** Add or subtract a constant from all event velocities.
    Use SelectionPropertyCommand if you want to do something more
    creative. */
class ChangeVelocityCommand : public BasicSelectionCommand
{
public:
    ChangeVelocityCommand(int delta, Rosegarden::EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(delta), selection, true),
	m_selection(&selection), m_delta(delta) { }

    static QString getGlobalName(int delta = 0) {
	if (delta > 0) return i18n("&Increase Velocity");
	else return i18n("&Reduce Velocity");
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_delta;
};

class AddMarkerCommand : public KNamedCommand
{
public:
    AddMarkerCommand(Rosegarden::Composition *comp,
                     Rosegarden::timeT time,
                     const std::string &name,
                     const std::string &description);
    ~AddMarkerCommand();

    static QString getGlobalName() { return i18n("&Add Marker"); }

    virtual void execute();
    virtual void unexecute();

protected:

    Rosegarden::Composition     *m_composition;
    Rosegarden::Marker          *m_marker;
    bool                         m_detached;

};


class RemoveMarkerCommand : public KNamedCommand
{
public:
    RemoveMarkerCommand(Rosegarden::Composition *comp,
                        Rosegarden::timeT time,
                        const std::string &name,
                        const std::string &description);
    ~RemoveMarkerCommand();

    static QString getGlobalName() { return i18n("&Remove Marker"); }

    virtual void execute();
    virtual void unexecute();

protected:

    Rosegarden::Composition     *m_composition;
    Rosegarden::Marker          *m_marker;
    Rosegarden::timeT            m_time;
    std::string                  m_name;
    std::string                  m_descr;
    bool                         m_detached;

};

class ModifyMarkerCommand : public KNamedCommand
{
public:
    ModifyMarkerCommand(Rosegarden::Composition *comp,
                        Rosegarden::timeT time,
                        Rosegarden::timeT newTime,
                        const std::string &name,
                        const std::string &des);
    ~ModifyMarkerCommand();

    static QString getGlobalName() { return i18n("&Modify Marker"); }

    virtual void execute();
    virtual void unexecute();

protected:

    Rosegarden::Composition     *m_composition;
    Rosegarden::timeT            m_time;
    Rosegarden::timeT            m_newTime;

    std::string                  m_name;
    std::string                  m_description;
    std::string                  m_oldName;
    std::string                  m_oldDescription;

};


class SetTriggerCommand : public BasicSelectionCommand
{
public:
    SetTriggerCommand(Rosegarden::EventSelection &selection,
		      Rosegarden::TriggerSegmentId triggerSegmentId,
		      bool notesOnly,
		      bool retune,
		      bool adjustDuration,
		      QString name = 0) :
	BasicSelectionCommand(name ? name : getGlobalName(), selection, true),
	m_selection(&selection),
	m_triggerSegmentId(triggerSegmentId),
	m_notesOnly(notesOnly),
	m_retune(retune),
	m_adjustDuration(adjustDuration)
    { }

    static QString getGlobalName() {
	return i18n("Tri&gger Segment");
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    Rosegarden::TriggerSegmentId m_triggerSegmentId;
    bool m_notesOnly;
    bool m_retune;
    bool m_adjustDuration;
};


class ClearTriggersCommand : public BasicSelectionCommand
{
public:
    ClearTriggersCommand(Rosegarden::EventSelection &selection,
			 QString name = 0) :
	BasicSelectionCommand(name ? name : getGlobalName(), selection, true),
	m_selection(&selection)
    { }

    static QString getGlobalName() {
	return i18n("&Clear Triggers");
    }

protected:
    virtual void modifySegment();

private:
    Rosegarden::EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
};


class InsertTriggerNoteCommand : public BasicCommand
{
public:
    InsertTriggerNoteCommand(Rosegarden::Segment &,
			     Rosegarden::timeT time,
			     Rosegarden::Note note,
			     int pitch,
			     int velocity,
			     NoteStyleName noteStyle,
			     Rosegarden::TriggerSegmentId id,
			     bool retune,
			     bool adjustDuration);
    virtual ~InsertTriggerNoteCommand();

protected:
    virtual void modifySegment();

    Rosegarden::timeT m_time;
    Rosegarden::Note m_note;
    int m_pitch;
    int m_velocity;
    NoteStyleName m_noteStyle;
    Rosegarden::TriggerSegmentId m_id;
    bool m_retune;
    bool m_adjustDuration;
};


#endif
