/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioPlugin.h"

#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include <QColor>
#include <QString>


namespace Rosegarden
{

AudioPlugin::AudioPlugin(const QString &identifier,
                         const QString &name,
                         unsigned long uniqueId,
                         const QString &label,
                         const QString &author,
                         const QString &copyright,
                         bool isSynth,
                         bool isGrouped,
                         const QString &category):
        m_identifier(identifier),
        m_name(name),
        m_uniqueId(uniqueId),
        m_label(label),
        m_author(author),
        m_copyright(copyright),
        m_isSynth(isSynth),
        m_isGrouped(isGrouped),
        m_category(category),
        m_colour(QColor(Qt::darkRed))
{}

void
AudioPlugin::addPort(int number,
                     const QString &name,
                     PluginPort::PortType type,
                     PluginPort::PortDisplayHint hint,
                     PortData lowerBound,
                     PortData upperBound,
                     PortData defaultValue)
{
    PluginPort *port = new PluginPort(number,
                                      qstrtostr(name),
                                      type,
                                      hint,
                                      lowerBound,
                                      upperBound,
                                      defaultValue);
    m_ports.push_back(port);

}

}
