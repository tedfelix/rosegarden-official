/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SYSTEMFONT_H
#define RG_SYSTEMFONT_H

#include <QPixmap>
#include "gui/editors/notation/NoteCharacterNames.h"


class SystemFontSpec;


namespace Rosegarden
{

typedef std::pair<QString, int> SystemFontSpec;

class SystemFont
{
public:
    enum Strategy {
        PreferGlyphs, PreferCodes, OnlyGlyphs, OnlyCodes
    };

    virtual ~SystemFont();
    virtual QPixmap renderChar(CharName charName,
                               int glyph, int code,
                               Strategy strategy,
                               bool &success) = 0;

    static SystemFont *loadSystemFont(const SystemFontSpec &spec);

protected:
    static void unbundleFonts();
    static void addFont(QString filename);
};


}

#endif
