/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    This file is Copyright 2011 Daren Beattie <dtbeattie@gmail.com>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef LILYPONDLANGUAGE_H_
#define LILYPONDLANGUAGE_H_

#include "base/NotationTypes.h"

#include <string>

namespace Rosegarden
{

/**
 * Abstract base class for all supported LilyPond note name languages
 */
class LilyPondLanguage
{
public:
    static const unsigned int ARABIC = 0;
    static const unsigned int CATALAN = 1;
    static const unsigned int DEUTSCH = 2;
    static const unsigned int ENGLISH = 3;
    static const unsigned int ESPANOL = 4;
    static const unsigned int ITALIANO = 5;
    static const unsigned int NEDERLANDS = 6;
    static const unsigned int NORSK = 7;
    static const unsigned int PORTUGUES = 8;
    static const unsigned int SUOMI = 9;
    static const unsigned int SVENSKA = 10;
    static const unsigned int VLAAMS = 11;

    /**
     * Returns a concrete class for the given language
     */
    static LilyPondLanguage *create(const unsigned int language);
    /**
     * Returns the "\include" directive for the chosen language
     * (e.g., \include "nederlands.ly")
     */
    virtual const std::string getImportStatement() const = 0;
    /**
     * Returns the LilyPond note (name and accidental)
     * based on the chosen language
     */
    virtual const std::string getLilyNote(
            const char noteName, const Accidental accidental
            ) const;
    virtual ~LilyPondLanguage() {}
protected:
    virtual const std::string getLilyNoteName(const char noteName) const = 0;
    virtual const std::string applyAccidental(
            const std::string lilyNoteName, const Accidental accidental
            ) const = 0;
};

/**
 * Abstract base class for languages that use Solfege note names
 */
class LilyPondSolfege : public LilyPondLanguage
{
public:
    virtual ~LilyPondSolfege() {}
protected:
    virtual const std::string getLilyNoteName(const char noteName) const;
};

/**
 * Solfege scale with Arabic sharps/flats (d/b)
 */
class LilyPondArabic : public LilyPondSolfege
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondArabic() {}
protected:
    virtual const std::string applyAccidental(
            const std::string lilyNoteName, const Accidental accidental
            ) const;
};

/**
 * Solfege scale with Italian sharps/flats (d/b)
 */
class LilyPondItaliano : public LilyPondArabic
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondItaliano() {}
};

/**
 * Solfege scale with Vlaams sharps/flats (k/b)
 */
class LilyPondVlaams : public LilyPondSolfege
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondVlaams() {}
protected:
    virtual const std::string applyAccidental(
            const std::string lilyNoteName, const Accidental accidental
            ) const;
};

/**
 * Solfege scale with Spanish sharps/flats (s/b)
 */
class LilyPondEspanol : public LilyPondSolfege
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondEspanol() {}
protected:
    virtual const std::string applyAccidental(
            const std::string lilyNoteName, const Accidental accidental
            ) const;
};

/**
 * Solfege scale with Catalan sharps/flats (s/b)
 */
class LilyPondCatalan : public LilyPondEspanol
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondCatalan() {}
};

/**
 * Solfege scale with Portuguese sharps/flats (s/b)
 */
class LilyPondPortugues : public LilyPondEspanol
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondPortugues() {}
};

/**
 * Alphabetic scale with Germanic sharps/flats (is/es)
 */
class LilyPondDeutsch : public LilyPondLanguage
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondDeutsch() {}
protected:
    virtual const std::string getLilyNoteName(const char noteName) const;
    virtual const std::string applyAccidental(
            const std::string lilyNoteName, const Accidental accidental
            ) const;
};

/**
 * Alphabetic scale with Germanic sharps/flats (is/es)
 */
class LilyPondNederlands : public LilyPondDeutsch
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondNederlands() {}
protected:
    virtual const std::string getLilyNoteName(const char noteName) const;
    virtual const std::string applyAccidental(
            const std::string lilyNoteName, const Accidental accidental
            ) const;
};

/**
 * Alphabetic scale with Germanic sharps/flats (is/es)
 */
class LilyPondNorsk : public LilyPondDeutsch
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondNorsk() {}
};

/**
 * Alphabetic scale with Germanic sharps/flats (is/es)
 */
class LilyPondSuomi : public LilyPondDeutsch
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondSuomi() {}
};

/**
 * Alphabetic scale with Svenska sharps/flats (iss/ess)
 */
class LilyPondSvenska : public LilyPondDeutsch
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondSvenska() {}
protected:
    virtual const std::string applyAccidental(
            const std::string lilyNoteName, const Accidental accidental
            ) const;
};

/**
 * Alphabetic scale with English sharps/flats (s/f)
 */
class LilyPondEnglish : public LilyPondLanguage
{
public:
    virtual const std::string getImportStatement() const;
    virtual ~LilyPondEnglish() {}
protected:
    virtual const std::string getLilyNoteName(const char noteName) const;
    virtual const std::string applyAccidental(
            const std::string lilyNoteName, const Accidental accidental
            ) const;
};

}

#endif /* LILYPONDLANGUAGE_H_ */
