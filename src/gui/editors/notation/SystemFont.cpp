/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SystemFont.h"
#include "SystemFontQt.h"
#include "SystemFontXft.h"

#include "misc/Debug.h"

#include <kstddirs.h>
#include "NoteFontMap.h"
#include <qfont.h>
#include <qfontinfo.h>
#include <qpixmap.h>
#include <qstring.h>


namespace Rosegarden
{

SystemFont *
SystemFont::loadSystemFont(const SystemFontSpec &spec)
{
    QString name = spec.first;
    int size = spec.second;

    NOTATION_DEBUG << "SystemFont::loadSystemFont: name is " << name << ", size " << size << endl;

    if (name == "DEFAULT") {
        QFont font;
        font.setPixelSize(size);
        return new SystemFontQt(font);
    }

#ifdef HAVE_XFT

    FcPattern *pattern, *match;
    FcResult result;
    FcChar8 *matchFamily;
    XftFont *xfont = 0;

    Display *dpy = QPaintDevice::x11AppDisplay();
    static bool haveFcDirectory = false;

    if (!dpy) {
        std::cerr << "SystemFont::loadSystemFont[Xft]: Xft support requested but no X11 display available!" << std::endl;
        goto qfont;
    }

    if (!haveFcDirectory) {
        QString fontDir = KGlobal::dirs()->findResource("appdata", "fonts/");
        if (!FcConfigAppFontAddDir(FcConfigGetCurrent(),
                                   (const FcChar8 *)fontDir.latin1())) {
            NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: Failed to add font directory " << fontDir << " to fontconfig, continuing without it" << endl;
        }
        haveFcDirectory = true;
    }

    pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *)name.latin1());
    FcPatternAddInteger(pattern, FC_PIXEL_SIZE, size);
    FcConfigSubstitute(FcConfigGetCurrent(), pattern, FcMatchPattern);

    result = FcResultMatch;
    match = FcFontMatch(FcConfigGetCurrent(), pattern, &result);
    FcPatternDestroy(pattern);

    if (!match || result != FcResultMatch) {
        NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: No match for font "
        << name << " (result is " << result
        << "), falling back on QFont" << endl;
        if (match)
            FcPatternDestroy(match);
        goto qfont;
    }

    FcPatternGetString(match, FC_FAMILY, 0, &matchFamily);
    NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: match family is "
    << (char *)matchFamily << endl;

    if (QString((char *)matchFamily).lower() != name.lower()) {
        NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: Wrong family returned, falling back on QFont" << endl;
        FcPatternDestroy(match);
        goto qfont;
    }

    xfont = XftFontOpenPattern(dpy, match);
    if (!xfont) {
        FcPatternDestroy(match);
        NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: Unable to load font "
        << name << " via Xft, falling back on QFont" << endl;
        goto qfont;
    }

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: successfully loaded font "
    << name << " through Xft" << endl;

    return new SystemFontXft(dpy, xfont);


qfont:

#endif

    QFont qfont(name, size, QFont::Normal);
    qfont.setPixelSize(size);

    QFontInfo info(qfont);

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Qt]: have family " << info.family() << " (exactMatch " << info.exactMatch() << ")" << endl;

    //    return info.exactMatch();

    // The Qt documentation says:
    //
    //   bool QFontInfo::exactMatch() const
    //   Returns TRUE if the matched window system font is exactly the
    //   same as the one specified by the font; otherwise returns FALSE.
    //
    // My arse.  I specify "feta", I get "Verdana", and exactMatch
    // returns true.  Uh huh.
    //
    // UPDATE: in newer versions of Qt, I specify "fughetta", I get
    // "Fughetta [macromedia]", and exactMatch returns false.  Just as
    // useless, but in a different way.

    QString family = info.family().lower();

    if (family == name.lower())
        return new SystemFontQt(qfont);
    else {
        int bracket = family.find(" [");
        if (bracket > 1)
            family = family.left(bracket);
        if (family == name.lower())
            return new SystemFontQt(qfont);
    }

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Qt]: Wrong family returned, failing" << endl;
    return 0;
}

}
