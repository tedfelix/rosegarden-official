// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

#include <vector>
#include <set>
#include "config.h"
#include "Instrument.h"

#ifndef _LADSPAPLUGIN_H_
#define _LADSPAPLUGIN_H_

#ifdef HAVE_LADSPA

#include <ladspa.h>

namespace Rosegarden
{

// LADSPA plugin instance
//
class LADSPAPluginInstance
{
public:
    LADSPAPluginInstance(Rosegarden::InstrumentId instrument,
                         unsigned long ladspaId,
                         int position,
                         const LADSPA_Descriptor* descriptor);

    ~LADSPAPluginInstance();

    Rosegarden::InstrumentId getInstrument() const { return m_instrument; }
    unsigned long getLADSPAId() const { return m_ladspaId; }
    int getPosition() const { return m_position; }

    // Connection of data (and behind the scenes control) ports
    //
    void connectPorts(LADSPA_Data *dataIn1,
                      LADSPA_Data *dataIn2,
                      LADSPA_Data *dataOut1,
                      LADSPA_Data *dataOut2);

    // Set control ports
    //
    void setPortValue(unsigned long portNumber, LADSPA_Data value);

    // Plugin control
    //
    void instantiate(unsigned long sampleRate);
    void activate();
    void run(unsigned long sampleCount);
    void deactivate();
    void cleanup();

    // Audio channels out to mix
    unsigned int getAudioChannelsOut() const { return m_audioPortsOut.size(); }

    // During operation we need to know which plugins have
    // already been processed as part of the playable audio
    // file loop and which should still be run() outside 
    // this loop.
    //
    bool hasBeenProcessed() const { return m_processed; }
    void setProcessed(bool value) { m_processed = value; }

    // Do we want to bypass this plugin?
    //
    bool isBypassed() const { return m_bypassed; }
    void setBypassed(bool value) { m_bypassed = value; }

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
    
    Rosegarden::InstrumentId  m_instrument;
    unsigned long             m_ladspaId;
    int                       m_position;
    LADSPA_Handle             m_instanceHandle;
    const LADSPA_Descriptor  *m_descriptor;

    std::vector<std::pair<unsigned long, LADSPA_Data*> > m_controlPorts;

    std::vector<int>          m_audioPortsIn;
    std::vector<int>          m_audioPortsOut;

    bool                      m_processed;
    bool                      m_bypassed;

};

typedef std::vector<LADSPAPluginInstance*> PluginInstances;
typedef std::vector<LADSPAPluginInstance*>::iterator PluginIterator;
typedef std::multiset<LADSPAPluginInstance*,
          LADSPAPluginInstance::PluginCmp> OrderedPluginList;
typedef std::multiset<LADSPAPluginInstance*,
          LADSPAPluginInstance::PluginCmp>::iterator OrderedPluginIterator;

};

#endif // HAVE_LADSPA

#endif // _LADSPAPLUGIN_H_

