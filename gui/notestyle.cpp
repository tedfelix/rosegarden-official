// -*- c-basic-offset: 4 -*-

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

#include "notestyle.h"
#include "notefont.h"

#include <qfileinfo.h>
#include <qdir.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>

#include "rosestrings.h"
#include "rosedebug.h"


using Rosegarden::Accidental;
using Rosegarden::Accidentals;
using Rosegarden::Mark;
using Rosegarden::Marks;
using Rosegarden::Clef;
using Rosegarden::Note;


namespace StandardNoteStyleNames
{
    const NoteStyleName Classical = "_Classical";
    const NoteStyleName Cross     = "_Cross";
    const NoteStyleName Triangle  = "_Triangle";
    const NoteStyleName Mensural  = "_Mensural";

    std::vector<NoteStyleName> getStandardStyles() {

	static NoteStyleName a[] = {
	    Classical, Cross, Triangle, Mensural
	};

	static std::vector<NoteStyleName> v;
	if (v.size() == 0) {
	    for (unsigned int i = 0; i < sizeof(a)/sizeof(a[0]); ++i)
		v.push_back(a[i]);
	}
	return v;
    }

}


NoteStyle *
NoteStyleFactory::getStyle(NoteStyleName name)
{
    StyleMap::iterator i = m_styles.find(name);

    if (i == m_styles.end()) {

	NoteStyle *newStyle = 0;

	if (name == StandardNoteStyleNames::Classical) {
	    newStyle = new ClassicalNoteStyle();

	} else if (name == StandardNoteStyleNames::Cross) {
	    newStyle = new CrossNoteStyle(); 

	} else if (name == StandardNoteStyleNames::Triangle) {
	    newStyle = new TriangleNoteStyle();

	} else if (name == StandardNoteStyleNames::Mensural) {
	    newStyle = new MensuralNoteStyle();

	} else {
	    newStyle = new CustomNoteStyle(name);
	}
	
	m_styles[name] = newStyle;
	return newStyle;

    } else {
	return i->second;
    }
}

NoteStyleFactory::StyleMap NoteStyleFactory::m_styles;



NoteStyle::~NoteStyle()
{
    // nothing
}



//!!! Need to amend all uses of the Note presentation accessors
// to use these instead -- so we can get rid of the Note methods
// that these are intended to replace


NoteStyle::NoteHeadShape
NoteStyle::getShape(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getShape(type);
	//!!! blow up nicely, instead of just doing this, please
	throw(-1);
    }

    return i->second.shape;
}


bool
NoteStyle::isFilled(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->isFilled(type);
	//!!! blow up nicely, instead of just doing this, please
	throw(-1);
    }

    return i->second.black;
}


bool
NoteStyle::hasStem(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->hasStem(type);
	//!!! blow up nicely, instead of just doing this, please
	throw(-1);
    }

    return i->second.stem;
}


int
NoteStyle::getFlagCount(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getFlagCount(type);
	//!!! blow up nicely, instead of just doing this, please
	throw(-1);
    }

    return i->second.flags;
}


CharName
NoteStyle::getNoteHeadCharName(Rosegarden::Note::Type type)
{
    if (m_notes.empty()) initialiseNotes();

    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getNoteHeadCharName(type);
	//!!! blow up nicely, instead of just doing this, please
	throw(-1);
    }
    const NoteDescription &desc(i->second);

    switch (desc.shape) {

    case AngledOval:
	return desc.black ? NoteCharacterNames::NOTEHEAD_BLACK
	                  : NoteCharacterNames::VOID_NOTEHEAD;

    case LevelOval:
	return desc.black ? NoteCharacterNames::UNKNOWN //!!!
	                  : NoteCharacterNames::WHOLE_NOTE;
	
    case Breve:
	return desc.black ? NoteCharacterNames::UNKNOWN //!!!
	                  : NoteCharacterNames::BREVE;
	
    case Cross:
	return desc.black ? NoteCharacterNames::X_NOTEHEAD
	                  : NoteCharacterNames::CIRCLE_X_NOTEHEAD;

    case TriangleUp:
	return desc.black ? NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_BLACK
                          : NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_WHITE;

    case TriangleDown:
	return NoteCharacterNames::UNKNOWN; //!!!

    case Diamond:
	return desc.black ? NoteCharacterNames::SEMIBREVIS_BLACK
 	                  : NoteCharacterNames::SEMIBREVIS_WHITE;

    case Rectangle:
	return NoteCharacterNames::UNKNOWN; //!!!
	
    case Number:
	return NoteCharacterNames::UNKNOWN; //!!!
    }

    return NoteCharacterNames::UNKNOWN;
}

CharName
NoteStyle::getAccidentalCharName(const Accidental &a)
{
    if      (a == Accidentals::Sharp)        return NoteCharacterNames::SHARP;
    else if (a == Accidentals::Flat)         return NoteCharacterNames::FLAT;
    else if (a == Accidentals::Natural)      return NoteCharacterNames::NATURAL;
    else if (a == Accidentals::DoubleSharp)  return NoteCharacterNames::DOUBLE_SHARP;
    else if (a == Accidentals::DoubleFlat)   return NoteCharacterNames::DOUBLE_FLAT;
    return NoteCharacterNames::UNKNOWN;
}

CharName
NoteStyle::getMarkCharName(const Mark &mark)
{
    if      (mark == Marks::Accent)    return NoteCharacterNames::ACCENT;
    else if (mark == Marks::Tenuto)    return NoteCharacterNames::TENUTO;
    else if (mark == Marks::Staccato)  return NoteCharacterNames::STACCATO;
    else if (mark == Marks::Staccatissimo) return NoteCharacterNames::STACCATISSIMO;
    else if (mark == Marks::Marcato)   return NoteCharacterNames::MARCATO;
    else if (mark == Marks::Trill)     return NoteCharacterNames::TRILL;
    else if (mark == Marks::Turn)      return NoteCharacterNames::TURN;
    else if (mark == Marks::Pause)     return NoteCharacterNames::FERMATA;
    else if (mark == Marks::UpBow)     return NoteCharacterNames::UP_BOW;
    else if (mark == Marks::DownBow)   return NoteCharacterNames::DOWN_BOW;
    // Things like "sf" and "rf" are generated from text fonts
    return NoteCharacterNames::UNKNOWN;
}
 
CharName
NoteStyle::getClefCharName(const Clef &clef)
{
    string clefType(clef.getClefType());

    if (clefType == Clef::Bass) {
        return NoteCharacterNames::F_CLEF;
    } else if (clefType == Clef::Treble) {
        return NoteCharacterNames::G_CLEF;
    } else {
        return NoteCharacterNames::C_CLEF;
    }
}

CharName
NoteStyle::getRestCharName(Note::Type type)
{
    switch (type) {
    case Note::Hemidemisemiquaver:  return NoteCharacterNames::SIXTY_FOURTH_REST;
    case Note::Demisemiquaver:      return NoteCharacterNames::THIRTY_SECOND_REST;
    case Note::Semiquaver:          return NoteCharacterNames::SIXTEENTH_REST;
    case Note::Quaver:              return NoteCharacterNames::EIGHTH_REST;
    case Note::Crotchet:            return NoteCharacterNames::QUARTER_REST;
    case Note::Minim:               return NoteCharacterNames::HALF_REST;
    case Note::Semibreve:           return NoteCharacterNames::WHOLE_REST;
    case Note::Breve:               return NoteCharacterNames::MULTI_REST;
    default:
        return NoteCharacterNames::UNKNOWN;
    }
}

CharName
NoteStyle::getFlagCharName(int flagCount)
{
    switch (flagCount) {
    case 1:  return NoteCharacterNames::FLAG_1;
    case 2:  return NoteCharacterNames::FLAG_2;
    case 3:  return NoteCharacterNames::FLAG_3;
    case 4:  return NoteCharacterNames::FLAG_4;
    default: return NoteCharacterNames::UNKNOWN;
    }
}


void
ClassicalNoteStyle::initialiseNotes()
{
    for (Note::Type type = Note::Shortest; type <= Note::Crotchet; ++type) {
	m_notes[type] = NoteDescription
	    (AngledOval, true, true, Note::Crotchet - type);
    }

    m_notes[Note::Minim]     = NoteDescription(AngledOval, false, true,  0);
    m_notes[Note::Semibreve] = NoteDescription(LevelOval,  false, false, 0);
    m_notes[Note::Breve]     = NoteDescription(Breve,      false, false, 0);
}


void
CrossNoteStyle::initialiseNotes()
{
    for (Note::Type type = Note::Shortest; type <= Note::Crotchet; ++type) {
	m_notes[type] = NoteDescription
	    (Cross, true, true, Note::Crotchet - type);
    }

    m_notes[Note::Minim]     = NoteDescription(Cross, false, true,  0);
    m_notes[Note::Semibreve] = NoteDescription(Cross, false, false, 0);
    m_notes[Note::Breve]     = NoteDescription(Cross, false, false, 0);
}


void
TriangleNoteStyle::initialiseNotes()
{
    for (Note::Type type = Note::Shortest; type <= Note::Crotchet; ++type) {
	m_notes[type] = NoteDescription
	    (TriangleUp, true, true, Note::Crotchet - type);
    }

    m_notes[Note::Minim]     = NoteDescription(TriangleUp, false, true,  0);
    m_notes[Note::Semibreve] = NoteDescription(TriangleUp, false, false, 0);
    m_notes[Note::Breve]     = NoteDescription(TriangleUp, false, false, 0);
}


void
MensuralNoteStyle::initialiseNotes()
{
    for (Note::Type type = Note::Shortest; type <= Note::Crotchet; ++type) {
	m_notes[type] = NoteDescription
	    (Diamond, true, true, Note::Crotchet - type);
    }

    m_notes[Note::Minim]     = NoteDescription(Diamond, false, true,  0);
    m_notes[Note::Semibreve] = NoteDescription(Diamond, false, false, 0);
    m_notes[Note::Breve]     = NoteDescription(Diamond, false, false, 0);
}


CustomNoteStyle::CustomNoteStyle(std::string name)
{
    if (name[0] == '_') {
	throw StyleFileReadFailed
	    (qstrtostr(i18n("Leading underscore (in ") +
		       strtoqstr(name) +
		       i18n(") reserved for built-in styles")));
    }

    QString styleDirectory =
	KGlobal::dirs()->findResource("appdata", "styles/");

    QString styleFileName =
	QString("%1/%2.xml").arg(styleDirectory).arg(strtoqstr(name));

    QFileInfo fileInfo(styleFileName);

    if (!fileInfo.isReadable()) {
        throw StyleFileReadFailed
	    (qstrtostr(i18n("Can't open style file ") +
		       styleFileName));
    }

    QFile styleFile(styleFileName);

    QXmlInputSource source(styleFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    styleFile.close();

    if (!ok) {
	throw StyleFileReadFailed(qstrtostr(m_errorString));
    }
}

void
CustomNoteStyle::initialiseNotes()
{
    // already done in constructor, no need to do anything here
}

void
CustomNoteStyle::setBaseStyle(NoteStyleName name)
{
    m_baseStyle = NoteStyleFactory::getStyle(name);
    if (m_baseStyle == this) m_baseStyle = 0;
}

void
CustomNoteStyle::setShape(Note::Type note, NoteHeadShape shape)
{
    m_notes[note].shape = shape;
}

void
CustomNoteStyle::setFilled(Note::Type note, bool filled)
{
    m_notes[note].black = filled;
}

void
CustomNoteStyle::setStem(Note::Type note, bool stem)
{
    m_notes[note].stem = stem;
}

void
CustomNoteStyle::setFlagCount(Note::Type note, int flags)
{
    m_notes[note].flags = flags;
}

bool
CustomNoteStyle::startElement(const QString &, const QString &,
			      const QString &qName,
			      const QXmlAttributes &attributes)
{
    QString lcName = qName.lower();

    if (lcName == "rosegarden-note-style") {

	QString s;
	
	s = attributes.value("base-style");
	if (s) setBaseStyle(qstrtostr(s));
	else {
	    m_errorString = i18n("base-style is a required attribute of rosegarden-note-style");
	    return false;
	}

    } else if (lcName == "note") {

	QString s;

	s = attributes.value("type");
	if (s) {

	    Note note(qstrtostr(s)); //!!! can throw
	    NoteDescription desc;

	    s = attributes.value("shape");
	    if (s) {
		//!!! how to handle this? map? store names or numbers?
	    }

	    s = attributes.value("filled");
	    if (s) desc.black = (s.lower() == "true");

	    s = attributes.value("stem");
	    if (s) desc.stem = (s.lower() == "true");

	    s = attributes.value("flags");
	    if (s) desc.flags = (s.lower() == "true");
	    
	    m_notes[note.getNoteType()] = desc;

	} else {

	    m_errorString = i18n("type is a required attribute of note");
	    return false;
	}
    }

    return true;
}

