/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PLUGINCONTAINER_H
#define RG_PLUGINCONTAINER_H

#include <string>
#include <vector>

namespace Rosegarden
{


class AudioPluginInstance;
// ??? Use QSharedPointer.
typedef std::vector<AudioPluginInstance *> AudioPluginVector;

/// Essentially a vector of AudioPluginInstance.  ABC.
class PluginContainer
{
public:
    static constexpr unsigned PLUGIN_COUNT = 5; // for non-synth plugins

    AudioPluginVector::iterator beginPlugins() { return m_audioPlugins.begin(); }
    AudioPluginVector::iterator endPlugins() { return m_audioPlugins.end(); }

    // Plugin management
    void addPlugin(AudioPluginInstance *instance);
    bool removePlugin(unsigned int position);
    void clearPlugins();
    void emptyPlugins(); // empty the plugins but don't clear them down

    // Get a plugin for this container
    AudioPluginInstance *getPlugin(unsigned int position) const;

    virtual unsigned int getId() const = 0;
    virtual std::string getName() const = 0;
    virtual std::string getPresentationName() const = 0;
    virtual std::string getAlias() const = 0;

protected:
    PluginContainer(bool havePlugins);
    virtual ~PluginContainer();

    AudioPluginVector m_audioPlugins;
};


}

#endif
