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

#ifndef RG_AUDIOSTRIP_H
#define RG_AUDIOSTRIP_H

#include "base/Instrument.h"

#include <QPixmap>
#include <QTimer>
#include <QWidget>

class QGridLayout;
class QPushButton;


namespace Rosegarden
{


class AudioRouteMenu;
class AudioVUMeter;
class Fader;
class Label;
class PluginPushButton;
class Rotary;


/// The strips on the "Audio Mixer" window.
class AudioStrip : public QWidget
{
    Q_OBJECT

public:
    AudioStrip(QWidget *parent, InstrumentId i_id = NoInstrument);
    virtual ~AudioStrip();

    void setId(InstrumentId id);
    InstrumentId getId() const  { return m_id; }

    void setExternalControllerChannel(unsigned channel)
            { m_externalControllerChannel = channel; }

    void updateWidgets();

    /// For "external controller" port support.
    void faderLevelChanged(float dB)  { slotFaderLevelChanged(dB); }
    /// For "external controller" port support.
    void panChanged(float pan)  { slotPanChanged(pan); }

signals:
    /// Launch AudioPluginDialog.
    /**
     * Connected to RosegardenMainWindow::slotShowPluginDialog().
     *
     * ??? Get rid of this.  See caller and RMW::slotShowPluginDialog()
     *     for details.
     */
    void selectPlugin(
        QWidget *parent, InstrumentId instrumentId, int pluginIndex);

private slots:
    /// This needs to go away.
    void slotInstrumentChanged(Instrument *instrument);

    void slotLabelClicked();
    void slotFaderLevelChanged(float dB);
    void slotPanChanged(float pan);
    void slotChannelsChanged();
    void slotSelectPlugin();

    /// Called on a timer to keep the meter updated.
    void slotUpdateMeter();

private:
    /// Buss/Instrument ID.
    InstrumentId m_id;

    bool isMaster() const  { return (m_id == 0); }
    bool isSubmaster() const
            { return (m_id > 0  &&  m_id < AudioInstrumentBase); }
    bool isInput() const  { return (m_id >= AudioInstrumentBase); }

    /// Channel on the "external controller" port.
    unsigned m_externalControllerChannel;

    // Widgets

    Label *m_label;
    AudioRouteMenu *m_input;
    AudioRouteMenu *m_output;
    Fader *m_fader;
    AudioVUMeter *m_meter;
    Rotary *m_pan;

    QPixmap m_monoPixmap;
    QPixmap m_stereoPixmap;
    QPushButton *m_stereoButton;
    // Cache for updateInputMeter().
    bool m_stereo;

    std::vector<PluginPushButton *> m_plugins;

    QGridLayout *m_layout;

    void createWidgets();

    // Timer for updating the meters.
    QTimer m_timer;
    void updateInputMeter();
    void updateSubmasterMeter();
    void updateMasterMeter();

};


}

#endif
