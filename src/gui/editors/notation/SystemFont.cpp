/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
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

    NOTATION_DEBUG << "SystemFont::loadSystemFont: name is " << name << ", size " << size;

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

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Qt]: wanted family " << name << " at size " << size << ", got family " << info.family() << " (exactMatch " << info.exactMatch() << ")";

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

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Qt]: Wrong family returned, failing";
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

    NOTATION_DEBUG << "Found font files: " << fontFiles;

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
    QFontDatabase::addApplicationFont(fileName);
    NOTATION_DEBUG << "Added font file \"" << fileName << "\" to Qt font database";
}

}
