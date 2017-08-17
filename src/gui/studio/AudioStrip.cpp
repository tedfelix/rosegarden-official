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

#define RG_MODULE_STRING "[AudioStrip]"

#include "AudioStrip.h"

#include "gui/widgets/AudioRouteMenu.h"
#include "gui/widgets/AudioVUMeter.h"
#include "misc/Debug.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/InputDialog.h"
#include "base/InstrumentStaticSignals.h"
#include "gui/widgets/Label.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/seqmanager/SequenceManager.h"
#include "sound/SequencerDataBlock.h"
#include "base/Studio.h"
#include "StudioControl.h"

#include <QFont>
#include <QGridLayout>

namespace Rosegarden
{


AudioStrip::AudioStrip(QWidget *parent, InstrumentId id) :
    QWidget(parent),
    m_id(NoInstrument),
    m_label(NULL),
    m_input(NULL),
    m_output(NULL),
    m_fader(NULL),
    m_meter(NULL),
    m_layout(new QGridLayout(this))
{
    QFont font;
    font.setPointSize(6);
    font.setBold(false);
    setFont(font);

    QFont boldFont(font);
    boldFont.setBold(true);

    // Give the parent control over spacing between strips.
    m_layout->setContentsMargins(0,0,0,0);
    // Keep the widgets close together.
    m_layout->setSpacing(2);

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            SLOT(slotInstrumentChanged(Instrument *)));

    // We have to have an id in order to create the proper widgets and
    // initialize them.  If we don't, don't worry about it.  Handle it
    // later in setId().
    if (id != NoInstrument)
        setId(id);

    // Meter timer.
    connect(&m_timer, SIGNAL(timeout()),
            SLOT(slotUpdateMeter()));
    // 20fps should be responsive enough.
    m_timer.start(50);
}

AudioStrip::~AudioStrip()
{

}

void AudioStrip::setId(InstrumentId id)
{
    // No change?  Bail.
    if (m_id == id)
        return;

    m_id = id;

    // If the widgets haven't been created yet, create them.
    if (!m_label)
        createWidgets();

    // Pass on the new id to widgets that care.

    if (m_input)
        m_input->setInstrument(m_id);

    if (m_output)
        m_output->setInstrument(m_id);
}

void AudioStrip::createWidgets()
{
    // No ID yet?  Bail.
    if (m_id == NoInstrument)
        return;

    QFont boldFont(font());
    boldFont.setBold(true);

    const int maxWidth = 45;

    // Label

    m_label = new Label(this);
    m_label->setFont(boldFont);
    m_label->setMinimumWidth(maxWidth);
    m_label->setMaximumWidth(maxWidth);
    m_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    connect(m_label, SIGNAL(clicked()),
            SLOT(slotLabelClicked()));

    // Input

    if (isInput()) {
        m_input = new AudioRouteMenu(this,
                                     AudioRouteMenu::In,
                                     AudioRouteMenu::Compact,
                                     m_id);
        m_input->getWidget()->setToolTip(tr("Record input source"));
        m_input->getWidget()->setMaximumWidth(maxWidth);
        m_input->getWidget()->setMaximumHeight(15);
    }

    // Output

    if (isInput()) {
        m_output = new AudioRouteMenu(this,
                                      AudioRouteMenu::Out,
                                      AudioRouteMenu::Compact,
                                      m_id);
        m_output->getWidget()->setToolTip(tr("Output destination"));
        m_output->getWidget()->setMaximumWidth(maxWidth);
        m_output->getWidget()->setMaximumHeight(15);
    }

    // Fader

    m_fader = new Fader(AudioLevel::LongFader, 20, 240, this);
    m_fader->setToolTip(tr("Audio level"));

    connect(m_fader, SIGNAL(faderChanged(float)),
            this, SLOT(slotFaderLevelChanged(float)));

    // Meter

    m_meter = new AudioVUMeter(
            this,  // parent
            VUMeter::AudioPeakHoldIECLong,  // type
            true,  // stereo
            isInput(),  // hasRecord
            20,  // width
            240);  // height
    m_meter->setToolTip(tr("Audio level"));

    // Layout

    m_layout->addWidget(m_label, 0, 0, 1, 2, Qt::AlignLeft);

    if (m_input)
        m_layout->addWidget(m_input->getWidget(), 1, 0, 1, 2, Qt::AlignLeft);

    if (m_output)
        m_layout->addWidget(m_output->getWidget(), 2, 0, 1, 2, Qt::AlignLeft);

    m_layout->addWidget(m_fader, 3, 0, Qt::AlignCenter);
    m_layout->addWidget(m_meter, 3, 1, Qt::AlignCenter);
}

void AudioStrip::updateWidgets()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // Get the appropriate instrument based on the ID.
    Instrument *instrument = NULL;
    if (isInput())
        instrument = studio.getInstrumentById(m_id);

    // Update each widget efficiently.

    // Label

    if (isInput()) {
        m_label->setText(strtoqstr(instrument->getAlias()));
        m_label->setToolTip(strtoqstr(instrument->getAlias()) + "\n" +
                tr("Click to rename this instrument"));
    } else if (isSubmaster()) {
        m_label->setText(tr("Sub %1").arg(m_id));
    } else {  // Master
        m_label->setText(tr("Master"));
    }

    // Input

    if (m_input)
        m_input->updateWidget();

    // Output

    if (m_output)
        m_output->updateWidget();

    // Fader

    if (isInput()) {
        m_fader->setFader(instrument->getLevel());
    } else {  // buss
        BussList busses = studio.getBusses();
        Buss *buss = busses[m_id];

        m_fader->setFader(buss->getLevel());
    }
}

void
AudioStrip::slotInstrumentChanged(Instrument * /*instrument*/)
{
    // ??? This routine needs to go away.  Instead, we should be using
    //     RosegardenDocument::documentModified(bool) for low-frequency
    //     updates, and a new high-frequency controller update notification.

    updateWidgets();
}

void AudioStrip::slotLabelClicked()
{
    // Can only change alias on input strips.
    if (!isInput())
        return;

    QString oldAlias = m_label->text();
    bool ok = false;

    QString newAlias = InputDialog::getText(
            this,  // parent
            tr("Rosegarden"),  // title
            tr("Enter instrument alias:"),  // label
            LineEdit::Normal,  // mode (echo)
            m_label->text(),  // text
            &ok);  // ok

    // Cancelled?  Bail.
    if (!ok)
        return;

    // No change?  Bail.
    if (newAlias == oldAlias)
        return;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // Get the appropriate instrument based on the ID.
    Instrument *instrument = studio.getInstrumentById(m_id);

    instrument->setAlias(newAlias.toStdString());
    // ??? For now, we need this to update AIPP.  Over time, this will go
    //     away, and only the call to slotDocumentModified() will be needed.
    instrument->changed();
    doc->slotDocumentModified();
}

void
AudioStrip::slotFaderLevelChanged(float dB)
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    // If this is an input Fader
    if (m_id >= AudioInstrumentBase) {
        Instrument *instrument =
            studio.getInstrumentById(m_id);

        if (!instrument)
            return;

        StudioControl::setStudioObjectProperty(
                MappedObjectId(instrument->getMappedId()),
                MappedAudioFader::FaderLevel,
                MappedObjectValue(dB));

        instrument->setLevel(dB);
        instrument->changed();

#if 0
        // ??? We don't have this anymore.  Need to have AMW2 set this
        //     for us.  It's just the physical position from the left
        //     of each fader starting with 0.
        int externalControllerChannel =
                m_fader->property("externalControllerChannel").toInt();

        // Send out to "external controller" port as well.
        // ??? Would be nice to know whether anything is connected
        //     to the "external controller" port.  Otherwise this is
        //     a waste.  Especially with a potentially very frequent
        //     update such as this.
        if (externalControllerChannel < 16) {
            int value = AudioLevel::dB_to_fader(
                    dB, 127, AudioLevel::LongFader);

            MappedEvent mE(m_id,
                           MappedEvent::MidiController,
                           MIDI_CONTROLLER_VOLUME,
                           MidiByte(value));
            mE.setRecordedChannel(externalControllerChannel);
            mE.setRecordedDevice(Device::CONTROL_DEVICE);

            StudioControl::sendMappedEvent(mE);
        }
#endif

        return;
    }

    // If this is the master or a submaster Fader
    if (m_id < AudioInstrumentBase) {

        BussList busses = studio.getBusses();

        // If the buss ID is out of range, bail.
        if (m_id >= busses.size())
            return;

        StudioControl::setStudioObjectProperty(
                MappedObjectId(busses[m_id]->getMappedId()),
                MappedAudioBuss::Level,
                MappedObjectValue(dB));

        busses[m_id]->setLevel(dB);

        return;
    }
}

void
AudioStrip::slotUpdateMeter()
{
    if (m_meter == NULL)
        return;
    if (m_id == NoInstrument)
        return;

    if (isInput())
        updateInputMeter();
    else if (isSubmaster())
        updateSubmasterMeter();
    else if (isMaster())
        updateMasterMeter();
}

void
AudioStrip::updateInputMeter()
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();

    // No SequenceManager?  Bail.
    if (!doc->getSequenceManager())
        return;

    // If we're playing, show the playback level on the meter.
    if (doc->getSequenceManager()->getTransportStatus() == PLAYING) {

        LevelInfo info;

        // Get the level.  If there was no change, bail.
        if (!SequencerDataBlock::getInstance()->
                getInstrumentLevelForMixer(m_id, info))
            return;

        // Convert to dB for display.
        // The values passed through are long-fader values
        float dBleft = AudioLevel::fader_to_dB(
                info.level, 127, AudioLevel::LongFader);

        // ??? For now.  Need to handle stereo properly.
        bool m_stereo = false;

        if (m_stereo) {
            // Convert to dB for display.
            float dBright = AudioLevel::fader_to_dB(
                    info.levelRight, 127, AudioLevel::LongFader);

            m_meter->setLevel(dBleft, dBright);
        } else {  // mono
            m_meter->setLevel(dBleft);
        }

    } else {  // STOPPED or RECORDING, show the monitor level on the meter.

        LevelInfo info;

        // Get the record level.  If there was no change, bail.
        if (!SequencerDataBlock::getInstance()->
                getInstrumentRecordLevelForMixer(m_id, info))
            return;

        // Does this Instrument have a Track that is armed?
        // ??? Pull out into a function.

        bool armed = false;

        Composition &comp = doc->getComposition();
        Composition::trackcontainer &tracks = comp.getTracks();

        // For each Track in the Composition
        // ??? Performance: LINEAR SEARCH
        //     I see no easy fix.  Each Instrument would need to keep a list
        //     of the Tracks it is on.  Or something equally complicated.
        for (Composition::trackcontainer::iterator ti =
                 tracks.begin();
             ti != tracks.end();
             ++ti) {
            Track *track = ti->second;

            // If this Track has this Instrument
            if (track->getInstrument() == m_id) {
                // ??? Performance: LINEAR SEARCH
                if (comp.isTrackRecording(track->getId())) {
                    armed = true;
                    // Only one Track can be armed per Instrument.  Regardless,
                    // we only need to know that a Track is in record mode.
                    break;
                }
            }
        }

        if (!armed)
            return;

        // Convert to dB for display.
        // The values passed through are long-fader values
        float dBleft = AudioLevel::fader_to_dB(
                info.level, 127, AudioLevel::LongFader);

        // ??? For now.  Need to handle stereo properly.
        bool m_stereo = false;

        if (m_stereo) {
            // Convert to dB for display.
            float dBright = AudioLevel::fader_to_dB(
                    info.levelRight, 127, AudioLevel::LongFader);

            m_meter->setRecordLevel(dBleft, dBright);
        } else {
            m_meter->setRecordLevel(dBleft);
        }

    }
}

void
AudioStrip::updateSubmasterMeter()
{
    LevelInfo info;

    // Get the level.  If there was no change, bail.
    if (!SequencerDataBlock::getInstance()->getSubmasterLevel(m_id-1, info))
        return;

    // Convert to dB for display.
    // The values passed through are long-fader values
    float dBleft = AudioLevel::fader_to_dB(
            info.level, 127, AudioLevel::LongFader);
    float dBright = AudioLevel::fader_to_dB(
            info.levelRight, 127, AudioLevel::LongFader);

    // Update the meter.
    m_meter->setLevel(dBleft, dBright);
}

void
AudioStrip::updateMasterMeter()
{
    LevelInfo masterInfo;

    // Get the master level.  If there was no change, bail.
    if (!SequencerDataBlock::getInstance()->getMasterLevel(masterInfo))
        return;

    // Convert to dB for display.
    float dBleft = AudioLevel::fader_to_dB(
            masterInfo.level, 127, AudioLevel::LongFader);
    float dBright = AudioLevel::fader_to_dB(
            masterInfo.levelRight, 127, AudioLevel::LongFader);

    // Update the meter.
    m_meter->setLevel(dBleft, dBright);
}


}
