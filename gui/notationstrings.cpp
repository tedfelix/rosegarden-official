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

#include "notationstrings.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardenconfigurationpage.h"

#include <kconfig.h>
#include <kapp.h>

using Rosegarden::Note;

QString
NotationStrings::addDots(QString s, int dots,
			 bool hyphenate, bool internationalize)
{
    if (!dots) return s;

    if (internationalize) {
	if (dots > 1) {
	    if (hyphenate) return i18n("%1-dotted-%2").arg(dots).arg(s);
	    else	   return i18n("%1-dotted %2").arg(dots).arg(s);
	} else {
	    if (hyphenate) return i18n("dotted-%1").arg(s);
	    else           return i18n("dotted %1").arg(s);
	}
    } else {
	if (dots > 1) {
	    if (hyphenate) return QString("%1-dotted-%2").arg(dots).arg(s);
	    else	   return QString("%1-dotted %2").arg(dots).arg(s);
	} else {
	    if (hyphenate) return QString("dotted-%1").arg(s);
	    else	   return QString("dotted %1").arg(s);
	}
    }
}

/**
 * Get the name of a note.  The default return values are American
 * (e.g. quarter note, dotted sixteenth note). If the app is
 * internationalised, you will get return names local to your
 * region.  Note that this includes English note names-
 * set your LC_LANG to en_GB.
 */
QString
NotationStrings::getNoteName(Note note, bool plural, bool triplet)
{
    Note::Type type = note.getNoteType();
    int dots = note.getDots();

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
        return addDots(i18n("%1 triplets").arg(names[type]), dots, false, true);
    } else if (plural) {
        return addDots(pluralnames[type], dots, false, true);
    } else if (triplet) {
        return addDots(i18n("%1 triplet").arg(names[type]), dots, false, true);
    } else {
	return addDots(names[type], dots, false, true);
    }
}

/**
 * Get the UNTRANSLATED American name of a note.
 */
QString
NotationStrings::getAmericanName(Note note, bool plural, bool triplet)
{
    Note::Type type = note.getNoteType();
    int dots = note.getDots();

    static const QString names[] = {
        "sixty-fourth note", "thirty-second note",
        "sixteenth note", "eighth note",
        "quarter note", "half note",
        "whole note", "double whole note"
    };
    static const QString pluralnames[] = {
        "sixty-fourth notes", "thirty-second notes",
        "sixteenth notes", "eighth notes",
        "quarter notes", "half notes",
        "whole notes", "double whole notes"
    };

    if (plural && triplet) {        
        return addDots(QString("%1 triplets").arg(names[type]), dots, false, false);
    } else if (plural) {
        return addDots(pluralnames[type], dots, false, false);
    } else if (triplet) {
        return addDots(QString("%1 triplet").arg(names[type]), dots, false, false);
    } else {
	return addDots(names[type], dots, false, false);
    }
}

/**
 * Get the short name of a note (e.g. quarter, dotted 16th).
 */
QString
NotationStrings::getShortNoteName(Note note, bool plural, bool triplet)
{
    Note::Type type = note.getNoteType();
    int dots = note.getDots();

    static const QString names[] = {
        i18n("64th"), i18n("32nd"), i18n("16th"), i18n("8th"),
        i18n("quarter"), i18n("half"), i18n("whole"),
        i18n("double whole")
    };
    static const QString pluralnames[] = {
        i18n("64ths"), i18n("32nds"), i18n("16ths"), i18n("8ths"),
        i18n("quarters"), i18n("halves"), i18n("wholes"),
        i18n("double wholes")
    };

    if (plural && triplet) {        
        return addDots(i18n("%1 triplets").arg(names[type]), dots, false, true);
    } else if (plural) {
        return addDots(pluralnames[type], dots, false, true);
    } else if (triplet) {
        return addDots(i18n("%1 triplet").arg(names[type]), dots, false, true);
    } else {
	return addDots(names[type], dots, false, true);
    }
}

/**
 * Get the UNTRANSLATED reference name of a note.
 */
QString
NotationStrings::getReferenceName(Note note, bool isRest)
{
    Note::Type type = note.getNoteType();
    int dots = note.getDots();

    static const QString names[] = {
        "hemidemisemi", "demisemi", "semiquaver",
        "quaver", "crotchet", "minim", "semibreve", "breve"
    };

    QString name(names[type]);
    if (isRest) name = "rest-" + name;
    return addDots(name, dots, true, false);
}

/**
 * Get the note corresponding to the given string, which must be a
 * reference name or an untranslated British, American or short name.
 * May throw MalformedNoteName.
 */
Note
NotationStrings::getNoteForName(QString name)
{
    std::string origName(qstrtostr(name));
    int pos = name.find('-');
    int dots = 0;

    if (pos > 0 && pos < 6 && pos < name.length()-1) {
	dots = name.left(pos).toInt();
	name = name.right(name.length() - pos - 1);
	if (dots < 2) {
	    throw MalformedNoteName("Non-numeric or invalid dot count in \"" +
                                    origName + "\"");
	}
    }

    if (name.length() > 7 && 
	(name.left(7) == "dotted " || name.left(7) == "dotted-")) {
	if (dots == 0) dots = 1;
	name = name.right(name.length() - 7);
    } else {
	if (dots > 1) {
	    throw MalformedNoteName("Dot count without dotted tag in \"" +
                                    origName + "\"");
	}
    }

    if (name.length() > 5 && name.right(5) == " note") {
	name = name.left(name.length() - 5);
    }

    Note::Type type;
    
    static const char *names[][4] = {
	{ "64th",         "sixty-fourth",  "hemidemisemi", "hemidemisemiquaver" },
	{ "32nd",	  "thirty-second", "demisemi",     "demisemiquaver"	},
	{ "16th",	  "sixteenth",     "semi",	   "semiquaver"		},
	{ "8th",          "eighth",	   0, 		   "quaver"		},
	{ "quarter",      0, 		   0, 		   "crotchet",		},
	{ "half",	  0, 		   0, 		   "minim"		},
	{ "whole",	  0, 		   0, 		   "semibreve"		},
	{ "double whole", 0, 		   0, 		   "breve"		}
    };

    for (type = Note::Shortest; type <= Note::Longest; ++type) {
	for (int i = 0; i < 4; ++i) {
	    if (!names[type][i]) continue;
	    if (name == names[type][i]) return Note(type, dots);
	}
    }

    throw MalformedNoteName("Can't parse note name \"" + origName + "\"");
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

    KConfig *config = kapp->config();
    config->setGroup("General Options");
    Rosegarden::GeneralConfigurationPage::NoteNameStyle noteNameStyle =
	(Rosegarden::GeneralConfigurationPage::NoteNameStyle)
	config->readUnsignedNumEntry
	("notenamestyle", Rosegarden::GeneralConfigurationPage::Local);

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
	QString noteName;

	switch (noteNameStyle) {

	case Rosegarden::GeneralConfigurationPage::American:
	    noteName = getAmericanName(nearestNote, plural, triplet);
	    break;

	case Rosegarden::GeneralConfigurationPage::Local:
            noteName = getNoteName(nearestNote, plural, triplet);
	    break;
	}

        // Already internationalised, if appropriate
	return noteName;
    }
}
