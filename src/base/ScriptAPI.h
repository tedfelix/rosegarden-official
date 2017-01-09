/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SCRIPT_API_H
#define RG_SCRIPT_API_H

#include "base/Segment.h"

namespace Rosegarden
{

class Composition;

class ScriptInterface
{
public:
    typedef int ScriptId;
    typedef int SegmentId;
    typedef int EventId;
    typedef int ScriptTime;

    // Resolution defines the meaning of ScriptTime units.  If set to
    // the QuantizedNN values, each ScriptTime unit will correspond to
    // the duration of an NN-th note.  If Unquantized, ScriptTime will
    // correspond to timeT, i.e. 960 to a quarter note.
    // And Notation is like Quantized64 except that the times are
    // obtained from the notation time and duration properties of each
    // event instead of the raw ones.

    enum Resolution {
        Unquantized,
        Notation,
        Quantized64,
        Quantized32,
        Quantized16
    };

    enum Scope {
        Global,
        Segment
    };

    class ScriptEvent {
        EventId    id;
        int        bar;   // number, 1-based
        ScriptTime time;  // within bar
        ScriptTime duration;
        int        pitch; // 0-127 if note, -1 otherwise
    };

    class ScriptTimeSignature {
        int        numerator;
        int        denominator;
        ScriptTime duration;
    };

    class ScriptKeySignature {
        int accidentals;
        bool sharps;
        bool minor;
    };

    ScriptInterface(Composition *composition);
    virtual ~ScriptInterface();

    ScriptId createScript(SegmentId target, Resolution resolution, Scope scope);
    void destroyScript(ScriptId id);

    // A script can only proceed forwards.  The getEvent and getNote
    // methods get the next event (including notes) or note within the
    // current chord or timeslice; the advance method moves forwards
    // to the next chord or other event.  So to process through all
    // events, call advance() followed by a loop of getEvent() calls
    // before the next advance(), and so on.  An event with id -1
    // marks the end of a slice.  ( -1 is an out-of-range value for
    // all types of id.)

    ScriptEvent getEvent(ScriptId id);
    ScriptEvent getNote(ScriptId id);

    bool advance(ScriptId id);

    ScriptTimeSignature getTimeSignature(ScriptId id);
    ScriptKeySignature getKeySignature(ScriptId id);

    EventId addNote(ScriptId id,
                    int bar, ScriptTime time, ScriptTime duration, int pitch);

    EventId addEvent(ScriptId id,
                     std::string type, int bar, ScriptTime time, ScriptTime duration);

    void deleteEvent(ScriptId id, EventId event);

    std::string getEventType(ScriptId id, EventId event);
    std::string getProperty(ScriptId id, EventId event, std::string property);
    void setProperty(ScriptId id, EventId event, std::string property, std::string value);

protected:
    Composition *m_composition;

    class ScriptContainer;
    ScriptContainer *m_scripts;
};

}


#endif

        
