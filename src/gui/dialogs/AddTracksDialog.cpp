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
    int row = 0;

    // Number of Tracks
    layout->addWidget(new QLabel(tr("Number of Tracks")), row, 0);

    m_numberOfTracks = new QSpinBox();
    m_numberOfTracks->setMinimum(1);
    m_numberOfTracks->setMaximum(256);
    m_numberOfTracks->setValue(1);
    layout->addWidget(m_numberOfTracks, row, 1);

    ++row;

    // Location
    layout->addWidget(new QLabel(tr("Location")), row, 0);

    m_location = new QComboBox(this);
    m_location->addItem(tr("At the top"));
    m_location->addItem(tr("Above the current selected track"));
    m_location->addItem(tr("Below the current selected track"));
    m_location->addItem(tr("At the bottom"));

    QSettings settings;
    settings.beginGroup(AddTracksDialogGroup);
    m_location->setCurrentIndex(
            settings.value("Location", 2).toUInt());

    layout->addWidget(m_location, row, 1);

    ++row;

    // Device
    layout->addWidget(new QLabel(tr("Device")), row, 0);

    m_device = new QComboBox(this);
    initDeviceComboBox();

    layout->addWidget(m_device, row, 1);

    ++row;

    // Spacer

    layout->setRowMinimumHeight(row, 10);

    ++row;

    // Button Box

    QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(buttonBox, row, 0, 1, 2);
}

namespace
{
    // Sort the devices in a logic way for the UI.
    class DeviceLess {
    public:
        bool operator()(const Device *lhs, const Device *rhs)
        {
            if (lhs->getType() == rhs->getType())
                return (lhs->getName() < rhs->getName());
            if (getPriority(lhs) < getPriority(rhs))
                return true;
            return false;
        }

    private:
        int getPriority(const Device *device)
        {
            switch (device->getType()) {
            case Device::Midi:
                return 1;
            case Device::SoftSynth:
                return 2;
            case Device::Audio:
                return 3;
            default:
                return 4;
            }
        }
    };
}

void
AddTracksDialog::initDeviceComboBox()
{
    const DeviceList &deviceList =
            *(RosegardenMainWindow::self()->getDocument()->getStudio().getDevices());

    // Sort them into this set.
    std::set<const Device *, DeviceLess> deviceSet;

    // For each output Device, add them to the set.
    for (const Device *device : deviceList) {
        // If this is an input device, skip it.
        if (device->isInput())
            continue;

        deviceSet.insert(device);
    }

    // For each Device in the set, add them to the combobox.
    for (const Device *device : deviceSet) {
        m_device->addItem(QObject::tr(device->getName().c_str()));
        m_deviceList.push_back(device);
    }
}

namespace
{
    int getSelectedTrackPosition()
    {
        const Composition &comp =
                RosegardenMainWindow::self()->getDocument()->getComposition();
        const Track *track = comp.getTrackById(comp.getSelectedTrack());

        if (track)
            return track->getPosition();

        return 0;
    }
}

int
AddTracksDialog::getInsertPosition()
{
    switch (m_location->currentIndex()) {
    case 0: // at the top
        return 0;
    case 1: // above the current selected track
        return getSelectedTrackPosition();
    case 2: // below the current selected track
        return getSelectedTrackPosition() + 1;
    case 3: // at the bottom
        return -1;
    }

    return 0;
}

void AddTracksDialog::accept()
{
    QSettings settings;
    settings.beginGroup(AddTracksDialogGroup);
    settings.setValue("Location", m_location->currentIndex());

    if (m_device->currentIndex() < 0)
        return;

    if ((unsigned)m_device->currentIndex() >= m_deviceList.size())
        return;

    const Device *device = m_deviceList[m_device->currentIndex()];

    InstrumentList instrumentList = device->getPresentationInstruments();
    if (instrumentList.empty())
        return;

    const Instrument *instrument = instrumentList[0];
    if (!instrument)
        return;

    // Add the tracks.

    // ??? Why is addTracks() in the view?  Shouldn't we go to the
    //     Composition for this?
    RosegardenMainWindow::self()->getView()->addTracks(
            m_numberOfTracks->value(),
            instrument->getId(),
            getInsertPosition());

    QDialog::accept();
}


}
