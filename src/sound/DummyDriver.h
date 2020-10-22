/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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

#ifndef RG_DUMMYDRIVER_H
#define RG_DUMMYDRIVER_H

namespace Rosegarden
{


class PlayableAudioFile;


class DummyDriver : public SoundDriver
{
public:
    DummyDriver(MappedStudio *studio);
    DummyDriver(MappedStudio *studio, QString pastLog);

    void setPluginInstanceBypass(InstrumentId /*id*/,
                                         int /*position*/,
                                         bool /*value*/) override { }

    QStringList getPluginInstancePrograms(InstrumentId ,
                                                  int ) override { return QStringList(); }

    QString getPluginInstanceProgram(InstrumentId,
                                             int ) override { return QString(); }

    QString getPluginInstanceProgram(InstrumentId,
                                             int,
                                             int,
                                             int) override { return QString(); }

    unsigned long getPluginInstanceProgram(InstrumentId,
                                                   int ,
                                                   QString) override { return 0; }
    
    void setPluginInstanceProgram(InstrumentId,
                                          int ,
                                          QString ) override { }

    QString configurePlugin(InstrumentId,
                                    int,
                                    QString ,
                                    QString ) override { return QString(); }

    void setAudioBussLevels(int ,
                                    float ,
                                    float ) override { }

    void setAudioInstrumentLevels(InstrumentId,
                                          float,
                                          float) override { }

    void checkForNewClients() override  { }

    void setLoop(const RealTime &/*loopStart*/,
                         const RealTime &/*loopEnd*/) override { }

    QString getStatusLog() override;

    virtual std::vector<PlayableAudioFile*> getPlayingAudioFiles()
        { return std::vector<PlayableAudioFile*>(); }

    void getAudioInstrumentNumbers(InstrumentId &i, int &n) override {
        i = 0; n = 0;
    }
    void getSoftSynthInstrumentNumbers(InstrumentId &i, int &n) override {
        i = 0; n = 0;
    }

    void claimUnwantedPlugin(void */* plugin */) override { }
    void scavengePlugins() override { }

    bool areClocksRunning() const override { return true; }

protected:
    QString m_pastLog;
};

}

#endif // RG_DUMMYDRIVER_H

