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

#define RG_MODULE_STRING "[MidiStrip]"

#include "MidiStrip.h"

//#include "MidiMixerWindow.h"
#include "MidiMixerVUMeter.h"

#include "base/Controllable.h"  // ControlList
#include "base/MidiDevice.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "sound/ExternalController.h"
#include "sound/Midi.h"  // MIDI_CONTROLLER_VOLUME
#include "sound/SequencerDataBlock.h"

#include <QLabel>
#include <QVBoxLayout>


namespace Rosegarden
{


MidiStrip::MidiStrip(QWidget *parent, InstrumentId instrumentID, int stripNumber) :
    QWidget(parent),
    m_id(instrumentID),
    m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(1,1,1,1);

    connect(Instrument::getStaticSignals().data(),
                &InstrumentStaticSignals::controlChange,
            this, &MidiStrip::slotControlChange);

    createWidgets(stripNumber);

    // Meter timer.
    connect(&m_timer, &QTimer::timeout,
            this, &MidiStrip::slotUpdateMeter);
    // 20fps should be responsive enough.
    m_timer.start(50);

}

void MidiStrip::createWidgets(int stripNumber)
{
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    if (!doc)
        return;
    Studio &studio = doc->getStudio();

    Instrument *instrument = studio.getInstrumentById(m_id);
    if (!instrument)
        return;
    const MidiDevice *midiDevice =
            dynamic_cast<const MidiDevice *>(instrument->getDevice());
    if (!midiDevice)
        return;

    // Get the control parameters that are on the IPB (and hence can
    // be shown here too).
    const ControlList controls = midiDevice->getIPBControlParameters();

    // For each controller...
    for (size_t controllerIndex = 0;
         controllerIndex < controls.size();
         ++controllerIndex) {

        // Controller name label
        // ??? This is pretty odd looking.  We need to default to a
        //     larger overall MMW size and get the label closer to each
        //     Rotary.  Maybe even add a label feature to Rotary so it
        //     can handle it better.
        QString controllerName = QObject::tr(
                controls[controllerIndex].getName().c_str());
        QLabel *label = new QLabel(controllerName.left(3), this);
        QFont font = label->font();
        font.setPointSize((font.pointSize() * 8) / 10);
        label->setFont(font);
        m_layout->addWidget(label, 0, Qt::AlignHCenter | Qt::AlignBottom);

        // Controller rotary
        const MidiByte controllerNumber =
                controls[controllerIndex].getControllerNumber();
        const bool centred =
                (controls[controllerIndex].getDefault() == 64);

        Rotary *rotary = new Rotary(
                this,  // parent
                controls[controllerIndex].getMin(),  // minimum
                controls[controllerIndex].getMax(),  // maximum
                1.0,  // step
                5.0,  // pageStep
                controls[controllerIndex].getDefault(),  // initialPosition
                20,  // size
                Rotary::NoTicks,  // ticks
                centred,
                false);
        rotary->setLabel(controllerName);

        // Color
        QColor knobColour = QColor(Qt::white);
        if (controls[controllerIndex].getColourIndex() > 0) {
            knobColour = doc->getComposition().
                    getGeneralColourMap().getColour(
                            controls[controllerIndex].getColourIndex());
        }
        rotary->setKnobColour(knobColour);

        rotary->setProperty("controllerNumber", controllerNumber);

        float value{0};
        if (instrument->hasController(controllerNumber))
            value = float(instrument->getControllerValue(
                    controllerNumber));
        rotary->setPosition(value);

        connect(rotary, &Rotary::valueChanged,
                this, &MidiStrip::slotControllerChanged);

        m_layout->addWidget(rotary, 0, Qt::AlignCenter);

        m_controllerRotaries.push_back(rotary);
    }

    // VU meter
    MidiMixerVUMeter *meter = new MidiMixerVUMeter(
            this,  // parent
            VUMeter::FixedHeightVisiblePeakHold,  // type
            6,  // width
            30);  // height
    m_layout->addWidget(meter, 0, Qt::AlignCenter);
    m_vuMeter = meter;

    // Volume
    Fader *fader = new Fader(
            0,  // min
            127,  // max
            100,  // i_default
            20,  // i_width
            80,  // i_height
            this);  // parent
    MidiByte volumeValue{0};
    if (instrument->hasController(MIDI_CONTROLLER_VOLUME))
        volumeValue = instrument->
                getControllerValue(MIDI_CONTROLLER_VOLUME);
    fader->setFader(float(volumeValue));
    connect(fader, &Fader::faderChanged,
            this, &MidiStrip::slotFaderLevelChanged);
    m_layout->addWidget(fader, 0, Qt::AlignCenter);
    m_volumeFader = fader;

    // Instrument number
    QLabel *instrumentNumberLabel = new QLabel(
            QString("%1").arg(stripNumber),
            this);
    m_layout->addWidget(
            instrumentNumberLabel, 0, Qt::AlignCenter);
}

void
MidiStrip::slotFaderLevelChanged(float value)
{
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    if (!doc)
        return;
    Studio &studio = doc->getStudio();

    const Fader * const fader = dynamic_cast<const Fader *>(sender());
    if (!fader)
        return;

    Instrument *instrument = studio.getInstrumentById(m_id);
    if (!instrument)
        return;

    instrument->setControllerValue(MIDI_CONTROLLER_VOLUME, MidiByte(value));
    Instrument::emitControlChange(instrument, MIDI_CONTROLLER_VOLUME);

    doc->setModified();

    // Check whether we need to send the update out the external controller port.
    // ??? Would also be nice to check if anything is actually connected.
    if (ExternalController::self().isNative()  &&
        instrument->hasFixedChannel()) {
        ExternalController::send(
                instrument->getNaturalMidiChannel(),
                MIDI_CONTROLLER_VOLUME,
                MidiByte(value));
    }
}

void
MidiStrip::slotControllerChanged(float value)
{
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    if (!doc)
        return;
    Studio &studio = doc->getStudio();

    const Rotary *rotary = dynamic_cast<const Rotary *>(sender());
    if (!rotary)
        return;

    const MidiByte controllerNumber = rotary->property("controllerNumber").toUInt();

    Instrument *instrument = studio.getInstrumentById(m_id);
    if (!instrument)
        return;

    instrument->setControllerValue(controllerNumber, MidiByte(value));
    Instrument::emitControlChange(instrument, controllerNumber);

    doc->setModified();

    // Check whether we need to send the update out the external controller port.
    // ??? Would also be nice to check if anything is actually connected.
    if (ExternalController::self().isNative()  &&
        instrument->hasFixedChannel()) {
        ExternalController::send(
                instrument->getNaturalMidiChannel(),
                controllerNumber,
                MidiByte(value));
    }
}

void
MidiStrip::slotControlChange(
        Instrument *instrument, const int controllerNumber)
{
    if (!instrument)
        return;
    // Not ours?  Bail.
    if (instrument->getId() != m_id)
        return;
    //if (!instrument->hasController(controllerNumber))
    //    return;

    const MidiByte controllerValue = instrument->getControllerValue(
            controllerNumber);

    // Based on the controllerNumber, update the appropriate Fader or Rotary.

    if (controllerNumber == MIDI_CONTROLLER_VOLUME) {

        // Update the volume Fader.
        m_volumeFader->setFader(controllerValue);

    } else {

        // Update the appropriate Rotary.

        const MidiDevice *midiDevice =
                dynamic_cast<const MidiDevice *>(instrument->getDevice());
        if (!midiDevice)
            return;

        const ControlList controls = midiDevice->getIPBControlParameters();

        // For each controller...
        for (size_t controllerIndex = 0;
             controllerIndex < controls.size();
             ++controllerIndex) {
            // If this is the one, set the rotary.
            if (controllerNumber == controls[controllerIndex].getControllerNumber()) {
                m_controllerRotaries[controllerIndex]->setPosition(controllerValue);
                break;
            }
        }

    }
}

void MidiStrip::slotUpdateMeter()
{
    LevelInfo info;
    if (!SequencerDataBlock::getInstance()->getInstrumentLevelForMixer(
            m_id, info)) {
        return;
    }

    if (m_vuMeter)
        m_vuMeter->setLevel(double(info.level / 127.0));
}


}
