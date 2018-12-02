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

#define RG_MODULE_STRING "[AudioRouteMenu]"

#include "AudioRouteMenu.h"

#include "gui/application/RosegardenMainWindow.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/WheelyButton.h"
#include "base/Instrument.h"
#include "base/Studio.h"
#include "gui/studio/StudioControl.h"
#include "sound/MappedCommon.h"  // MappedObjectId, MappedObjectValue
#include "sound/MappedStudio.h"  // MappedAudioFader
#include "misc/Debug.h"

#include <QComboBox>
#include <QCursor>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <QAction>
#include <QMenu>


namespace Rosegarden
{


AudioRouteMenu::AudioRouteMenu(QWidget *parent,
                               Direction direction,
                               Format format,
                               InstrumentId instrumentId) :
        QObject(parent),
        m_instrumentId(instrumentId),
        m_direction(direction),
        m_format(format)
{
    switch (format) {

    case Compact: {
            m_combo = nullptr;
            m_button = new WheelyButton(parent);
            connect(m_button, &WheelyButton::wheel,
                    this, &AudioRouteMenu::slotWheel);
            connect(m_button, &QAbstractButton::clicked,
                    this, &AudioRouteMenu::slotShowMenu);
            break;
        }

    case Regular: {
            m_button = nullptr;
            m_combo = new QComboBox(parent);
            connect(m_combo, SIGNAL(activated(int)),
                    SLOT(slotEntrySelected(int)));
            break;
        }

    }

    updateWidget();
}

QWidget *
AudioRouteMenu::getWidget()
{
    if (m_button)
        return m_button;
    else
        return m_combo;
}

void
AudioRouteMenu::updateWidget()
{
    switch (m_format) {

    case Compact:
        m_button->setText(getEntryText(getCurrentEntry()));
        break;

    case Regular:
        m_combo->clear();
        for (int i = 0; i < getNumEntries(); ++i) {
            m_combo->addItem(getEntryText(i));
        }
        m_combo->setCurrentIndex(getCurrentEntry());
        break;
    }
}

void
AudioRouteMenu::setInstrument(InstrumentId instrumentId)
{
    m_instrumentId = instrumentId;

    updateWidget();
}

void
AudioRouteMenu::slotWheel(bool up)
{
    int current = getCurrentEntry();

    if (up) {
        if (current > 0)
            slotEntrySelected(current - 1);
    } else {
        if (current < getNumEntries() - 1)
            slotEntrySelected(current + 1);
    }
}

void
AudioRouteMenu::slotShowMenu()
{
    // No entries to show?  Bail.
    if (getNumEntries() == 0)
        return;

    QMenu *menu = new QMenu(qobject_cast<QWidget *>(parent()));

    // Populate the menu.
    for (int i = 0; i < getNumEntries(); ++i) {
        QAction *a = menu->addAction(getEntryText(i));
        // Used by slotEntrySelected() to figure out what was selected.
        a->setObjectName(QString("%1").arg(i));
    }

    connect(menu, SIGNAL(triggered(QAction *)),
            SLOT(slotEntrySelected(QAction *)));

    // Compute the position for the pop-up menu.

    // QMenu::popup() can do this for us, but it doesn't place the
    // cursor over top of the current selection.

    // Get the QRect for the current entry.
    QRect actionRect =
            menu->actionGeometry(menu->actions().value(getCurrentEntry()));

    QPoint pos = QCursor::pos();
    pos.rx() -= 10;
    pos.ry() -= actionRect.top() + actionRect.height() / 2;

    // Display the menu.
    menu->popup(pos);
}

int
AudioRouteMenu::getNumEntries()
{
    if (m_instrumentId == NoInstrument)
        return 0;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();

    switch (m_direction) {

    case In: {
            int stereoIns =
                studio.getRecordIns().size() +
                studio.getBusses().size();

            Instrument *instrument = studio.getInstrumentById(m_instrumentId);
            if (!instrument)
                return 0;

            // If this is a stereo instrument
            if (instrument->getAudioChannels() > 1) {
                return stereoIns;
            } else {  // this is a mono instrument
                // We'll have separate "L" and "R" entries for each input,
                // so double it.
                return stereoIns * 2;
            }

            break;
        }

    case Out:
        return studio.getBusses().size();
    }

    return 0;
}

int
AudioRouteMenu::getCurrentEntry()
{
    if (m_instrumentId == NoInstrument)
        return 0;

    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();
    Instrument *instrument = studio.getInstrumentById(m_instrumentId);
    if (!instrument)
        return 0;

    switch (m_direction) {

    case In: {
            bool stereo = (instrument->getAudioChannels() > 1);

            bool isBuss;
            int channel;
            int input = instrument->getAudioInput(isBuss, channel);

            // If a buss is currently selected
            if (isBuss) {
                // We need to skip over the record ins to get to the
                // buss entries.
                int recordIns = studio.getRecordIns().size();
                if (stereo) {
                    return recordIns + input;
                } else {
                    // In mono we multiply by two since each stereo channel
                    // expands into an "L" and an "R" entry.
                    return recordIns * 2 + input * 2 + channel;
                }
            } else {
                if (stereo) {
                    return input;
                } else {
                    // In mono we multiply by two since each stereo channel
                    // expands into an "L" and an "R" entry.
                    return input * 2 + channel;
                }
            }

            break;
        }

    case Out:
        return instrument->getAudioOutput();
    }

    return 0;
}

QString
AudioRouteMenu::getEntryText(int entry)
{
    if (m_instrumentId == NoInstrument)
        return tr("none");

    switch (m_direction) {

    case In: {
            RosegardenDocument *doc =
                    RosegardenMainWindow::self()->getDocument();
            Studio &studio = doc->getStudio();
            Instrument *instrument = studio.getInstrumentById(m_instrumentId);
            if (!instrument)
                return nullptr;

            bool stereo = (instrument->getAudioChannels() > 1);
            int recordIns = studio.getRecordIns().size();

            if (stereo) {
                // If we have a record input
                if (entry < recordIns) {
                    return tr("In %1").arg(entry + 1);
                } else if (entry == recordIns) {  // master
                    return tr("Master");
                } else {  // submaster
                    return tr("Sub %1").arg(entry - recordIns);
                }
            } else {
                // 0 == left, 1 == right
                int channel = entry % 2;
                int input = entry / 2;

                // If we have a record input
                if (input < recordIns) {
                    return (channel ? tr("In %1 R") :
                            tr("In %1 L")).arg(input + 1);
                } else if (input == recordIns) {  // master
                    return (channel ? tr("Master R") :
                            tr("Master L"));
                } else {  // submaster
                    return (channel ? tr("Sub %1 R") :
                            tr("Sub %1 L")).arg(input - recordIns);
                }
            }
            break;
        }

    case Out:
        if (entry == 0)
            return tr("Master");
        else
            return tr("Sub %1").arg(entry);
    }

    return QString();
}

void
AudioRouteMenu::slotEntrySelected(QAction *a)
{
    // Extract the entry number from the action's object name.
    slotEntrySelected(a->objectName().toInt());
}

void
AudioRouteMenu::slotEntrySelected(int i)
{
    if (m_instrumentId == NoInstrument)
        return;

    RosegardenDocument *doc =
            RosegardenMainWindow::self()->getDocument();
    Studio &studio = doc->getStudio();
    Instrument *instrument = studio.getInstrumentById(m_instrumentId);
    if (!instrument)
        return;

    switch (m_direction) {

    case In: {
            bool oldIsBuss;
            // 0 == left, 1 == right, mono only
            int oldChannel;
            int oldInput = instrument->getAudioInput(oldIsBuss, oldChannel);

            // Compute old mapped ID

            MappedObjectId oldMappedId = 0;

            if (oldIsBuss) {
                Buss *buss = studio.getBussById(oldInput);
                if (buss)
                    oldMappedId = buss->getMappedId();
            } else {
                RecordIn *in = studio.getRecordIn(oldInput);
                if (in)
                    oldMappedId = in->getMappedId();
            }

            // Compute selected (new) entry's input and channel.

            bool stereo = (instrument->getAudioChannels() > 1);
            int recordIns = studio.getRecordIns().size();

            bool newIsBuss;
            // 0 == left, 1 == right, mono only
            int newChannel = 0;
            int newInput;

            if (stereo) {
                newIsBuss = (i >= recordIns);
                if (newIsBuss) {
                    newInput = i - recordIns;
                } else {
                    newInput = i;
                }
            } else {
                newIsBuss = (i >= recordIns * 2);
                newChannel = i % 2;
                if (newIsBuss) {
                    newInput = i / 2 - recordIns;
                } else {
                    newInput = i / 2;
                }
            }

            // Compute new mapped ID

            MappedObjectId newMappedId = 0;

            if (newIsBuss) {
                Buss *buss = studio.getBussById(newInput);
                if (!buss)
                    return;
                newMappedId = buss->getMappedId();
            } else {
                RecordIn *in = studio.getRecordIn(newInput);
                if (!in)
                    return;
                newMappedId = in->getMappedId();
            }

            // Update the Studio

            if (oldMappedId != 0) {
                StudioControl::disconnectStudioObjects(
                        oldMappedId, instrument->getMappedId());
            } else {
                StudioControl::disconnectStudioObject(
                        instrument->getMappedId());
            }

            StudioControl::setStudioObjectProperty(
                    instrument->getMappedId(),
                    MappedAudioFader::InputChannel,
                    MappedObjectValue(newChannel));

            if (newMappedId != 0) {
                // Connect the input to the instrument.
                StudioControl::connectStudioObjects(
                        newMappedId, instrument->getMappedId());
            }

            // Update the Instrument

            if (newIsBuss) {
                instrument->setAudioInputToBuss(newInput, newChannel);
            } else {
                instrument->setAudioInputToRecord(newInput, newChannel);
            }

            doc->slotDocumentModified();

            break;
        }

    case Out: {
            BussId bussId = instrument->getAudioOutput();
            Buss *oldBuss = studio.getBussById(bussId);

            Buss *newBuss = studio.getBussById(i);
            if (!newBuss)
                return;

            // Update the Studio

            if (oldBuss) {
                StudioControl::disconnectStudioObjects(
                        instrument->getMappedId(), oldBuss->getMappedId());
            } else {
                StudioControl::disconnectStudioObject(
                        instrument->getMappedId());
            }

            StudioControl::connectStudioObjects(
                    instrument->getMappedId(), newBuss->getMappedId());

            // Update the Instrument

            instrument->setAudioOutput(i);

            doc->slotDocumentModified();

            break;
        }
    }

    updateWidget();
}


}
