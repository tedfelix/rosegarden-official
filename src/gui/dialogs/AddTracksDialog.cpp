/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AddTracksDialog]"

#include "AddTracksDialog.h"

#include "base/Composition.h"
#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainViewWidget.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Studio.h"
#include "base/Track.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>


namespace Rosegarden
{


AddTracksDialog::AddTracksDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Add Tracks"));

    QWidget *vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    setLayout(vBoxLayout);

    QGroupBox *gbox = new QGroupBox(tr("How many tracks do you want to add?"), vBox);
    vBoxLayout->addWidget(gbox);

    QVBoxLayout *gboxLayout = new QVBoxLayout;
    gbox->setLayout(gboxLayout);
    
    m_count = new QSpinBox();
    gboxLayout->addWidget(m_count);
    m_count->setMinimum(1);
    m_count->setMaximum(256); // why not 256?  32 seemed like a silly upper bound
    m_count->setValue(1);

    QWidget *posBox = new QWidget(vBox);
    gboxLayout->addWidget(posBox);

    QHBoxLayout *posBoxLayout = new QHBoxLayout;
    posBox->setLayout(posBoxLayout);

    posBoxLayout->addWidget(new QLabel(tr("Add tracks")));

    m_position = new QComboBox(posBox);
    posBoxLayout->addWidget(m_position);
    m_position->addItem(tr("At the top"));
    m_position->addItem(tr("Above the current selected track"));
    m_position->addItem(tr("Below the current selected track"));
    m_position->addItem(tr("At the bottom"));

    QString metric(tr("Above the current selected track"));
    m_position->setMinimumContentsLength(metric.size());

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_position->setCurrentIndex(
            settings.value("lastaddtracksposition", 2).toUInt());

    QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vBoxLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    settings.endGroup();
}

int
AddTracksDialog::getTracks()
{
    return m_count->value();
}

int
AddTracksDialog::getInsertPosition()
{
    const Composition &comp =
            RosegardenMainWindow::self()->getDocument()->getComposition();
    const Track *track = comp.getTrackById(comp.getSelectedTrack());

    int selectedTrackPosition = 0;
    if (track)
        selectedTrackPosition = track->getPosition();

    int opt = m_position->currentIndex();

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    settings.setValue("lastaddtracksposition", opt);

    int pos = 0;

    switch (opt) {
    case 0: // at top
        pos = 0;
        break;
    case 1: // above current track
        pos = selectedTrackPosition;
        break;
    case 2: // below current track
        pos = selectedTrackPosition + 1;
        break;
    case 3: // at bottom
        pos = -1;
        break;
    }

    settings.endGroup();

    return pos;
}

void AddTracksDialog::accept()
{
    //RG_DEBUG << "accept()";

    // default to the base number
    // ??? This might not actually exist.  If not, perhaps we should
    //     let the user know that they need to add a Device first.
    InstrumentId id = MidiInstrumentBase;

    // Get the first MIDI Instrument.

    const DeviceList &devices =
            *(RosegardenMainWindow::self()->getDocument()->getStudio().getDevices());

    // For each Device
    for (const Device *device : devices) {

        // Not a MIDI device?  Try the next.
        if (device->getType() != Device::Midi)
            continue;

        InstrumentList instruments = device->getAllInstruments();
        if (instruments.empty())
            continue;

        // Just check the first one to make sure it is legit.
        Instrument *instrument = instruments[0];
        if (!instrument)
            continue;

        if (instrument->getId() >= MidiInstrumentBase) {
            id = instrument->getId();
            break;
        }
    }

    // Add the tracks.

    RosegardenMainWindow::self()->getView()->addTracks(
            getTracks(), id, getInsertPosition());

    QDialog::accept();
}


}
