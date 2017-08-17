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

#ifndef RG_AUDIOMIXERWINDOW2_H
#define RG_AUDIOMIXERWINDOW2_H

#include "gui/general/ActionFileClient.h"

#include <QMainWindow>

class QWidget;


namespace Rosegarden
{


class AudioStrip;


/// The "Audio Mixer" window (v2).
class AudioMixerWindow2 : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    AudioMixerWindow2(QWidget *parent);
    virtual ~AudioMixerWindow2();

private slots:
    /// Connected to RosegardenDocument::documentModified(bool).
    void slotDocumentModified(bool modified);

    /// File > Close
    void slotClose();

    /// Transport > Play
    void slotPlay();

    /// Transport > Stop
    void slotStop();

    /// Transport > Rewind
    void slotRewind();

    /// Transport > Fast Forward
    void slotFastforward();

    /// Transport > Rewind To Beginning
    void slotRewindToBeginning();

    /// Transport > Fast Forward To End
    void slotFastForwardToEnd();

    /// Transport > Record
    void slotRecord();

    /// Transport > Panic
    void slotPanic();

    /// Settings > Number of Stereo Inputs
    void slotNumberOfStereoInputs();

    /// Settings > Number of Submasters
    void slotNumberOfSubmasters();

    /// Settings > Panning Law
    void slotPanningLaw();

    /// Settings > Show Audio Faders
    void slotShowAudioFaders();

    /// Settings > Show Synth Faders
    void slotShowSynthFaders();

    /// Settings > Show Audio Submasters
    void slotShowAudioSubmasters();

    // Settings > Show Plugin Buttons
    void slotShowPluginButtons();

    /// Settings > Show Unassigned Faders
    void slotShowUnassignedFaders();

    /// Help > Help
    void slotHelp();

    /// Help > About Rosegarden
    void slotAboutRosegarden();

private:
    std::vector<AudioStrip *> m_inputStrips;
    std::vector<AudioStrip *> m_submasterStrips;
    AudioStrip *m_masterStrip;

    void updateStripCounts();
    void updateWidgets();

};


}

#endif
