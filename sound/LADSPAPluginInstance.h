// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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
#include <set>
#include "config.h"
#include "Instrument.h"

#ifndef _LADSPAPLUGIN_H_
#define _LADSPAPLUGIN_H_

#ifdef HAVE_LADSPA

#include <ladspa.h>
#include "RunnablePluginInstance.h"

namespace Rosegarden
{

// LADSPA plugin instance.  LADSPA is a variable block size API, but
// for one reason and another it's more convenient to use a fixed
// block size in this wrapper.
//
class LADSPAPluginInstance : public RunnablePluginInstance
{
public:
    // Constructor that creates the buffers internally
    // 
    LADSPAPluginInstance(Rosegarden::InstrumentId instrument,
                         unsigned long ladspaId,
                         int position,
			 unsigned long sampleRate,
			 size_t bufferSize,
			 int idealChannelCount,
                         const LADSPA_Descriptor* descriptor);

    // Constructor that uses shared buffers
    // 
    LADSPAPluginInstance(Rosegarden::InstrumentId instrument,
                         unsigned long ladspaId,
                         int position,
			 unsigned long sampleRate,
			 size_t bufferSize,
			 sample_t **inputBuffers,
			 sample_t **outputBuffers,
                         const LADSPA_Descriptor* descriptor);

    virtual ~LADSPAPluginInstance();

    bool isOK() const { return m_instanceHandles.size() != 0; }

    Rosegarden::InstrumentId getInstrument() const { return m_instrument; }
    unsigned long getLADSPAId() const { return m_ladspaId; }
    int getPosition() const { return m_position; }

    // Set control ports
    //
    virtual void setPortValue(unsigned int portNumber, float value);

    // RunnablePluginInstance API
    //
    virtual void run();
    virtual size_t getBufferSize() { return m_bufferSize; }
    virtual size_t getAudioInputCount() { return m_instanceCount * m_audioPortsIn.size(); }
    virtual size_t getAudioOutputCount() { return m_instanceCount * m_audioPortsOut.size(); }
    virtual sample_t **getAudioInputBuffers() { return m_inputBuffers; }
    virtual sample_t **getAudioOutputBuffers() { return m_outputBuffers; }

    // Plugin control
    //
    void activate();
    void deactivate();

    virtual bool isBypassed() const { return m_bypassed; }
    virtual void setBypassed(bool bypassed) { m_bypassed = bypassed; }

    // Order by instrument and then position
    //
    struct PluginCmp
    {
        bool operator()(const LADSPAPluginInstance *p1,
                        const LADSPAPluginInstance *p2)
        {
            if (p1->getInstrument() != p2->getInstrument())
                return p1->getInstrument() < p2->getInstrument();
            else
                return p1->getPosition() < p2->getPosition();
        }
    };

protected:
    void init(int idealChannelCount = 0);
    void instantiate(unsigned long sampleRate);
    void cleanup();

    // Connection of data (and behind the scenes control) ports
    //
    void connectPorts();
    
    Rosegarden::InstrumentId   m_instrument;
    unsigned long              m_ladspaId;
    int                        m_position;
    std::vector<LADSPA_Handle> m_instanceHandles;
    int                        m_instanceCount;
    const LADSPA_Descriptor   *m_descriptor;

    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPorts;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    size_t                    m_bufferSize;
    sample_t                **m_inputBuffers;
    sample_t                **m_outputBuffers;
    bool                      m_ownBuffers;
    
    bool                      m_bypassed;
};

typedef std::vector<LADSPAPluginInstance*> PluginInstances;

typedef std::multiset<LADSPAPluginInstance*,
		      LADSPAPluginInstance::PluginCmp> OrderedPluginList;

};

#endif // HAVE_LADSPA

#endif // _LADSPAPLUGIN_H_

