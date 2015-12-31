/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[KeyInsertionCommand]"

#include "KeyInsertionCommand.h"

#include "misc/Debug.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Studio.h"
#include "document/BasicCommand.h"
#include "base/BaseProperties.h"
#include "base/Selection.h"
#include <QString>


namespace Rosegarden
{

using namespace BaseProperties;


KeyInsertionCommand::KeyInsertionCommand(Segment &segment, timeT time,
        Key key,
        bool convert,
        bool transpose,
        bool transposeKey,
	bool ignorePercussion) :
        BasicCommand(getGlobalName(&key), segment, time, segment.getEndTime()),
        m_key(key),
        m_lastInsertedEvent(0),
        m_convert(convert),
        m_transpose(transpose),
        m_transposeKey(transposeKey),
	m_ignorePercussion(ignorePercussion)

{
    // nothing
}

KeyInsertionCommand::~KeyInsertionCommand()
{
    // nothing
}

EventSelection *
KeyInsertionCommand::getSubsequentSelection()
{
    EventSelection *selection = new EventSelection(getSegment());
    selection->addEvent(getLastInsertedEvent());
    return selection;
}

void
KeyInsertionCommand::modifySegment()
{
    SegmentNotationHelper helper(getSegment());
    Key oldKey;

    if (m_convert || m_transpose) {
        oldKey = getSegment().getKeyAtTime(getStartTime());
    }

    Segment::iterator i = getSegment().findTime(getStartTime());
    while (getSegment().isBeforeEndMarker(i)) {
        if ((*i)->getAbsoluteTime() > getStartTime()) {
            break;
        }
        if ((*i)->isa(Key::EventType)) {
            getSegment().erase(i);
            break;
        }
        ++i;
    }

    // transpose if desired, according to new dialog option
    if (m_transposeKey) {
        // we don't really care about major/minor for this, so pass it through
        // from the original key
        bool keyIsMinor = m_key.isMinor();

        // get whether the original key is flat or sharp, so we know what to
        // prefer for the new key
        bool keyIsSharp = m_key.isSharp();

        // get the tonic pitch of the user-specified key, reported as a 0-11 int, then
        // add an extra octave to it to avoid winding up with negative numbers
        // (the octave will be stripped back off)
        int specifiedKeyTonic = m_key.getTonicPitch() + 12;

        // get the transpose factor for the segment we're working on
        int segTranspose = getSegment().getTranspose();

        // subtract the transpose factor from the tonic pitch of the
        // user-specified key, because we want to move in the opposite
        // direction for notation (eg. notation is in C major concert, at Bb
        // transposition, we have -2 from the segment, and want to go +2 for
        // the key, from tonic pitch 0 (C) to tonic pitch 2 (D) for the key as
        // written for a Bb instrument
        //
        // sanity check: 0 == C; 0 + 12 == 12; (12 - -2) % 12 == 2; 2 == D
        int transposedKeyTonic = (specifiedKeyTonic - segTranspose) % 12;

        // create a new key with the new tonic pitch, and major/minor from the
        // original key
        std::string newKeyName = "";

        switch (transposedKeyTonic) {
            // 0 C | 1 C# | 2 D | 3 D# | 4 E | 5 F | 6 F# | 7 G | 8 G# | 9 A | 10 A# | 11 B
        case 0 :  // C
            newKeyName = "C";
            break;
        case 2 :  // D
            newKeyName = "D";
            break;
        case 4 :  // E
            newKeyName = "E";
            break;
        case 5 :  // F
            newKeyName = "F";
            break;
        case 7 :  // G
            newKeyName = "G";
            break;
        case 9 :  // A
            newKeyName = "A";
            break;
        case 11:  // B
            newKeyName = "B";
            break;
            // the glorious, glorious black keys need special treatment
            // again, so we pick flat or sharp spellings based on the
            // condition of the original, user-specified key we're
            // transposing
        case 1 :  // C#/Db
            newKeyName = (keyIsSharp ? "C#" : "Db");
            break;
        case 3 :  // D#/Eb
            newKeyName = (keyIsSharp ? "D#" : "Eb");
            break;
        case 6 :  // F#/Gb
            newKeyName = (keyIsSharp ? "F#" : "Gb");
            break;
        case 8 :  // G#/Ab
            newKeyName = (keyIsSharp ? "G#" : "Ab");
            break;
        case 10:   // A#/Bb
            newKeyName = (keyIsSharp ? "A#" : "Bb");
            break;
        default:
            // if this fails, we won't have a valid key name, and
            // there will be some crashing exception I don't know how
            // to intercept and avoid, so I'm doing this lame failsafe
            // instead, which should never, ever actually run under
            // any conceivable cirumstance anyway
            RG_DEBUG << "KeyInsertionCommand: by the pricking of my thumbs, something wicked this way comes.  :("
            << endl;
            return ;
        }

        newKeyName += (keyIsMinor ? " minor" : " major");

        //for f in C# D# E# F# G# A# B# Cb Db Eb Fb Gb Ab Bb;do grep "$f
        //major" NotationTypes.C > /dev/null||echo "invalid key: $f
        //major";grep "$f minor" NotationTypes.C > /dev/null||echo "invalid
        //key: $f minor";done|sort
        //invalid key: A# major
        //invalid key: B# major
        //invalid key: B# minor
        //invalid key: Cb minor
        //invalid key: Db minor
        //invalid key: D# major
        //invalid key: E# major
        //invalid key: E# minor
        //invalid key: Fb major
        //invalid key: Fb minor
        //invalid key: Gb minor
        //invalid key: G# major

        // some kludgery to avoid creating invalid key names with some if/then
        // swapping to manually respell things generated incorrectly by the
        // above, rather than adding all kinds of nonsense to avoid this
        // necessity
        if (newKeyName == "A# major")
            newKeyName = "Bb major";
        else if (newKeyName == "B# major")
            newKeyName = "C major";
        else if (newKeyName == "Cb minor")
            newKeyName = "B minor";
        else if (newKeyName == "Db minor")
            newKeyName = "C# minor";
        else if (newKeyName == "D# major")
            newKeyName = "Eb major";
        else if (newKeyName == "E# major")
            newKeyName = "F major";
        else if (newKeyName == "E# minor")
            newKeyName = "F minor";
        else if (newKeyName == "Fb major")
            newKeyName = "E major";
        else if (newKeyName == "Fb minor")
            newKeyName = "E minor";
        else if (newKeyName == "Gb minor")
            newKeyName = "F# minor";
        else if (newKeyName == "G# major")
            newKeyName = "Ab major";

        // create a new key with the newly derived name, and swap it for the
        // user-specified version
        Key k(newKeyName);
        RG_DEBUG << "KeyInsertCommand: inserting transposed key" << endl
        << "        user key was: " << m_key.getName() << endl
        << "    tranposed key is: " << k.getName() << endl;
        m_key = k;
    } // if (m_transposeKey)

    i = helper.insertKey(getStartTime(), m_key);

    if (i != helper.segment().end()) {

        m_lastInsertedEvent = *i;
        if (!m_convert && !m_transpose)
            return ;

        while (++i != helper.segment().end()) {

            //!!! what if we get two keys at the same time...?
            if ((*i)->isa(Key::EventType))
                break;

            if ((*i)->isa(Note::EventType) &&
                    (*i)->has(PITCH)) {

                long pitch = (*i)->get
                             <Int>(PITCH);

                if (m_convert) {
                    (*i)->set
                    <Int>(PITCH, m_key.convertFrom(pitch, oldKey));
                } else {
                    (*i)->set
                    <Int>(PITCH, m_key.transposeFrom(pitch, oldKey));
                }

                (*i)->unset(ACCIDENTAL);
            }
        }
    }
}

}
