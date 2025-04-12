
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOPLUGIN_H
#define RG_AUDIOPLUGIN_H

#include "base/AudioPluginInstance.h"

#include <QColor>
#include <QSharedPointer>
#include <QString>

#include <vector>


namespace Rosegarden
{


class AudioPlugin
{
public:
    AudioPlugin(const QString &identifier,
                PluginArch arch,
                const QString &name,
                unsigned long uniqueId,
                const QString &label,
                const QString &author,
                const QString &copyright,
                bool isSynth,
                bool isGrouped,
                const QString &category);

    QString getIdentifier() const { return m_identifier; }
    PluginArch getArch() const  { return m_arch; }

    QString getName() const { return m_name; }
    unsigned long getUniqueId() const { return m_uniqueId; }
    QString getLabel() const { return m_label; }
    QString getAuthor() const { return m_author; }
    QString getCopyright() const { return m_copyright; }
    bool isSynth() const { return m_isSynth; }
    bool isEffect() const { // true if >0 audio inputs
        for (unsigned int i = 0; i < m_ports.size(); ++i) {
            if ((m_ports[i]->getType() & PluginPort::Input) &&
                (m_ports[i]->getType() & PluginPort::Audio)) {
                return true;
            }
        }
        return false;
    }
    bool isGrouped() const { return m_isGrouped; }
    QString getCategory() const { return m_category; }

    void addPort(int number,
                 const QString &name,
                 PluginPort::PortType type,
                 PluginPort::PortDisplayHint hint,
                 PortData lowerBound,
                 PortData upperBound,
                 PortData defaultValue);

    typedef std::vector<QSharedPointer<PluginPort> > PluginPortVector;

    PluginPortVector::iterator begin() { return m_ports.begin(); }
    PluginPortVector::iterator end() { return m_ports.end(); }

    QColor getColour() const { return m_colour; }
    void setColour(const QColor &colour) { m_colour = colour; }

protected:

    QString                    m_identifier;
    PluginArch m_arch;

    QString                    m_name;
    unsigned long              m_uniqueId;
    QString                    m_label;
    QString                    m_author;
    QString                    m_copyright;
    bool                       m_isSynth;
    bool                       m_isGrouped;
    QString                    m_category;

    // our ports and associated hints
    PluginPortVector m_ports;

    // Colour of this activated plugin
    //
    QColor                    m_colour;
};


}

#endif
