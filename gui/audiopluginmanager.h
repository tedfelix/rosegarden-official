// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <vector>
#include <map>
#include <string>

#include <qstring.h>
#include <qcolor.h>

#include "AudioPluginInstance.h"

#ifndef _AUDIOPLUGINMANAGER_H_
#define _AUDIOPLUGINMANAGER_H_

// It's a bit confusing having all these various pluginmanagers -
// there's one in the sound library too.  This one is the gui end
// to it all - it displays and allows selection and management of
// the sequencer plugins.
//
// We should be plugin api-agnostic at this level.
//
//

// The AudioPlugin itself holds port information and gui related
// information (maybe pixmaps etc) and hints for displaying.
//
//

namespace Rosegarden
{

typedef std::vector<PluginPort*>::iterator PortIterator;

// We use this to remember what we're copying and pasting
//
struct AudioPluginClipboard
{
    int                 m_pluginNumber;
    std::map<std::string, std::string> m_configuration;
    std::string         m_program;
    std::vector<float>  m_controlValues;
};

class AudioPlugin
{
public:
    AudioPlugin(const QString &identifier,
                const QString &name,
                unsigned long uniqueId,
                const QString &label,
                const QString &author,
                const QString &copyright,
		bool isSynth,
		bool isGrouped,
		const QString &category);

    QString getIdentifier() const { return m_identifier; }

    QString getName() const { return m_name; }
    unsigned long getUniqueId() const { return m_uniqueId; }
    QString getLabel() const { return m_label; }
    QString getAuthor() const { return m_author; }
    QString getCopyright() const { return m_copyright; }
    bool isSynth() const { return m_isSynth; }
    bool isGrouped() const { return m_isGrouped; }
    QString getCategory() const { return m_category; }

    void addPort(int number,
                 const QString &name,
                 PluginPort::PortType type,
                 PluginPort::PortDisplayHint hint,
                 PortData lowerBound,
                 PortData upperBound,
		 PortData defaultVale);

    PortIterator begin() { return m_ports.begin(); }
    PortIterator end() { return m_ports.end(); }

    QColor getColour() const { return m_colour; }
    void setColour(const QColor &colour) { m_colour = colour; }

protected:

    QString                    m_identifier;

    QString                    m_name;
    unsigned long              m_uniqueId;
    QString                    m_label;
    QString                    m_author;
    QString                    m_copyright;
    bool                       m_isSynth;
    bool                       m_isGrouped;
    QString                    m_category;

    // our ports and associated hints
    std::vector<PluginPort*>   m_ports;

    // Colour of this activated plugin
    //
    QColor                    m_colour;
};

typedef std::vector<AudioPlugin*>::iterator PluginIterator;

class AudioPluginManager
{
public:
    AudioPluginManager();

    AudioPlugin* addPlugin(const QString &identifier,
                           const QString &name,
                           unsigned long uniqueId,
                           const QString &label,
                           const QString &author,
                           const QString &copyright,
			   bool isSynth,
			   bool isGrouped,
			   const QString &category);

    bool removePlugin(const QString &identifier);

    // Get a straight list of names
    //
    std::vector<QString> getPluginNames();

    // Some useful members
    //
    AudioPlugin* getPlugin(int number);

    AudioPlugin* getPluginByIdentifier(QString identifier);
    int getPositionByIdentifier(QString identifier);

    // Deprecated -- the GUI shouldn't be using unique ID because it's
    // bound to a particular plugin type (and not necessarily unique
    // anyway).  It should use the identifier instead, which is a
    // structured string managed by the sequencer.  Keep this in only
    // for compatibility with old .rg files.
    //
    AudioPlugin* getPluginByUniqueId(unsigned long uniqueId);

//    int getPositionByUniqueId(unsigned long uniqueId);

    PluginIterator begin() { return m_plugins.begin(); }
    PluginIterator end() { return m_plugins.end(); }

    // Sample rate
    //
    void setSampleRate(unsigned int rate) { m_sampleRate = rate; }
    unsigned int getSampleRate() const { return m_sampleRate; }

    AudioPluginClipboard* getPluginClipboard() { return &m_pluginClipboard; }

protected:
    void fetchSampleRate();

    std::vector<AudioPlugin*> m_plugins;

    unsigned int              m_sampleRate;

    AudioPluginClipboard      m_pluginClipboard;

};

}

#endif // _AUDIOPLUGINMANAGER_H_
