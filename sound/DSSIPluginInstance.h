// -*- c-basic-offset: 4 -*-

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
#include <qstring.h>
#include "config.h"
#include "Instrument.h"

#ifndef _DSSIPLUGININSTANCE_H_
#define _DSSIPLUGININSTANCE_H_

#ifdef HAVE_DSSI

#include <dssi.h>
#include "RunnablePluginInstance.h"

namespace Rosegarden
{

class DSSIPluginInstance : public RunnablePluginInstance
{
public:
    virtual ~DSSIPluginInstance();

    virtual bool isOK() const { return m_instanceHandle != 0; }

    Rosegarden::InstrumentId getInstrument() const { return m_instrument; }
    virtual QString getIdentifier() const { return m_identifier; }
    int getPosition() const { return m_position; }

    // Set control ports
    //
    virtual void setPortValue(unsigned int portNumber, float value);

    // RunnablePluginInstance API
    //
    virtual void run();
    virtual size_t getBufferSize() { return m_bufferSize; }
    virtual size_t getAudioInputCount() { return m_audioPortsIn.size(); }
    virtual size_t getAudioOutputCount() { return m_idealChannelCount; }
    virtual sample_t **getAudioInputBuffers() { return m_inputBuffers; }
    virtual sample_t **getAudioOutputBuffers() { return m_outputBuffers; }

    virtual void activate();
    virtual void deactivate();

    virtual bool isBypassed() const { return m_bypassed; }
    virtual void setBypassed(bool bypassed) { m_bypassed = bypassed; }

    virtual void setIdealChannelCount(unsigned long sampleRate,
				      int channels); // may re-instantiate

protected:
    // To be constructed only by DSSIPluginFactory
    friend class DSSIPluginFactory;

    // Constructor that creates the buffers internally
    // 
    DSSIPluginInstance(PluginFactory *factory,
		       Rosegarden::InstrumentId instrument,
		       QString identifier,
		       int position,
		       unsigned long sampleRate,
		       size_t bufferSize,
		       int idealChannelCount,
		       const DSSI_Descriptor* descriptor);
    
    // Constructor that uses shared buffers
    // 
    DSSIPluginInstance(PluginFactory *factory,
		       Rosegarden::InstrumentId instrument,
		       QString identifier,
		       int position,
		       unsigned long sampleRate,
		       size_t bufferSize,
		       sample_t **inputBuffers,
		       sample_t **outputBuffers,
		       const DSSI_Descriptor* descriptor);

    void init();
    void instantiate(unsigned long sampleRate);
    void cleanup();

    // Connection of data (and behind the scenes control) ports
    //
    void connectPorts();
    
    Rosegarden::InstrumentId   m_instrument;
    int                        m_position;
    LADSPA_Handle              m_instanceHandle;
    const DSSI_Descriptor     *m_descriptor;

    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsIn;
    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsOut;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    size_t                    m_bufferSize;
    sample_t                **m_inputBuffers;
    sample_t                **m_outputBuffers;
    bool                      m_ownBuffers;
    int                       m_idealChannelCount;
    int                       m_outputBufferCount;
    
    bool                      m_bypassed;
};

};

#endif // HAVE_DSSI

#endif // _DSSIPLUGININSTANCE_H_

