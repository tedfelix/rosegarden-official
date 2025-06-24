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


MidiMixerWindow::MidiMixerWindow(QWidget *parent,
                                 RosegardenDocument *document):
    MixerWindow(parent, document)
{
    setWindowTitle(tr("MIDI Mixer"));
    setWindowIcon(IconLoader::loadPixmap("window-midimixer"));

    // ??? Inline this?  I think once we pull out MidiStrip like AudioStrip,
    //     that will make a lot of sense.
    setupTabs();

    createAction("file_close", &MidiMixerWindow::slotClose);

    // ??? Connect all these directly to RMW like AudioMixerWindow does.
    createAction("play", &MidiMixerWindow::play);
    createAction("stop", &MidiMixerWindow::stop);
    createAction("playback_pointer_back_bar", &MidiMixerWindow::rewindPlayback);
    createAction("playback_pointer_forward_bar", &MidiMixerWindow::fastForwardPlayback);
    createAction("playback_pointer_start", &MidiMixerWindow::rewindPlaybackToBeginning);
    createAction("playback_pointer_end", &MidiMixerWindow::fastForwardPlaybackToEnd);
    createAction("record", &MidiMixerWindow::record);
    createAction("panic", &MidiMixerWindow::panic);
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

    // ??? What about restoring window geometry?
    // ??? Once that's in place, add to AudioMixerWindow2 as well.
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

    int deviceCount = 1;

    // For each Device in the Studio...
    for (const Device *device : m_studio->getDevicesRef()) {
        const MidiDevice *midiDevice = dynamic_cast<const MidiDevice *>(device);
        if (!midiDevice)
            continue;

        // Get the control parameters that are on the IPB (and hence can
        // be shown here too).
        ControlList controls = getIPBControlParameters(midiDevice);

        InstrumentVector instruments = midiDevice->getPresentationInstruments();
        // Don't add a frame for empty devices
        if (!instruments.size())
            continue;

        m_tabFrame = new QFrame(m_tabWidget);
        m_tabFrame->setContentsMargins(10, 10, 10, 10);
        const QString name = QString("%1 (%2)").
                arg(QObject::tr(midiDevice->getName().c_str())).
                arg(deviceCount++);
        // Add the tab to the QTabWidget.
        m_tabWidget->addTab(m_tabFrame, name);

        QGridLayout *gridLayout = new QGridLayout(m_tabFrame);

        QLabel *label;

        // Controller labels
        // ??? Does this make sense for input devices?
        for (size_t controlIndex = 0;
             controlIndex < controls.size();
             ++controlIndex) {
            label = new QLabel(
                    QObject::tr(controls[controlIndex].getName().c_str()),
                    m_tabFrame);
            gridLayout->addWidget(label, controlIndex, 0, Qt::AlignRight);
        }

        // Volume label
        label = new QLabel(tr("Volume"), m_tabFrame);
        gridLayout->addWidget(label, controls.size() + 1, 0, Qt::AlignRight);

        // Instrument label
        const QString instrument = tr("Instrument");
        label = new QLabel(instrument, m_tabFrame);
        label->setFixedWidth(fontMetrics().boundingRect(instrument).width() + 2);
        gridLayout->addWidget(label, controls.size() + 2, 0, Qt::AlignRight);

        // Add a column spacer.
        gridLayout->setColumnMinimumWidth(1, 10);

        // Grid layout column.
        int col = 2;

        int stripNum = 1;

        // For each Instrument in this MidiDevice...
        for (const Instrument *instrument : instruments) {

            // Add a new MidiStrip.
            m_midiStrips.push_back(std::make_shared<MidiStrip>());
            std::shared_ptr<MidiStrip> midiStrip = m_midiStrips.back();
            midiStrip->m_id = instrument->getId();

            // For each controller...
            for (size_t controllerIndex = 0;
                 controllerIndex < controls.size();
                 ++controllerIndex) {

                // ??? Is this really the right way to detect a controller that
                //     is centered?  And how does this actually affect Rotary?
                //     There are no comments on Rotary's ctor.
                const bool centred =
                        (controls[controllerIndex].getDefault() == 64);

                Rotary *controller = new Rotary(
                        m_tabFrame,  // parent
                        controls[controllerIndex].getMin(),  // minimum
                        controls[controllerIndex].getMax(),  // maximum
                        1.0,  // step
                        5.0,  // pageStep
                        controls[controllerIndex].getDefault(),  // initialPosition
                        20,  // size
                        Rotary::NoTicks,  // ticks
                        false,  // snapToTicks
                        centred);

                // Color
                QColor knobColour = QColor(Qt::white);
                if (controls[controllerIndex].getColourIndex() > 0) {
                    knobColour = m_document->getComposition().
                            getGeneralColourMap().getColour(
                                    controls[controllerIndex].getColourIndex());
                }
                controller->setKnobColour(knobColour);

                connect(controller, &Rotary::valueChanged,
                        this, &MidiMixerWindow::slotControllerChanged);

                gridLayout->addWidget(
                        controller, controllerIndex, col, Qt::AlignCenter);

                midiStrip->m_controllerRotaries.push_back(
                        std::pair<MidiByte, Rotary *>(
                                controls[controllerIndex].getControllerNumber(),
                                controller));
            }

            // VU meter
            MidiMixerVUMeter *meter = new MidiMixerVUMeter(
                    m_tabFrame,  // parent
                    VUMeter::FixedHeightVisiblePeakHold,  // type
                    6,  // width
                    30);  // height
            gridLayout->addWidget(meter, controls.size(), col, Qt::AlignCenter);
            midiStrip->m_vuMeter = meter;

            // Volume
            Fader *fader = new Fader(
                    0,  // min
                    127,  // max
                    100,  // i_default
                    20,  // i_width
                    80,  // i_height
                    m_tabFrame);  // parent
            connect(fader, &Fader::faderChanged,
                    this, &MidiMixerWindow::slotFaderLevelChanged);
            gridLayout->addWidget(
                    fader, controls.size() + 1, col, Qt::AlignCenter);
            midiStrip->m_volumeFader = fader;

            // Instrument number
            QLabel *instrumentNumberLabel = new QLabel(
                    QString("%1").arg(stripNum++),
                    m_tabFrame);
            gridLayout->addWidget(
                    instrumentNumberLabel,  // widget
                    controls.size() + 2,  // row
                    col,  // column
                    Qt::AlignCenter);  // alignment

            // Update all the faders and controllers for this Instrument.
            updateWidgets(instrument);

            ++col;
        }
    }
}

void
MidiMixerWindow::slotFaderLevelChanged(float value)
{
    const QObject * const l_sender = sender();

    // For each MidiStrip...
    // ??? Linear search.  Why not add the instrument ID to the fader
    //     object as a QObject property?  That would eliminate this.
    //     Look through AudioMixerWindow2 as well.
    for (std::shared_ptr<const MidiStrip> midiStrip : m_midiStrips) {
        // If this is the one that is changing...
        if (midiStrip->m_volumeFader == l_sender) {
            Instrument *instrument =
                    m_studio->getInstrumentById(midiStrip->m_id);
            if (!instrument)
                return;

            instrument->setControllerValue(MIDI_CONTROLLER_VOLUME, MidiByte(value));
            Instrument::emitControlChange(instrument, MIDI_CONTROLLER_VOLUME);

            m_document->setModified();

            // Have an external controller port?  Send it there as well.
            if (ExternalController::self().isNative()  &&
                instrument->hasFixedChannel())
            {
                int currentTabIndex = m_tabWidget->currentIndex();
                if (currentTabIndex < 0)
                    currentTabIndex = 0;

                int loopTabIndex = 0;

                // For each Device...
                // ??? We should keep a table of tab index to MidiDevice *.
                //     Then all this becomes a one-liner.  At the very least
                //     factor this out into a routine to get started.  This
                //     is done 3 or more times in this code.  Find MidiDevice
                //     for the current tab.
                for (const Device *device : m_studio->getDevicesRef()) {
                    RG_DEBUG << "slotFaderLevelChanged: i = " << loopTabIndex << ", tabIndex " << currentTabIndex;
                    const MidiDevice *midiDevice =
                            dynamic_cast<const MidiDevice *>(device);
                    if (!midiDevice)
                        continue;

                    if (loopTabIndex != currentTabIndex) {
                        ++loopTabIndex;
                        continue;
                    }

                    RG_DEBUG << "slotFaderLevelChanged: device id = " << instrument->getDevice()->getId() << ", visible device id " << midiDevice->getId();

                    // ??? Isn't this redundant?  We've already found the one
                    //     the tab is referencing.  This should always be true.
                    if (instrument->getDevice()->getId() == midiDevice->getId()) {
                        RG_DEBUG << "slotFaderLevelChanged: sending control device mapped event for channel " << instrument->getNaturalMidiChannel();

                        ExternalController::send(
                                instrument->getNaturalMidiChannel(),
                                MIDI_CONTROLLER_VOLUME,
                                MidiByte(value));
                    }

                    break;
                }
            }

            return;
        }
    }
}

void
MidiMixerWindow::slotControllerChanged(float value)
{
    const QObject *l_sender = sender();
    size_t midiStripIndex = 0;
    size_t rotaryIndex = 0;

    // For each MidiStrip...
    // ??? We could avoid this search by storing the InstrumentId and
    //     controller number in each Rotary.
    for (midiStripIndex = 0;
         midiStripIndex < m_midiStrips.size();
         ++midiStripIndex) {
        // For each Rotary...
        for (rotaryIndex = 0;
             rotaryIndex < m_midiStrips[midiStripIndex]->m_controllerRotaries.size();
             ++rotaryIndex) {
            // Found the Rotary?
            if (m_midiStrips[midiStripIndex]->m_controllerRotaries[rotaryIndex].second == l_sender)
                break;
        }

        // Found the Rotary?
        if (rotaryIndex != m_midiStrips[midiStripIndex]->m_controllerRotaries.size())
            break;
    }

    // MIDI strip not found?  Bail.
    if (midiStripIndex == m_midiStrips.size())
        return;
    // Rotary not found?  Bail.
    if (rotaryIndex == m_midiStrips[midiStripIndex]->m_controllerRotaries.size())
        return;

    //RG_DEBUG << "slotControllerChanged - found a controller";

    Instrument *instrument = m_studio->getInstrumentById(
                            m_midiStrips[midiStripIndex]->m_id);
    if (!instrument)
        return;

    //RG_DEBUG << "slotControllerChanged() - got instrument to change";

    const MidiByte controllerNumber =
            m_midiStrips[midiStripIndex]->m_controllerRotaries[rotaryIndex].first;

    instrument->setControllerValue(controllerNumber, MidiByte(value));
    Instrument::emitControlChange(instrument, controllerNumber);
    m_document->setModified();

    if (ExternalController::self().isNative()  &&
        instrument->hasFixedChannel()) {

        // Send out the external controller port as well.

        int currentTabIndex = m_tabWidget->currentIndex();
        if (currentTabIndex < 0)
            currentTabIndex = 0;
        int tabIndex = 0;
        // For each Device in the Studio...
        //for (DeviceList::const_iterator deviceIter = m_studio->begin();
        //     deviceIter != m_studio->end();
        //     ++deviceIter) {
        for (const Device *device : m_studio->getDevicesRef()) {
            RG_DEBUG << "slotControllerChanged(): tabIndex = " << tabIndex << ", currentTabIndex " << currentTabIndex;
            const MidiDevice *midiDevice = dynamic_cast<const MidiDevice *>(device);
            if (!midiDevice)
                continue;
            // Not the current tab?  Try the next.
            if (tabIndex != currentTabIndex) {
                ++tabIndex;
                continue;
            }

            RG_DEBUG << "slotControllerChanged(): device id = " << instrument->getDevice()->getId() << ", visible device id " << midiDevice->getId();

            // If this MidiDevice is the Instrument's...
            if (midiDevice->getId() == instrument->getDevice()->getId()) {
                RG_DEBUG << "slotControllerChanged(): sending external controller mapped event for channel " << instrument->getNaturalMidiChannel();
                // send out to external controller port as well.
                // !!! really want some notification of whether we have any!
                ExternalController::send(
                        instrument->getNaturalMidiChannel(),
                        controllerNumber,
                        MidiByte(value));

                break;
            }
        }
    }
}

void
MidiMixerWindow::updateWidgets(const Instrument *i_instrument)
{
    //RG_DEBUG << "updateWidgets(): Instrument ID = " << instrument->getId();

    // ??? Feels like both of these loops could go away if we inline this
    //     routine inside its only caller.  If we inline this carefully,
    //     I suspect it ends up being only about ten lines of code.

    const InstrumentId instrumentId = i_instrument->getId();

    size_t midiStripIndex = 0;

    // For each device in the Studio
    // ??? There is only one caller of this routine.  And they happen to
    //     have the MidiDevice *, so there is no need for this Device search
    //     loop.
    for (const Device *device : m_studio->getDevicesRef()) {
        const MidiDevice *midiDevice = dynamic_cast<const MidiDevice *>(device);
        // If this isn't a MidiDevice, try the next.
        if (!midiDevice)
            continue;

        InstrumentVector instruments = midiDevice->getPresentationInstruments();

        // For each Instrument in the Device...
        // ??? If this is just to get the strip index, the caller already
        //     has that and should send it in!!!
        for (const Instrument *loopInstrument : instruments) {

            // Is this the Instrument we are updating?
            if (loopInstrument->getId() == instrumentId) {

                // Volume fader
                MidiByte volumeValue;
                try {
                    volumeValue = loopInstrument->
                            getControllerValue(MIDI_CONTROLLER_VOLUME);
                } catch (std::string& s) {
                    // This should never get called.
                    volumeValue = loopInstrument->getVolume();
                }

                // setFader() emits a signal.  If we don't block it, we crash
                // due to an endless loop.
                // ??? Can we make Fader::setFader() only emit a signal on a
                //     user change?  Then blockSignals() would be unnecessary.
                m_midiStrips[midiStripIndex]->m_volumeFader->blockSignals(true);
                m_midiStrips[midiStripIndex]->m_volumeFader->setFader(float(volumeValue));
                m_midiStrips[midiStripIndex]->m_volumeFader->blockSignals(false);

                //RG_DEBUG << "STATIC CONTROLS SIZE = " << (*iIt)->getStaticControllers().size();

                ControlList controls = getIPBControlParameters(midiDevice);

                // Set all controllers for this Instrument
                //
                for (size_t i = 0; i < controls.size(); ++i) {
                    float value = 0.0;

                    // The ControllerValues might not yet be set on
                    // the actual Instrument so don't always expect
                    // to find one.  There might be a hole here for
                    // deleted Controllers to hang around on
                    // Instruments..
                    //
                    try {
                        value = float(loopInstrument->getControllerValue
                                      (controls[i].getControllerNumber()));
                    } catch (const std::string &s) {
                        //RG_DEBUG << "slotUpdateInstrument - can't match controller " << int(controls[i].getControllerNumber()) << " - \"" << s << "\"";
                        continue;
                    }

                    //RG_DEBUG << "MidiMixerWindow::slotUpdateInstrument - MATCHED " << int(controls[i].getControllerNumber());

                    m_midiStrips[midiStripIndex]->m_controllerRotaries[i].second->setPosition(value);
                }

                // ??? Aren't we done at this point.  Can't we break?
            }
            ++midiStripIndex;
        }
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

    // ??? Why doesn't the signal already have this for us?
    const MidiByte controllerValue = instrument->getControllerValue(
            controllerNumber);

    // Find the appropriate strip index given the InstrumentId.

    size_t stripIndex = 0;
    bool found = false;

    // ??? Performance: LINEAR SEARCH
    //     We've got to be able to do better.  A
    //     std::map<InstrumentId, StripIndex> or similar should be better.

    // For each Device in the Studio...
    for (const Device *device : m_studio->getDevicesRef()) {
        const MidiDevice *midiDevice = dynamic_cast<const MidiDevice *>(device);
        // If this isn't a MidiDevice, try the next.
        if (!midiDevice)
            continue;

        InstrumentVector instruments = midiDevice->getPresentationInstruments();

        // For each Instrument in the Device...
        for (const Instrument *currentInstrument : instruments) {
            if (currentInstrument->getId() == instrument->getId()) {
                found = true;
                break;
            }

            ++stripIndex;
        }

        if (found)
            break;
    }

    // If the strip wasn't found, bail.
    if (!found)
        return;

    if (stripIndex >= m_midiStrips.size())
        return;

    // Based on the controllerNumber, update the appropriate Fader or Rotary.

    if (controllerNumber == MIDI_CONTROLLER_VOLUME) {

        // Update the volume Fader.

        // setFader() emits a signal.  If we don't block it, we crash
        // due to an endless loop.
        // ??? Can we make Fader::setFader() only emit a signal on a
        //     user change?  Then blockSignals() would be unnecessary.
        m_midiStrips[stripIndex]->m_volumeFader->blockSignals(true);
        m_midiStrips[stripIndex]->m_volumeFader->setFader(controllerValue);
        m_midiStrips[stripIndex]->m_volumeFader->blockSignals(false);

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
                m_midiStrips[stripIndex]->m_controllerRotaries[controllerIndex].
                        second->setPosition(controllerValue);
                break;
            }
        }

    }
}

void
MidiMixerWindow::updateMeters()
{
    // For each strip...
    for (size_t stripIndex = 0;
         stripIndex != m_midiStrips.size();
         ++stripIndex) {
        LevelInfo info;
        if (!SequencerDataBlock::getInstance()->getInstrumentLevelForMixer(
                m_midiStrips[stripIndex]->m_id, info)) {
            continue;
        }
        if (m_midiStrips[stripIndex]->m_vuMeter) {
            m_midiStrips[stripIndex]->m_vuMeter->setLevel(double(info.level / 127.0));
            RG_DEBUG << "updateMeters() - level  " << info.level;
        } else {
            RG_DEBUG << "updateMeters() - m_vuMeter for m_faders[" << stripIndex << "] is nullptr!";
        }
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

    int tabIndex = m_tabWidget->currentIndex();

    int loopTabIndex = 0;

    // For each Device in the Studio...
    for (const Device *device : m_studio->getDevicesRef()) {

        const MidiDevice *midiDevice = dynamic_cast<const MidiDevice *>(device);
        if (!midiDevice)
            continue;

        if (loopTabIndex != tabIndex) {
            ++loopTabIndex;
            continue;
        }

        // At this point, we've found the Device for the current tab.
        // ??? Why not a std::vector<MidiDevice *> member variable?  Then this
        //     device loop is unnecessary.

        // Next we need the Instrument for a specific channel in the Device.

        const InstrumentVector instruments =
                midiDevice->getPresentationInstruments();

        // For each Instrument in the Device...
        for (Instrument *instrument : instruments) {

            // Not the right one?  Try the next.
            if (instrument->getNaturalMidiChannel() != channel)
                continue;

            // Finally we need the specific controller on the Instrument.
            // ??? WHY?  This doesn't appear to do anything other than filter
            //     out controllers it cannot find in the MidiDevice.

            const ControlList controllerList =
                    midiDevice->getControlParameters();

            // For each Controller in the Instrument...
            // ??? MidiDevice::getControlParameter() already does this.
            for (const ControlParameter &controlParameter : controllerList) {
                // If this is the right controller...
                if (controlParameter.getControllerNumber() == controllerNumber) {
                    RG_DEBUG << "slotExternalController(): Setting controller " << controllerNumber << " for instrument " << instrument->getId() << " to " << value;

                    instrument->setControllerValue(controllerNumber, value);
                    Instrument::emitControlChange(instrument, controllerNumber);
                    m_document->setModified();

                    break;
                }
            }

            break;
        }

        break;
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

    const int currentTabIndex = m_tabWidget->currentIndex();
    if (currentTabIndex < 0)
        return;

    int loopTabIndex = 0;

    // For each Device in the Studio...
    for (const Device *device: m_studio->getDevicesRef()) {

        const MidiDevice *midiDevice = dynamic_cast<const MidiDevice *>(device);
        // Not a MIDI device?  Then try the next.
        if (!midiDevice)
            continue;

        // Not the MidiDevice for the current tab?  Try the next.
        if (loopTabIndex != currentTabIndex) {
            // Keep count of the MidiDevice objects.
            ++loopTabIndex;
            continue;
        }

        // Found the MidiDevice for the current tab.

        const InstrumentVector instruments =
                midiDevice->getPresentationInstruments();

        // For each Instrument...
        for (const Instrument *instrument : instruments) {

            // No fixed channel?  Try the next.
            if (!instrument->hasFixedChannel())
                continue;

            ExternalController::sendAllCCs(instrument);

        }

        break;
    }
}

void
MidiMixerWindow::slotSynchronise()
{
    RG_DEBUG << "MidiMixer::slotSynchronise";

    // This is connected to DeviceManagerDialog::deviceNamesChanged() but it
    // does nothing.

    // ??? We should probably connect to document changed and refresh
    //     everything.  See what AudioMixerWindow does and follow its lead.

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
    // ??? Instrument::getStaticControllers() might simplify all
    //     this quite a bit.  See RosegardenMainWindow::changeEvent().

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
    QWidget::changeEvent(event);

    // We only care about this if the external controller port is
    // in Rosegarden native mode.
    if (!ExternalController::self().isNative())
        return;

    // ??? Double updates seem to go out so we might want to be a little
    //     more picky about the event we react to.

    if (event->type() != QEvent::ActivationChange)
        return;

    if (!isActiveWindow())
        return;

    ExternalController::self().activeWindow =
            ExternalController::MidiMixer;

    sendControllerRefresh();
}


}
