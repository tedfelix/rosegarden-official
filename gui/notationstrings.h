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

#include <qstring.h>
#include "NotationTypes.h"


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


    /**
     * Get the name of a note.  The default return values are American
     * (e.g. quarter note, dotted sixteenth note). If the app is
     * internationalised, you will get return names local to your
     * region.  Note that this includes English note names- set your
     * LC_LANG to en_GB.
     */
    static QString getNoteName(Rosegarden::Note note,
			       bool plural = false, bool triplet = false);

    /**
     * Get the UNTRANSLATED American name of a note.  This may be
     * useful if the user has specified that they'd prefer American
     * names to local names.
     */
    static QString getAmericanName(Rosegarden::Note note,
				   bool plural = false, bool triplet = false);

    /**
     * Get the short name of a note.  The default return values are
     * American (e.g. quarter, dotted 16th). If the app is
     * internationalised, you will get return names local to your
     * region.  Note that this includes English note names- set your
     * LC_LANG to en_GB.
     */
    static QString getShortNoteName(Rosegarden::Note note,
				    bool plural = false, bool triplet = false);

#ifdef NOT_DEFINED
    /**
     * Get the name of a note.  The default return values are American
     * (e.g. quarter note, dotted sixteenth note). If the app is
     * internationalised, you will get return names local to your
     * region.  Note that this includes English note names-
     * set your LC_LANG to en_GB.
     * The default arguments are the values of the note on which the
     * method is called; non-default arguments specify another note type.
     */
    static QString getNoteName(Rosegarden::Note::Type type, int dots = 0);

    /**
     * Get the short name of a note (e.g. quarter, dotted 16th).
     */
    static QString getShortNoteName(Rosegarden::Note::Type type, int dots = 0);

    /**
     * Get the UNTRANSLATED US name of a note (e.g. quarter, dotted sixteenth).
     */
    static QString getAmericanName(Note &note, bool plural = false, bool triplet = false);

    /**
     * Get the UNTRANSLATED British name of a note (e.g. crotchet, dotted minim).
     */
    static QString getBritishName(Note &note, bool plural = false, bool triplet = false);
#endif

    /**
     * Get the UNTRANSLATED reference name of a note or rest.  This is the
     * formal name used to name pixmap files and the like, so the exact
     * values of these strings are pretty sensitive.
     */
    static QString getReferenceName(Rosegarden::Note note, bool isRest = false);

    typedef Rosegarden::Exception MalformedNoteName;

    /**
     * Get the note corresponding to the given string, which must be a
     * reference name or an untranslated British, American or short name.
     * May throw MalformedNoteName.
     */
    static Rosegarden::Note getNoteForName(QString name);

    /**
     * Construct a label to describe the given duration as a note name in
     * the proper locale.  Uses the nearest available note to the duration
     * and returns a non-zero value in errorReturn if it was not an exact
     * match for the required duration.
     */
    static QString makeNoteMenuLabel(Rosegarden::timeT duration,
				     bool brief,
				     Rosegarden::timeT &errorReturn,
				     bool plural = false);

private:
    /**
     * Return a string representing the dotted version of the input str.
     */
    static QString addDots(QString s, int dots,
			   bool hyphenate, bool internationalize);

};    
#endif // NOTATIONSTRINGS_H
