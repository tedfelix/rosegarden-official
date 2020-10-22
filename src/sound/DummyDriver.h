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

#ifndef RG_DUMMYDRIVER_H
#define RG_DUMMYDRIVER_H

namespace Rosegarden
{


class PlayableAudioFile;


/// Allow Rosegarden to run without a sound support.
class DummyDriver : public SoundDriver
{
public:
    DummyDriver(MappedStudio *studio);
    DummyDriver(MappedStudio *studio, QString pastLog);

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

