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

#include <cstdio>

#include <vector>
#include <set>
#include <string>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>

#include "rosedebug.h"
#include "notationstrings.h"
#include "NotationTypes.h"

#include <iostream>
using std::cerr;
using std::endl;


using Rosegarden::Note;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::TimeSignature;

using Rosegarden::Accidental;
using namespace Rosegarden::Accidentals;

using std::set;
using std::string;
using std::vector;

QString
NotationStrings::addDots(int dots, QString s, bool hyphenate)
{
    QString dotStr("dotted");
    if (dots > 1) {
        dotStr.prepend("-").prepend(dots);
    }
    if (hyphenate) {
        dotStr.append("-");
    }
    dotStr.append(s);
    return dotStr;
}

/**
 * Get the name of a note.  The default return values are American
 * (e.g. quarter note, dotted sixteenth note). If the app is
 * internationalised, you will get return names local to your
 * region.  Note that this includes English note names-
 * set your LC_LANG to en_GB.
 * The default arguments are the values of the note on which the
 * method is called; non-default arguments specify another note type.
 */
QString
NotationStrings::getNoteName(Note &note, bool plural, bool triplet)
{
    Note::Type type = note.getNoteType();
    static const QString names[] = {
        i18n("sixty-fourth note"), i18n("thirty-second note"),
        i18n("sixteenth note"), i18n("eighth note"),
        i18n("quarter note"), i18n("half note"),
        i18n("whole note"), i18n("double whole note")
    };
    static const QString pluralnames[] = {
        i18n("sixty-fourth notes"), i18n("thirty-second notes"),
        i18n("sixteenth notes"), i18n("eighth notes"),
        i18n("quarter notes"), i18n("half notes"),
        i18n("whole notes"), i18n("double whole notes")
    };
    if (plural && triplet) {        
        return i18n("%1 triplets").arg(names[type]);
    } else if (plural) {
        return pluralnames[type];
    } else if (triplet) {
        return i18n("%1 triplet").arg(names[type]);
    }
    return names[type];
}


/**
 * Get the name of a note.  The default return values are American
 * (e.g. quarter note, dotted sixteenth note). If the app is
 * internationalised, you will get return names local to your
 * region.  Note that this includes English note names-
 * set your LC_LANG to en_GB.
 * The default arguments are the values of the note on which the
 * method is called; non-default arguments specify another note type.
 */
QString
NotationStrings::getNoteName(Note::Type type, int dots)
{
    return getShortNoteName(type, dots);
}

/**
 * Get the short name of a note (e.g. quarter, dotted 16th).
 * The default arguments are the values of the note on which the
 * method is called; non-default arguments specify another note type.
 */
QString
NotationStrings::getShortNoteName(Note::Type type, int dots)
{
    static const QString names[] = {
        i18n("64th"), i18n("32nd"), i18n("16th"), i18n("8th"),
        i18n("quarter"), i18n("half"), i18n("whole"),
        i18n("double whole")
    };
    if (dots) return addDots(dots, names[type]);
    else return names[type];
}

/**
 * Get the UNTRANSLATED American name of a note
 */
QString
NotationStrings::getAmericanName(Note &note, bool plural, bool triplet)
{
    Note::Type type = note.getNoteType();
    static const QString names[] = {
        QString("sixty-fourth note"), QString("thirty-second note"),
        QString("sixteenth note"), QString("eighth note"),
        QString("quarter note"), QString("half note"),
        QString("whole note"), QString("double whole note")
    };
    if (plural && triplet) {        
        return QString("%1 triplets").arg(names[type]);
    } else if (plural) {
        return QString("%1s").arg(names[type]);
    } else if (triplet) {
        return QString("%1 triplet").arg(names[type]);
    }
    return names[type];
}

/**
 * Get the UNTRANSLATED British name of a note
 */
QString
NotationStrings::getBritishName(Note &note, bool plural, bool triplet)
{
    Note::Type type = note.getNoteType();
    static const QString names[] = {
        QString("hemidemisemiquaver"), QString("demisemiquaver"),
        QString("semiquaver"), QString("quaver"),
        QString("crotchet"), QString("minim"),
        QString("semibreve"), QString("double whole note")
    };
    if (plural && triplet) {        
        return QString("%1 triplets").arg(names[type]);
    } else if (plural) {
        return QString("%1s").arg(names[type]);
    } else if (triplet) {
        return QString("%1 triplet").arg(names[type]);
    }
    return names[type];
}

QString
NotationStrings::makeNoteMenuLabel(Rosegarden::timeT duration,
				     bool brief,
				     Rosegarden::timeT &errorReturn,
				     bool plural)
{
    Note nearestNote = Note::getNearestNote(duration);
    bool triplet = false;
    errorReturn = 0;

    if (duration == 0) return "0";

    if (nearestNote.getDuration() != duration) {
	Note tripletNote = Note::getNearestNote(duration * 3 / 2);
	if (tripletNote.getDuration() == duration * 3 / 2) {
	    nearestNote = tripletNote;
	    triplet = true;
	} else {
	    errorReturn = duration - nearestNote.getDuration();
	    duration = nearestNote.getDuration();
	}
    }

    if (brief) {

	Rosegarden::timeT wholeNote = Note(Note::Semibreve).getDuration();
	if ((wholeNote / duration) * duration == wholeNote) {
	    return QString("1/%1").arg(wholeNote / duration);
	} else if ((duration / wholeNote) * wholeNote == duration) {
	    return QString("%1/1").arg(duration / wholeNote);
	} else {
	    return i18n("%1 ticks").arg(duration);
	    plural = false;
	}

    } else {
	QString noteName = NotationStrings::getNoteName(nearestNote,
                                                        plural,
                                                        triplet);
        // Already internationalised
	return noteName;
    }
}
