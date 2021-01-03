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

#include <vector>
#include <set>
#include <QString>
#include "base/Instrument.h"

#ifndef RG_LADSPAPLUGININSTANCE_H
#define RG_LADSPAPLUGININSTANCE_H

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
    ~LADSPAPluginInstance() override;

    bool isOK() const override { return !m_instanceHandles.empty(); }

    InstrumentId getInstrument() const { return m_instrument; }
    QString getIdentifier() const override { return m_identifier; }
    int getPosition() const { return m_position; }

    void run(const RealTime &rt) override;

    void setPortValue(unsigned int portNumber, float value) override;
    float getPortValue(unsigned int portNumber) override;

    size_t getBufferSize() override { return m_blockSize; }
    size_t getAudioInputCount() override { return m_instanceCount * m_audioPortsIn.size(); }
    size_t getAudioOutputCount() override { return m_instanceCount * m_audioPortsOut.size(); }
    sample_t **getAudioInputBuffers() override { return m_inputBuffers; }
    sample_t **getAudioOutputBuffers() override { return m_outputBuffers; }

    bool isBypassed() const override { return m_bypassed; }
    void setBypassed(bool bypassed) override { m_bypassed = bypassed; }

    size_t getLatency() override;

    void silence() override;
    void setIdealChannelCount(size_t channels) override; // may re-instantiate

protected:
    // To be constructed only by LADSPAPluginFactory
    friend class LADSPAPluginFactory;

    // Constructor that creates the buffers internally
    // 
    LADSPAPluginInstance(PluginFactory *factory,
                         InstrumentId instrument,
                         QString identifier,
                         int position,
                         unsigned long sampleRate,
                         size_t blockSize,
                         int idealChannelCount,
                         const LADSPA_Descriptor* descriptor);

    // Constructor that uses shared buffers
    // 
    LADSPAPluginInstance(PluginFactory *factory,
                         InstrumentId instrument,
                         QString identifier,
                         int position,
                         unsigned long sampleRate,
                         size_t blockSize,
                         sample_t **inputBuffers,
                         sample_t **outputBuffers,
                         const LADSPA_Descriptor* descriptor);

    void init(int idealChannelCount = 0);
    void instantiate(unsigned long sampleRate);
    void cleanup();
    void activate();
    void deactivate();

    // Connection of data (and behind the scenes control) ports
    //
    void connectPorts();
    
    InstrumentId   m_instrument;
    int                        m_position;
    std::vector<LADSPA_Handle> m_instanceHandles;
    size_t                     m_instanceCount;
    const LADSPA_Descriptor   *m_descriptor;

    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsIn;
    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsOut;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    size_t                    m_blockSize;
    sample_t                **m_inputBuffers;
    sample_t                **m_outputBuffers;
    bool                      m_ownBuffers;
    size_t                    m_sampleRate;
    float                    *m_latencyPort;
    bool                      m_run;
    
    bool                      m_bypassed;
};

}

#endif // RG_LADSPAPLUGININSTANCE_H

