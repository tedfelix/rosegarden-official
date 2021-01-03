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

#ifndef RG_NOTEFONT_H
#define RG_NOTEFONT_H

#include "base/Exception.h"
#include <map>
#include "NoteCharacter.h"
#include "NoteFontMap.h"
#include <set>
#include <QString>
#include <QPoint>
#include <utility>
#include "gui/editors/notation/NoteCharacterNames.h"
#include "gui/general/PixmapFunctions.h"


class QPixmap;


namespace Rosegarden
{

class PixmapMap;
class NoteCharacterDrawRep;
class FontPixmapMap;
class DrawRepMap;

// Encapsulates NoteFontMap, and loads pixmaps etc on demand

class NoteFont
{
public:
    enum CharacterType { Screen, Printer };

    typedef Exception BadNoteFont;
    ~NoteFont();

    QString getName() const { return m_fontMap.getName(); }
    int getSize() const { return m_size; }
    bool isSmooth() const { return m_fontMap.isSmooth(); }
    const NoteFontMap &getNoteFontMap() const { return m_fontMap; }

    /// Returns false + thickness=1 if not specified
    bool getStemThickness(unsigned int &thickness) const;

    /// Returns false + a guess at suitable thickness if not specified
    bool getBeamThickness(unsigned int &thickness) const;

    /// Returns false + a guess at suitable length if not specified
    bool getStemLength(unsigned int &length) const;

    /// Returns false + a guess at suitable spacing if not specified
    bool getFlagSpacing(unsigned int &spacing) const;

    /// Returns false + thickness=1 if not specified
    bool getStaffLineThickness(unsigned int &thickness) const;

    /// Returns false + thickness=1 if not specified
    bool getLegerLineThickness(unsigned int &thickness) const;

    /// Returns false if not available
    bool getCharacter(CharName charName,
                      NoteCharacter &character,
                      CharacterType type = Screen,
                      bool inverted = false);

    /// Returns an empty character if not available
    NoteCharacter getCharacter(CharName charName,
                               CharacterType type = Screen,
                               bool inverted = false);

    /** Returns false if not available, otherwise returns a coloured note
       character using either a specified hue (H) and minimum value (V), or
       using full H, S, V if the optional final saturation (S) parameter is
       specified.  The final parameter is out of place with the HSV scheme in
       order to make it an optional parameter that doesn't require rewriting old
       code that expected the oldest scheme.  The new full HSV scheme is
       required to produce the full spectrum of colours on demand.
     */
    bool getCharacterColoured(CharName charName,
                              int hue, int minimum,
                              NoteCharacter &character,
                              CharacterType type = Screen,
                              bool inverted = false,
                              int saturation = PixmapFunctions::SaturationNotSpecified);

    /// Returns an empty character if not available
    NoteCharacter getCharacterColoured(CharName charName,
                                       int hue, int minimum,
                                       CharacterType type = Screen,
                                       bool inverted = false);

    /// Returns an empty character if not available
    NoteCharacter getCharacterShaded(CharName charName,
                                     CharacterType type = Screen,
                                     bool inverted = false);

    /// Returns false + dimensions of blank pixmap if none found
    bool getDimensions(CharName charName, int &x, int &y,
                       bool inverted = false) const;

    /// Ignores problems, returning dimension of blank pixmap if necessary
    int getWidth(CharName charName) const;

    /// Ignores problems, returning dimension of blank pixmap if necessary
    int getHeight(CharName charName) const;

    /// Returns false + centre-left of pixmap if no hotspot specified
    bool getHotspot(CharName charName, int &x, int &y,
                    bool inverted = false) const;

    /// Ignores problems, returns centre-left of pixmap if necessary
    QPoint getHotspot(CharName charName, bool inverted = false) const;

private:
    /// Returns false + blank pixmap if it can't find the right one
    bool getPixmap(CharName charName, QPixmap &pixmap,
                   bool inverted = false) const;

    /// Returns false + blank pixmap if it can't find the right one
    bool getColouredPixmap(CharName charName, QPixmap &pixmap,
                           int hue, int minimum,
                           bool inverted = false,
                           int saturation = PixmapFunctions::SaturationNotSpecified) const;

    /// Returns false + blank pixmap if it can't find the right one
    bool getShadedPixmap(CharName charName, QPixmap &pixmap,
                         bool inverted = false) const;

    friend class NoteFontFactory;
    NoteFont(QString fontName, int size = 0);
    std::set<int> getSizes() const { return m_fontMap.getSizes(); }

    bool lookup(CharName charName, bool inverted, QPixmap *&pixmap) const;
    void add(CharName charName, bool inverted, QPixmap *pixmap) const;

    NoteCharacterDrawRep *lookupDrawRep(QPixmap *pixmap) const;

    CharName getNameWithColour(CharName origName, int hue) const;
    CharName getNameShaded(CharName origName) const;

    typedef std::pair<QPixmap *, QPixmap *>    PixmapPair;
    typedef std::map<CharName, PixmapPair>     PixmapMap;
    typedef std::map<QString, PixmapMap *> FontPixmapMap;

    typedef std::map<QPixmap *, NoteCharacterDrawRep *> DrawRepMap;

    //--------------- Data members ---------------------------------

    int m_size;
    NoteFontMap m_fontMap;

    mutable PixmapMap *m_map; // pointer at a member of m_fontPixmapMap

    static FontPixmapMap *m_fontPixmapMap;
    static DrawRepMap *m_drawRepMap;

    static QPixmap *m_blankPixmap;
};



}

#endif
