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

#define RG_MODULE_STRING "[MidiMixerWindow]"

// Disable RG_DEBUG output.  Must be defined prior to including Debug.h.
#define RG_NO_DEBUG_PRINT

#include "MidiMixerWindow.h"

#include "sound/Midi.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/VUMeter.h"
#include "gui/general/IconLoader.h"
#include "gui/general/ActionFileClient.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "gui/dialogs/AboutDialog.h"
#include "MidiMixerVUMeter.h"
#include "MixerWindow.h"
#include "sound/MappedEvent.h"
#include "sound/SequencerDataBlock.h"
#include "StudioControl.h"

#include <QAction>
#include <QColor>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QLayout>
#include <QShortcut>
#include <QDesktopServices>
#include <QToolButton>
#include <QToolBar>


namespace Rosegarden
{

MidiMixerWindow::MidiMixerWindow(QWidget *parent,
                                 RosegardenDocument *document):
    MixerWindow(parent, document),
    m_tabFrame(0)
{
    // Initial setup
    //
    setupTabs();

    createAction("file_close", SLOT(slotClose()));

    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("record", SIGNAL(record()));
    createAction("panic", SIGNAL(panic()));
    createAction("midimix_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    createMenusAndToolbars("midimixer.rc");

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "playback_pointer_back_bar");
    enableAutoRepeat("Transport Toolbar", "playback_pointer_forward_bar");

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            this,
            SLOT(slotInstrumentChanged(Instrument *)));
    connect(Instrument::getStaticSignals().data(),
                SIGNAL(controlChange(Instrument *, int)),
            SLOT(slotControlChange(Instrument *, int)));
}

void
MidiMixerWindow::setupTabs()
{
    DeviceListConstIterator it;
    MidiDevice *dev = 0;
    InstrumentList instruments;
    InstrumentList::const_iterator iIt;
    int faderCount = 0, deviceCount = 1;

    if (m_tabFrame)
        delete m_tabFrame;

    // Setup m_tabFrame
    //

    QWidget *blackWidget = new QWidget(this);
    setCentralWidget(blackWidget);
    QVBoxLayout *centralLayout = new QVBoxLayout;
    blackWidget->setLayout(centralLayout);

    m_tabWidget = new QTabWidget;
    centralLayout->addWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(slotCurrentTabChanged(int)));
    m_tabWidget->setTabPosition(QTabWidget::South);
    setWindowTitle(tr("MIDI Mixer"));
    setWindowIcon(IconLoader().loadPixmap("window-midimixer"));


    for (it = m_studio->begin(); it != m_studio->end(); ++it) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev) {
            // Get the control parameters that are on the IPB (and hence can
            // be shown here too).
            //
            ControlList controls = getIPBForMidiMixer(dev);

            instruments = dev->getPresentationInstruments();

            // Don't add a frame for empty devices
            //
            if (!instruments.size())
                continue;

            m_tabFrame = new QFrame(m_tabWidget);
            m_tabFrame->setContentsMargins(10, 10, 10, 10);

            // m_tabFrame->setContentsMargins(5, 5, 5, 5); ???
            QGridLayout *mainLayout = new QGridLayout(m_tabFrame);

            // MIDI Mixer label
            QLabel *label = new QLabel("", m_tabFrame);
            mainLayout->addWidget(label, 0, 0, 0, 16, Qt::AlignCenter);

            // control labels
            for (size_t i = 0; i < controls.size(); ++i) {
                label = new QLabel(QObject::tr(controls[i].getName().c_str()), m_tabFrame);
                mainLayout->addWidget(label, i + 1, 0, Qt::AlignCenter);
            }

            // meter label
            // (obsolete abandoned code deleted here)

            // volume label
            label = new QLabel(tr("Volume"), m_tabFrame);
            mainLayout->addWidget(label, controls.size() + 2, 0,
                                  Qt::AlignCenter);

            // instrument label
            label = new QLabel(tr("Instrument"), m_tabFrame);
            label->setFixedWidth(80); //!!! this should come from metrics
            mainLayout->addWidget(label, controls.size() + 3, 0,
                                  Qt::AlignLeft);

            int posCount = 1;
            int firstInstrument = -1;

            for (iIt = instruments.begin(); iIt != instruments.end(); ++iIt) {

                // Add new fader struct
                //
                m_faders.push_back(new FaderStruct());

                // Store the first ID
                //
                if (firstInstrument == -1)
                    firstInstrument = (*iIt)->getId();


                // Add the controls
                //
                for (size_t i = 0; i < controls.size(); ++i) {
                    QColor knobColour = QColor(Qt::white);

                    if (controls[i].getColourIndex() > 0) {
                        Colour c =
                            m_document->getComposition().getGeneralColourMap().
                            getColourByIndex(controls[i].getColourIndex());

                        knobColour = QColor(c.getRed(),
                                            c.getGreen(), c.getBlue());
                    }

                    Rotary *controller =
                        new Rotary(m_tabFrame,
                                   controls[i].getMin(),
                                   controls[i].getMax(),
                                   1.0,
                                   5.0,
                                   controls[i].getDefault(),
                                   20,
                                   Rotary::NoTicks,
                                   false,
                                   controls[i].getDefault() == 64); //!!! hacky

                    controller->setKnobColour(knobColour);

                    connect(controller, SIGNAL(valueChanged(float)),
                            this, SLOT(slotControllerChanged(float)));

                    mainLayout->addWidget(controller, i + 1, posCount,
                                          Qt::AlignCenter);

                    // Store the rotary
                    //
                    m_faders[faderCount]->m_controllerRotaries.push_back(
                        std::pair<MidiByte, Rotary*>
                        (controls[i].getControllerValue(), controller));
                }

                // VU meter
                //
                MidiMixerVUMeter *meter =
                    new MidiMixerVUMeter(m_tabFrame,
                                         VUMeter::FixedHeightVisiblePeakHold, 6, 30);
                mainLayout->addWidget(meter, controls.size() + 1,
                                      posCount, Qt::AlignCenter);
                m_faders[faderCount]->m_vuMeter = meter;

                // Volume fader
                //
                Fader *fader =
                    new Fader(0, 127, 100, 20, 80, m_tabFrame);
                mainLayout->addWidget(fader, controls.size() + 2,
                                      posCount, Qt::AlignCenter);
                m_faders[faderCount]->m_volumeFader = fader;

                // Label
                //
                QLabel *idLabel = new QLabel(QString("%1").
                                             arg((*iIt)->getId() - firstInstrument + 1),
                                             m_tabFrame);
                idLabel->setObjectName("idLabel");

                mainLayout->addWidget(idLabel, controls.size() + 3,
                                      posCount, Qt::AlignCenter);

                // store id in struct
                m_faders[faderCount]->m_id = (*iIt)->getId();

                // Connect them up
                //
                connect(fader, SIGNAL(faderChanged(float)),
                        this, SLOT(slotFaderLevelChanged(float)));

                // Update all the faders and controllers
                //
                slotInstrumentChanged(*iIt);

                // Increment counters
                //
                posCount++;
                faderCount++;
            }

            QString name = QString("%1 (%2)")
                           .arg(QObject::tr(dev->getName().c_str()))
                           .arg(deviceCount++);

            addTab(m_tabFrame, name);
        }
    }
}

void
MidiMixerWindow::addTab(QWidget *tab, const QString &title)
{
    m_tabWidget->addTab(tab, title);
}

void
MidiMixerWindow::slotFaderLevelChanged(float value)
{
    const QObject *s = sender();

    // For each fader
    // ??? Wouldn't QSignalMapper be better?
    for (FaderVector::const_iterator it = m_faders.begin();
            it != m_faders.end(); ++it) {
        if ((*it)->m_volumeFader == s) {
            Instrument *instrument = m_studio->
                                getInstrumentById((*it)->m_id);

            if (instrument) {

                instrument->setControllerValue(MIDI_CONTROLLER_VOLUME, MidiByte(value));
                Instrument::getStaticSignals()->
                        emitControlChange(instrument, MIDI_CONTROLLER_VOLUME);
                m_document->setModified();

                if (instrument->hasFixedChannel())
                {
                    // Send out the external controller port as well.
                    
                    //!!! really want some notification of whether we have any!
                    int tabIndex = m_tabWidget->currentIndex();
                    if (tabIndex < 0)
                        tabIndex = 0;
                    int i = 0;
                    for (DeviceList::const_iterator dit = m_studio->begin();
                         dit != m_studio->end(); ++dit) {
                        RG_DEBUG << "slotFaderLevelChanged: i = " << i << ", tabIndex " << tabIndex;
                        if (!dynamic_cast<MidiDevice*>(*dit))
                            continue;
                        if (i != tabIndex) {
                            ++i;
                            continue;
                        }
                        RG_DEBUG << "slotFaderLevelChanged: device id = " << instrument->getDevice()->getId() << ", visible device id " << (*dit)->getId();
                        if (instrument->getDevice()->getId() == (*dit)->getId()) {
                            RG_DEBUG << "slotFaderLevelChanged: sending control device mapped event for channel " << instrument->getNaturalChannel();

                            MappedEvent mE((*it)->m_id,
                                           MappedEvent::MidiController,
                                           MIDI_CONTROLLER_VOLUME,
                                           MidiByte(value));

                            mE.setRecordedChannel(instrument->getNaturalChannel());
                            mE.setRecordedDevice(Device::CONTROL_DEVICE);
                            StudioControl::sendMappedEvent(mE);
                        }
                        break;
                    }
                }
            }

            return ;
        }
    }
}

void
MidiMixerWindow::slotControllerChanged(float value)
{
    const QObject *s = sender();
    size_t i = 0, j = 0;

    // For each fader
    // ??? Wouldn't QSignalMapper be better?
    for (i = 0; i < m_faders.size(); ++i) {
        for (j = 0; j < m_faders[i]->m_controllerRotaries.size(); ++j) {
            if (m_faders[i]->m_controllerRotaries[j].second == s)
                break;
        }

        // break out on match
        if (j != m_faders[i]->m_controllerRotaries.size())
            break;
    }

    // Don't do anything if we've not matched and got solid values
    // for i and j
    //
    if (i == m_faders.size() || j == m_faders[i]->m_controllerRotaries.size())
        return ;

    //RG_DEBUG << "MidiMixerWindow::slotControllerChanged - found a controller"
    //<< endl;

    Instrument *instrument = m_studio->getInstrumentById(
                            m_faders[i]->m_id);

    if (instrument) {

        //RG_DEBUG << "MidiMixerWindow::slotControllerChanged - "
        //<< "got instrument to change";

        MidiByte cc = m_faders[i]->m_controllerRotaries[j].first;

        instrument->setControllerValue(cc, MidiByte(value));
        Instrument::getStaticSignals()->
                emitControlChange(instrument, cc);
        m_document->setModified();

        if (instrument->hasFixedChannel()) {

            // Send out the external controller port as well.

            int tabIndex = m_tabWidget->currentIndex();
            if (tabIndex < 0)
                tabIndex = 0;
            int k = 0;
            for (DeviceList::const_iterator dit = m_studio->begin();
                 dit != m_studio->end(); ++dit) {
                RG_DEBUG << "slotControllerChanged: k = " << k << ", tabIndex " << tabIndex;
                if (!dynamic_cast<MidiDevice*>(*dit))
                    continue;
                if (k != tabIndex) {
                    ++k;
                    continue;
                }
                RG_DEBUG << "slotControllerChanged: device id = " << instrument->getDevice()->getId() << ", visible device id " << (*dit)->getId();
                if (instrument->getDevice()->getId() == (*dit)->getId()) {
                    RG_DEBUG << "slotControllerChanged: sending control device mapped event for channel " << instrument->getNaturalChannel();
                    // send out to external controllers as well.
                    //!!! really want some notification of whether we have any!
                    MappedEvent mE(m_faders[i]->m_id,
                                   MappedEvent::MidiController,
                                   m_faders[i]->m_controllerRotaries[j].first,
                                   MidiByte(value));
                    mE.setRecordedChannel(instrument->getNaturalChannel());
                    mE.setRecordedDevice(Device::CONTROL_DEVICE);
                    StudioControl::sendMappedEvent(mE);
                }
            }
        }
    }
}

void
MidiMixerWindow::slotInstrumentChanged(Instrument *instrument)
{
    //RG_DEBUG << "slotInstrumentChanged(): Instrument ID = " << instrument->getId();

    int count = 0;

    // For each device in the Studio
    for (DeviceListConstIterator it = m_studio->begin(); it != m_studio->end(); ++it) {
        MidiDevice *dev = dynamic_cast<MidiDevice *>(*it);

        // If this isn't a MidiDevice, try the next.
        if (!dev)
            continue;

        InstrumentList instruments = dev->getPresentationInstruments();

        // For each Instrument in the Device
        for (InstrumentList::const_iterator iIt = instruments.begin();
             iIt != instruments.end(); ++iIt) {
            // Match and set
            //
            if ((*iIt)->getId() == instrument->getId()) {
                // Set Volume fader
                //

                MidiByte volumeValue;
                try {
                    volumeValue = (*iIt)->
                            getControllerValue(MIDI_CONTROLLER_VOLUME);
                } catch (std::string s) {
                    // This should never get called.
                    volumeValue = (*iIt)->getVolume();
                }

                // setFader() emits a signal.  If we don't block it, we crash
                // due to an endless loop.
                // ??? Need to examine more closely and see if we can redesign
                //     things to avoid this crash and remove the
                //     blockSignals() calls around every call to setFader().
                m_faders[count]->m_volumeFader->blockSignals(true);
                m_faders[count]->m_volumeFader->setFader(float(volumeValue));
                m_faders[count]->m_volumeFader->blockSignals(false);

                //RG_DEBUG << "STATIC CONTROLS SIZE = " << (*iIt)->getStaticControllers().size();

                ControlList controls = getIPBForMidiMixer(dev);

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
                        value = float((*iIt)->getControllerValue
                                      (controls[i].getControllerValue()));
                    } catch (std::string s) {
                        //RG_DEBUG << "slotUpdateInstrument - can't match controller " << int(controls[i].getControllerValue()) << " - \"" << s << "\"";
                        continue;
                    }

                    //RG_DEBUG << "MidiMixerWindow::slotUpdateInstrument - MATCHED " << int(controls[i].getControllerValue());

                    m_faders[count]->m_controllerRotaries[i].second->setPosition(value);
                }
            }
            count++;
        }
    }
}

void
MidiMixerWindow::slotControlChange(Instrument *instrument, int cc)
{
    if (!instrument)
        return;

    // Find the appropriate strip.

    size_t stripCount = 0;
    bool found = false;

    // ??? Performance: LINEAR SEARCH
    //     We've got to be able to do better.  A
    //     std::map<InstrumentId, StripIndex> or similar should work well.

    // For each device in the Studio
    for (DeviceListConstIterator deviceIter = m_studio->begin();
         deviceIter != m_studio->end();
         ++deviceIter) {
        MidiDevice *device = dynamic_cast<MidiDevice *>(*deviceIter);

        // If this isn't a MidiDevice, try the next.
        if (!device)
            continue;

        InstrumentList instruments = device->getPresentationInstruments();

        // For each Instrument in the Device
        for (InstrumentList::const_iterator instrumentIter =
                    instruments.begin();
             instrumentIter != instruments.end();
             ++instrumentIter) {

            Instrument *currentInstrument = *instrumentIter;

            if (currentInstrument->getId() == instrument->getId()) {
                found = true;
                break;
            }

            ++stripCount;
        }

        if (found)
            break;
    }

    // If the strip wasn't found, bail.
    if (!found)
        return;

    if (stripCount >= m_faders.size())
        return;

    // At this point, stripCount is the proper index for m_faders.

    if (cc == MIDI_CONTROLLER_VOLUME) {

        // Update the volume fader.

        MidiByte volumeValue;

        try {
            volumeValue = instrument->
                    getControllerValue(MIDI_CONTROLLER_VOLUME);
        } catch (...) {
            // This should never get called.
            volumeValue = instrument->getVolume();
        }

        // setFader() emits a signal.  If we don't block it, we crash
        // due to an endless loop.
        // ??? Need to examine more closely and see if we can redesign
        //     things to avoid this crash and remove the
        //     blockSignals() calls around every call to setFader().
        m_faders[stripCount]->m_volumeFader->blockSignals(true);
        m_faders[stripCount]->m_volumeFader->setFader(float(volumeValue));
        m_faders[stripCount]->m_volumeFader->blockSignals(false);

    } else {

        // Update the appropriate cc rotary.

        ControlList controls = getIPBForMidiMixer(
                dynamic_cast<MidiDevice *>(instrument->getDevice()));

        // For each controller
        for (size_t i = 0; i < controls.size(); ++i) {
            // If this is the one...
            if (cc == controls[i].getControllerValue()) {

                // The ControllerValues might not yet be set on
                // the actual Instrument so don't always expect
                // to find one.  There might be a hole here for
                // deleted Controllers to hang around on
                // Instruments..
                //
                try {
                    MidiByte value = instrument->getControllerValue(cc);
                    m_faders[stripCount]->m_controllerRotaries[i].second->
                            setPosition(value);
                } catch (...) {
                    RG_WARNING << "slotControlChange(): WARNING: cc not found " << cc;
                }

                break;

            }
        }

    }
}

void
MidiMixerWindow::updateMeters()
{
    for (size_t i = 0; i != m_faders.size(); ++i) {
        LevelInfo info;
        if (!SequencerDataBlock::getInstance()->
            getInstrumentLevelForMixer(m_faders[i]->m_id, info)) {
            continue;
        }
        if (m_faders[i]->m_vuMeter) {
            m_faders[i]->m_vuMeter->setLevel(double(info.level / 127.0));
            RG_DEBUG << "MidiMixerWindow::updateMeters - level  " << info.level;
        } else {
            RG_DEBUG << "MidiMixerWindow::updateMeters - m_vuMeter for m_faders[" << i << "] is NULL!";
        }
    }
}

void
MidiMixerWindow::updateMonitorMeter()
{
    // none here
}

void
MidiMixerWindow::slotControllerDeviceEventReceived(MappedEvent *e,
        const void *preferredCustomer)
{
    if (preferredCustomer != this)
        return ;

    RG_DEBUG << "slotControllerDeviceEventReceived(): this one's for me";

    raise();

    // get channel number n from event
    // get nth instrument on current tab

    if (e->getType() != MappedEvent::MidiController)
        return ;
    unsigned int channel = e->getRecordedChannel();
    MidiByte controller = e->getData1();
    MidiByte value = e->getData2();

    int tabIndex = m_tabWidget->currentIndex();

    int i = 0;

    for (DeviceList::const_iterator it = m_studio->begin();
            it != m_studio->end(); ++it) {

        MidiDevice *dev =
            dynamic_cast<MidiDevice*>(*it);

        if (!dev)
            continue;
        if (i != tabIndex) {
            ++i;
            continue;
        }

        InstrumentList instruments = dev->getPresentationInstruments();

        for (InstrumentList::const_iterator iIt =
                    instruments.begin(); iIt != instruments.end(); ++iIt) {

            Instrument *instrument = *iIt;

            if (instrument->getNaturalChannel() != channel)
                continue;

            ControlList cl = dev->getControlParameters();
            for (ControlList::const_iterator controlIter = cl.begin();
                    controlIter != cl.end(); ++controlIter) {
                if ((*controlIter).getControllerValue() == controller) {
                    RG_DEBUG << "slotControllerDeviceEventReceived(): Setting controller " << controller << " for instrument " << instrument->getId() << " to " << value;
                    instrument->setControllerValue(controller, value);
                    Instrument::getStaticSignals()->
                            emitControlChange(instrument, controller);
                    m_document->setModified();

                    break;
                }
            }
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
    // To keep the device connected to the "external controller" port in
    // sync with the "MIDI Mixer" window, send out MIDI volume and pan
    // messages to it.

    // !!! Really want some notification of whether we have a device
    //     connected to the "external controller" port!  Otherwise this
    //     is a waste of time.  Is there a way in ALSA to ask if the port
    //     is connected?

    int tabIndex = m_tabWidget->currentIndex();
    RG_DEBUG << "MidiMixerWindow::slotCurrentTabChanged: current is " << tabIndex;

    if (tabIndex < 0)
        return ;

    int i = 0;

    for (DeviceList::const_iterator dit = m_studio->begin();
            dit != m_studio->end(); ++dit) {

        MidiDevice *dev = dynamic_cast<MidiDevice*>(*dit);
        RG_DEBUG << "device is " << (*dit)->getId() << ", dev " << dev;

        if (!dev)
            continue;
        if (i != tabIndex) {
            ++i;
            continue;
        }

        InstrumentList instruments = dev->getPresentationInstruments();
        ControlList controls = getIPBForMidiMixer(dev);

        RG_DEBUG << "device has" << instruments.size() << "presentation instruments," << dev->getAllInstruments().size() << " instruments";

        for (InstrumentList::const_iterator iIt =
                    instruments.begin(); iIt != instruments.end(); ++iIt) {

            Instrument *instrument = *iIt;
            if (!instrument->hasFixedChannel()) { continue; }
            int channel = instrument->getNaturalChannel();

            RG_DEBUG << "instrument is " << instrument->getId();

            for (ControlList::const_iterator cIt =
                        controls.begin(); cIt != controls.end(); ++cIt) {

                int controller = (*cIt).getControllerValue();
                int value;
                try {
                    value = instrument->getControllerValue(controller);
                } catch (std::string s) {
                    RG_WARNING << "Exception in MidiMixerWindow::currentChanged: " << s << " (controller " << controller << ", instrument " << instrument->getId() << ")";
                    value = 0;
                }

                MappedEvent mE(instrument->getId(),
                               MappedEvent::MidiController,
                               controller, value);
                mE.setRecordedChannel(channel);
                mE.setRecordedDevice(Device::CONTROL_DEVICE);
                StudioControl::sendMappedEvent(mE);
            }

            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiController,
                           MIDI_CONTROLLER_VOLUME,
                           instrument->getVolume());
            mE.setRecordedChannel(channel);
            mE.setRecordedDevice(Device::CONTROL_DEVICE);
            RG_DEBUG << "sending controller mapped event for channel " << channel << ", volume " << instrument->getVolume();
            StudioControl::sendMappedEvent(mE);
        }

        break;
    }
}

void
MidiMixerWindow::slotSynchronise()
{
    RG_DEBUG << "MidiMixer::slotSynchronise";
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

// Code stolen From src/base/MidiDevice
ControlList
MidiMixerWindow::getIPBForMidiMixer(MidiDevice *dev) const
{
    ControlList controlList = dev->getIPBControlParameters();
    ControlList retList;

    Rosegarden::MidiByte MIDI_CONTROLLER_VOLUME = 0x07;

    for (ControlList::const_iterator it = controlList.begin();
         it != controlList.end(); ++it)
    {
        if (it->getIPBPosition() != -1 && 
            it->getControllerValue() != MIDI_CONTROLLER_VOLUME)
            retList.push_back(*it);
    }

    return retList;
}


}
