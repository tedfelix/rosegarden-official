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
            // Don't include devices without instruments.
            // ??? Is this even possible?
            if (midiDevice->getPresentationInstruments().size() == 0)
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

    // Tab widget
    m_tabWidget = new QTabWidget(this);
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &MidiMixerWindow::slotCurrentTabChanged);
    m_tabWidget->setTabPosition(QTabWidget::South);
    setCentralWidget(m_tabWidget);

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

    connect(&ExternalController::self(),
                &ExternalController::externalControllerMMW,
            this, &MidiMixerWindow::slotExternalController);

    // Connect for RosegardenDocument changes.
    connect(RosegardenDocument::currentDocument,
                    &RosegardenDocument::documentModified,
            this, &MidiMixerWindow::slotDocumentModified);

    // Make sure we close if the document is changing.
    connect(RosegardenMainWindow::self(),
                    &RosegardenMainWindow::documentAboutToChange,
            this, &QWidget::close);

    // Let RMW know we are closing.
    connect(this, &MixerWindow::closing,
            RosegardenMainWindow::self(),
                    &RosegardenMainWindow::slotMidiMixerClosed);

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

    // ??? This is done only once.  But the number of devices is dynamic and
    //     can change during a run.  We need to monitor for changes to the
    //     Studio and update this display to match.  AudioMixerWindow2 does
    //     this.  See AudioMixerWindow2::updateWidgets().

    const MidiDeviceVector devices = getMidiOutputDevices(m_studio);

    int deviceCount = 1;

    // For each MidiDevice in the Studio...
    for (const MidiDevice *midiDevice : devices) {
        InstrumentVector instruments = midiDevice->getPresentationInstruments();

        // Get the control parameters that are on the IPB (and hence can
        // be shown here too).
        const ControlList controls = midiDevice->getIPBControlParameters();

        QFrame *page = new QFrame(m_tabWidget);
        page->setProperty("deviceID", midiDevice->getId());
        page->setContentsMargins(10, 10, 10, 10);
        const QString name = QString("%1 (%2)").
                arg(QObject::tr(midiDevice->getName().c_str())).
                arg(deviceCount++);
        // Add the page to the QTabWidget.
        m_tabWidget->addTab(page, name);

        QHBoxLayout *layout = new QHBoxLayout(page);

        int stripNum = 1;

        // For each Instrument in this MidiDevice...
        for (const Instrument *instrument : instruments) {
            const InstrumentId instrumentId = instrument->getId();

            // Add a new MidiStrip.
            MidiStrip *midiStrip = new MidiStrip(page, instrumentId, stripNum++);
            layout->addWidget(midiStrip);
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

void MidiMixerWindow::slotDocumentModified(bool /*modified*/)
{
    // Count number of devices in studio that would be tabs.
    const MidiDeviceVector devices = getMidiOutputDevices(m_studio);
    const size_t studioDeviceCount = devices.size();

    const size_t tabCount = m_tabWidget->count();

    // No change in the number of devices?
    if (studioDeviceCount == tabCount) {
        int tabIndex = 0;

        // For each device in the studio, fix the tab name if needed.
        for (const MidiDevice *device : devices) {
            const QString deviceName = QString("%1 (%2)").
                    arg(QObject::tr(device->getName().c_str())).
                    arg(tabIndex + 1);

            // Make sure the device's name matches the name on its tab.
            if (m_tabWidget->tabText(tabIndex) != deviceName)
                m_tabWidget->setTabText(tabIndex, deviceName);

            ++tabIndex;
        }

        return;
    }

    // Number of devices has changed.  Recreate all tabs.

    // Works, but leaks at runtime.  Pages are not deleted.
    //m_tabWidget->clear();

    // While there are still tab pages, delete the first.
    while (m_tabWidget->count() != 0) {
        QWidget *page = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        // QTabWidget::clear() does not delete the pages, so we have to roll
        // our own clear loop.  Otherwise we leak at runtime.
        delete page;
    }

    setupTabs();
}


}
