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


#include "NoteFontFactory.h"
#include "misc/Debug.h"
#include <QApplication>

#include <QDir>
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/Exception.h"
#include "gui/widgets/StartupLogo.h"
#include "NoteFont.h"
#include "NoteFontMap.h"
#include <QSettings>
#include "gui/general/ResourceFinder.h"
#include <QMessageBox>
#include <QDir>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <QMutex>

namespace Rosegarden
{

static QMutex *
mutex()
{
    static QMutex m;
    return &m;
}

std::set<QString>
NoteFontFactory::getFontNames(bool forceRescan)
{
    NOTATION_DEBUG << "NoteFontFactory::getFontNames: forceRescan = " << forceRescan << endl;

    QMutexLocker locker(mutex());

    if (forceRescan) m_fontNames.clear();
    if (!m_fontNames.empty()) return m_fontNames;

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    QString fontNameList = "";
    if (!forceRescan) {
        fontNameList = settings.value("notefontlist", "").toString();
    }
    settings.endGroup();

    NOTATION_DEBUG << "NoteFontFactory::getFontNames: read from cache: " << fontNameList << endl;

    //QStringList names = QStringList::split(",", fontNameList);
    QStringList names = fontNameList.split(",", QString::SkipEmptyParts);

    ResourceFinder rf;

    if (names.empty()) {

        NOTATION_DEBUG << "NoteFontFactory::getFontNames: No names available, rescanning..." << endl;

        QStringList files = rf.getResourceFiles("fonts/mappings", "xml");

        for (QStringList::Iterator i = files.begin(); i != files.end(); ++i) {

            QString filepath = *i;
            QString name = QFileInfo(filepath).baseName();

            try {
                NoteFontMap map(name);
                if (map.ok()) names.append(map.getName());
            } catch (Exception e) {
                StartupLogo::hideIfStillThere();
                QMessageBox::critical(0, tr("Rosegarden"), strtoqstr(e.getMessage()));
                throw;
            }
        }
    }

    QString savedNames = "";

    for (QStringList::Iterator i = names.begin(); i != names.end(); ++i) {
        m_fontNames.insert(*i);
        if (i != names.begin()) savedNames += ",";
        savedNames += *i;
    }

    settings.beginGroup( NotationViewConfigGroup );
    settings.setValue("notefontlist", savedNames);
    settings.endGroup();

    return m_fontNames;
}

std::vector<int>
NoteFontFactory::getAllSizes(QString fontName)
{
    NoteFont *font = getFont(fontName, 0);
    if (!font) return std::vector<int>();

    std::set<int> s(font->getSizes());
    std::vector<int> v;
    for (std::set<int>::iterator i = s.begin(); i != s.end(); ++i) {
        v.push_back(*i);
    }

    std::sort(v.begin(), v.end());
    return v;
}

std::vector<int>
NoteFontFactory::getScreenSizes(QString fontName)
{
    NoteFont *font = getFont(fontName, 0);
    if (!font) return std::vector<int>();

    std::set<int> s(font->getSizes());
    std::vector<int> v;
    for (std::set<int>::iterator i = s.begin(); i != s.end(); ++i) {
        if (*i <= 16) v.push_back(*i);
    }
    std::sort(v.begin(), v.end());
    return v;
}

NoteFont *
NoteFontFactory::getFont(QString fontName, int size)
{
    QMutexLocker locker(mutex());

    std::map<std::pair<QString, int>, NoteFont *>::iterator i =
        m_fonts.find(std::pair<QString, int>(fontName, size));

    if (i == m_fonts.end()) {
        try {
            NoteFont *font = new NoteFont(fontName, size);
            m_fonts[std::pair<QString, int>(fontName, size)] = font;
            return font;
        } catch (Exception e) {
            StartupLogo::hideIfStillThere();
            QMessageBox::critical(0, tr("Rosegarden"), strtoqstr(e.getMessage()));
            throw;
        }
    } else {
        return i->second;
    }
}

QString
NoteFontFactory::getDefaultFontName()
{
    static QString defaultFont = "";
    if (defaultFont != "") return defaultFont;

    std::set<QString> fontNames = getFontNames();

    if (fontNames.find("Feta") != fontNames.end()) {
        defaultFont = "Feta";
    } else {
        fontNames = getFontNames(true);
        if (fontNames.find("Feta") != fontNames.end()) {
            defaultFont = "Feta";
        } else if (!fontNames.empty()) {
            defaultFont = *fontNames.begin();
        } else {
            QString message = tr("Can't obtain a default font -- no fonts found");
            StartupLogo::hideIfStillThere();
            QMessageBox::critical(0, tr("Rosegarden"), message);
            throw NoFontsAvailable(qstrtostr(message));
        }
    }

    return defaultFont;
}

int
NoteFontFactory::getDefaultSize(QString fontName)
{
    // always return 8 if it's supported!
    std::vector<int> sizes(getScreenSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == 8) return sizes[i];
    }
    return sizes[sizes.size() / 2];
}

int
NoteFontFactory::getDefaultMultiSize(QString fontName)
{
    // always return 6 if it's supported!
    std::vector<int> sizes(getScreenSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == 6) return sizes[i];
    }
    return sizes[sizes.size() / 2];
}

bool
NoteFontFactory::isAvailableInSize(QString fontName, int size)
{
    std::vector<int> sizes(getAllSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size) return true;
    }
    return false;
}

std::set<QString> NoteFontFactory::m_fontNames;
std::map<std::pair<QString, int>, NoteFont *> NoteFontFactory::m_fonts;

}
