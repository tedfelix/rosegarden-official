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


#include "ManageMetronomeDialog.h"
#include <QLayout>

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/Studio.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/PitchChooser.h"
#include "sound/MappedEvent.h"
#include "sound/PluginIdentifier.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

ManageMetronomeDialog::ManageMetronomeDialog(QDialogButtonBox::QWidget *parent,
        RosegardenGUIDoc *doc) :
        QDialog(parent),
        m_doc(doc)
{
    setHelp("studio-metronome");

    setModal(true);
    setWindowTitle(i18n("Metronome"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *hbox = new QWidget(this);
    QHBoxLayout hboxLayout = new QHBoxLayout;
    metagrid->addWidget(hbox, 0, 0);


    // I think having this as well probably just overcomplicates things
    m_instrumentParameterBox = 0;
    //    m_instrumentParameterBox = new InstrumentParameterBox(doc, hbox);

    QWidget *vbox = new QWidget(hbox);
    QVBoxLayout vboxLayout = new QVBoxLayout;
    hboxLayout->addWidget(vbox);

    QGroupBox *deviceBox = new QGroupBox( i18n("Metronome Instrument"), vbox );
    vboxLayout->addWidget(deviceBox);

    QFrame *frame = new QFrame(deviceBox);
    QGridLayout *layout = new QGridLayout(frame, 2, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Device"), frame), 0, 0);
    m_metronomeDevice = new QComboBox(frame);
    layout->addWidget(m_metronomeDevice, 0, 1);

    DeviceList *devices = doc->getStudio().getDevices();
    DeviceListConstIterator it;

    Studio &studio = m_doc->getStudio();
    DeviceId deviceId = studio.getMetronomeDevice();

    for (it = devices->begin(); it != devices->end(); it++) {

        Device *dev = *it;
        bool hasConnection = false;
        if (!isSuitable(dev, &hasConnection)) continue;

        QString label = strtoqstr(dev->getName());
        QString connection = strtoqstr(dev->getConnection());

        if (hasConnection && connection != "") {
            label = i18n("%1 - %2").arg(label).arg(connection);
        } else if (!hasConnection) {
            label = i18n("%1 - No connection").arg(label);
        }
        m_metronomeDevice->addItem(label);
        if (dev->getId() == deviceId) {
            m_metronomeDevice->setCurrentIndex(m_metronomeDevice->count() - 1);
        }
    }

    layout->addWidget(new QLabel(i18n("Instrument"), frame), 1, 0);
    m_metronomeInstrument = new QComboBox(frame);
    connect(m_metronomeInstrument, SIGNAL(activated(int)), this, SLOT(slotSetModified()));
    connect(m_metronomeInstrument, SIGNAL(activated(int)), this, SLOT(slotInstrumentChanged(int)));
    layout->addWidget(m_metronomeInstrument, 1, 1);

    QGroupBox *beatBox = new QGroupBox( i18n("Beats"), vbox );
    vboxLayout->addWidget(beatBox);

    frame = new QFrame(beatBox);
    layout = new QGridLayout(frame, 4, 2, 10, 5);

    layout->addWidget(new QLabel(i18n("Resolution"), frame), 0, 0);
    m_metronomeResolution = new QComboBox(frame);
    m_metronomeResolution->addItem(i18n("None"));
    m_metronomeResolution->addItem(i18n("Bars only"));
    m_metronomeResolution->addItem(i18n("Bars and beats"));
    m_metronomeResolution->addItem(i18n("Bars, beats, and divisions"));
    connect(m_metronomeResolution, SIGNAL(activated(int)), this, SLOT(slotResolutionChanged(int)));
    layout->addWidget(m_metronomeResolution, 0, 1);

    layout->addWidget(new QLabel(i18n("Bar velocity"), frame), 1, 0);
    m_metronomeBarVely = new QSpinBox(frame);
    m_metronomeBarVely->setMinimum(0);
    m_metronomeBarVely->setMaximum(127);
    connect(m_metronomeBarVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeBarVely, 1, 1);

    layout->addWidget(new QLabel(i18n("Beat velocity"), frame), 2, 0);
    m_metronomeBeatVely = new QSpinBox(frame);
    m_metronomeBeatVely->setMinimum(0);
    m_metronomeBeatVely->setMaximum(127);
    connect(m_metronomeBeatVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeBeatVely, 2, 1);

    layout->addWidget(new QLabel(i18n("Sub-beat velocity"), frame), 3, 0);
    m_metronomeSubBeatVely = new QSpinBox(frame);
    m_metronomeSubBeatVely->setMinimum(0);
    m_metronomeSubBeatVely->setMaximum(127);
    connect(m_metronomeSubBeatVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    layout->addWidget(m_metronomeSubBeatVely, 3, 1);

    vbox = new QVBox( hbox );
    hboxLayout->addWidget(vbox);
    hbox->setLayout(hboxLayout);

    m_metronomePitch = new PitchChooser(i18n("Pitch"), vbox , 60);
    vboxLayout->addWidget(m_metronomePitch);
    connect(m_metronomePitch, SIGNAL(pitchChanged(int)), this, SLOT(slotPitchChanged(int)));
    connect(m_metronomePitch, SIGNAL(preview(int)), this, SLOT(slotPreviewPitch(int)));

    m_metronomePitchSelector = new QComboBox(m_metronomePitch);
    m_metronomePitchSelector->addItem(i18n("for Bar"));
    m_metronomePitchSelector->addItem(i18n("for Beat"));
    m_metronomePitchSelector->addItem(i18n("for Sub-beat"));
    connect(m_metronomePitchSelector, SIGNAL(activated(int)), this, SLOT(slotPitchSelectorChanged(int)));

    QGroupBox *enableBox = new QGroupBox( i18n("Metronome Activated"), vbox );
    vboxLayout->addWidget(enableBox);
    vbox->setLayout(vboxLayout);
    m_playEnabled = new QCheckBox(i18n("Playing"), enableBox);
    m_recordEnabled = new QCheckBox(i18n("Recording"), enableBox);
    connect(m_playEnabled, SIGNAL(clicked()), this, SLOT(slotSetModified()));
    connect(m_recordEnabled, SIGNAL(clicked()), this, SLOT(slotSetModified()));

    // populate the dialog
    populate(m_metronomeDevice->currentIndex());

    // connect up the device list
    connect(m_metronomeDevice, SIGNAL(activated(int)),
            this, SLOT(populate(int)));
    // connect up the device list
    connect(m_metronomeDevice, SIGNAL(activated(int)),
            this, SLOT(slotSetModified()));

    setModified(false);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Close | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
    Device *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {

        dev = *it;
        if (!isSuitable(dev)) continue;

        if (count == deviceIndex) break;
        count++;
    }

    // sanity
    if (count < 0 || dev == 0 || !isSuitable(dev)) {
        if (m_instrumentParameterBox)
            m_instrumentParameterBox->useInstrument(0);
        return ;
    }

    // populate instrument list
    InstrumentList list = dev->getPresentationInstruments();
    InstrumentList::iterator iit;

    const MidiMetronome *metronome = getMetronome(dev);

    // if we've got no metronome against this device then create one
    if (metronome == 0) {
        InstrumentId id = SystemInstrumentBase;

        for (iit = list.begin(); iit != list.end(); ++iit) {
            if ((*iit)->isPercussion()) {
                id = (*iit)->getId();
                break;
            }
        }

        setMetronome(dev, MidiMetronome(id));

        metronome = getMetronome(dev);
    }

    // metronome should now be set but we still check it
    if (metronome) {
        int position = 0;
        int count = 0;

        for (iit = list.begin(); iit != list.end(); ++iit) {

            QString iname(strtoqstr((*iit)->getName()));
            QString ipname(strtoqstr((*iit)->getPresentationName()));
            QString pname(strtoqstr((*iit)->getProgramName()));

            QString text;

            if ((*iit)->getType() == Instrument::SoftSynth) {

                iname.replace(i18n("Synth plugin "), "");
                pname = "";

                AudioPluginInstance *plugin = (*iit)->getPlugin
                    (Instrument::SYNTH_PLUGIN_POSITION);
                if (plugin) {
                    pname = strtoqstr(plugin->getProgram());
                    QString identifier = strtoqstr(plugin->getIdentifier());
                    if (identifier != "") {
                        QString type, soName, label;
                        PluginIdentifier::parseIdentifier
                            (identifier, type, soName, label);
                        if (pname == "") {
                            pname = strtoqstr(plugin->getDistinctiveConfigurationText());
                        }
                        if (pname != "") {
                            pname = QString("%1: %2").arg(label).arg(pname);
                        } else {
                            pname = label;
                        }
                    }
                }

            } else {

                iname = ipname;
            }

            if (pname != "") {
                text = i18n("%1 (%2)").arg(iname).arg(pname);
            } else {
                text = iname;
            }

            m_metronomeInstrument->addItem(text);

            if ((*iit)->getId() == metronome->getInstrument()) {
                position = count;
            }
            count++;
        }
        m_metronomeInstrument->setCurrentIndex(position);
        slotInstrumentChanged(position);

        m_barPitch = metronome->getBarPitch();
        m_beatPitch = metronome->getBeatPitch();
        m_subBeatPitch = metronome->getSubBeatPitch();
        slotPitchSelectorChanged(0);
        m_metronomeResolution->setCurrentIndex(metronome->getDepth());
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

    int deviceIndex = m_metronomeDevice->currentIndex();

    DeviceList *devices = m_doc->getStudio().getDevices();
    DeviceListConstIterator it;
    int count = 0;
    Device *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {

        dev = *it;
        if (!isSuitable(dev)) continue;

        if (count == deviceIndex) break;
        count++;
    }

    // sanity
    if (count < 0 || dev == 0 || !isSuitable(dev)) {
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
    Device *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {

        dev = *it;
        if (!isSuitable(dev)) continue;

        if (count == m_metronomeDevice->currentIndex()) break;
        count++;
    }

    if (!dev || !isSuitable(dev)) {
        std::cerr << "Warning: ManageMetronomeDialog::slotApply: no " << m_metronomeDevice->currentIndex() << "th device" << std::endl;
        return ;
    }

    DeviceId deviceId = dev->getId();
    studio.setMetronomeDevice(deviceId);

    if (getMetronome(dev) == 0) return ;
    MidiMetronome metronome(*getMetronome(dev));

    // get instrument
    InstrumentList list = dev->getPresentationInstruments();

    Instrument *inst =
        list[m_metronomeInstrument->currentIndex()];

    if (inst) {
        metronome.setInstrument(inst->getId());
    }

    metronome.setBarPitch(m_barPitch);
    metronome.setBeatPitch(m_beatPitch);
    metronome.setSubBeatPitch(m_subBeatPitch);

    metronome.setDepth(
        m_metronomeResolution->currentIndex());

    metronome.setBarVelocity(
        MidiByte(m_metronomeBarVely->value()));

    metronome.setBeatVelocity(
        MidiByte(m_metronomeBeatVely->value()));

    metronome.setSubBeatVelocity(
        MidiByte(m_metronomeSubBeatVely->value()));

    setMetronome(dev, metronome);

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
    Device *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {

        dev = *it;
        if (!isSuitable(dev)) continue;

        if (count == m_metronomeDevice->currentIndex()) break;
        count++;
    }

    if (!dev || !isSuitable(dev)) return;

    const MidiMetronome *metronome = getMetronome(dev);
    if (metronome == 0) return;

    InstrumentList list = dev->getPresentationInstruments();

    Instrument *inst =
        list[m_metronomeInstrument->currentIndex()];

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
    switch (m_metronomePitchSelector->currentIndex()) {
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

bool
ManageMetronomeDialog::isSuitable(Device *dev, bool *hasConnectionReturn)
{
    MidiDevice *md = dynamic_cast<MidiDevice *>(dev);
    if (md && md->getDirection() == MidiDevice::Play) {
        if (hasConnectionReturn) {
            if (md->getConnection() == "") *hasConnectionReturn = false;
            else *hasConnectionReturn = true;
        }
        return true;
    }
    if (dynamic_cast<SoftSynthDevice *>(dev)) {
        if (hasConnectionReturn) *hasConnectionReturn = true;
        return true;
    }
    return false;
}

void
ManageMetronomeDialog::setMetronome(Device *dev, const MidiMetronome &metronome)
{
    MidiDevice *md = dynamic_cast<MidiDevice *>(dev);
    if (md) {
        md->setMetronome(metronome);
        return;
    }
    SoftSynthDevice *ssd = dynamic_cast<SoftSynthDevice *>(dev);
    if (ssd) {
        ssd->setMetronome(metronome);
        return;
    }
}

const MidiMetronome *
ManageMetronomeDialog::getMetronome(Device *dev)
{
    MidiDevice *md = dynamic_cast<MidiDevice *>(dev);
    if (md) {
        return md->getMetronome();
    }
    SoftSynthDevice *ssd = dynamic_cast<SoftSynthDevice *>(dev);
    if (ssd) {
        return ssd->getMetronome();
    }
    return 0;
}


}
#include "ManageMetronomeDialog.moc"
