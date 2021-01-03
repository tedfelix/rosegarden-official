/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PercussionMap.h"

namespace Rosegarden
{

using namespace BaseProperties;

int
PercussionMap::getPitch(int pitch)
{
    if (m_data.find(pitch) != m_data.end()) {
        return m_data[pitch].m_pitch;
    } else {
        return pitch;
    }
}

std::string
PercussionMap::getNoteHead(int pitch)
{
    if (m_data.find(pitch) != m_data.end()) {
        return m_data[pitch].m_notehead;
    } else {
        return "normal";
    }
}

int
PercussionMap::getVoice(int pitch)
{
    if (m_data.find(pitch) != m_data.end()) {
        return m_data[pitch].m_voice;
    } else {
        return 0;
    }
}

bool
PercussionMap::loadDefaultPercussionMap()
{
    return loadPercussionMap(ResourceFinder().getResourcePath("percussion", "PercussionMap.xml"));
}

bool
PercussionMap::loadPercussionMap(const QString &filename)
{

    QFile mapFile(filename);
    bool ok = mapFile.open(QIODevice::ReadOnly);
//     if (!ok)
//         QMessageBox::critical(0, tr("Rosegarden"), tr("couldn't open file '%1'").arg(handler.errorString()));

    QXmlInputSource source(&mapFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);

    ok = reader.parse(source);

//     if (!ok)
//         QMessageBox::critical(0, tr("Rosegarden"), tr("couldn't parse chord dictionary : %1").arg(handler.errorString()));
    return ok;
}

bool
PercussionMap::startElement(const QString& /*namespaceURI*/,
                            const QString& /*localName*/,
                            const QString& qName,
                            const QXmlAttributes& atts)
{
    if (qName.toLower() == "percussion-map") {
        m_data.clear();
    } else if (qName.toLower() == "instrument") {
        m_xmlPitchIn = atts.value("pitch").toInt();
        m_xmlPitchOut = m_xmlPitchIn;
        m_xmlNotehead = "normal";
        m_xmlStemUp = true;
    } else if (qName.toLower() == "display") {
        if (atts.index("pitch") >= 0) {
            m_xmlPitchOut = atts.value("pitch").toInt();
        }
        if (atts.index("notehead") >= 0) {
            m_xmlNotehead = atts.value("notehead").toStdString();
        }
        if (atts.index("stem") >= 0) {
            m_xmlStemUp = (atts.value("stem") == "down") ? false : true;
        }
    }

    return true;
}

bool
PercussionMap::endElement(const QString& /*namespaceURI*/,
                          const QString& /*localName*/,
                          const QString& qName)
{
    if (qName.toLower() == "instrument") {
        m_data[m_xmlPitchIn] = PMapData(m_xmlPitchOut, m_xmlNotehead, m_xmlStemUp);
    }

    return true;
}

}

