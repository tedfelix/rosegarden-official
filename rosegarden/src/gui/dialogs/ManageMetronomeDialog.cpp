/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ManageMetronomeDialog.h"
#include <qlayout.h>

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/Studio.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/PitchChooser.h"
#include "sound/MappedEvent.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qcheckbox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

ManageMetronomeDialog::ManageMetronomeDialog(QWidget *parent,
        RosegardenGUIDoc *doc) :
        KDialogBase(parent, 0, true, i18n("Metronome"), Ok | Apply | Close | Help),
        m_doc(doc)
{
    setHelp("studio-metronome");

    QHBox *hbox = makeHBoxMainWidget();

    // I think having this as well probably just overcomplicates things
    m_instrumentParameterBox = 0;
    //    m_instrumentParameterBox = new InstrumentParameterBox(doc, hbox);

    QVBox *vbox = new QVBox(hbox);

    QGroupBox *deviceBox = new QGroupBox
                           (1, Horizontal, i18n("Metronome Instrument"), vbox);

    QFrame *frame = new QFrame(deviceBox);
    QGridLayout *layout = new QGridLayout(frame, 2, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Device"), frame), 0, 0);
    m_metronomeDevice = new KComboBox(frame);
    layout->addWidget(m_metronomeDevice, 0, 1);

    DeviceList *devices = doc->getStudio().getDevices();
    DeviceListConstIterator it;

    Studio &studio = m_doc->getStudio();
    DeviceId deviceId = studio.getMetronomeDevice();

    for (it = devices->begin(); it != devices->end(); it++) {
        MidiDevice *dev =
            dynamic_cast<MidiDevice*>(*it);

        if (dev && dev->getDirection() == MidiDevice::Play) {
            QString label = strtoqstr(dev->getName());
            QString connection = strtoqstr(dev->getConnection());
            label += " - ";
            if (connection == "")
                label += i18n("No connection");
            else
                label += connection;
            m_metronomeDevice->insertItem(label);
            if (dev->getId() == deviceId) {
                m_metronomeDevice->setCurrentItem(m_metronomeDevice->count() - 1);
            }
        }
    }

    layout->addWidget(new QLabel(i18n("Instrument"), frame), 1, 0);
    m_metronomeInstrument = new KComboBox(frame);
    connect(m_metronomeInstrument, SIGNAL(activated(int)), this, SLOT(slotSetModified()));
    connect(m_metronomeInstrument, SIGNAL(activated(int)), this, SLOT(slotInstrumentChanged(int)));
    layout->addWidget(m_metronomeInstrument, 1, 1);

    QGroupBox *beatBox = new QGroupBox
                         (1, Horizontal, i18n("Beats"), vbox);

    frame = new QFrame(beatBox);
    layout = new QGridLayout(frame, 4, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Resolution"), frame), 0, 0);
    m_metronomeResolution = new KComboBox(frame);
    m_metronomeResolution->insertItem(i18n("None"));
    m_metronomeResolution->insertItem(i18n("Bars only"));
    m_metronomeResolution->insertItem(i18n("Bars and beats"));
    m_metronomeResolution->insertItem(i18n("Bars, beats, and divisions"));
    connect(m_metronomeResolution, SIGNAL(activated(int)), this, SLOT(slotResolutionChanged(int)));
    layout->addWidget(m_metronomeResolution, 0, 1);

    layout->addWidget(new QLabel(i18n("Bar velocity"), frame), 1, 0);
    m_metronomeBarVely = new QSpinBox(frame);
    m_metronomeBarVely->setMinValue(0);
    m_metronomeBarVely->setMaxValue(127);
    connect(m_metronomeBarVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeBarVely, 1, 1);

    layout->addWidget(new QLabel(i18n("Beat velocity"), frame), 2, 0);
    m_metronomeBeatVely = new QSpinBox(frame);
    m_metronomeBeatVely->setMinValue(0);
    m_metronomeBeatVely->setMaxValue(127);
    connect(m_metronomeBeatVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeBeatVely, 2, 1);

    layout->addWidget(new QLabel(i18n("Sub-beat velocity"), frame), 3, 0);
    m_metronomeSubBeatVely = new QSpinBox(frame);
    m_metronomeSubBeatVely->setMinValue(0);
    m_metronomeSubBeatVely->setMaxValue(127);
    connect(m_metronomeSubBeatVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeSubBeatVely, 3, 1);

    vbox = new QVBox(hbox);

    m_metronomePitch = new PitchChooser(i18n("Pitch"), vbox, 60);
    connect(m_metronomePitch, SIGNAL(pitchChanged(int)), this, SLOT(slotPitchChanged(int)));
    connect(m_metronomePitch, SIGNAL(preview(int)), this, SLOT(slotPreviewPitch(int)));

    m_metronomePitchSelector = new KComboBox(m_metronomePitch);
    m_metronomePitchSelector->insertItem(i18n("for Bar"));
    m_metronomePitchSelector->insertItem(i18n("for Beat"));
    m_metronomePitchSelector->insertItem(i18n("for Sub-beat"));
    connect(m_metronomePitchSelector, SIGNAL(activated(int)), this, SLOT(slotPitchSelectorChanged(int)));

    QGroupBox *enableBox = new QGroupBox
                           (1, Horizontal, i18n("Metronome Activated"), vbox);
    m_playEnabled = new QCheckBox(i18n("Playing"), enableBox);
    m_recordEnabled = new QCheckBox(i18n("Recording"), enableBox);
    connect(m_playEnabled, SIGNAL(clicked()), this, SLOT(slotSetModified()));
    connect(m_recordEnabled, SIGNAL(clicked()), this, SLOT(slotSetModified()));

    // populate the dialog
    populate(m_metronomeDevice->currentItem());

    // connect up the device list
    connect(m_metronomeDevice, SIGNAL(activated(int)),
            this, SLOT(populate(int)));
    // connect up the device list
    connect(m_metronomeDevice, SIGNAL(activated(int)),
            this, SLOT(slotSetModified()));

    setModified(false);
}

void
ManageMetronomeDialog::slotResolutionChanged(int depth)
{
    m_metronomeBeatVely->setEnabled(depth > 1);
    m_metronomeSubBeatVely->setEnabled(depth > 2);
    slotSetModified();
}

void
ManageMetronomeDialog::populate(int deviceIndex)
{
    m_metronomeInstrument->clear();

    DeviceList *devices = m_doc->getStudio().getDevices();
    DeviceListConstIterator it;
    int count = 0;
    MidiDevice *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev && dev->getDirection() == MidiDevice::Play) {
            if (count == deviceIndex)
                break;

            count++;
        }
    }

    // sanity
    if (count < 0 || dev == 0) {
        if (m_instrumentParameterBox)
            m_instrumentParameterBox->useInstrument(0);
        return ;
    }

    // populate instrument list
    InstrumentList list = dev->getPresentationInstruments();
    InstrumentList::iterator iit;

    const MidiMetronome *metronome = dev->getMetronome();

    // if we've got no metronome against this device then create one
    if (metronome == 0) {
        InstrumentId id = SystemInstrumentBase;

        for (iit = list.begin(); iit != list.end(); ++iit) {
            if ((*iit)->isPercussion()) {
                id = (*iit)->getId();
                break;
            }
        }

        dev->setMetronome(MidiMetronome(id));

        metronome = dev->getMetronome();
    }

    // metronome should now be set but we still check it
    if (metronome) {
        int position = 0;
        int count = 0;
        for (iit = list.begin(); iit != list.end(); ++iit) {
            QString iname(strtoqstr((*iit)->getPresentationName()));
            QString pname(strtoqstr((*iit)->getProgramName()));
            if (pname != "")
                iname += " (" + pname + ")";

            bool used = false;
            for (Composition::trackcontainer::iterator tit =
                        m_doc->getComposition().getTracks().begin();
                    tit != m_doc->getComposition().getTracks().end(); ++tit) {

                if (tit->second->getInstrument() == (*iit)->getId()) {
                    used = true;
                    break;
                }
            }

            //	    if (used) iname = i18n("%1 [used]").arg(iname);

            m_metronomeInstrument->insertItem(iname);

            if ((*iit)->getId() == metronome->getInstrument()) {
                position = count;
            }
            count++;
        }
        m_metronomeInstrument->setCurrentItem(position);
        slotInstrumentChanged(position);

        m_barPitch = metronome->getBarPitch();
        m_beatPitch = metronome->getBeatPitch();
        m_subBeatPitch = metronome->getSubBeatPitch();
        slotPitchSelectorChanged(0);
        m_metronomeResolution->setCurrentItem(metronome->getDepth());
        m_metronomeBarVely->setValue(metronome->getBarVelocity());
        m_metronomeBeatVely->setValue(metronome->getBeatVelocity());
        m_metronomeSubBeatVely->setValue(metronome->getSubBeatVelocity());
        m_playEnabled->setChecked(m_doc->getComposition().usePlayMetronome());
        m_recordEnabled->setChecked(m_doc->getComposition().useRecordMetronome());
        slotResolutionChanged(metronome->getDepth());
    }
}

void
ManageMetronomeDialog::slotInstrumentChanged(int i)
{
    if (!m_instrumentParameterBox)
        return ;

    int deviceIndex = m_metronomeDevice->currentItem();

    DeviceList *devices = m_doc->getStudio().getDevices();
    DeviceListConstIterator it;
    int count = 0;
    MidiDevice *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev && dev->getDirection() == MidiDevice::Play) {
            if (count == deviceIndex)
                break;

            count++;
        }
    }

    // sanity
    if (count < 0 || dev == 0) {
        m_instrumentParameterBox->useInstrument(0);
        return ;
    }

    // populate instrument list
    InstrumentList list = dev->getPresentationInstruments();

    if (i < 0 || i >= (int)list.size())
        return ;

    m_instrumentParameterBox->useInstrument(list[i]);
}

void
ManageMetronomeDialog::slotOk()
{
    slotApply();
    accept();
}

void
ManageMetronomeDialog::slotSetModified()
{
    setModified(true);
}

void
ManageMetronomeDialog::setModified(bool value)
{
    if (m_modified == value)
        return ;

    if (value) {
        enableButtonApply(true);
    } else {
        enableButtonApply(false);
    }

    m_modified = value;
}

void
ManageMetronomeDialog::slotApply()
{
    Studio &studio = m_doc->getStudio();

    DeviceList *devices = m_doc->getStudio().getDevices();
    DeviceListConstIterator it;
    int count = 0;
    MidiDevice *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev && dev->getDirection() == MidiDevice::Play) {
            if (count == m_metronomeDevice->currentItem())
                break;

            count++;
        }
    }

    if (!dev) {
        std::cerr << "Warning: ManageMetronomeDialog::slotApply: no " << m_metronomeDevice->currentItem() << "th device" << std::endl;
        return ;
    }

    DeviceId deviceId = dev->getId();
    studio.setMetronomeDevice(deviceId);

    if (dev->getMetronome() == 0)
        return ;
    MidiMetronome metronome(*dev->getMetronome());

    // get instrument
    InstrumentList list = dev->getPresentationInstruments();

    Instrument *inst =
        list[m_metronomeInstrument->currentItem()];

    if (inst) {
        metronome.setInstrument(inst->getId());
    }

    metronome.setBarPitch(m_barPitch);
    metronome.setBeatPitch(m_beatPitch);
    metronome.setSubBeatPitch(m_subBeatPitch);

    metronome.setDepth(
        m_metronomeResolution->currentItem());

    metronome.setBarVelocity(
        MidiByte(m_metronomeBarVely->value()));

    metronome.setBeatVelocity(
        MidiByte(m_metronomeBeatVely->value()));

    metronome.setSubBeatVelocity(
        MidiByte(m_metronomeSubBeatVely->value()));

    dev->setMetronome(metronome);

    m_doc->getComposition().setPlayMetronome(m_playEnabled->isChecked());
    m_doc->getComposition().setRecordMetronome(m_recordEnabled->isChecked());

    m_doc->getSequenceManager()->metronomeChanged(inst->getId(), true);
    m_doc->slotDocumentModified();
    setModified(false);
}

void
ManageMetronomeDialog::slotPreviewPitch(int pitch)
{
    RG_DEBUG << "ManageMetronomeDialog::slotPreviewPitch" << endl;

    DeviceList *devices = m_doc->getStudio().getDevices();
    DeviceListConstIterator it;
    int count = 0;
    MidiDevice *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {
        dev = dynamic_cast<MidiDevice*>(*it);

        if (dev && dev->getDirection() == MidiDevice::Play) {
            if (count == m_metronomeDevice->currentItem())
                break;

            count++;
        }
    }

    if (!dev)
        return ;

    const MidiMetronome *metronome = dev->getMetronome();
    if (metronome == 0)
        return ;

    InstrumentList list = dev->getPresentationInstruments();

    Instrument *inst =
        list[m_metronomeInstrument->currentItem()];

    if (inst) {
        RG_DEBUG << "ManageMetronomeDialog::slotPreviewPitch"
        << " - previewing" << endl;
        MappedEvent mE(inst->getId(),
                       MappedEvent::MidiNoteOneShot,
                       pitch,
                       MidiMaxValue,
                       RealTime::zeroTime,
                       RealTime(0, 10000000),
                       RealTime::zeroTime);

        StudioControl::sendMappedEvent(mE);
    }
}

void
ManageMetronomeDialog::slotPitchChanged(int pitch)
{
    switch (m_metronomePitchSelector->currentItem()) {
    case 0:
        m_barPitch = pitch;
        break;
    case 1:
        m_beatPitch = pitch;
        break;
    case 2:
        m_subBeatPitch = pitch;
        break;
    }
    setModified(true);
}

void
ManageMetronomeDialog::slotPitchSelectorChanged(int selection)
{
    switch (selection) {
    case 0:
        m_metronomePitch->slotSetPitch(m_barPitch);
        break;
    case 1:
        m_metronomePitch->slotSetPitch(m_beatPitch);
        break;
    case 2:
        m_metronomePitch->slotSetPitch(m_subBeatPitch);
        break;
    }
}

}
#include "ManageMetronomeDialog.moc"
