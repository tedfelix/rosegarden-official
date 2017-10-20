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

class QHBoxLayout;
class QWidget;


namespace Rosegarden
{


class AudioStrip;
class MappedEvent;
class Instrument;


/// The "Audio Mixer" window (v2).
class AudioMixerWindow2 : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    AudioMixerWindow2(QWidget *parent);
    virtual ~AudioMixerWindow2();

signals:
    /// Let RMVW know we are now the active window for external controller events.
    /**
     * Connected to RosegardenMainViewWidget::slotActiveMainWindowChanged().
     */
    void windowActivated();

protected:
    void changeEvent(QEvent *event);

private slots:
    /// Connected to RosegardenDocument::documentModified(bool).
    void slotDocumentModified(bool modified);

    /// File > Close
    void slotClose();

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

    /// Event received on the "external controller" port.
    void slotExternalControllerEvent(
            MappedEvent *event,
            const void *preferredCustomer);

    /// Connected to InstrumentStaticSignals::controlChange().
    void slotControlChange(Instrument *instrument, int cc);

private:
    /// Central widget required for using a layout with QMainWindow.
    QWidget *m_centralWidget;
    /// Horizontal box layout that holds the strips.
    QHBoxLayout *m_layout;

    std::vector<AudioStrip *> m_inputStrips;
    std::vector<AudioStrip *> m_submasterStrips;
    AudioStrip *m_masterStrip;

    void updateStripCounts();
    void updateWidgets();

};


}

#endif
