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
#include "RingBuffer.h"
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

    virtual void run(const RealTime &);

    virtual void setPortValue(unsigned int portNumber, float value);
    virtual void configure(QString key, QString value);
    virtual void sendEvent(const RealTime &eventTime,
			   const snd_seq_event_t *event);

    virtual size_t getBufferSize() { return m_blockSize; }
    virtual size_t getAudioInputCount() { return m_audioPortsIn.size(); }
    virtual size_t getAudioOutputCount() { return m_idealChannelCount; }
    virtual sample_t **getAudioInputBuffers() { return m_inputBuffers; }
    virtual sample_t **getAudioOutputBuffers() { return m_outputBuffers; }

    virtual QStringList getPrograms();
    virtual QString getCurrentProgram();
    virtual QString getProgram(int bank, int program);
    virtual unsigned long getProgram(QString name);
    virtual void selectProgram(QString program);

    virtual bool isBypassed() const { return m_bypassed; }
    virtual void setBypassed(bool bypassed) { m_bypassed = bypassed; }

    virtual void silence();
    virtual void setIdealChannelCount(size_t channels); // may re-instantiate

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
		       size_t blockSize,
		       int idealChannelCount,
		       const DSSI_Descriptor* descriptor);
    
    // Constructor that uses shared buffers
    // 
    DSSIPluginInstance(PluginFactory *factory,
		       Rosegarden::InstrumentId instrument,
		       QString identifier,
		       int position,
		       unsigned long sampleRate,
		       size_t blockSize,
		       sample_t **inputBuffers,
		       sample_t **outputBuffers,
		       const DSSI_Descriptor* descriptor);

    void init();
    void instantiate(unsigned long sampleRate);
    void cleanup();
    void activate();
    void deactivate();

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

    RingBuffer<snd_seq_event_t> m_eventBuffer;

    size_t                    m_blockSize;
    sample_t                **m_inputBuffers;
    sample_t                **m_outputBuffers;
    bool                      m_ownBuffers;
    size_t                    m_idealChannelCount;
    size_t                    m_outputBufferCount;
    size_t                    m_sampleRate;
    
    bool                      m_bypassed;
    QString                   m_program;
};

};

#endif // HAVE_DSSI

#endif // _DSSIPLUGININSTANCE_H_

