
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

#ifndef RG_AUDIOMIXERWINDOW_H
#define RG_AUDIOMIXERWINDOW_H

#include "MixerWindow.h"
#include "gui/general/ActionFileClient.h"

#include <QPixmap>

#include <vector>
#include <map>


class QWidget;
class QPushButton;
class QHBoxLayout;
class QFrame;


namespace Rosegarden
{

class Rotary;
class RosegardenDocument;
class MappedEvent;
class Fader;
class AudioVUMeter;
class AudioRouteMenu;
class PluginPushButton;
class Instrument;


/// The "Audio Mixer" window.
class AudioMixerWindow : public MixerWindow, public ActionFileClient
{
    Q_OBJECT

public:
    AudioMixerWindow(QWidget *parent, RosegardenDocument *document);
    ~AudioMixerWindow();

    /// Update all meters with playback levels from SequencerDataBlock.
    void updateMeters();
    /// Update the input meters with record levels from SequencerDataBlock.
    void updateMonitorMeters();

public slots:
    /// Handle events from the external controller port.
    /**
     * @see RosegardenMainViewWidget::slotControllerDeviceEventReceived()
     * @see MidiMixerWindow::slotControllerDeviceEventReceived()
     */
    void slotControllerDeviceEventReceived(MappedEvent *,
                                           const void *preferredCustomer);

signals:
    /// Emitted when a plugin button is clicked.
    /**
     * Connected to RosegardenMainWindow::slotShowPluginDialog().
     */
    void selectPlugin(QWidget *, InstrumentId id, int index);

    // The following signals are emitted when the buttons are pressed.
    // RosegardenMainWindow::slotOpenAudioMixer() connects these to
    // appropriate handler slots.

    void play();
    void stop();
    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();
    void record();
    void panic();

protected slots:

    // The following slots are connected to the various widgets.

    void slotFaderLevelChanged(float level);
    void slotPanChanged(float value);
    void slotChannelsChanged();
    void slotSoloChanged();
    void slotMuteChanged();
    void slotRecordChanged();

    /// Handler for plugin button press.  Emits selectPlugin().
    void slotSelectPlugin();

    /// Used only by the alias button to force a refresh of the widgets.
    void slotRepopulate();

    // The following slots handle updates from the outside.

    // ??? These all (except maybe slotInstrumentChanged()) need to be
    //     removed.  Observer pattern should be used to monitor changes to
    //     the Composition.  In response, an updateWidgets() routine should
    //     be called which will update all widgets *efficiently*.
    // ??? Perhaps a special path/routine could be used for volume control
    //     changes since they might be relatively high-frequency when the
    //     user is dragging a volume control around.  This probably won't
    //     be necessary, though.

    /// Called when an Instrument changes.
    void slotInstrumentChanged(Instrument *);

    /// Ensure the faders are in sync with the document.
    /**
     * In case a Track's Instrument has been changed, this routine updates
     * the relevant fader.
     *
     * Called by RosegardenMainViewWidget::slotUpdateInstrumentParameterBox().
     *
     * ??? The connection is made by RMW.  This should be public.
     */
    void slotTrackAssignmentsChanged();

    /// Update the label for the plugin.
    /**
     * RosegardenMainWindow::pluginSelected() signal is connected to this by
     * RosegardenMainWindow::slotOpenAudioMixer().
     */
    void slotPluginSelected(InstrumentId id, int index, int plugin);
    /// Update the PluginPushButton's state.
    /**
     * RosegardenMainWindow::pluginBypassed() signal is connected to this by
     * RosegardenMainWindow::slotOpenAudioMixer().
     */
    void slotPluginBypassed(InstrumentId id, int pluginIndex, bool bypass);

    /// Handle the inputs_1, inputs_2, inputs_4, inputs_8, and inputs_16 actions.
    void slotSetInputCountFromAction();
    /// Handle the submasters_0, submasters_2, submasters_4, and submasters_8 actions.
    void slotSetSubmasterCountFromAction();
    /// Handle the panlaw_0, panlaw_1, panlaw_2, and panlaw_3 actions.
    void slotSetPanLaw();

    // ??? Do any of these names, slots or actions, actually match the UI?
    //     If not, make them match!

    /// Handle show_audio_faders action.
    void slotToggleFaders();
    /// Handle show_synth_faders action.
    void slotToggleSynthFaders();
    /// Handle show_audio_submasters action.
    void slotToggleSubmasters();
    /// Handle show_plugin_buttons action.
    void slotTogglePluginButtons();
    /// Handle show_unassigned_faders action.
    void slotToggleUnassignedFaders();

    /// Handle mixer_help action.
    void slotHelpRequested();
    /// Handle help_about_app action.
    void slotHelpAbout();

protected:
    /// Send MIDI volume and pan messages to each device.
    /**
     * MixerWindow override.  Called by MixerWindow::windowActivationChange().
     */
    virtual void sendControllerRefresh();

private:
    /// show_audio_faders
    void updateFaderVisibility();
    /// show_synth_faders
    void updateSynthFaderVisibility();
    /// show_audio_submasters
    void updateSubmasterVisibility();
    /// show_plugin_buttons
    void updatePluginButtonVisibility();

    /// Show/Hide widgets by name.  NOT WORKING.
    void toggleNamedWidgets(bool show, const char *);

    /// A vertical strip of controls representing a mixer channel.
    /**
     * ??? This should derive from QWidget and act as a single widget.
     *     It should manage all the widgets inside of it.  That should
     *     reduce the burden on the client.  Then this should be moved
     *     out of AudioMixerWindow and renamed AudioMixerStrip.
     */
    struct Strip {

        Strip() :
            m_populated(false),
            m_input(0), m_output(0), m_pan(0), m_fader(0), m_meter(0),
            m_recordButton(0), m_stereoButton(0), m_stereoness(false),
            m_pluginBox(0)
        { }

        void setVisible(bool);
        void setPluginButtonsVisible(bool);
        
        bool m_populated;

        AudioRouteMenu *m_input;
        AudioRouteMenu *m_output;

        Rotary *m_pan;
        Fader *m_fader;
        AudioVUMeter *m_meter;

        QPushButton *m_recordButton;
        QPushButton *m_stereoButton;
        bool m_stereoness;

        QWidget *m_pluginBox;
        std::vector<PluginPushButton *> m_plugins;

    private:
        // Hide copy ctor and op=
        // ??? Unfortunately, these are required for std::vector and
        //     std::map.  Recommend storing QSharedPointer<Strip> instead.
        //     It's a reasonable trade-off I think.
        //Strip(const Strip &);
        //Strip &operator=(const Strip &);
    };

    QWidget *m_surroundBox;
    QHBoxLayout *m_surroundBoxLayout;
    QFrame *m_mainBox;

    // Input strips.
    typedef std::map<InstrumentId, Strip> StripMap;
    StripMap m_inputs;

    typedef std::vector<Strip> StripVector;
    StripVector m_submasters;

    Strip m_monitor;
    Strip m_master;

    void depopulate();
    void populate();

    bool isInstrumentAssigned(InstrumentId id);

    void updateFader(int id); // instrument id if large enough, monitor if -1, master/sub otherwise
    void updateRouteButtons(int id);
    void updateStereoButton(int id);
    void updatePluginButtons(int id);
    void updateMiscButtons(int id);

    QPixmap m_monoPixmap;
    QPixmap m_stereoPixmap;

    void setRewFFwdToAutoRepeat();
};



}

#endif
