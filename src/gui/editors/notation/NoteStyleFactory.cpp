/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "NoteStyleFactory.h"

#include <QDir>
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "NotationProperties.h"
#include "NoteStyle.h"
#include "NoteStyleFileReader.h"
#include "gui/general/ResourceFinder.h"
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>


namespace Rosegarden
{

const NoteStyleName NoteStyleFactory::DefaultStyle = "Classical";

std::vector<NoteStyleName>
NoteStyleFactory::getAvailableStyleNames()
{
    std::vector<NoteStyleName> names;
    
    ResourceFinder rf;
    QStringList files = rf.getResourceFiles("styles", "xml");

    bool foundDefault = false;

    for (QStringList::Iterator i = files.begin(); i != files.end(); ++i) {
        QString styleName = QFileInfo(*i).baseName();
        if (styleName == DefaultStyle) foundDefault = true;
        names.push_back(styleName);
    }

    if (!foundDefault) {
        RG_WARNING << "NoteStyleFactory::getAvailableStyleNames: WARNING: Default style name \"" << DefaultStyle << "\" not found";
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
            RG_WARNING << "NoteStyleFactory::getStyle: Style file read failed:" << f.getMessage();
            throw StyleUnavailable("Style file read failed: " + f.getMessage());
        }

    } else {
        return i->second;
    }
}

NoteStyle *
NoteStyleFactory::getStyleForEvent(Event *event)
{
    std::string sname;
    if (event->get<String>(NotationProperties::NOTE_STYLE, sname)) {
        NoteStyleName styleName = strtoqstr(sname);
        return getStyle(styleName);
    }
    else {
        return getStyle(DefaultStyle);
    }
}

NoteStyleFactory::StyleMap NoteStyleFactory::m_styles;


}
