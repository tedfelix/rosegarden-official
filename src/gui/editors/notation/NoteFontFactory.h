
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTEFONTFACTORY_H_
#define _RG_NOTEFONTFACTORY_H_

#include "base/Exception.h"
#include <map>
#include <set>
#include <string>
#include <vector>




namespace Rosegarden
{

class NoteFont;


class NoteFontFactory
{
public:
    typedef Exception NoFontsAvailable;

    // Any method passed a fontName argument may throw BadFont or
    // MappingFileReadFailed; any other method may throw NoFontsAvailable

    static NoteFont *getFont(std::string fontName, int size);

    static std::set<std::string> getFontNames(bool forceRescan = false);
    static std::vector<int> getAllSizes(std::string fontName); // sorted
    static std::vector<int> getScreenSizes(std::string fontName); // sorted

    static std::string getDefaultFontName();
    static int getDefaultSize(std::string fontName);
    static bool isAvailableInSize(std::string fontName, int size);

private:
    static std::set<std::string> m_fontNames;
    static std::map<std::pair<std::string, int>, NoteFont *> m_fonts;
};



}

#endif
