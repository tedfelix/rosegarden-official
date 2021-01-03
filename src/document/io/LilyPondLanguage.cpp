/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file is Copyright 2011 Daren Beattie <dtbeattie@gmail.com>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "document/io/LilyPondLanguage.h"

namespace Rosegarden
{

LilyPondLanguage *
LilyPondLanguage::create(unsigned int language) {
    switch (language) {
    case ARABIC:
        return new LilyPondArabic();
        break;

    case CATALAN:
        return new LilyPondCatalan();
        break;

    case DEUTSCH:
        return new LilyPondDeutsch();
        break;

    case ENGLISH:
        return new LilyPondEnglish();
        break;

    case ESPANOL:
        return new LilyPondEspanol();
        break;

    case ITALIANO:
        return new LilyPondItaliano();
        break;

    case NORSK:
        return new LilyPondNorsk();
        break;

    case PORTUGUES:
        return new LilyPondPortugues();
        break;

    case SUOMI:
        return new LilyPondSuomi();
        break;

    case SVENSKA:
        return new LilyPondSvenska();
        break;

    case VLAAMS:
        return new LilyPondVlaams();
        break;

    default:
        return new LilyPondNederlands();
        break;
    }
}

const std::string
LilyPondLanguage::getLilyNote(
        const char noteName, const Accidental accidental
        ) const {
    std::string lilyNoteName = getLilyNoteName(noteName);
    std::string lilyNote = applyAccidental(lilyNoteName, accidental);
    return lilyNote;
}

const std::string
LilyPondSolfege::getLilyNoteName(const char noteName) const {
    std::string lilyNoteName = "";
    switch (noteName) {
    case 'c':
        lilyNoteName = "do";
        break;

    case 'd':
        lilyNoteName = "re";
        break;

    case 'e':
        lilyNoteName = "mi";
        break;

    case 'f':
        lilyNoteName = "fa";
        break;

    case 'g':
        lilyNoteName = "sol";
        break;

    case 'a':
        lilyNoteName = "la";
        break;

    case 'b':
        lilyNoteName = "si";
        break;
    }
    return lilyNoteName;
}

//
// Arabic
//
const std::string
LilyPondArabic::getImportStatement() const {
    return "\\include \"arabic.ly\"\n";
}

const std::string
LilyPondArabic::applyAccidental(
        const std::string lilyNoteName, const Accidental accidental
        ) const {
    std::string lilyNote = lilyNoteName;

    if (accidental == Accidentals::Sharp)
            lilyNote += "d";
    else if (accidental == Accidentals::DoubleSharp)
            lilyNote += "dd";
    else if (accidental == Accidentals::Flat)
            lilyNote += "b";
    else if (accidental == Accidentals::DoubleFlat)
            lilyNote += "bb";

    return lilyNote;
}

//
// Italiano
//
const std::string
LilyPondItaliano::getImportStatement() const {
    return "\\include \"italiano.ly\"\n";
}

//
// Vlaams
//
const std::string
LilyPondVlaams::getImportStatement() const {
    return "\\include \"vlaams.ly\"\n";
}

const std::string
LilyPondVlaams::applyAccidental(
        const std::string lilyNoteName, const Accidental accidental
        ) const {
    std::string lilyNote = lilyNoteName;

    if (accidental == Accidentals::Sharp)
        lilyNote += "k";
    else if (accidental == Accidentals::DoubleSharp)
        lilyNote += "kk";
    else if (accidental == Accidentals::Flat)
        lilyNote += "b";
    else if (accidental == Accidentals::DoubleFlat)
        lilyNote += "bb";

    return lilyNote;
}

//
// Espanol
//
const std::string
LilyPondEspanol::getImportStatement() const {
    return "\\include \"espanol.ly\"\n";
}

const std::string
LilyPondEspanol::applyAccidental(
        const std::string lilyNoteName, const Accidental accidental
        ) const {
    std::string lilyNote = lilyNoteName;

    if (accidental == Accidentals::Sharp)
        lilyNote += "s";
    else if (accidental == Accidentals::DoubleSharp)
        lilyNote += "ss";
    else if (accidental == Accidentals::Flat)
        lilyNote += "b";
    else if (accidental == Accidentals::DoubleFlat)
        lilyNote += "bb";

    return lilyNote;
}

//
// Catalan
//
const std::string
LilyPondCatalan::getImportStatement() const {
    return "\\include \"catalan.ly\"\n";
}

//
// Portugues
//
const std::string
LilyPondPortugues::getImportStatement() const {
    return "\\include \"portugues.ly\"\n";
}

//
// Deutsch
//
const std::string
LilyPondDeutsch::getImportStatement() const {
    return "\\include \"deutsch.ly\"\n";
}

const std::string
LilyPondDeutsch::getLilyNoteName(const char noteName) const {
    std::string lilyNoteName = "";
    if (noteName == 'b') {
        lilyNoteName = "h";
    }
    else {
        lilyNoteName += noteName;
    }
    return lilyNoteName;
}

const std::string
LilyPondDeutsch::applyAccidental(
        const std::string lilyNoteName, const Accidental accidental
        ) const {
    std::string lilyNote = lilyNoteName;

    if (accidental == Accidentals::Sharp) {
        lilyNote += "is";
    }
    else if (accidental == Accidentals::DoubleSharp) {
        lilyNote += "isis";
    }
    else if (accidental == Accidentals::Flat) {
        if (lilyNoteName == "h") {
            lilyNote = "b";
        }
        else if (lilyNoteName == "a" || lilyNoteName == "e") {
            lilyNote += "s";
        }
        else {
            lilyNote += "es";
        }
    }
    else if (accidental == Accidentals::DoubleFlat) {
        if (lilyNoteName == "h") {
            lilyNote = "bes";
        }
        else if (lilyNoteName == "a" || lilyNoteName == "e") {
            lilyNote += "ses";
        }
        else {
            lilyNote += "eses";
        }
    }

    return lilyNote;
}

//
// Nederlands
//
const std::string
LilyPondNederlands::getImportStatement() const {
    return "\\include \"nederlands.ly\"\n";
}

const std::string
LilyPondNederlands::getLilyNoteName(const char noteName) const {
    return std::string(1, noteName);
}

const std::string
LilyPondNederlands::applyAccidental(
        const std::string lilyNoteName, const Accidental accidental
        ) const {
    std::string lilyNote = lilyNoteName;

    if (accidental == Accidentals::Sharp)
        lilyNote += "is";
    else if (accidental == Accidentals::DoubleSharp)
        lilyNote += "isis";
    else if (accidental == Accidentals::Flat)
        lilyNote += "es";
    else if (accidental == Accidentals::DoubleFlat)
        lilyNote += "eses";

    return lilyNote;
}

//
// Norsk
//
const std::string
LilyPondNorsk::getImportStatement() const {
    return "\\include \"norsk.ly\"\n";
}

//
// Suomi
//
const std::string
LilyPondSuomi::getImportStatement() const {
    return "\\include \"suomi.ly\"\n";
}

//
// Svenska
//
const std::string
LilyPondSvenska::getImportStatement() const {
    return "\\include \"svenska.ly\"\n";
}

const std::string
LilyPondSvenska::applyAccidental(
        const std::string lilyNoteName, const Accidental accidental
        ) const {
    std::string lilyNote = lilyNoteName;

    if (accidental == Accidentals::Sharp) {
        if (lilyNoteName == "a" || lilyNoteName == "e") {
            lilyNote += "ss";
        }
        else {
            lilyNote += "iss";
        }
    }
    else if (accidental == Accidentals::DoubleSharp) {
        if (lilyNoteName == "a" || lilyNoteName == "e") {
            lilyNote += "ssiss";
        }
        else {
            lilyNote += "ississ";
        }
    }
    else if (accidental == Accidentals::Flat) {
        if (lilyNoteName == "a" || lilyNoteName == "e") {
            lilyNote += "ss";
        }
        else if (lilyNoteName != "h") {
            lilyNote += "ess";
        }
    }
    else if (accidental == Accidentals::DoubleFlat) {
        if (lilyNoteName == "h") {
            lilyNote += "ess";
        }
        else if (lilyNoteName == "a" || lilyNoteName == "e") {
            lilyNote += "ssess";
        }
        else {
            lilyNote += "essess";
        }
    }

    return lilyNote;
}

//
// English
//
const std::string
LilyPondEnglish::getImportStatement() const {
    return "\\include \"english.ly\"\n";
}

const std::string
LilyPondEnglish::getLilyNoteName(const char noteName) const {
    return std::string(1, noteName);
}

const std::string
LilyPondEnglish::applyAccidental(
        const std::string lilyNoteName, const Accidental accidental
        ) const {
    std::string lilyNote = lilyNoteName;

    if (accidental == Accidentals::Sharp)
        lilyNote += "s";
    else if (accidental == Accidentals::DoubleSharp)
        lilyNote += "ss";
    else if (accidental == Accidentals::Flat)
        lilyNote += "f";
    else if (accidental == Accidentals::DoubleFlat)
        lilyNote += "ff";

    return lilyNote;
}

}
