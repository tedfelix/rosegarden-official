/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ManageMetronomeDialog]"

#include "ManageMetronomeDialog.h"
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
#include "document/RosegardenDocument.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/PitchChooser.h"
#include "sound/MappedEvent.h"
#include "sound/PluginIdentifier.h"
#include "sequencer/RosegardenSequencer.h"

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
#include <QLayout>
#include <QPushButton>



namespace Rosegarden
{

ManageMetronomeDialog::ManageMetronomeDialog(QWidget *parent,
        RosegardenDocument *doc) :
        QDialog(parent),
        m_doc(doc),
        m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | 
                                         QDialogButtonBox::Close))
{
    setModal(true);
    setWindowTitle(tr("Metronome"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *hbox = new QWidget(this);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hboxLayout->setMargin(0);
    metagrid->addWidget(hbox, 0, 0);

    QWidget *vbox = new QWidget(hbox);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vboxLayout->setMargin(0);
    hboxLayout->addWidget(vbox);

    QGroupBox *deviceBox = new QGroupBox( tr("Metronome Instrument"), vbox );
    deviceBox->setContentsMargins(10, 10, 10, 10);
    QGridLayout *deviceBoxLayout = new QGridLayout(deviceBox);
    deviceBoxLayout->setSpacing(5);
    vboxLayout->addWidget(deviceBox);

    deviceBoxLayout->addWidget(new QLabel(tr("Device"), deviceBox), 0, 0);
    m_metronomeDevice = new QComboBox(deviceBox);
    m_metronomeDevice->setToolTip(tr("<qt>Choose the device you want to use to play the metronome</qt>"));
    deviceBoxLayout->addWidget(m_metronomeDevice, 0, 1);

    DeviceList *devices = doc->getStudio().getDevices();
    DeviceListConstIterator it;

    Studio &studio = m_doc->getStudio();
    DeviceId deviceId = studio.getMetronomeDevice();

    for (it = devices->begin(); it != devices->end(); it++) {

        Device *dev = *it;
        bool hasConnection = false;
        if (!isSuitable(dev, &hasConnection)) continue;

        QString label = QObject::tr(dev->getName().c_str());
        // connections imply some untranslatable external string
        QString connection = RosegardenSequencer::getInstance()->getConnection
            (dev->getId());

        if (hasConnection && connection != "") {
            label = tr("%1 - %2").arg(label).arg(connection);
        } else if (!hasConnection) {
            label = tr("%1 - No connection").arg(label);
        }
        m_metronomeDevice->addItem(label);
        if (dev->getId() == deviceId) {
            m_metronomeDevice->setCurrentIndex(m_metronomeDevice->count() - 1);
        }
    }

    deviceBoxLayout->addWidget(new QLabel(tr("Instrument"), deviceBox), 1, 0);
    m_metronomeInstrument = new QComboBox(deviceBox);
    m_metronomeInstrument->setToolTip(tr("<qt>Choose the instrument you want to use to play the metronome (typically #10)</qt>"));
    connect(m_metronomeInstrument, SIGNAL(activated(int)), this, SLOT(slotSetModified()));
    deviceBoxLayout->addWidget(m_metronomeInstrument, 1, 1);
    deviceBox->setLayout(deviceBoxLayout);

    QGroupBox *beatBox = new QGroupBox( tr("Beats"), vbox );
    beatBox->setContentsMargins(10, 10, 10, 10);
    QGridLayout *beatBoxLayout = new QGridLayout(beatBox);
    beatBoxLayout->setSpacing(5);
    vboxLayout->addWidget(beatBox);

    beatBoxLayout->addWidget(new QLabel(tr("Resolution"), beatBox), 0, 0);
    m_metronomeResolution = new QComboBox(beatBox);
    m_metronomeResolution->setToolTip(tr("<qt>The metronome can sound bars only, bars and beats, or bars, beats and sub-beats.  The latter mode can be particularly useful for playing in compound time signatures like 12/8.</qt>"));
    m_metronomeResolution->addItem(tr("None"));
    m_metronomeResolution->addItem(tr("Bars only"));
    m_metronomeResolution->addItem(tr("Bars and beats"));
    m_metronomeResolution->addItem(tr("Bars, beats, and sub-beats"));
    connect(m_metronomeResolution, SIGNAL(activated(int)), this, SLOT(slotResolutionChanged(int)));
    beatBoxLayout->addWidget(m_metronomeResolution, 0, 1);

    beatBoxLayout->addWidget(new QLabel(tr("Bar velocity"), beatBox), 1, 0);
    m_metronomeBarVely = new QSpinBox(beatBox);
    m_metronomeBarVely->setToolTip(tr("<qt>Controls how forcefully the bar division notes will be struck.  (These are typically the loudest of all.)</qt>"));
    m_metronomeBarVely->setMinimum(0);
    m_metronomeBarVely->setMaximum(127);
    connect(m_metronomeBarVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    beatBoxLayout->addWidget(m_metronomeBarVely, 1, 1);

    beatBoxLayout->addWidget(new QLabel(tr("Beat velocity"), beatBox), 2, 0);
    m_metronomeBeatVely = new QSpinBox(beatBox);
    m_metronomeBeatVely->setToolTip(tr("<qt>Controls how forcefully the beat division notes will be struck.  (These are typically more quiet than beat division notes.)</qt>"));
    m_metronomeBeatVely->setMinimum(0);
    m_metronomeBeatVely->setMaximum(127);
    connect(m_metronomeBeatVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    beatBoxLayout->addWidget(m_metronomeBeatVely, 2, 1);

    beatBoxLayout->addWidget(new QLabel(tr("Sub-beat velocity"), beatBox), 3, 0);
    m_metronomeSubBeatVely = new QSpinBox(beatBox);
    m_metronomeSubBeatVely->setToolTip(tr("<qt>Controls how forcefully the sub-beat division notes will be struck.  (These are typically the most quiet of all, and are not heard unless you are working in compound time.)</qt>"));
    m_metronomeSubBeatVely->setMinimum(0);
    m_metronomeSubBeatVely->setMaximum(127);
    connect(m_metronomeSubBeatVely, SIGNAL(valueChanged(int)), this, SLOT(slotSetModified()));
    beatBoxLayout->addWidget(m_metronomeSubBeatVely, 3, 1);
    beatBox->setLayout(beatBoxLayout);

    vbox->setLayout(vboxLayout);

    vbox = new QWidget(hbox);
    vboxLayout = new QVBoxLayout;
    vboxLayout->setMargin(0);
    hboxLayout->addWidget(vbox);
    hbox->setLayout(hboxLayout);

    m_metronomePitch = new PitchChooser(tr("Pitch"), vbox , 60);
    m_metronomePitch->setToolTip(tr("<qt>It is typical to use a percussion instrument for the metronome, so the pitch normally controls which sort of drum will sound for each tick.  You may configure a different pitch for each of the bar, beat, and sub-beat ticks.</qt>"));
    vboxLayout->addWidget(m_metronomePitch);
    connect(m_metronomePitch, SIGNAL(pitchChanged(int)), this, SLOT(slotPitchChanged(int)));
    connect(m_metronomePitch, SIGNAL(preview(int)), this, SLOT(slotPreviewPitch(int)));

    m_metronomePitchSelector = new QComboBox();
    m_metronomePitchSelector->addItem(tr("for Bar"));
    m_metronomePitchSelector->addItem(tr("for Beat"));
    m_metronomePitchSelector->addItem(tr("for Sub-beat"));
    m_metronomePitch->addWidgetToLayout(m_metronomePitchSelector);
    connect(m_metronomePitchSelector, SIGNAL(activated(int)), this, SLOT(slotPitchSelectorChanged(int)));

    QGroupBox *enableBox = new QGroupBox( tr("Metronome Activated"), vbox );
    QVBoxLayout *enableBoxLayout = new QVBoxLayout;
    vboxLayout->addWidget(enableBox);
    m_playEnabled = new QCheckBox(tr("Playing"), enableBox);
    enableBoxLayout->addWidget(m_playEnabled);
    m_recordEnabled = new QCheckBox(tr("Recording"), enableBox);
    enableBoxLayout->addWidget(m_recordEnabled);
    connect(m_playEnabled, SIGNAL(clicked()), this, SLOT(slotSetModified()));
    connect(m_recordEnabled, SIGNAL(clicked()), this, SLOT(slotSetModified()));
    enableBox->setLayout(enableBoxLayout);

    vbox->setLayout(vboxLayout);

    // populate the dialog
    populate(m_metronomeDevice->currentIndex());

    // connect up the device list
    connect(m_metronomeDevice, SIGNAL(activated(int)),
            this, SLOT(populate(int)));
    // connect up the device list
    connect(m_metronomeDevice, SIGNAL(activated(int)),
            this, SLOT(slotSetModified()));

    metagrid->addWidget(m_buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

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
    Device *dev = 0;

    for (it = devices->begin(); it != devices->end(); it++) {

        dev = *it;
        if (!isSuitable(dev)) continue;

        if (count == deviceIndex) break;
        count++;
    }

    // sanity
    if (count < 0 || dev == 0 || !isSuitable(dev)) {
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

            QString iname(QObject::tr((*iit)->getName().c_str()));
            QString ipname((*iit)->getLocalizedPresentationName());
            QString pname(QObject::tr((*iit)->getProgramName().c_str()));

            QString text;

            if ((*iit)->getType() == Instrument::SoftSynth) {

                iname.replace(QObject::tr("Synth plugin "), "");
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
                text = tr("%1 (%2)").arg(iname).arg(pname);
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
ManageMetronomeDialog::accept()
{
    slotApply();
    QDialog::accept();
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
        RG_WARNING << "Warning: ManageMetronomeDialog::slotApply: no " << m_metronomeDevice->currentIndex() << "th device";
        return ;
    }

    DeviceId deviceId = dev->getId();
    studio.setMetronomeDevice(deviceId);

    if (getMetronome(dev) == 0) {
        RG_WARNING << "Warning: ManageMetronomeDialog::slotApply: unable to extract metronome from device " << deviceId;
        return ;
    }
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
    StudioControl::playPreviewNote(inst, pitch, MidiMaxValue, 10000000);
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
            QString conn = RosegardenSequencer::getInstance()->getConnection
                (md->getId());
            if (conn == "") *hasConnectionReturn = false;
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
