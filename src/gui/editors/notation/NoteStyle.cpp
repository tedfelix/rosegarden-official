/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "NoteStyle.h"

#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "NoteCharacterNames.h"
#include "NoteStyleFactory.h"
#include "misc/Strings.h"
#include "misc/Debug.h"


namespace Rosegarden
{

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
        if (m_baseStyle)
            return m_baseStyle->getShape(type);
        RG_WARNING
        << "WARNING: NoteStyle::getShape: No shape defined for note type "
        << type << ", defaulting to AngledOval";
        return AngledOval;
    }

    return i->second.shape;
}

bool
NoteStyle::isFilled(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) {
        if (m_baseStyle)
            return m_baseStyle->isFilled(type);
        RG_WARNING
        << "WARNING: NoteStyle::isFilled: No definition for note type "
        << type << ", defaulting to true";
        return true;
    }

    return i->second.filled;
}

bool
NoteStyle::hasStem(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) {
        if (m_baseStyle)
            return m_baseStyle->hasStem(type);
        RG_WARNING
        << "WARNING: NoteStyle::hasStem: No definition for note type "
        << type << ", defaulting to true";
        return true;
    }

    return i->second.stem;
}

int
NoteStyle::getFlagCount(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) {
        if (m_baseStyle)
            return m_baseStyle->getFlagCount(type);
        RG_WARNING
        << "WARNING: NoteStyle::getFlagCount: No definition for note type "
        << type << ", defaulting to 0";
        return 0;
    }

    return i->second.flags;
}

int
NoteStyle::getSlashCount(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) {
        if (m_baseStyle)
            return m_baseStyle->getSlashCount(type);
        RG_WARNING
        << "WARNING: NoteStyle::getSlashCount: No definition for note type "
        << type << ", defaulting to 0";
        return 0;
    }

    return i->second.slashes;
}

void
NoteStyle::getStemFixPoints(Note::Type type,
                            HFixPoint &hfix, VFixPoint &vfix)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) {
        if (m_baseStyle) {
            m_baseStyle->getStemFixPoints(type, hfix, vfix);
            return ;
        }
        RG_WARNING
        << "WARNING: NoteStyle::getStemFixPoints: "
        << "No definition for note type " << type
        << ", defaulting to (Normal,Middle)";
        hfix = Normal;
        vfix = Middle;
        return ;
    }

    hfix = i->second.hfix;
    vfix = i->second.vfix;
}

NoteStyle::CharNameRec

NoteStyle::getNoteHeadCharName(Note::Type type)
{
    NoteDescriptionMap::iterator i = m_notes.find(type);
    if (i == m_notes.end()) {
        if (m_baseStyle)
            return m_baseStyle->getNoteHeadCharName(type);
        RG_WARNING
        << "WARNING: NoteStyle::getNoteHeadCharName: No definition for note type "
        << type << ", defaulting to NOTEHEAD_BLACK";
        return CharNameRec(NoteCharacterNames::NOTEHEAD_BLACK, false);
    }

    const NoteDescription &desc(i->second);
    CharName name = NoteCharacterNames::UNKNOWN;
    bool inverted = false;

    if (desc.shape == AngledOval) {

        name = desc.filled ? NoteCharacterNames::NOTEHEAD_BLACK
               : NoteCharacterNames::VOID_NOTEHEAD;

    } else if (desc.shape == LevelOval) {

        if (desc.filled) {
            RG_WARNING << "WARNING: NoteStyle::getNoteHeadCharName: No filled level oval head";
        }
        name = NoteCharacterNames::WHOLE_NOTE;

    } else if (desc.shape == Breve) {

        if (desc.filled) {
            RG_WARNING << "WARNING: NoteStyle::getNoteHeadCharName: No filled breve head";
        }
        name = NoteCharacterNames::BREVE;

    } else if (desc.shape == Cross) {

        name = desc.filled ? NoteCharacterNames::X_NOTEHEAD
               : NoteCharacterNames::CIRCLE_X_NOTEHEAD;

    } else if (desc.shape == TriangleUp) {

        name = desc.filled ? NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_BLACK
               : NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_WHITE;

    } else if (desc.shape == TriangleDown) {

        name = desc.filled ? NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_BLACK
               : NoteCharacterNames::TRIANGLE_NOTEHEAD_UP_WHITE;
        inverted = true;

    } else if (desc.shape == Diamond) {

        name = desc.filled ? NoteCharacterNames::SEMIBREVIS_BLACK
               : NoteCharacterNames::SEMIBREVIS_WHITE;

    } else if (desc.shape == Rectangle) {

        name = desc.filled ? NoteCharacterNames::SQUARE_NOTEHEAD_BLACK
               : NoteCharacterNames::SQUARE_NOTEHEAD_WHITE;

    } else if (desc.shape == Number) {

        RG_WARNING << "WARNING: NoteStyle::getNoteHeadCharName: Number not yet implemented";
        name = NoteCharacterNames::UNKNOWN; //!!!

    } else if (desc.shape == CustomCharName) {

        name = desc.charName;

    } else {

        name = NoteCharacterNames::UNKNOWN;
    }

    return CharNameRec(name, inverted);
}

CharName
NoteStyle::getAccidentalCharName(const Accidental &a)
{
    if (a == Accidentals::Sharp)
        return NoteCharacterNames::SHARP;
    else if (a == Accidentals::Flat)
        return NoteCharacterNames::FLAT;
    else if (a == Accidentals::Natural)
        return NoteCharacterNames::NATURAL;
    else if (a == Accidentals::DoubleSharp)
        return NoteCharacterNames::DOUBLE_SHARP;
    else if (a == Accidentals::DoubleFlat)
        return NoteCharacterNames::DOUBLE_FLAT;
    return NoteCharacterNames::UNKNOWN;
}

CharName
NoteStyle::getMarkCharName(const Mark &mark)
{
    if (mark == Marks::Accent)
        return NoteCharacterNames::ACCENT;
    else if (mark == Marks::Tenuto)
        return NoteCharacterNames::TENUTO;
    else if (mark == Marks::Staccato)
        return NoteCharacterNames::STACCATO;
    else if (mark == Marks::Staccatissimo)
        return NoteCharacterNames::STACCATISSIMO;
    else if (mark == Marks::Marcato)
        return NoteCharacterNames::MARCATO;
    else if (mark == Marks::Open)
        return NoteCharacterNames::OPEN;
    else if (mark == Marks::Stopped)
        return NoteCharacterNames::STOPPED;
    else if (mark == Marks::Trill)
        return NoteCharacterNames::TRILL;
    else if (mark == Marks::LongTrill)
        return NoteCharacterNames::TRILL;
    else if (mark == Marks::TrillLine)
        return NoteCharacterNames::TRILL_LINE;
    else if (mark == Marks::Turn)
        return NoteCharacterNames::TURN;
    else if (mark == Marks::Pause)
        return NoteCharacterNames::FERMATA;
    else if (mark == Marks::UpBow)
        return NoteCharacterNames::UP_BOW;
    else if (mark == Marks::DownBow)
        return NoteCharacterNames::DOWN_BOW;
    else if (mark == Marks::Harmonic)
        return NoteCharacterNames::HARMONIC;
    else if (mark == Marks::Mordent)
        return NoteCharacterNames::MORDENT;
    else if (mark == Marks::MordentInverted)
        return NoteCharacterNames::MORDENT_INVERTED;
    else if (mark == Marks::MordentLong)
        return NoteCharacterNames::MORDENT_LONG;
    else if (mark == Marks::MordentLongInverted)
        return NoteCharacterNames::MORDENT_LONG_INVERTED;
    // Things like "sf" and "rf" are generated from text fonts
    return NoteCharacterNames::UNKNOWN;
}

CharName
NoteStyle::getClefCharName(const Clef &clef)
{
    std::string clefType(clef.getClefType());

    if (clefType == Clef::Bass || clefType == Clef::Varbaritone || clefType == Clef::Subbass) {
        return NoteCharacterNames::F_CLEF;
    } else if (clefType == Clef::Treble || clefType == Clef::French) {
        return NoteCharacterNames::G_CLEF;
    } else if (clefType == Clef::TwoBar) {
        return NoteCharacterNames::TWO_BAR_CLEF;
    } else {
        return NoteCharacterNames::C_CLEF;
    }
}

CharName
NoteStyle::getSymbolCharName(const Symbol &symbol)
{
    std::string symbolType(symbol.getSymbolType());

    if (symbolType == Symbol::Segno)
        return NoteCharacterNames::SEGNO;
    else if (symbolType == Symbol::Coda)
        return NoteCharacterNames::CODA;
    else if (symbolType == Symbol::Breath)
        return NoteCharacterNames::BREATH_MARK;
    else
        return NoteCharacterNames::UNKNOWN;
}

CharName
NoteStyle::getRestCharName(Note::Type type, bool restOutsideStave)
{
    switch (type) {
    case Note::Hemidemisemiquaver:
        return NoteCharacterNames::SIXTY_FOURTH_REST;
    case Note::Demisemiquaver:
        return NoteCharacterNames::THIRTY_SECOND_REST;
    case Note::Semiquaver:
        return NoteCharacterNames::SIXTEENTH_REST;
    case Note::Quaver:
        return NoteCharacterNames::EIGHTH_REST;
    case Note::Crotchet:
        return NoteCharacterNames::QUARTER_REST;
    case Note::Minim:
        return restOutsideStave ?
               NoteCharacterNames::HALF_REST
               : NoteCharacterNames::HALF_REST_ON_STAFF;
    case Note::Semibreve:
        return restOutsideStave ?
               NoteCharacterNames::WHOLE_REST
               : NoteCharacterNames::WHOLE_REST_ON_STAFF;
    case Note::Breve:
        return restOutsideStave ?
               NoteCharacterNames::MULTI_REST
               : NoteCharacterNames::MULTI_REST_ON_STAFF;
    default:
        return NoteCharacterNames::UNKNOWN;
    }
}

CharName
NoteStyle::getPartialFlagCharName(bool final)
{
    if (final)
        return NoteCharacterNames::FLAG_PARTIAL_FINAL;
    else
        return NoteCharacterNames::FLAG_PARTIAL;
}

CharName
NoteStyle::getFlagCharName(int flagCount)
{
    switch (flagCount) {
    case 1:
        return NoteCharacterNames::FLAG_1;
    case 2:
        return NoteCharacterNames::FLAG_2;
    case 3:
        return NoteCharacterNames::FLAG_3;
    case 4:
        return NoteCharacterNames::FLAG_4;
    default:
        return NoteCharacterNames::UNKNOWN;
    }
}

CharName
NoteStyle::getTimeSignatureDigitName(int digit)
{
    switch (digit) {
    case 0:
        return NoteCharacterNames::DIGIT_ZERO;
    case 1:
        return NoteCharacterNames::DIGIT_ONE;
    case 2:
        return NoteCharacterNames::DIGIT_TWO;
    case 3:
        return NoteCharacterNames::DIGIT_THREE;
    case 4:
        return NoteCharacterNames::DIGIT_FOUR;
    case 5:
        return NoteCharacterNames::DIGIT_FIVE;
    case 6:
        return NoteCharacterNames::DIGIT_SIX;
    case 7:
        return NoteCharacterNames::DIGIT_SEVEN;
    case 8:
        return NoteCharacterNames::DIGIT_EIGHT;
    case 9:
        return NoteCharacterNames::DIGIT_NINE;
    default:
        return NoteCharacterNames::UNKNOWN;
    }
}

CharName
NoteStyle::getSomeCharName(QString qthing)
{
    CharName name;
    std::string thing = qstrtostr(qthing);

    try {
        name = getAccidentalCharName(Accidental(thing));
        if (!(name == NoteCharacterNames::UNKNOWN)) return name;
    } catch (const Exception &) { }

    try {
        name = getMarkCharName(Mark(thing));
        RG_DEBUG << thing << " -> " << name;
        if (!(name == NoteCharacterNames::UNKNOWN)) return name;
    } catch (const Exception &) { }

    try {
        name = getClefCharName(Clef(thing));
        if (!(name == NoteCharacterNames::UNKNOWN)) return name;
    } catch (const Exception &) { }

    return NoteCharacterNames::UNKNOWN;
}

void
NoteStyle::setBaseStyle(NoteStyleName name)
{
    try {
        m_baseStyle = NoteStyleFactory::getStyle(name);
        if (m_baseStyle == this)
            m_baseStyle.reset();
    } catch (const NoteStyleFactory::StyleUnavailable &u) {
        if (name != NoteStyleFactory::DefaultStyle) {
            RG_WARNING
                << "NoteStyle::setBaseStyle: Base style "
                << name << " not available, defaulting to "
                << NoteStyleFactory::DefaultStyle;
            setBaseStyle(NoteStyleFactory::DefaultStyle);
        } else {
            RG_WARNING
                << "NoteStyle::setBaseStyle: Base style "
                << name << " not available";
            m_baseStyle.reset();
        }
    }
}

void
NoteStyle::checkDescription(Note::Type note)
{
    if (m_baseStyle && (m_notes.find(note) == m_notes.end())) {
        m_baseStyle->checkDescription(note);
        m_notes[note] = m_baseStyle->m_notes[note];
    }
}

void
NoteStyle::setShape(Note::Type note, NoteHeadShape shape)
{
    checkDescription(note);
    m_notes[note].shape = shape;
}

void
NoteStyle::setCharName(Note::Type note, CharName charName)
{
    checkDescription(note);
    m_notes[note].charName = charName;
}

void
NoteStyle::setFilled(Note::Type note, bool filled)
{
    checkDescription(note);
    m_notes[note].filled = filled;
}

void
NoteStyle::setStem(Note::Type note, bool stem)
{
    checkDescription(note);
    m_notes[note].stem = stem;
}

void
NoteStyle::setFlagCount(Note::Type note, int flags)
{
    checkDescription(note);
    m_notes[note].flags = flags;
}

void
NoteStyle::setSlashCount(Note::Type note, int slashes)
{
    checkDescription(note);
    m_notes[note].slashes = slashes;
}

void
NoteStyle::setStemFixPoints(Note::Type note, HFixPoint hfix, VFixPoint vfix)
{
    checkDescription(note);
    m_notes[note].hfix = hfix;
    m_notes[note].vfix = vfix;
}

}
