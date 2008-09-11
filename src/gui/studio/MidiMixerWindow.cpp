/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MidiMixerWindow.h"
#include <QLayout>

#include "sound/Midi.h"
#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/notation/NotePixmapFactory.h"
#include "gui/seqmanager/SequencerMapper.h"
#include "gui/widgets/Fader.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/VUMeter.h"
#include "MidiMixerVUMeter.h"
#include "MixerWindow.h"
#include "sound/MappedEvent.h"
#include "StudioControl.h"
#include <kaction.h>
#include <kmainwindow.h>
#include <kstandardaction.h>
#include <qshortcut.h>
#include <QColor>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QTabWidget>
#include <QWidget>


namespace Rosegarden
{

MidiMixerWindow::MidiMixerWindow(QWidget *parent,
                                 RosegardenGUIDoc *document):
        MixerWindow(parent, document),
        m_tabFrame(0)
{
    // Initial setup
    //
    setupTabs();

    KStandardAction::close(this,
                      SLOT(slotClose()),
                      actionCollection());

    QIcon icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                             ("transport-play")));
    KAction *play = new KAction(i18n("&Play"), icon, Qt::Key_Enter, this,
                SIGNAL(play()), actionCollection(), "play");
    // Alternative shortcut for Play
    KShortcut playShortcut = play->shortcut();
    playShortcut.append( KKey(Key_Return + CTRL) );
    play->setShortcut(playShortcut);

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Qt::Key_Insert, this,
                SIGNAL(stop()), actionCollection(), "stop");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Qt::Key_End, this,
                SIGNAL(rewindPlayback()), actionCollection(),
                "playback_pointer_back_bar");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Qt::Key_PageDown, this,
                SIGNAL(fastForwardPlayback()), actionCollection(),
                "playback_pointer_forward_bar");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
                SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
                "playback_pointer_start");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
                SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
                "playback_pointer_end");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-record")));
    new KAction(i18n("&Record"), icon, 0, this,
                SIGNAL(record()), actionCollection(),
                "record");

    icon = QIcon(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                    ("transport-panic")));
    new KAction(i18n("Panic"), icon, Qt::Key_P + CTRL + ALT, this,
                SIGNAL(panic()), actionCollection(),
                "panic");

    createGUI("midimixer.rc");

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
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    connect(m_tabWidget, SIGNAL(currentChanged(QWidget *)),
            this, SLOT(slotCurrentTabChanged(QWidget *)));
    m_tabWidget->setTabPosition(QTabWidget::Bottom);
    setCaption(i18n("MIDI Mixer"));

    for (it = m_studio->begin(); it != m_studio->end(); ++it) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev) {
            // Get the control parameters that are on the IPB (and hence can
            // be shown here too).
            //
            ControlList controls = dev->getIPBControlParameters();

            instruments = dev->getPresentationInstruments();

            // Don't add a frame for empty devices
            //
            if (!instruments.size())
                continue;

            m_tabFrame = new QFrame(m_tabWidget);
            m_tabFrame->setFrameStyle(QFrame::StyledPanel);
            m_tabFrame->setMargin(10);

            QGridLayout *mainLayout = new QGridLayout
                                      (m_tabFrame, instruments.size() + 4, controls.size() + 4, 5);

            // MIDI Mixer label
            //
            //QLabel *label = new QLabel(QString("%1 %2").
            //arg(strtoqstr(dev->getName()))
            //.arg(i18n("MIDI Mixer")), m_tabFrame);

            QLabel *label = new QLabel("", m_tabFrame);
            mainLayout->addWidget(label, 0, 0, 0- 0+1, 16- 1, Qt::AlignCenter);

            // control labels
            for (unsigned int i = 0; i < controls.size(); ++i) {
                label = new QLabel(strtoqstr(controls[i].getName()), m_tabFrame);
                mainLayout->addWidget(label, i + 1, 0, Qt::AlignCenter);
            }

            // meter label
            //
            //label = new QLabel(i18n("Meter"), m_tabFrame);
            //mainLayout->addWidget(label,
            //controls.size() + 1, 0, Qt::AlignCenter);

            // volume label
            label = new QLabel(i18n("Volume"), m_tabFrame);
            mainLayout->addWidget(label, controls.size() + 2, 0,
                                  Qt::AlignCenter);

            // instrument label
            label = new QLabel(i18n("Instrument"), m_tabFrame);
            mainLayout->addWidget(label, controls.size() + 3, 0,
                                  Qt::AlignCenter);

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
                for (unsigned int i = 0; i < controls.size(); ++i) {
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

                // Pan rotary
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
                //fader->setFader(float((*iIt)->getVolume()));

                // Label
                //
                QLabel *idLabel = new QLabel(QString("%1").
                                             arg((*iIt)->getId() - firstInstrument + 1),
                                             m_tabFrame, "idLabel");

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
                slotUpdateInstrument((*iIt)->getId());

                // Increment counters
                //
                posCount++;
                faderCount++;
            }

            QString name = QString("%1 (%2)").arg(strtoqstr(dev->getName()))
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

    for (FaderVector::const_iterator it = m_faders.begin();
            it != m_faders.end(); ++it) {
        if ((*it)->m_volumeFader == s) {
            Instrument *instr = m_studio->
                                getInstrumentById((*it)->m_id);

            if (instr) {

                instr->setVolume(MidiByte(value));

                MappedEvent mE((*it)->m_id,
                               MappedEvent::MidiController,
                               MIDI_CONTROLLER_VOLUME,
                               MidiByte(value));
                StudioControl::sendMappedEvent(mE);

                // send out to external controllers as well.
                //!!! really want some notification of whether we have any!
                int tabIndex = m_tabWidget->currentPageIndex();
                if (tabIndex < 0)
                    tabIndex = 0;
                int i = 0;
                for (DeviceList::const_iterator dit = m_studio->begin();
                        dit != m_studio->end(); ++dit) {
                    RG_DEBUG << "slotFaderLevelChanged: i = " << i << ", tabIndex " << tabIndex << endl;
                    if (!dynamic_cast<MidiDevice*>(*dit))
                        continue;
                    if (i != tabIndex) {
                        ++i;
                        continue;
                    }
                    RG_DEBUG << "slotFaderLevelChanged: device id = " << instr->getDevice()->getId() << ", visible device id " << (*dit)->getId() << endl;
                    if (instr->getDevice()->getId() == (*dit)->getId()) {
                        RG_DEBUG << "slotFaderLevelChanged: sending control device mapped event for channel " << instr->getMidiChannel() << endl;
                        mE.setRecordedChannel(instr->getMidiChannel());
                        mE.setRecordedDevice(Device::CONTROL_DEVICE);
                        StudioControl::sendMappedEvent(mE);
                    }
                    break;
                }
            }

            emit instrumentParametersChanged((*it)->m_id);
            return ;
        }
    }
}

void
MidiMixerWindow::slotControllerChanged(float value)
{
    const QObject *s = sender();
    unsigned int i = 0, j = 0;

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

    Instrument *instr = m_studio->getInstrumentById(
                            m_faders[i]->m_id);

    if (instr) {

        //RG_DEBUG << "MidiMixerWindow::slotControllerChanged - "
        //<< "got instrument to change" << endl;

        if (m_faders[i]->m_controllerRotaries[j].first ==
                MIDI_CONTROLLER_PAN)
            instr->setPan(MidiByte(value));
        else {
            instr->setControllerValue(m_faders[i]->
                                      m_controllerRotaries[j].first,
                                      MidiByte(value));
        }

        MappedEvent mE(m_faders[i]->m_id,
                       MappedEvent::MidiController,
                       m_faders[i]->m_controllerRotaries[j].first,
                       MidiByte(value));
        StudioControl::sendMappedEvent(mE);

        int tabIndex = m_tabWidget->currentPageIndex();
        if (tabIndex < 0)
            tabIndex = 0;
        int i = 0;
        for (DeviceList::const_iterator dit = m_studio->begin();
                dit != m_studio->end(); ++dit) {
            RG_DEBUG << "slotControllerChanged: i = " << i << ", tabIndex " << tabIndex << endl;
            if (!dynamic_cast<MidiDevice*>(*dit))
                continue;
            if (i != tabIndex) {
                ++i;
                continue;
            }
            RG_DEBUG << "slotControllerChanged: device id = " << instr->getDevice()->getId() << ", visible device id " << (*dit)->getId() << endl;
            if (instr->getDevice()->getId() == (*dit)->getId()) {
                RG_DEBUG << "slotControllerChanged: sending control device mapped event for channel " << instr->getMidiChannel() << endl;
                // send out to external controllers as well.
                //!!! really want some notification of whether we have any!
                mE.setRecordedChannel(instr->getMidiChannel());
                mE.setRecordedDevice(Device::CONTROL_DEVICE);
                StudioControl::sendMappedEvent(mE);
            }
        }

        emit instrumentParametersChanged(m_faders[i]->m_id);
    }
}

void
MidiMixerWindow::slotUpdateInstrument(InstrumentId id)
{
    //RG_DEBUG << "MidiMixerWindow::slotUpdateInstrument - id = " << id << endl;

    DeviceListConstIterator it;
    MidiDevice *dev = 0;
    InstrumentList instruments;
    InstrumentList::const_iterator iIt;
    int count = 0;

    blockSignals(true);

    for (it = m_studio->begin(); it != m_studio->end(); ++it) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev) {
            instruments = dev->getPresentationInstruments();
            ControlList controls = dev->getIPBControlParameters();

            for (iIt = instruments.begin(); iIt != instruments.end(); ++iIt) {
                // Match and set
                //
                if ((*iIt)->getId() == id) {
                    // Set Volume fader
                    //
                    m_faders[count]->m_volumeFader->blockSignals(true);
                    m_faders[count]->m_volumeFader->
                    setFader(float((*iIt)->getVolume()));
                    m_faders[count]->m_volumeFader->blockSignals(false);

                    /*
                    StaticControllers &staticControls = 
                        (*iIt)->getStaticControllers();
                    RG_DEBUG << "STATIC CONTROLS SIZE = " 
                             << staticControls.size() << endl;
                    */

                    // Set all controllers for this Instrument
                    //
                    for (unsigned int i = 0; i < controls.size(); ++i) {
                        float value = 0.0;

                        m_faders[count]->m_controllerRotaries[i].second->blockSignals(true);

                        if (controls[i].getControllerValue() ==
                                MIDI_CONTROLLER_PAN) {
                            m_faders[count]->m_controllerRotaries[i].
                            second->setPosition((*iIt)->getPan());
                        } else {
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
                                /*
                                RG_DEBUG << 
                                "MidiMixerWindow::slotUpdateInstrument - "
                                         << "can't match controller " 
                                         << int(controls[i].
                                             getControllerValue()) << " - \""
                                         << s << "\"" << endl;
                                         */
                                continue;
                            }

                            /*
                            RG_DEBUG << "MidiMixerWindow::slotUpdateInstrument"
                                     << " - MATCHED "
                                     << int(controls[i].getControllerValue())
                                     << endl;
                                     */

                            m_faders[count]->m_controllerRotaries[i].
                            second->setPosition(value);
                        }

                        m_faders[count]->m_controllerRotaries[i].second->blockSignals(false);
                    }
                }
                count++;
            }
        }
    }

    blockSignals(false);
}

void
MidiMixerWindow::updateMeters(SequencerMapper *mapper)
{
    for (unsigned int i = 0; i != m_faders.size(); ++i) {
        LevelInfo info;
        if (!mapper->
                getInstrumentLevelForMixer(m_faders[i]->m_id, info))
            continue;
        m_faders[i]->m_vuMeter->setLevel(double(info.level / 127.0));
        RG_DEBUG << "MidiMixerWindow::updateMeters - level  " << info.level << endl;
    }
}

void
MidiMixerWindow::updateMonitorMeter(SequencerMapper *)
{
    // none here
}

void
MidiMixerWindow::slotControllerDeviceEventReceived(MappedEvent *e,
        const void *preferredCustomer)
{
    if (preferredCustomer != this)
        return ;
    RG_DEBUG << "MidiMixerWindow::slotControllerDeviceEventReceived: this one's for me" << endl;
    raise();

    // get channel number n from event
    // get nth instrument on current tab

    if (e->getType() != MappedEvent::MidiController)
        return ;
    unsigned int channel = e->getRecordedChannel();
    MidiByte controller = e->getData1();
    MidiByte value = e->getData2();

    int tabIndex = m_tabWidget->currentPageIndex();

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

            if (instrument->getMidiChannel() != channel)
                continue;

            switch (controller) {

            case MIDI_CONTROLLER_VOLUME:
                RG_DEBUG << "Setting volume for instrument " << instrument->getId() << " to " << value << endl;
                instrument->setVolume(value);
                break;

            case MIDI_CONTROLLER_PAN:
                RG_DEBUG << "Setting pan for instrument " << instrument->getId() << " to " << value << endl;
                instrument->setPan(value);
                break;

            default: {
                    ControlList cl = dev->getIPBControlParameters();
                    for (ControlList::const_iterator i = cl.begin();
                            i != cl.end(); ++i) {
                        if ((*i).getControllerValue() == controller) {
                            RG_DEBUG << "Setting controller " << controller << " for instrument " << instrument->getId() << " to " << value << endl;
                            instrument->setControllerValue(controller, value);
                            break;
                        }
                    }
                    break;
                }
            }

            MappedEvent mE(instrument->getId(),
                           MappedEvent::MidiController,
                           MidiByte(controller),
                           MidiByte(value));
            StudioControl::sendMappedEvent(mE);

            slotUpdateInstrument(instrument->getId());
            emit instrumentParametersChanged(instrument->getId());
        }

        break;
    }
}

void
MidiMixerWindow::slotCurrentTabChanged(QWidget *)
{
    sendControllerRefresh();
}

void
MidiMixerWindow::sendControllerRefresh()
{
    //!!! need to know if we have a current external controller device,
    // as this is expensive

    int tabIndex = m_tabWidget->currentPageIndex();
    RG_DEBUG << "MidiMixerWindow::slotCurrentTabChanged: current is " << tabIndex << endl;

    if (tabIndex < 0)
        return ;

    int i = 0;

    for (DeviceList::const_iterator dit = m_studio->begin();
            dit != m_studio->end(); ++dit) {

        MidiDevice *dev = dynamic_cast<MidiDevice*>(*dit);
        RG_DEBUG << "device is " << (*dit)->getId() << ", dev " << dev << endl;

        if (!dev)
            continue;
        if (i != tabIndex) {
            ++i;
            continue;
        }

        InstrumentList instruments = dev->getPresentationInstruments();
        ControlList controls = dev->getIPBControlParameters();

        RG_DEBUG << "device has " << instruments.size() << " presentation instruments, " << dev->getAllInstruments().size() << " instruments " << endl;

        for (InstrumentList::const_iterator iIt =
                    instruments.begin(); iIt != instruments.end(); ++iIt) {

            Instrument *instrument = *iIt;
            int channel = instrument->getMidiChannel();

            RG_DEBUG << "instrument is " << instrument->getId() << endl;

            for (ControlList::const_iterator cIt =
                        controls.begin(); cIt != controls.end(); ++cIt) {

                int controller = (*cIt).getControllerValue();
                int value;
                if (controller == MIDI_CONTROLLER_PAN) {
                    value = instrument->getPan();
                } else {
                    try {
                        value = instrument->getControllerValue(controller);
                    } catch (std::string s) {
                        std::cerr << "Exception in MidiMixerWindow::currentChanged: " << s << " (controller " << controller << ", instrument " << instrument->getId() << ")" << std::endl;
                        value = 0;
                    }
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
            RG_DEBUG << "sending controller mapped event for channel " << channel << ", volume " << instrument->getVolume() << endl;
            StudioControl::sendMappedEvent(mE);
        }

        break;
    }
}

void
MidiMixerWindow::slotSynchronise()
{
    RG_DEBUG << "MidiMixer::slotSynchronise" << endl;
    //setupTabs();
}

}
#include "MidiMixerWindow.moc"
