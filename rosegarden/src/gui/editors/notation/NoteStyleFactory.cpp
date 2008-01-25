/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "NoteStyleFactory.h"

#include <kstddirs.h>
#include "misc/Strings.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "NotationProperties.h"
#include "NoteStyle.h"
#include "NoteStyleFileReader.h"
#include <kglobal.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <qstringlist.h>


namespace Rosegarden
{

const NoteStyleName NoteStyleFactory::DefaultStyle = "Classical";

std::vector<NoteStyleName>
NoteStyleFactory::getAvailableStyleNames()
{
    std::vector<NoteStyleName> names;

    QString styleDir = KGlobal::dirs()->findResource("appdata", "styles/");
    QDir dir(styleDir);
    if (!dir.exists()) {
        std::cerr << "NoteStyle::getAvailableStyleNames: directory \"" << styleDir
        << "\" not found" << std::endl;
        return names;
    }

    dir.setFilter(QDir::Files | QDir::Readable);
    QStringList files = dir.entryList();
    bool foundDefault = false;

    for (QStringList::Iterator i = files.begin(); i != files.end(); ++i) {
        if ((*i).length() > 4 && (*i).right(4) == ".xml") {
            QFileInfo fileInfo(QString("%1/%2").arg(styleDir).arg(*i));
            if (fileInfo.exists() && fileInfo.isReadable()) {
                std::string styleName = qstrtostr((*i).left((*i).length() - 4));
                if (styleName == DefaultStyle)
                    foundDefault = true;
                names.push_back(styleName);
            }
        }
    }

    if (!foundDefault) {
        std::cerr << "NoteStyleFactory::getAvailableStyleNames: WARNING: Default style name \"" << DefaultStyle << "\" not found" << std::endl;
    }

    return names;
}

NoteStyle *
NoteStyleFactory::getStyle(NoteStyleName name)
{
    StyleMap::iterator i = m_styles.find(name);

    if (i == m_styles.end()) {

        try {
            NoteStyle *newStyle = NoteStyleFileReader(name).getStyle();
            m_styles[name] = newStyle;
            return newStyle;

        } catch (NoteStyleFileReader::StyleFileReadFailed f) {
            std::cerr
            << "NoteStyleFactory::getStyle: Style file read failed: "
            << f.getMessage() << std::endl;
            throw StyleUnavailable("Style file read failed: " +
                                   f.getMessage());
        }

    } else {
        return i->second;
    }
}

NoteStyle *
NoteStyleFactory::getStyleForEvent(Event *event)
{
    NoteStyleName styleName;
    if (event->get
            <String>(NotationProperties::NOTE_STYLE, styleName)) {
        return getStyle(styleName);
    }
    else {
        return getStyle(DefaultStyle);
    }
}

NoteStyleFactory::StyleMap NoteStyleFactory::m_styles;


}
