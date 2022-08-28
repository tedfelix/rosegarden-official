/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

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

#include "commands/segment/AddTracksCommand.h"
#include "document/CommandHistory.h"
#include "base/Composition.h"
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

#include <algorithm>  // For std::min()
#include <vector>


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

    // Instrument
    layout->addWidget(new QLabel(tr("Instrument")), row, 0);

    m_instrument = new QComboBox(this);
    updateInstrumentComboBox();

    layout->addWidget(m_instrument, row, 1);

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

    // Connections

    connect(m_device, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &AddTracksDialog::slotDeviceChanged);

}

namespace
{
    // Sort the devices in a logic way for the UI.
    class DeviceLess {
    public:
        bool operator()(const Device *lhs, const Device *rhs) const
        {
            if (lhs->getType() == rhs->getType())
                return (lhs->getName() < rhs->getName());
            if (getPriority(lhs) < getPriority(rhs))
                return true;
            return false;
        }

    private:
        static int getPriority(const Device *device)
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
            *(RosegardenDocument::currentDocument->getStudio().getDevices());

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
        m_device->addItem(QObject::tr(device->getName().c_str()),
                          device->getId());
    }
}

void AddTracksDialog::updateInstrumentComboBox()
{
    m_instrument->clear();

    Studio &studio = RosegardenDocument::currentDocument->getStudio();

    const Device *device = studio.getDevice(m_device->currentData().toUInt());
    if (!device)
        return;

    InstrumentList instrumentList = device->getPresentationInstruments();
    if (instrumentList.empty())
        return;

    // For each Instrument, add it to the ComboBox.
    for (const Instrument *instrument : instrumentList) {
        m_instrument->addItem(QObject::tr(instrument->getName().c_str()),
                              instrument->getId());
    }
}

namespace
{
    int getSelectedTrackPosition()
    {
        const Composition &comp =
                RosegardenDocument::currentDocument->getComposition();
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

    Studio &studio = RosegardenDocument::currentDocument->getStudio();

    const Device *device = studio.getDevice(m_device->currentData().toUInt());
    if (!device)
        return;

    // Use the instrument IDs in order up to the last.

    InstrumentId startInstrumentId = m_instrument->currentData().toUInt();

    InstrumentList instrumentList = device->getPresentationInstruments();
    if (instrumentList.empty())
        return;

    std::vector<InstrumentId> instrumentIds;

    // For each Instrument in the Device
    for (const Instrument *instrument : instrumentList) {
        InstrumentId instrumentId = instrument->getId();
        if (instrumentId >= startInstrumentId)
            instrumentIds.push_back(instrumentId);
    }

    CommandHistory::getInstance()->addCommand(
            new AddTracksCommand(
                    m_numberOfTracks->value(),
                    instrumentIds,
                    getInsertPosition()));

    QDialog::accept();
}


}
