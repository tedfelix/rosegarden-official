// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
/*
  Rosegarden
  A sequencer and musical notation editor.

  This program is Copyright 2000-2006
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

#include "SoundDriver.h"

// An empty sound driver for when we don't want sound support
// but still want to build the sequencer.
//

#ifndef _DUMMYDRIVER_H_
#define _DUMMYDRIVER_H_

namespace Rosegarden
{

class DummyDriver : public SoundDriver
{
public:
    DummyDriver(MappedStudio *studio):
        SoundDriver(studio, std::string("DummyDriver - no sound")) { }
    DummyDriver(MappedStudio *studio, const std::string & name):
        SoundDriver(studio, std::string("DummyDriver: " + name)) { }
    virtual ~DummyDriver() { }

    virtual bool initialise()  { m_recordComposition.clear(); return true; }
    virtual void initialisePlayback(const RealTime & /*position*/) { }
    virtual void stopPlayback() { }
    virtual void punchOut() { }
    virtual void resetPlayback(const RealTime & /*old position*/,
                               const RealTime & /*position*/) { }
    virtual void allNotesOff()  { }
    
    virtual RealTime getSequencerTime() { return RealTime(0, 0);}

    virtual MappedComposition* getMappedComposition()
        { return &m_recordComposition;}

    virtual void processEventsOut(const MappedComposition & /*mC*/) { }

    virtual void processEventsOut(const MappedComposition &,
                                  const RealTime &,
                                  const RealTime &) { }

    // Activate a recording state
    //
    virtual bool record(RecordStatus /*recordStatus*/,
                        const std::vector<InstrumentId> */*armedInstruments = 0*/,
                        const std::vector<QString> */*audioFileNames = 0*/)
        { return false; }

    // Process anything that's pending
    //
    virtual void processPending() { }

    // Sample rate
    //
    virtual unsigned int getSampleRate() const { return 0; }

    // Return the last recorded audio level
    //
    virtual float getLastRecordedAudioLevel() { return 0.0; }

    // Plugin instance management
    //
    virtual void setPluginInstance(InstrumentId /*id*/,
                                   QString /*pluginIdent*/,
                                   int /*position*/) { }

    virtual void removePluginInstance(InstrumentId /*id*/,
                                      int /*position*/) { }

    virtual void removePluginInstances() { }

    virtual void setPluginInstancePortValue(InstrumentId /*id*/,
                                            int /*position*/,
                                            unsigned long /*portNumber*/,
                                            float /*value*/) { }

    virtual float getPluginInstancePortValue(InstrumentId ,
                                             int ,
                                             unsigned long ) { return 0; }

    virtual void setPluginInstanceBypass(InstrumentId /*id*/,
                                         int /*position*/,
                                         bool /*value*/) { }

    virtual QStringList getPluginInstancePrograms(InstrumentId ,
                                                  int ) { return QStringList(); }

    virtual QString getPluginInstanceProgram(InstrumentId,
                                             int ) { return QString(); }

    virtual QString getPluginInstanceProgram(InstrumentId,
                                             int,
                                             int,
                                             int) { return QString(); }

    virtual unsigned long getPluginInstanceProgram(InstrumentId,
                                                   int ,
                                                   QString) { return 0; }
    
    virtual void setPluginInstanceProgram(InstrumentId,
                                          int ,
                                          QString ) { }

    virtual QString configurePlugin(InstrumentId,
                                    int,
                                    QString ,
                                    QString ) { return QString(); }

    virtual void setAudioBussLevels(int ,
                                    float ,
                                    float ) { }

    virtual void setAudioInstrumentLevels(InstrumentId,
                                          float,
                                          float) { }

    virtual bool checkForNewClients() { return false; }

    virtual void setLoop(const RealTime &/*loopStart*/,
                         const RealTime &/*loopEnd*/) { }

    virtual std::vector<PlayableAudioFile*> getPlayingAudioFiles()
        { return std::vector<PlayableAudioFile*>(); }

    virtual void getAudioInstrumentNumbers(InstrumentId &i, int &n) {
        i = 0; n = 0;
    }
    virtual void getSoftSynthInstrumentNumbers(InstrumentId &i, int &n) {
        i = 0; n = 0;
    }

    virtual void claimUnwantedPlugin(void *plugin) { }
    virtual void scavengePlugins() { }

    virtual bool areClocksRunning() const { return true; }

protected:
    virtual void processMidiOut(const MappedComposition & /*mC*/,
                                const RealTime &, const RealTime &) { }
    virtual void generateInstruments()  { }

};

}

#endif // _DUMMYDRIVER_H_

