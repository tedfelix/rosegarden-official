 
// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

namespace Rosegarden
{
	
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

    virtual void run() = 0;
    
    virtual size_t getBufferSize() = 0;

    virtual size_t getAudioInputCount() = 0;
    virtual size_t getAudioOutputCount() = 0;

    virtual sample_t **getAudioInputBuffers() = 0;
    virtual sample_t **getAudioOutputBuffers() = 0;

    virtual void setPortValue(unsigned int port, float value) = 0;

    bool isBypassed() const { return m_bypassed; }
    void setBypassed(bool value) { m_bypassed = value; }

private:
    bool m_bypassed;
};

typedef std::vector<RunnablePluginInstance *> RunnablePluginInstances;

}

#endif
