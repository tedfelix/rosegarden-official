/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioMixerWindow2]"

#include "AudioMixerWindow2.h"

#include "misc/Debug.h"
#include "gui/general/IconLoader.h"


namespace Rosegarden
{


AudioMixerWindow2::AudioMixerWindow2(QWidget *parent) :
        QMainWindow(parent)
{
    setObjectName("AudioMixerWindow2");

    setWindowTitle(tr("Audio Mixer"));
    setWindowIcon(IconLoader().loadPixmap("window-audiomixer"));

    // This avoids using CPU when we're "closed".  Normally with Qt,
    // "closed" really just means "hidden".
    setAttribute(Qt::WA_DeleteOnClose);

    createAction("file_close", SLOT(slotClose()));

#if 0
    // Transport ToolBar
    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("record", SIGNAL(record()));
    createAction("panic", SIGNAL(panic()));

    // Help > Help
    createAction("mixer_help", SLOT(slotHelp()));
    // Help > About Rosegarden
    createAction("help_about_app", SLOT(slotAboutRosegarden()));

    // Settings > Show Audio Faders
    createAction("show_audio_faders", SLOT(slotShowAudioFaders()));

    // Settings > Show Synth Faders
    createAction("show_synth_faders", SLOT(slotShowSynthFaders()));

    // Settings > Show Audio Submasters
    createAction("show_audio_submasters", SLOT(slotShowAudioSubmasters()));

    // Settings > Show Plugin Buttons
    createAction("show_plugin_buttons", SLOT(slotShowPluginButtons()));

    // Settings > Show Unassigned Faders
    createAction("show_unassigned_faders", SLOT(slotShowUnassignedFaders()));

    // "Settings > Number of Stereo Inputs" Actions
    // For i in {1,2,4,8,16}
    for (int i = 1; i <= 16; i *= 2) {
        createAction(QString("inputs_%1").arg(i),
                     SLOT(slotNumberOfStereoInputs()));
    }

    // "Settings > Number of Submasters" Actions
    createAction("submasters_0", SLOT(slotNumberOfSubmasters()));
    createAction("submasters_2", SLOT(slotNumberOfSubmasters()));
    createAction("submasters_4", SLOT(slotNumberOfSubmasters()));
    createAction("submasters_8", SLOT(slotNumberOfSubmasters()));

    // "Settings > Panning Law" Actions
    // ??? Can we do something easier to understand than panlaw_0,
    //     panlaw_1, etc...?  E.g. panlaw_0dB, panlaw_-3dB, panlaw_-6dB,
    //     panlaw_alt-3dB.
    createAction("panlaw_0", SLOT(slotPanningLaw()));
    createAction("panlaw_1", SLOT(slotPanningLaw()));
    createAction("panlaw_2", SLOT(slotPanningLaw()));
    createAction("panlaw_3", SLOT(slotPanningLaw()));
#endif

    createMenusAndToolbars("mixer.rc");

#if 0
    // The action->setChecked() stuff must be done after createMenusAndToolbars().

    // Update "Settings > Number of Stereo Inputs"
    findAction(QString("inputs_%1").arg(m_studio->getRecordIns().size()))->
            setChecked(true);

    // Update "Settings > Number of Submasters"
    findAction(QString("submasters_%1").arg(m_studio->getBusses().size()-1))->
            setChecked(true);

    // Update "Settings > Panning Law"
    findAction(QString("panlaw_%1").arg(AudioLevel::getPanLaw()))->
            setChecked(true);

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "playback_pointer_back_bar");
    enableAutoRepeat("Transport Toolbar", "playback_pointer_forward_bar");
#endif

    show();
}

AudioMixerWindow2::~AudioMixerWindow2()
{
}

void
AudioMixerWindow2::slotClose()
{
    close();
}


}
