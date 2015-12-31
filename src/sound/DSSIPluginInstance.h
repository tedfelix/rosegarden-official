/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_DSSIPLUGININSTANCE_H
#define RG_DSSIPLUGININSTANCE_H

#include <vector>
#include <set>
#include <map>
#include <QString>
#include "base/Instrument.h"

#include <dssi.h>
#include "RingBuffer.h"
#include "RunnablePluginInstance.h"
#include "Scavenger.h"
#include <pthread.h>

namespace Rosegarden
{

class DSSIPluginInstance : public RunnablePluginInstance
{
public:
    virtual ~DSSIPluginInstance();

    virtual bool isOK() const { return m_instanceHandle != 0; }

    InstrumentId getInstrument() const { return m_instrument; }
    virtual QString getIdentifier() const { return m_identifier; }
    int getPosition() const { return m_position; }

    virtual void run(const RealTime &);

    virtual void setPortValue(unsigned int portNumber, float value);
    virtual float getPortValue(unsigned int portNumber);
    virtual QString configure(QString key, QString value);
    virtual void sendEvent(const RealTime &eventTime,
                           const void *event);

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

    virtual size_t getLatency();

    virtual void silence();
    virtual void discardEvents();
    virtual void setIdealChannelCount(size_t channels); // may re-instantiate

    virtual bool isInGroup() const { return m_grouped; }
    virtual void detachFromGroup();

protected:
    // To be constructed only by DSSIPluginFactory
    friend class DSSIPluginFactory;

    // Constructor that creates the buffers internally
    // 
    DSSIPluginInstance(PluginFactory *factory,
                       InstrumentId instrument,
                       QString identifier,
                       int position,
                       unsigned long sampleRate,
                       size_t blockSize,
                       int idealChannelCount,
                       const DSSI_Descriptor* descriptor);
    
    // Constructor that uses shared buffers
    // 
    DSSIPluginInstance(PluginFactory *factory,
                       InstrumentId instrument,
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
    void connectPorts();

    bool handleController(snd_seq_event_t *ev);
    void setPortValueFromController(unsigned int portNumber, int controlValue);
    void selectProgramAux(QString program, bool backupPortValues);
    void checkProgramCache();

    void initialiseGroupMembership();
    void runGrouped(const RealTime &);

    InstrumentId   m_instrument;
    int                        m_position;
    LADSPA_Handle              m_instanceHandle;
    const DSSI_Descriptor     *m_descriptor;

    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsIn;
    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPortsOut;

    std::vector<LADSPA_Data>  m_backupControlPortsIn;
    std::vector<bool>  m_portChangedSinceProgramChange;

    std::map<int, int>        m_controllerMap;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    struct ProgramControl {
        int msb;
        int lsb;
        int program;
    };
    ProgramControl m_pending;

    struct ProgramDescriptor {
        int bank;
        int program;
        QString name;
    };
    std::vector<ProgramDescriptor> m_cachedPrograms;
    bool m_programCacheValid;

    RingBuffer<snd_seq_event_t> m_eventBuffer;

    size_t                    m_blockSize;
    sample_t                **m_inputBuffers;
    sample_t                **m_outputBuffers;
    bool                      m_ownBuffers;
    size_t                    m_idealChannelCount;
    size_t                    m_outputBufferCount;
    size_t                    m_sampleRate;
    float                    *m_latencyPort;

    bool                      m_run;
    bool                      m_runSinceReset;
    
    bool                      m_bypassed;
    QString                   m_program;
    bool                      m_grouped;
    RealTime                  m_lastRunTime;

    pthread_mutex_t           m_processLock;

    typedef std::set<DSSIPluginInstance *> PluginSet;
    typedef std::map<QString, PluginSet> GroupMap;
    static GroupMap m_groupMap;
    static snd_seq_event_t **m_groupLocalEventBuffers;
    static size_t m_groupLocalEventBufferCount;

    static Scavenger<ScavengerArrayWrapper<snd_seq_event_t *> > m_bufferScavenger;
};

};

#endif // RG_DSSIPLUGININSTANCE_H

