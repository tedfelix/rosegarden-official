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
#include "notationproperties.h"


using Rosegarden::Accidental;
using Rosegarden::Accidentals;
using Rosegarden::Mark;
using Rosegarden::Marks;
using Rosegarden::Clef;
using Rosegarden::Note;
using Rosegarden::String;


class NoteStyleFileReader : public QXmlDefaultHandler
{
public:
    NoteStyleFileReader(NoteStyleName name);

    struct StyleFileReadFailed {
        StyleFileReadFailed(std::string r) : reason(r) { }
        std::string reason;
    };
    
    NoteStyle *getStyle() { return m_style; }

    // Xml handler methods:

    virtual bool startElement
    (const QString& namespaceURI, const QString& localName,
     const QString& qName, const QXmlAttributes& atts);
    
private:
    bool setFromAttributes(Note::Type type, const QXmlAttributes &attributes);

    QString m_errorString;
    NoteStyle *m_style;
    bool m_haveNote;
};


namespace StandardNoteStyleNames
{
    const NoteStyleName Classical = "Classical";
    const NoteStyleName Cross = "Cross";
    const NoteStyleName Triangle = "Triangle";
    const NoteStyleName Mensural = "Mensural";

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

	try {
	    NoteStyle *newStyle = NoteStyleFileReader(name).getStyle();
	    m_styles[name] = newStyle;
	    return newStyle;

	} catch (NoteStyleFileReader::StyleFileReadFailed f) {
	    kdDebug(KDEBUG_AREA)
		<< "NoteStyleFactory::getStyle: Style file read failed: "
		<< f.reason << endl;
	    throw StyleUnavailable("Style file read failed: " + f.reason);
	}

    } else {
	return i->second;
    }
}

NoteStyle *
NoteStyleFactory::getStyleForEvent(Rosegarden::Event *event)
{
    NoteStyleName styleName;
    if (event->get<String>(NotationProperties::NOTE_STYLE, styleName)) {
	return getStyle(styleName);
    } else {
	return getStyle(StandardNoteStyleNames::Classical);
    }
}

NoteStyleFactory::StyleMap NoteStyleFactory::m_styles;



NoteStyle::~NoteStyle()
{
    // nothing
}

const NoteStyle::NoteHeadShape NoteStyle::AngledOval = "angled oval";
const NoteStyle::NoteHeadShape NoteStyle::LevelOval = "level oval";
const NoteStyle::NoteHeadShape NoteStyle::Breve = "breve";
const NoteStyle::NoteHeadShape NoteStyle::Cross = "cross";
const NoteStyle::NoteHeadShape NoteStyle::TriangleUp = "triangle up";
const NoteStyle::NoteHeadShape NoteStyle::TriangleDown = "triangle down";
const NoteStyle::NoteHeadShape NoteStyle::Diamond = "diamond";
const NoteStyle::NoteHeadShape NoteStyle::Rectangle = "rectangle";
const NoteStyle::NoteHeadShape NoteStyle::Number = "number";
const NoteStyle::NoteHeadShape NoteStyle::CustomCharName = "custom character";


NoteStyle::NoteHeadShape
NoteStyle::getShape(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getShape(type);
	kdDebug(KDEBUG_AREA)
	    << "WARNING: NoteStyle::getShape: No shape defined for note type "
	    << type << ", defaulting to AngledOval" << endl;
	return AngledOval;
    }

    return i->second.shape;
}


bool
NoteStyle::isFilled(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->isFilled(type);
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::isFilled: No definition for note type "
	    << type << ", defaulting to true" << endl;
	return true;
    }

    return i->second.filled;
}


bool
NoteStyle::hasStem(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->hasStem(type);
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::hasStem: No definition for note type "
	    << type << ", defaulting to true" << endl;
	return true;
    }

    return i->second.stem;
}


int
NoteStyle::getFlagCount(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getFlagCount(type);
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::getFlagCount: No definition for note type "
	    << type << ", defaulting to 0" << endl;
	return 0;
    }

    return i->second.flags;
}


void
NoteStyle::getStemFixPoints(Note::Type type,
			    HFixPoint &hfix, VFixPoint &vfix)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) {
	    m_baseStyle->getStemFixPoints(type, hfix, vfix);
	    return;
	}
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::getStemFixPoints: "
	    << "No definition for note type " << type
	    << ", defaulting to (Normal,Middle)" << endl;
	hfix = Normal;
	vfix = Middle;
	return;
    }

    hfix = i->second.hfix;
    vfix = i->second.vfix;
}
 


CharName
NoteStyle::getNoteHeadCharName(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) { 
	if (m_baseStyle) return m_baseStyle->getNoteHeadCharName(type);
	kdDebug(KDEBUG_AREA) 
	    << "WARNING: NoteStyle::getNoteHeadCharName: No definition for note type "
	    << type << ", defaulting to NOTEHEAD_BLACK" << endl;
	return NoteCharacterNames::NOTEHEAD_BLACK;
    }

    const NoteDescription &desc(i->second);

    if (desc.shape == AngledOval) {

	return desc.filled ? NoteCharacterNames::NOTEHEAD_BLACK
	                   : NoteCharacterNames::VOID_NOTEHEAD;

    } else if (desc.shape == LevelOval) {

	return desc.filled ? NoteCharacterNames::UNKNOWN //!!!
	                   : NoteCharacterNames::WHOLE_NOTE;
	
    } else if (desc.shape == Breve) {

	return desc.filled ? NoteCharacterNames::UNKNOWN //!!!
	                   : NoteCharacterNames::BREVE;
	
    } else if (desc.shape == Cross) {

	return desc.filled ? NoteCharacterNames::X_NOTEHEAD
	                   : NoteCharacterNames::CIRCLE_X_NOTEHEAD;

    } else if (desc.shape == TriangleUp) {

	return desc.filled ? NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_BLACK
                           : NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_WHITE;

    } else if (desc.shape == TriangleDown) {

	kdDebug(KDEBUG_AREA) << "WARNING: NoteStyle::getNoteHeadCharName: TriangleDown not yet implemented" << endl;
	return NoteCharacterNames::UNKNOWN; //!!!

    } else if (desc.shape == Diamond) {

	return desc.filled ? NoteCharacterNames::SEMIBREVIS_BLACK
 	                   : NoteCharacterNames::SEMIBREVIS_WHITE;

    } else if (desc.shape == Rectangle) {

	kdDebug(KDEBUG_AREA) << "WARNING: NoteStyle::getNoteHeadCharName: Rectangle not yet implemented" << endl;
	return NoteCharacterNames::UNKNOWN; //!!!
	
    } else if (desc.shape == Number) {

	kdDebug(KDEBUG_AREA) << "WARNING: NoteStyle::getNoteHeadCharName: Number not yet implemented" << endl;
	return NoteCharacterNames::UNKNOWN; //!!!

    } else if (desc.shape == CustomCharName) {
	
	return desc.charName;

    } else {

	return NoteCharacterNames::UNKNOWN;
    }
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
    std::string clefType(clef.getClefType());

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
NoteStyle::setBaseStyle(NoteStyleName name)
{
    try {
	m_baseStyle = NoteStyleFactory::getStyle(name);
	if (m_baseStyle == this) m_baseStyle = 0;
    } catch (NoteStyleFactory::StyleUnavailable u) {
	if (name != StandardNoteStyleNames::Classical) {
	    kdDebug(KDEBUG_AREA)
		<< "NoteStyle::setBaseStyle: Base style "
		<< name << " not available, defaulting to "
		<< StandardNoteStyleNames::Classical << endl;
	    setBaseStyle(StandardNoteStyleNames::Classical);
	} else {
	    kdDebug(KDEBUG_AREA)
		<< "NoteStyle::setBaseStyle: Base style "
		<< name << " not available" << endl;
	    m_baseStyle = 0;
	}
    }	    
}

void
NoteStyle::setShape(Note::Type note, NoteHeadShape shape)
{
    m_notes[note].shape = shape;
}

void
NoteStyle::setCharName(Note::Type note, CharName charName)
{
    m_notes[note].charName = charName;
}

void
NoteStyle::setFilled(Note::Type note, bool filled)
{
    m_notes[note].filled = filled;
}

void
NoteStyle::setStem(Note::Type note, bool stem)
{
    m_notes[note].stem = stem;
}

void
NoteStyle::setFlagCount(Note::Type note, int flags)
{
    m_notes[note].flags = flags;
}

void
NoteStyle::setStemFixPoints(Note::Type note, HFixPoint hfix, VFixPoint vfix)
{
    m_notes[note].hfix = hfix;
    m_notes[note].vfix = vfix;
}    


NoteStyleFileReader::NoteStyleFileReader(std::string name) :
    m_style(new NoteStyle),
    m_haveNote(false)
{
    QString styleDirectory =
	KGlobal::dirs()->findResource("appdata", "styles/");

    QString styleFileName =
	QString("%1/%2.xml").arg(styleDirectory).arg(strtoqstr(name));

    QFileInfo fileInfo(styleFileName);

    if (!fileInfo.isReadable()) {
        throw StyleFileReadFailed
	    (qstrtostr(i18n("Can't open style file ") + styleFileName));
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

bool
NoteStyleFileReader::startElement(const QString &, const QString &,
				  const QString &qName,
				  const QXmlAttributes &attributes)
{
    QString lcName = qName.lower();

    if (lcName == "rosegarden-note-style") {

	QString s = attributes.value("base-style");
	if (s) m_style->setBaseStyle(qstrtostr(s));

    } else if (lcName == "note") {

	m_haveNote = true;
	
	QString s = attributes.value("type");
	if (!s) {
	    m_errorString = i18n("type is a required attribute of note");
	    return false;
	}
	
	try {
	    Note::Type type = Note(qstrtostr(s)).getNoteType();
	    if (!setFromAttributes(type, attributes)) return false;

	} catch (Note::MalformedNoteName n) {
	    m_errorString = i18n("Unrecognised note name " + s);
	    return false;
	}

    } else if (lcName == "global") {

	if (m_haveNote) {
	    m_errorString = i18n("global element must precede note elements");
	    return false;
	}
	    
	for (Note::Type type = Note::Shortest; type <= Note::Longest; ++type) {
	    if (!setFromAttributes(type, attributes)) return false;
	}
    }

    return true;
}


bool
NoteStyleFileReader::setFromAttributes(Note::Type type,
				       const QXmlAttributes &attributes)
{
    QString s;
    bool haveShape = false;

    s = attributes.value("shape");
    if (s) {
	m_style->setShape(type, qstrtostr(s.lower()));
	haveShape = true;
    }
    
    s = attributes.value("charname");
    if (s) {
	if (haveShape) {
	    m_errorString = i18n("global and note elements may have shape "
				 "or charname attribute, but not both");
	    return false;
	}
	m_style->setShape(type, NoteStyle::CustomCharName);
	m_style->setCharName(type, qstrtostr(s));
    }

    s = attributes.value("filled");
    if (s) m_style->setFilled(type, s.lower() == "true");
    
    s = attributes.value("stem");
    if (s) m_style->setStem(type, s.lower() == "true");
    
    s = attributes.value("flags");
    if (s) m_style->setFlagCount(type, s.toInt());

    NoteStyle::HFixPoint hfix;
    NoteStyle::VFixPoint vfix;
    m_style->getStemFixPoints(type, hfix, vfix);
    bool haveHFix = false;
    bool haveVFix = false;

    s = attributes.value("hfixpoint");
    if (s) {
	s = s.lower();
	haveHFix = true;
	if (s == "normal") hfix = NoteStyle::Normal;
	else if (s == "central") hfix = NoteStyle::Central;
	else if (s == "reversed") hfix = NoteStyle::Reversed;
	else haveHFix = false;
    }

    s = attributes.value("vfixpoint");
    if (s) {
	s = s.lower();
	haveVFix = true;
	if (s == "near") vfix = NoteStyle::Near;
	else if (s == "middle") vfix = NoteStyle::Middle;
	else if (s == "far") vfix = NoteStyle::Far;
	else haveVFix = false;
    }

    if (haveHFix || haveVFix) {
	m_style->setStemFixPoints(type, hfix, vfix);
	// otherwise they inherit from base style, so avoid setting here
    }

    return true;
}


