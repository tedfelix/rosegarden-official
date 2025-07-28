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

#define RG_MODULE_STRING "[MidiMixerWindow]"
#define RG_NO_DEBUG_PRINT

#include "MidiMixerWindow.h"

#include "MidiMixerVUMeter.h"
#include "MidiStrip.h"

#include "sound/Midi.h"  // For MIDI_CONTROLLER_VOLUME, etc...
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/VUMeter.h"
#include "gui/general/IconLoader.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/application/RosegardenMainWindow.h"
#include "sound/MappedEvent.h"
#include "sound/SequencerDataBlock.h"
#include "sound/ExternalController.h"

#include <QColor>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QDesktopServices>


namespace Rosegarden
{


// ??? Use QObject properties to eliminate loops in MMW.

// ??? Use QObject properties to eliminate loops in AMW2.

namespace
{

    typedef std::vector<MidiDevice *> MidiDeviceVector;

    // ??? Studio member function candidate.
    MidiDeviceVector getMidiOutputDevices(const Studio *studio)
    {
        MidiDeviceVector devices;

        // For each Device in the Studio...
        for (Device *device : studio->getDevicesRef()) {
            MidiDevice *midiDevice =
                    dynamic_cast<MidiDevice *>(device);
            if (!midiDevice)
                continue;
            if (midiDevice->isInput())
                continue;

            devices.push_back(midiDevice);
        }

        return devices;
    }

}

MidiMixerWindow::MidiMixerWindow() :
    MixerWindow(RosegardenMainWindow::self(),
                RosegardenDocument::currentDocument)
{
    setWindowTitle(tr("MIDI Mixer"));
    setWindowIcon(IconLoader::loadPixmap("window-midimixer"));

    // ??? Inline this?  I think once we pull out MidiStrip like AudioStrip,
    //     that will make a lot of sense.
    setupTabs();

    createAction("file_close", &MidiMixerWindow::slotClose);

    createAction("play", RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotPlay);
    createAction("stop", RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotStop);
    createAction("playback_pointer_back_bar", RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotRewind);
    createAction("playback_pointer_forward_bar", RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotFastforward);
    createAction("playback_pointer_start", RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotRewindToBeginning);
    createAction("playback_pointer_end", RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotFastForwardToEnd);
    createAction("record", RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotRecord);
    createAction("panic", RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotPanic);

    createAction("midimix_help", &MidiMixerWindow::slotHelpRequested);
    createAction("help_about_app", &MidiMixerWindow::slotHelpAbout);

    createMenusAndToolbars("midimixer.rc");

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "playback_pointer_back_bar");
    enableAutoRepeat("Transport Toolbar", "playback_pointer_forward_bar");

    connect(Instrument::getStaticSignals().data(),
                &InstrumentStaticSignals::controlChange,
            this, &MidiMixerWindow::slotControlChange);

    connect(&ExternalController::self(),
                &ExternalController::externalControllerMMW,
            this, &MidiMixerWindow::slotExternalController);

    // Make sure we close if the document is changing.
    connect(RosegardenMainWindow::self(),
                    &RosegardenMainWindow::documentAboutToChange,
            this, &QWidget::close);

    // Let RMW know we are closing.
    connect(this, &MixerWindow::closing,
            RosegardenMainWindow::self(),
                    &RosegardenMainWindow::slotMidiMixerClosed);

    // Meter timer.
    connect(&m_timer, &QTimer::timeout,
            this, &MidiMixerWindow::updateMeters);
    // 20fps should be responsive enough.
    m_timer.start(50);

    show();

    // Lock the window size.
    // ??? Ideally, we need to allow resizing, scale the controls, and
    //     store/restore the window size on close/open.
    //     AMW2 needs this ability as well.
    //     Bug #1677
    setFixedSize(geometry().size());
}

void
MidiMixerWindow::setupTabs()
{
    // Tab widget
    m_tabWidget = new QTabWidget(this);
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &MidiMixerWindow::slotCurrentTabChanged);
    m_tabWidget->setTabPosition(QTabWidget::South);
    setCentralWidget(m_tabWidget);

    // ??? This is done only once.  But the number of devices is dynamic and
    //     can change during a run.  We need to monitor for changes to the
    //     Studio and update this display to match.  AudioMixerWindow2 does
    //     this.  See AudioMixerWindow2::updateWidgets().

    const MidiDeviceVector devices = getMidiOutputDevices(m_studio);

    int deviceCount = 1;

    // For each MidiDevice in the Studio...
    for (const MidiDevice *midiDevice : devices) {
        InstrumentVector instruments = midiDevice->getPresentationInstruments();
        // Don't add a frame for empty devices
        if (!instruments.size())
            continue;

        // Get the control parameters that are on the IPB (and hence can
        // be shown here too).
        const ControlList controls = getIPBControlParameters(midiDevice);

        QFrame *tabFrame = new QFrame(m_tabWidget);
        tabFrame->setContentsMargins(10, 10, 10, 10);
        const QString name = QString("%1 (%2)").
                arg(QObject::tr(midiDevice->getName().c_str())).
                arg(deviceCount++);
        // Add the tab to the QTabWidget.
        m_tabWidget->addTab(tabFrame, name);

        QGridLayout *gridLayout = new QGridLayout(tabFrame);

        QLabel *label;

        int col = 0;

        int stripNum = 1;

        // For each Instrument in this MidiDevice...
        for (const Instrument *instrument : instruments) {
            const InstrumentId instrumentId = instrument->getId();

            // Add a new MidiStrip.
            m_midiStrips.push_back(std::make_shared<MidiStrip>());
            std::shared_ptr<MidiStrip> midiStrip = m_midiStrips.back();
            midiStrip->m_id = instrumentId;
            m_instrumentIDToStripIndex[instrumentId] = m_midiStrips.size() - 1;

            int row = 0;

            // For each controller...
            for (size_t controllerIndex = 0;
                 controllerIndex < controls.size();
                 ++controllerIndex) {

                // Controller name label
                // ??? This is pretty odd looking.  We need to default to a
                //     larger overall MMW size and get the label closer to each
                //     Rotary.  Maybe even add a label feature to Rotary so it
                //     can handle it better.  We can also add the label to the
                //     Rotary tooltip.  It already has a default tool tip.  We
                //     need a way to add the label to it.
                QString controllerName = QObject::tr(
                        controls[controllerIndex].getName().c_str());
                label = new QLabel(controllerName.left(3), tabFrame);
                QFont font = label->font();
                font.setPointSize((font.pointSize() * 8) / 10);
                label->setFont(font);
                //label->setAlignment(Qt::AlignHCenter);
                gridLayout->addWidget(label, row, col, Qt::AlignHCenter | Qt::AlignBottom);

                ++row;

                // Controller rotary
                const MidiByte controllerNumber =
                        controls[controllerIndex].getControllerNumber();
                const bool centred =
                        (controls[controllerIndex].getDefault() == 64);

                Rotary *rotary = new Rotary(
                        tabFrame,  // parent
                        controls[controllerIndex].getMin(),  // minimum
                        controls[controllerIndex].getMax(),  // maximum
                        1.0,  // step
                        5.0,  // pageStep
                        controls[controllerIndex].getDefault(),  // initialPosition
                        20,  // size
                        Rotary::NoTicks,  // ticks
                        false,  // snapToTicks
                        centred);
                rotary->setLabel(controllerName);

                // Color
                QColor knobColour = QColor(Qt::white);
                if (controls[controllerIndex].getColourIndex() > 0) {
                    knobColour = m_document->getComposition().
                            getGeneralColourMap().getColour(
                                    controls[controllerIndex].getColourIndex());
                }
                rotary->setKnobColour(knobColour);

                rotary->setProperty("instrumentId", instrumentId);
                rotary->setProperty("controllerNumber", controllerNumber);

                float value{0};
                if (instrument->hasController(controllerNumber))
                    value = float(instrument->getControllerValue(
                            controllerNumber));
                rotary->setPosition(value);

                connect(rotary, &Rotary::valueChanged,
                        this, &MidiMixerWindow::slotControllerChanged);

                gridLayout->addWidget(
                        rotary, row, col, Qt::AlignCenter);

                midiStrip->m_controllerRotaries.push_back(rotary);

                ++row;
            }

            // VU meter
            MidiMixerVUMeter *meter = new MidiMixerVUMeter(
                    tabFrame,  // parent
                    VUMeter::FixedHeightVisiblePeakHold,  // type
                    6,  // width
                    30);  // height
            gridLayout->addWidget(meter, row, col, Qt::AlignCenter);
            midiStrip->m_vuMeter = meter;

            ++row;

            // Volume
            Fader *fader = new Fader(
                    0,  // min
                    127,  // max
                    100,  // i_default
                    20,  // i_width
                    80,  // i_height
                    tabFrame);  // parent
            fader->setProperty("instrumentId", instrumentId);
            MidiByte volumeValue{0};
            if (instrument->hasController(MIDI_CONTROLLER_VOLUME))
                volumeValue = instrument->
                        getControllerValue(MIDI_CONTROLLER_VOLUME);
            fader->setFader(float(volumeValue));
            connect(fader, &Fader::faderChanged,
                    this, &MidiMixerWindow::slotFaderLevelChanged);
            gridLayout->addWidget(
                    fader, row, col, Qt::AlignCenter);
            midiStrip->m_volumeFader = fader;

            ++row;

            // Instrument number
            QLabel *instrumentNumberLabel = new QLabel(
                    QString("%1").arg(stripNum++),
                    tabFrame);
            gridLayout->addWidget(
                    instrumentNumberLabel,  // widget
                    row,  // row
                    col,  // column
                    Qt::AlignCenter);  // alignment

            ++row;

            ++col;
        }
    }
}

void
MidiMixerWindow::slotFaderLevelChanged(float value)
{
    const Fader * const fader = dynamic_cast<const Fader *>(sender());
    if (!fader)
        return;

    // ??? Once we move all this to MidiStrip, we can store instrument ID
    //     in MidiStrip as a member.
    const InstrumentId instrumentId = fader->property("instrumentId").toUInt();

    Instrument *instrument = m_studio->getInstrumentById(instrumentId);
    if (!instrument)
        return;

    instrument->setControllerValue(MIDI_CONTROLLER_VOLUME, MidiByte(value));
    Instrument::emitControlChange(instrument, MIDI_CONTROLLER_VOLUME);

    m_document->setModified();

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
MidiMixerWindow::slotControllerChanged(float value)
{
    const Rotary *rotary = dynamic_cast<const Rotary *>(sender());
    if (!rotary)
        return;

    // ??? Once we move all this to MidiStrip, we can store instrument ID
    //     and controller number in MidiStrip as a member.
    const InstrumentId instrumentId = rotary->property("instrumentId").toUInt();
    const MidiByte controllerNumber = rotary->property("controllerNumber").toUInt();

    Instrument *instrument = m_studio->getInstrumentById(instrumentId);

    instrument->setControllerValue(controllerNumber, MidiByte(value));
    Instrument::emitControlChange(instrument, controllerNumber);

    m_document->setModified();

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
MidiMixerWindow::slotControlChange(
        Instrument *instrument, const int controllerNumber)
{
    if (!instrument)
        return;
    if (!instrument->hasController(controllerNumber))
        return;

    const MidiByte controllerValue = instrument->getControllerValue(
            controllerNumber);

    // Find the appropriate strip index given the InstrumentId.

    InstrumentIDToStripIndex::const_iterator iter =
            m_instrumentIDToStripIndex.find(instrument->getId());
    // Not found?  Bail.
    if (iter == m_instrumentIDToStripIndex.end())
        return;

    const size_t stripIndex = iter->second;
    if (stripIndex >= m_midiStrips.size())
        return;

    // Based on the controllerNumber, update the appropriate Fader or Rotary.

    if (controllerNumber == MIDI_CONTROLLER_VOLUME) {

        // Update the volume Fader.
        m_midiStrips[stripIndex]->m_volumeFader->setFader(controllerValue);

    } else {

        // Update the appropriate Rotary.

        const ControlList controls = getIPBControlParameters(
                dynamic_cast<MidiDevice *>(instrument->getDevice()));

        // For each controller...
        for (size_t controllerIndex = 0;
             controllerIndex < controls.size();
             ++controllerIndex) {
            // If this is the one, set the rotary.
            if (controllerNumber == controls[controllerIndex].getControllerNumber()) {
                m_midiStrips[stripIndex]->m_controllerRotaries[controllerIndex]->
                        setPosition(controllerValue);
                break;
            }
        }

    }
}

void
MidiMixerWindow::updateMeters()
{
    // For each strip...
    for (std::shared_ptr<MidiStrip> midiStrip : m_midiStrips) {
        LevelInfo info;
        if (!SequencerDataBlock::getInstance()->getInstrumentLevelForMixer(
                midiStrip->m_id, info)) {
            continue;
        }

        if (midiStrip->m_vuMeter)
            midiStrip->m_vuMeter->setLevel(double(info.level / 127.0));
    }
}

void
MidiMixerWindow::slotExternalController(const MappedEvent *event)
{
    //RG_DEBUG << "slotExternalController()...";

    // Some window managers (e.g. GNOME) do not allow the application to
    // change focus on the user.  So, this might not work.
    activateWindow();
    raise();

    if (event->getType() != MappedEvent::MidiController)
        return;

    const unsigned int channel = event->getRecordedChannel();
    const MidiByte controllerNumber = event->getData1();
    const MidiByte value = event->getData2();

    // Get the MidiDevice for the current tab.

    const MidiDeviceVector devices = getMidiOutputDevices(m_studio);

    const size_t currentTabIndex = m_tabWidget->currentIndex();
    if (currentTabIndex >= devices.size())
        return;

    const MidiDevice *midiDevice = devices[currentTabIndex];

    // Get the Instrument for a specific channel in the Device.

    const InstrumentVector instruments =
            midiDevice->getPresentationInstruments();

    // Find the Instrument for this channel.

    Instrument *instrument = nullptr;

    for (Instrument *loopInstrument : instruments) {
        if (!loopInstrument)
            continue;
        // If this is the one, we're done.
        if (loopInstrument->getNaturalMidiChannel() == channel) {
            instrument = loopInstrument;
            break;
        }
    }
    // Not found?  Bail.
    if (!instrument)
        return;

    // Only handle controllers that are defined for the MidiDevice.

    // Find the controller on the MidiDevice.
    const ControlParameter *controlParameter =
            midiDevice->getControlParameterConst(
                    Controller::EventType, controllerNumber);

    // If this MidiDevice has this controller, make the control change.
    if (controlParameter) {
        instrument->setControllerValue(controllerNumber, value);
        Instrument::emitControlChange(instrument, controllerNumber);
        m_document->setModified();
    }

}

void
MidiMixerWindow::slotCurrentTabChanged(int)
{
    sendControllerRefresh();
}

void
MidiMixerWindow::sendControllerRefresh()
{
    // We only do this if the external controller port is
    // in Rosegarden native mode.
    if (!ExternalController::self().isNative())
        return;

    // To keep the device connected to the "external controller" port in
    // sync with the "MIDI Mixer" window, send out MIDI volume and pan
    // messages to it.

    // ??? Would be nice if we could skip all this if nothing is actually
    //     connected to the "external controller" port.

    // Get the MidiDevice for the current tab.

    const MidiDeviceVector devices = getMidiOutputDevices(m_studio);

    const size_t currentTabIndex = m_tabWidget->currentIndex();
    if (currentTabIndex >= devices.size())
        return;

    const MidiDevice *midiDevice = devices[currentTabIndex];

    const InstrumentVector instruments =
            midiDevice->getPresentationInstruments();

    // For each Instrument...
    for (const Instrument *instrument : instruments) {

        // No fixed channel?  Try the next.
        if (!instrument->hasFixedChannel())
            continue;

        ExternalController::sendAllCCs(instrument);

    }
}

void
MidiMixerWindow::slotSynchronise()
{
    RG_DEBUG << "slotSynchronise()";

    // This is connected to DeviceManagerDialog::deviceNamesChanged() but it
    // does nothing.

    // ??? We should probably connect to document changed and refresh
    //     everything.  See AudioMixerWindow2::slotDocumentModified().

    //setupTabs();
}

void
MidiMixerWindow::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:midiMixerWindow-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:midiMixerWindow-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
MidiMixerWindow::slotHelpAbout()
{
    new AboutDialog(this);
}

ControlList
MidiMixerWindow::getIPBControlParameters(const MidiDevice *dev) const
{
    const ControlList allControllers = dev->getIPBControlParameters();
    ControlList controllersFiltered;

    // For each controller...
    for (const ControlParameter &controller : allControllers)
    {
        // If it is visible and not volume, add to filtered vector.
        if (controller.getIPBPosition() != -1  &&
            controller.getControllerNumber() != MIDI_CONTROLLER_VOLUME)
            controllersFiltered.push_back(controller);
    }

    return controllersFiltered;
}

void
MidiMixerWindow::changeEvent(QEvent *event)
{
    // Let baseclass handle first.
    MixerWindow::changeEvent(event);

    // We only care about this if the external controller port is
    // in Rosegarden native mode.
    if (!ExternalController::self().isNative())
        return;

    // We only want to handle window activation.
    if (event->type() != QEvent::ActivationChange)
        return;

    // If this is not an activation, bail.
    if (!isActiveWindow())
        return;

    ExternalController::self().activeWindow =
            ExternalController::MidiMixer;

    sendControllerRefresh();
}


}
