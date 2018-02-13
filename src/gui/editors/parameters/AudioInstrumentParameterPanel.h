
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOINSTRUMENTPARAMETERPANEL_H
#define RG_AUDIOINSTRUMENTPARAMETERPANEL_H

#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "InstrumentParameterPanel.h"


#include <QPixmap>
#include <QString>
#include <QSharedPointer>


class QWidget;
class QColor;


namespace Rosegarden
{

class RosegardenDocument;
class Instrument;
class InstrumentStaticSignals;
class InstrumentAliasButton;
class AudioFaderBox;


class AudioInstrumentParameterPanel : public InstrumentParameterPanel
{
    Q_OBJECT
public:
    AudioInstrumentParameterPanel(QWidget* parent);

    virtual void setupForInstrument(Instrument*);

    /// Set the audio meter to a given level for a maximum of two channels.
    /**
     * ??? Recommend moving RMVW::updateMeters() and updateMonitorMeters()
     *     into here.  AIPP should be responsible for sampling the levels in
     *     SequencerDataBlock and displaying them.  This way AIPP is
     *     autonomous and RMVW and IPB need not care.
     */
    void setAudioMeter(float dBleft, float dBright,
                       float recDBleft, float recDBright);

    // Set the button colour
    //
    void setButtonColour(int pluginIndex, bool bypassState, 
                         const QColor &color);

public slots:
    // From AudioFaderBox
    //
    void slotSelectAudioLevel(float dB);
    void slotSelectAudioRecordLevel(float dB);
    void slotAudioChannels(int channels);
    void slotSelectPlugin(int index);

    // From the parameter box clicks
    void slotSetPan(float pan);

    // From RosegardenMainWindow.
    void slotPluginSelected(InstrumentId id, int index, int plugin);
    void slotPluginBypassed(InstrumentId id, int pluginIndex, bool bp);

    void slotSynthButtonClicked();
    /// Editor button.
    void slotSynthGUIButtonClicked();

protected:
    //--------------- Data members ---------------------------------

    AudioFaderBox *m_audioFader;

private slots:
    void slotAliasChanged();

    /// Called when a new document is loaded.
    void slotNewDocument(RosegardenDocument *);
    /// Called when the document is modified in some way.
    void slotDocumentModified(bool);

    /// Connected to InstrumentStaticSignals::controlChange().
    void slotControlChange(Instrument *instrument, int cc);
private:

    // This is the tiny button in the upper left that allows the user
    // to change the name of the audio instrument.
    InstrumentAliasButton *m_aliasButton;

    QPixmap                                      m_monoPixmap;
    QPixmap                                      m_stereoPixmap;
};


}

#endif
