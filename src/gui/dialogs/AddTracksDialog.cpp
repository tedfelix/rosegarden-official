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
#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>


namespace
{
    const QString AddTracksDialogGroup = "AddTracksDialog";
}


namespace Rosegarden
{


AddTracksDialog::AddTracksDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Add Tracks"));
    setModal(true);

    QGridLayout *layout = new QGridLayout(this);
    layout->setVerticalSpacing(5);

    // Number of Tracks
    layout->addWidget(new QLabel(tr("Number of Tracks")), 0, 0);

    m_numberOfTracks = new QSpinBox();
    m_numberOfTracks->setMinimum(1);
    m_numberOfTracks->setMaximum(256);
    m_numberOfTracks->setValue(1);
    layout->addWidget(m_numberOfTracks, 0, 1);

    // Location
    layout->addWidget(new QLabel(tr("Location")), 1, 0);

    m_location = new QComboBox(this);
    m_location->addItem(tr("At the top"));
    m_location->addItem(tr("Above the current selected track"));
    m_location->addItem(tr("Below the current selected track"));
    m_location->addItem(tr("At the bottom"));

    QSettings settings;
    settings.beginGroup(AddTracksDialogGroup);
    m_location->setCurrentIndex(
            settings.value("Location", 2).toUInt());

    layout->addWidget(m_location, 1, 1);

    // Spacer

    layout->setRowMinimumHeight(2, 10);

    // Button Box

    QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(buttonBox, 3, 0, 1, 2);
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

    int opt = m_location->currentIndex();

    QSettings settings;
    settings.beginGroup(AddTracksDialogGroup);
    settings.setValue("Location", opt);

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
            m_numberOfTracks->value(),
            id,
            getInsertPosition());

    QDialog::accept();
}


}
