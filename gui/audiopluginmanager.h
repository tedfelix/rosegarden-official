/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <qstring.h>

#include "AudioPluginInstance.h"

#include "MappedCommon.h"

#ifndef _AUDIOPLUGINMANAGER_H_
#define _AUDIOPLUGINMANAGER_H_

// It's a bit confusing having all these various pluginmanagers -
// there's one in the sound library too.  This one is the gui end
// to it all - it displays and allows selection and management of
// the sequencer plugins (PluginManager ones - they'll be REALTIME
// capable plugins) plus all the static (non REALTIME) plugins that
// we might want to use in an audio editor.
//
// We should be plugin api-agnostic at this level.
//
//

// The AudioPlugin itself holds port information and gui related
// information (maybe pixmaps etc) and hints for displaying.  The
// plugin id relates directly to the relevant ID at the
// MappedStudio's plugin manager.
//
//

namespace Rosegarden
{

// Redefine and augment a lot of the LADSPA properties in an
// effort to understand them.  See /usr/include/ladspa.h for
// more information and descriptions of the properties.
//
class PluginPort
{
public:
    typedef enum
    {
        Input    = 0x01,
        Output   = 0x02,
        Control  = 0x04,
        Audio    = 0x08
    } PortType;


    typedef enum
    {
        Below      = 0x01,
        Above      = 0x02,
        Toggled    = 0x04,
        SampleRate = 0x08,
        Log        = 0x10,
        Integer    = 0x20

    } PortRange;

    PluginPort(MappedObjectId id,
               const QString &m_name,
               PortType type,
               PortRange range,
               PortData lowerBound,
               PortData upperBound);

    MappedObjectId getId() const { return m_id; }
    QString getName() const { return m_name; }
    PortType getType() const { return m_type; }
    PortRange getRange() const { return m_range; }
    PortData getLowerBound() const { return m_lowerBound; }
    PortData getUpperBound() const { return m_upperBound; }

protected:

    MappedObjectId m_id;
    QString        m_name;
    PortType       m_type;
    PortRange      m_range;
    PortData       m_lowerBound;
    PortData       m_upperBound;
};

typedef std::vector<PluginPort*>::iterator PortIterator;

class AudioPlugin
{
public:
    AudioPlugin(MappedObjectId id,
                const QString &name,
                unsigned long uniqueId,
                const QString &label,
                const QString &author,
                const QString &copyright);

    MappedObjectId getId() const { return m_id; }
    QString getName() const { return m_name; }
    unsigned long getUniqueId() const { return m_uniqueId; }
    QString getLabel() const { return m_label; }
    QString getAuthor() const { return m_author; }
    QString getCopyright() const { return m_copyright; }

    void addPort(MappedObjectId id,
                 const QString &name,
                 PluginPort::PortType type,
                 PluginPort::PortRange range,
                 PortData lowerBound,
                 PortData upperBound);

    PortIterator begin() { return m_ports.begin(); }
    PortIterator end() { return m_ports.end(); }

protected:

    MappedObjectId             m_id;
    QString                    m_name;
    unsigned long              m_uniqueId;
    QString                    m_label;
    QString                    m_author;
    QString                    m_copyright;

    // our ports and associated hints
    std::vector<PluginPort*>   m_ports;

};

typedef std::vector<AudioPlugin*>::iterator PluginIterator;

class AudioPluginManager
{
public:
    AudioPluginManager();

    AudioPlugin* addPlugin(MappedObjectId id,
                           const QString &name,
                           unsigned long uniqueId,
                           const QString &label,
                           const QString &author,
                           const QString &copyright);

    bool removePlugin(MappedObjectId id);

    // Get a straight list of names
    //
    std::vector<QString> getPluginNames();

    // Some useful members
    //
    AudioPlugin* getPlugin(int number);
    AudioPlugin* getPluginByUniqueId(unsigned long uniqueId);
    int getPositionByUniqueId(unsigned long uniqueId);

    PluginIterator begin() { return m_plugins.begin(); }
    PluginIterator end() { return m_plugins.end(); }

    // Sample rate
    //
    void setSampleRate(unsigned int rate) { m_sampleRate = rate; }
    unsigned int getSampleRate() const { return m_sampleRate; }

protected:
    void fetchSampleRate();

    std::vector<AudioPlugin*> m_plugins;

    unsigned int              m_sampleRate;

};

}

#endif // _AUDIOPLUGINMANAGER_H_
