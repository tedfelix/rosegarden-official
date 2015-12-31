
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTEFONTFACTORY_H
#define RG_NOTEFONTFACTORY_H

#include "base/Exception.h"
#include <map>
#include <set>
#include <vector>

#include <QString>
#include <QCoreApplication>


namespace Rosegarden
{

class NoteFont;


class NoteFontFactory
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::NoteFontFactory)

public:
    typedef Exception NoFontsAvailable;

    // Any method passed a fontName argument may throw BadFont or
    // MappingFileReadFailed; any other method may throw NoFontsAvailable

    static NoteFont *getFont(QString fontName, int size);

    // This is called with forceRescan from the startup tester thread;
    // at all other times, the cached results are used
    static std::set<QString> getFontNames(bool forceRescan = false);
    static std::vector<int> getAllSizes(QString fontName); // sorted
    static std::vector<int> getScreenSizes(QString fontName); // sorted

    static QString getDefaultFontName();

    /// Return the default single staff size (prefers 8)
    static int getDefaultSize(QString fontName);

    /// Return the default multi-staff size (prefers 6)
    static int getDefaultMultiSize(QString fontName);

    static bool isAvailableInSize(QString fontName, int size);

private:
    static std::set<QString> m_fontNames;
    static std::map<std::pair<QString, int>, NoteFont *> m_fonts;
};



}

#endif
