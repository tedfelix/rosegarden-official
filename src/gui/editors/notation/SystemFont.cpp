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


// "qtextstream.h must be included before any header file that defines Status"
#include <QTextStream>

// "qmetatype.h must be included before any header file that defines Bool"
#include <QMetaType>

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Profiler.h"
#include "gui/general/ResourceFinder.h"
#include "NoteFontMap.h"

#include <QFont>
#include <QFontInfo>
#include <QFontDatabase>
#include <QFileInfo>
#include <QPixmap>
#include <QString>

#include "SystemFont.h"
#include "SystemFontQt.h"
#include "SystemFontXft.h"

#include <iostream>


namespace Rosegarden
{

SystemFont::~SystemFont()
{}

SystemFont *
SystemFont::loadSystemFont(const SystemFontSpec &spec)
{
    Profiler profiler("SystemFont::loadSystemFont");

    QString name = spec.first;
    int size = spec.second;

    NOTATION_DEBUG << "SystemFont::loadSystemFont: name is " << name << ", size " << size << endl;

    if (name == "DEFAULT") {
        QFont font;
        font.setPixelSize(size);
        return new SystemFontQt(font);
    }

    bool haveFonts = false;
    if (!haveFonts) {
        unbundleFonts();
        haveFonts = true;
    }

#ifdef HAVE_XFT

    FcPattern *pattern, *match;
    FcResult result;
    FcChar8 *matchFamily;
    XftFont *xfont = 0;

    Display *dpy = QPaintDevice::x11AppDisplay();

    if (!dpy) {
        std::cerr << "SystemFont::loadSystemFont[Xft]: Xft support requested but no X11 display available!" << std::endl;
        goto qfont;
    }
	
    pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *)name.toLatin1().data());
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

    if (QString((char *)matchFamily).toLower() != name.toLower()) {
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

    static QHash<QString, QFont *> qFontMap;

    if (qFontMap.contains(name)) {
        if (qFontMap[name] == 0) return 0;
        QFont qfont(*qFontMap[name]);
        qfont.setPixelSize(size);
        return new SystemFontQt(qfont);
    }

    QFont qfont(name, size, QFont::Normal);
    qfont.setPixelSize(size);

    QFontInfo info(qfont);

    std::cerr << "SystemFont::loadSystemFont[Qt]: wanted family " << name << " at size " << size << ", got family " << info.family() << " (exactMatch " << info.exactMatch() << ")" << std::endl;

    QString family = info.family().toLower();

    if (family == name.toLower()) {
        qFontMap[name] = new QFont(qfont);
        return new SystemFontQt(qfont);
    } else {
        int bracket = family.indexOf(" [");
        if (bracket > 1) family = family.left(bracket);
        if (family == name.toLower()) {
            qFontMap[name] = new QFont(qfont);
            return new SystemFontQt(qfont);
        }
    }

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Qt]: Wrong family returned, failing" << endl;
    qFontMap[name] = 0;
    return 0;
}

void
SystemFont::unbundleFonts()
{
    QStringList fontFiles;
    fontFiles << ResourceFinder().getResourceFiles("fonts", "pfa");
    fontFiles << ResourceFinder().getResourceFiles("fonts", "pfb");
    fontFiles << ResourceFinder().getResourceFiles("fonts", "ttf");
    fontFiles << ResourceFinder().getResourceFiles("fonts", "otf");

    NOTATION_DEBUG << "Found font files: " << fontFiles << endl;

    for (QStringList::const_iterator i = fontFiles.begin();
         i != fontFiles.end(); ++i) {
        QString fontFile(*i);
        QString name = QFileInfo(fontFile).fileName();
        if (fontFile.startsWith(":")) {
            ResourceFinder().unbundleResource("fonts", name);
            fontFile = ResourceFinder().getResourcePath("fonts", name);
            if (fontFile.startsWith(":")) { // unbundling failed
                continue;
            }
        }
        addFont(fontFile);
    }
}

void
SystemFont::addFont(QString fileName)
{
#ifdef HAVE_XFT
    if (!FcConfigAppFontAddFile
        (FcConfigGetCurrent(),
         (const FcChar8 *)fileName.toLocal8Bit().data())) {
        NOTATION_DEBUG << "SystemFont::addFont[Xft]: Failed to add font file " << fileName << " to fontconfig, continuing without it" << endl;
    } else {
        NOTATION_DEBUG << "Added font file \"" << fileName << "\" to fontconfig" << endl;
    }
#else
    QFontDatabase::addApplicationFont(fileName);
    NOTATION_DEBUG << "Added font file \"" << fileName << "\" to Qt font database" << endl;
#endif
}

}
