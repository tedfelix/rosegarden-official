
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

#include "base/MidiProgram.h"
#include "MixerWindow.h"
#include "gui/general/ActionFileClient.h"
#include "gui/widgets/PluginPushButton.h"

#include <QPixmap>
#include <QSharedPointer>

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
class InstrumentStaticSignals;


/// The "Audio Mixer" window.
class AudioMixerWindow : public MixerWindow, public ActionFileClient
{
    Q_OBJECT

public:
    AudioMixerWindow(QWidget *parent, RosegardenDocument *document);
    ~AudioMixerWindow();

    void updateMeters();
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
    void selectPlugin(QWidget *, InstrumentId id, int index);

    void play();
    void stop();
    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();
    void record();
    void panic();

protected slots:
    void slotFaderLevelChanged(float level);
    void slotPanChanged(float value);
    void slotChannelsChanged();
    void slotSoloChanged();
    void slotMuteChanged();
    void slotRecordChanged();
    void slotSelectPlugin();
    void slotRepopulate();
    
    // to be called if something changes in an instrument parameter box
    void slotInstrumentChanged(Instrument *);

    /// Ensure the faders are in sync with the document.
    /**
     * In case a Track's Instrument has been changed, this routine updates
     * the relevant fader.
     *
     * Called by RosegardenMainViewWidget::slotUpdateInstrumentParameterBox().
     */
    void slotTrackAssignmentsChanged();

    // from Plugin dialog
    void slotPluginSelected(InstrumentId id, int index, int plugin);
    void slotPluginBypassed(InstrumentId id, int pluginIndex, bool bp);

    void slotSetInputCountFromAction();
    void slotSetSubmasterCountFromAction();
    void slotSetPanLaw();

    void slotToggleFaders();
    void slotToggleSynthFaders();
    void slotToggleSubmasters();
    void slotTogglePluginButtons();
    void slotToggleUnassignedFaders();

    void slotUpdateFaderVisibility();
    void slotUpdateSynthFaderVisibility();
    void slotUpdateSubmasterVisibility();
    void slotUpdatePluginButtonVisibility();
    void slotHelpRequested();
    void slotHelpAbout();

protected:
    virtual void sendControllerRefresh();

private:

    void toggleNamedWidgets(bool show, const char* const);

    /// A vertical strip of controls representing a mixer channel.
    /**
     * ??? This object gets copied quite a bit.  While it turns out
     *     to be harmless in the end, it's surprising.
     *     Recommend hiding copy ctor and op= and using references.
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
