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

#ifndef _RUNNABLE_PLUGIN_INSTANCE_H_
#define _RUNNABLE_PLUGIN_INSTANCE_H_

#include <qstring.h>
#include <qstringlist.h>
#include <vector>

#include "RealTime.h"

namespace Rosegarden
{

class PluginFactory;
	
/**
 * RunnablePluginInstance is a very trivial interface that an audio
 * process can use to refer to an instance of a plugin without needing
 * to know what type of plugin it is.
 *
 * The audio code calls run() on an instance that has been passed to
 * it, and assumes that the passing code has already initialised the
 * plugin, connected its inputs and outputs and so on, and that there
 * is an understanding in place about the sizes of the buffers in use
 * by the plugin.  All of this depends on the subclass implementation.
 */

class RunnablePluginInstance
{
public:
    typedef float sample_t;

    virtual ~RunnablePluginInstance();

    virtual bool isOK() const = 0;

    virtual QString getIdentifier() const = 0;

    /**
     * Run for one block, starting at the given time.  The start time
     * may be of interest to synths etc that may have queued events
     * waiting.  Other plugins can ignore it.
     */
    virtual void run(const RealTime &blockStartTime) = 0;
    
    virtual size_t getBufferSize() = 0;

    virtual size_t getAudioInputCount() = 0;
    virtual size_t getAudioOutputCount() = 0;

    virtual sample_t **getAudioInputBuffers() = 0;
    virtual sample_t **getAudioOutputBuffers() = 0;

    virtual QStringList getPrograms() { return QStringList(); }
    virtual QString getCurrentProgram() { return QString(); }
    virtual QString getProgram(int /* bank */, int /* program */) { return QString(); }
    virtual unsigned long getProgram(QString /* name */) { return 0; } // bank << 16 + program
    virtual void selectProgram(QString) { }

    virtual void setPortValue(unsigned int port, float value) = 0;
    virtual float getPortValue(unsigned int port) = 0;

    virtual QString configure(QString /* key */, QString /* value */) { return QString(); }

    virtual void sendEvent(const RealTime & /* eventTime */,
			   const void * /* event */) { }

    virtual bool isBypassed() const = 0;
    virtual void setBypassed(bool value) = 0;

    // This should be called after setup, but while not actually playing.
    virtual size_t getLatency() = 0;

    virtual void silence() = 0;
    virtual void setIdealChannelCount(size_t channels) = 0; // must also silence(); may also re-instantiate

    void setFactory(PluginFactory *f) { m_factory = f; } // ew

protected:
    RunnablePluginInstance(PluginFactory *factory, QString identifier) :
	m_factory(factory), m_identifier(identifier) { }

    PluginFactory *m_factory;
    QString m_identifier;

    friend class PluginFactory;
};

typedef std::vector<RunnablePluginInstance *> RunnablePluginInstances;

}

#endif
