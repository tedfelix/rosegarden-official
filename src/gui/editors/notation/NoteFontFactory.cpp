/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "NoteFontFactory.h"
#include "misc/Debug.h"
#include <kapplication.h>

#include <klocale.h>
#include <kstddirs.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Exception.h"
#include "gui/kdeext/KStartupLogo.h"
#include "NoteFont.h"
#include "NoteFontMap.h"
#include <kconfig.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <qdir.h>
#include <qstring.h>
#include <qstringlist.h>


namespace Rosegarden
{

std::set<std::string>
NoteFontFactory::getFontNames(bool forceRescan)
{
    NOTATION_DEBUG << "NoteFontFactory::getFontNames: forceRescan = " << forceRescan << endl;

    if (forceRescan)
        m_fontNames.clear();
    if (!m_fontNames.empty())
        return m_fontNames;

    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    QString fontNameList = "";
    if (!forceRescan) {
        fontNameList = config->readEntry("notefontlist");
    }

    NOTATION_DEBUG << "NoteFontFactory::getFontNames: read from cache: " << fontNameList << endl;

    QStringList names = QStringList::split(",", fontNameList);

    if (names.empty()) {

        NOTATION_DEBUG << "NoteFontFactory::getFontNames: No names available, rescanning..." << endl;

        QString mappingDir =
            KGlobal::dirs()->findResource("appdata", "fonts/mappings/");
        QDir dir(mappingDir);
        if (!dir.exists()) {
            std::cerr << "NoteFontFactory::getFontNames: mapping directory \""
                      << mappingDir << "\" not found" << std::endl;
            return m_fontNames;
        }

        dir.setFilter(QDir::Files | QDir::Readable);
        QStringList files = dir.entryList();
        for (QStringList::Iterator i = files.begin(); i != files.end(); ++i) {

            if ((*i).length() > 4 && (*i).right(4).lower() == ".xml") {

                std::string name(qstrtostr((*i).left((*i).length() - 4)));

                try {
                    NoteFontMap map(name);
                    if (map.ok())
                        names.append(strtoqstr(map.getName()));
                } catch (Exception e) {
                    KStartupLogo::hideIfStillThere();
                    KMessageBox::error(0, strtoqstr(e.getMessage()));
                    throw;
                }
            }
        }
    }

    QString savedNames = "";

    for (QStringList::Iterator i = names.begin(); i != names.end(); ++i) {
        m_fontNames.insert(qstrtostr(*i));
        if (i != names.begin())
            savedNames += ",";
        savedNames += *i;
    }

    config->writeEntry("notefontlist", savedNames);

    return m_fontNames;
}

std::vector<int>
NoteFontFactory::getAllSizes(std::string fontName)
{
    NoteFont *font = getFont(fontName, 0);
    if (!font)
        return std::vector<int>();

    std::set
        <int> s(font->getSizes());
    std::vector<int> v;
    for (std::set
                <int>::iterator i = s.begin(); i != s.end(); ++i) {
            v.push_back(*i);
        }

    std::sort(v.begin(), v.end());
    return v;
}

std::vector<int>
NoteFontFactory::getScreenSizes(std::string fontName)
{
    NoteFont *font = getFont(fontName, 0);
    if (!font)
        return std::vector<int>();

    std::set
        <int> s(font->getSizes());
    std::vector<int> v;
    for (std::set
                <int>::iterator i = s.begin(); i != s.end(); ++i) {
            if (*i <= 16)
                v.push_back(*i);
        }
    std::sort(v.begin(), v.end());
    return v;
}

NoteFont *
NoteFontFactory::getFont(std::string fontName, int size)
{
    std::map<std::pair<std::string, int>, NoteFont *>::iterator i =
        m_fonts.find(std::pair<std::string, int>(fontName, size));

    if (i == m_fonts.end()) {
        try {
            NoteFont *font = new NoteFont(fontName, size);
            m_fonts[std::pair<std::string, int>(fontName, size)] = font;
            return font;
        } catch (Exception e) {
            KStartupLogo::hideIfStillThere();
            KMessageBox::error(0, strtoqstr(e.getMessage()));
            throw;
        }
    } else {
        return i->second;
    }
}

std::string
NoteFontFactory::getDefaultFontName()
{
    static std::string defaultFont = "";
    if (defaultFont != "")
        return defaultFont;

    std::set
        <std::string> fontNames = getFontNames();

    if (fontNames.find("Feta") != fontNames.end())
        defaultFont = "Feta";
    else {
        fontNames = getFontNames(true);
        if (fontNames.find("Feta") != fontNames.end())
            defaultFont = "Feta";
        else if (fontNames.find("Feta Pixmaps") != fontNames.end())
            defaultFont = "Feta Pixmaps";
        else if (fontNames.size() > 0)
            defaultFont = *fontNames.begin();
        else {
            QString message = i18n("Can't obtain a default font -- no fonts found");
            KStartupLogo::hideIfStillThere();
            KMessageBox::error(0, message);
            throw NoFontsAvailable(qstrtostr(message));
        }
    }

    return defaultFont;
}

int
NoteFontFactory::getDefaultSize(std::string fontName)
{
    // always return 8 if it's supported!
    std::vector<int> sizes(getScreenSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == 8)
            return sizes[i];
    }
    return sizes[sizes.size() / 2];
}

bool
NoteFontFactory::isAvailableInSize(std::string fontName, int size)
{
    std::vector<int> sizes(getAllSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size)
            return true;
    }
    return false;
}

std::set<std::string> NoteFontFactory::m_fontNames;
std::map<std::pair<std::string, int>, NoteFont *> NoteFontFactory::m_fonts;

}
