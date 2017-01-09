
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTATIONSTRINGS_H
#define RG_NOTATIONSTRINGS_H

#include "base/Exception.h"
#include "base/NotationTypes.h"
#include <QString>
#include "base/Event.h"

#include <QCoreApplication>


namespace Rosegarden
{



/**
 * String factory for note names, etc. used in the GUI
 * Replaces use of base/NotationTypes.h strings which should
 * be used only for non-user purposes.
 */
class NotationStrings
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::NotationStrings)

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
    static QString getNoteName(Note note,
                               bool plural = false, bool triplet = false);

    /**
     * Get the UNTRANSLATED American name of a note.  This may be
     * useful if the user has specified that they'd prefer American
     * names to local names.
     */
    static QString getAmericanName(Note note,
                                   bool plural = false, bool triplet = false);

    /**
     * Get the short name of a note.  The default return values are
     * American (e.g. quarter, dotted 16th). If the app is
     * internationalised, you will get return names local to your
     * region.  Note that this includes English note names- set your
     * LC_LANG to en_GB.
     */
    static QString getShortNoteName(Note note,
                                    bool plural = false, bool triplet = false);


    /**
     * Get the UNTRANSLATED reference name of a note or rest.  This is the
     * formal name used to name pixmap files and the like, so the exact
     * values of these strings are pretty sensitive.
     */
    static QString getReferenceName(Note note, bool isRest = false);

    typedef Exception MalformedNoteName;

    /**
     * Get the note corresponding to the given string, which must be a
     * reference name or an untranslated British, American or short name.
     * May throw MalformedNoteName.
     */
    static Note getNoteForName(QString name);

    /**
     * Construct a label to describe the given duration as a note name in
     * the proper locale.  Uses the nearest available note to the duration
     * and returns a non-zero value in errorReturn if it was not an exact
     * match for the required duration.
     */
    static QString makeNoteMenuLabel(timeT duration,
                                     bool brief,
                                     timeT &errorReturn,
                                     bool plural = false);

private:
    /**
     * Return a string representing the dotted version of the input str.
     */
    static QString addDots(QString s, int dots,
                           bool hyphenate, bool internationalize);

};    

}

#endif
