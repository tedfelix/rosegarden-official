/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MIDIMIXERWINDOW_H
#define RG_MIDIMIXERWINDOW_H

#include "MixerWindow.h"

#include "base/Controllable.h"
#include "gui/general/ActionFileClient.h"

#include <memory>
#include <utility>
#include <vector>

class QWidget;
class QTabWidget;
class QString;
class QFrame;


namespace Rosegarden
{


class Fader;
class MappedEvent;
class MidiDevice;
class MidiMixerVUMeter;
class MidiStrip;
class RosegardenDocument;
class Rotary;


/// The MIDI Mixer.
class MidiMixerWindow : public MixerWindow, public ActionFileClient
{
    Q_OBJECT

public:

    MidiMixerWindow();

public slots:

    /// Used by DeviceManagerDialog to update Device names.
    /**
     * ??? I think this is going away.  We need to handle document modified
     *     and refresh like AMW2 does.  See AudioMixerWindow2::updateWidgets().
     */
    void slotSynchronise();

private slots:

    /// Calls sendControllerRefresh().
    void slotCurrentTabChanged(int);

    /// Handle events from the external controller port.
    /**
     * Modifies the Instrument on the currently displayed tab's Device.
     *
     * @see RosegardenMainViewWidget::slotExternalController()
     * @see AudioMixerWindow2::slotExternalController()
     */
    void slotExternalController(const MappedEvent *event);

    /// Help > Help
    void slotHelpRequested();
    /// Help > About Rosegarden
    void slotHelpAbout();

    /// File > Close
    void slotClose()  { close(); }

private:

    QTabWidget *m_tabWidget;

    /// Setup the tabs on the Mixer according to the Studio
    void setupTabs();

    // QWidget override.
    /// Calls sendControllerRefresh() in response to window activation.
    void changeEvent(QEvent *event) override;

    /// Send MIDI volume and pan messages to the "external controller" port.
    /**
     * This is called when the window is activated or the tab is changed.  It
     * allows the device connected to the "external controller" port to stay in
     * sync with whichever Mixer window is active.
     */
    void sendControllerRefresh();

    typedef std::vector<MidiStrip *> MidiStripVector;
    MidiStripVector m_midiStrips;

};


}

#endif
