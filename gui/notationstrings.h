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

#ifndef NOTATIONSTRINGS_H
#define NOTATIONSTRINGS_H

#include <vector>
#include <set>

#include <qcanvas.h>
#include <qpainter.h>

#include "NotationTypes.h"

using Rosegarden::Note;
/**
 * String factory for note names, etc. used in the GUI
 * Replaces use of base/NotationTypes.h strings which should
 * be used only for non-user purposes.
 */
class NotationStrings
{
public:
    NotationStrings();
    ~NotationStrings();


    //!!! We need a method to convert a note name (any format) to a
    //note, for use in NoteStyleFileReader etc (because the Note ctor
    //will no longer recognise non-reference names except through a
    //brutal hack)


    /**
     * Return a string representing the dotted version of the input str.
     */
    static QString addDots(int dots, QString s, bool hyphenate = false);

    /**
     * Get the name of a note.  The default return values are American
     * (e.g. quarter note, dotted sixteenth note). If the app is
     * internationalised, you will get return names local to your
     * region.  Note that this includes English note names-
     * set your LC_LANG to en_GB.
     * The default arguments are the values of the note on which the
     * method is called; non-default arguments specify another note type.
     */
    static QString getNoteName(Note &note, bool plural = false, bool triplet = false);

    /**
     * Get the name of a note.  The default return values are American
     * (e.g. quarter note, dotted sixteenth note). If the app is
     * internationalised, you will get return names local to your
     * region.  Note that this includes English note names-
     * set your LC_LANG to en_GB.
     * The default arguments are the values of the note on which the
     * method is called; non-default arguments specify another note type.
     */
    static QString getNoteName(Note::Type type, int dots = 0);

    /**
     * Get the short name of a note (e.g. quarter, dotted 16th).
     */
    static QString getShortNoteName(Note::Type type, int dots = 0);

    /**
     * Get the UNTRANSLATED US name of a note (e.g. quarter, dotted sixteenth).
     */
    static QString getAmericanName(Note &note, bool plural = false, bool triplet = false);

    /**
     * Get the UNTRANSLATED British name of a note (e.g. crotchet, dotted minim).
     */
    static QString getBritishName(Note &note, bool plural = false, bool triplet = false);


    QString makeNoteMenuLabel(Rosegarden::timeT duration,
			      bool brief,
			      Rosegarden::timeT &errorReturn,
			      bool plural = false);

};    
#endif // NOTATIONSTRINGS_H
