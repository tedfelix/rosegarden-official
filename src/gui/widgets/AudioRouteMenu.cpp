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


#include "AudioRouteMenu.h"
#include "WheelyButton.h"

#include "base/Instrument.h"
#include "base/Studio.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/RosegardenPopupMenu.h"
#include "sound/MappedCommon.h"
#include "sound/MappedStudio.h"

#include <QComboBox>
#include <QCursor>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <QAction>


namespace Rosegarden
{

AudioRouteMenu::AudioRouteMenu(QWidget *par,
                               Direction direction,
                               Format format,
                               Studio *studio,
                               Instrument *instrument) :
        QObject(par),
        m_studio(studio),
        m_instrument(instrument),
        m_direction(direction),
        m_format(format)
{
    if (instrument) {
        // Make instrument tell us if it gets destroyed.
        connect(instrument, SIGNAL(destroyed()),
                this, SLOT(slotInstrumentGone()));
    }
    
    switch (format) {

    case Compact: {
            m_combo = 0;
            m_button = new WheelyButton(par);
            connect(m_button, SIGNAL(wheel(bool)), this, SLOT(slotWheel(bool)));
            connect(m_button, SIGNAL(clicked()), this, SLOT(slotShowMenu()));
            break;
        }

    case Regular: {
            m_button = 0;
            m_combo = new QComboBox(par);
            connect(m_combo, SIGNAL(activated(int)), this, SLOT(slotEntrySelected(int)));
            break;
        }

    }

    slotRepopulate();
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
AudioRouteMenu::slotRepopulate()
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
AudioRouteMenu::slotSetInstrument(Studio *studio,
                                  Instrument *instrument)
{
    m_studio = studio;
    m_instrument = instrument;
    slotRepopulate();
    if (instrument) {
        // Make instrument tell us if it gets destroyed.
        connect(instrument, SIGNAL(destroyed()),
                this, SLOT(slotInstrumentGone()));
    }
}

void
AudioRouteMenu::slotWheel(bool up)
{
    int current = getCurrentEntry();
    if (up) { // actually moves down the list
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
    if (getNumEntries() == 0)
        return ;

    RosegardenPopupMenu *menu = new RosegardenPopupMenu((QWidget *)parent());

    for (int i = 0; i < getNumEntries(); ++i) {

        QAction *a = menu->addAction(getEntryText(i));
        a->setObjectName(QString("%1").arg(i));
    }

    connect(menu, SIGNAL(triggered(QAction *)), this, SLOT(slotEntrySelected(QAction *)));

    int itemHeight = menu->actionGeometry(menu->actions().value(0)).height() + 2;
    QPoint pos = QCursor::pos();

    pos.rx() -= 10;
    pos.ry() -= (itemHeight / 2 + getCurrentEntry() * itemHeight);

    menu->popup(pos);
}

int
AudioRouteMenu::getNumEntries()
{
    if (!m_instrument)
        return 0;

    switch (m_direction) {

    case In: {
            int stereoIns =
                m_studio->getRecordIns().size() +
                m_studio->getBusses().size();

            if (m_instrument->getAudioChannels() > 1) {
                return stereoIns;
            } else {
                return stereoIns * 2;
            }

            break;
        }

    case Out:
        return m_studio->getBusses().size();
    }

    return 0;
}

int
AudioRouteMenu::getCurrentEntry()
{
    if (!m_instrument)
        return 0;

    switch (m_direction) {

    case In: {
            bool stereo = (m_instrument->getAudioChannels() > 1);

            bool isBuss;
            int channel;
            int input = m_instrument->getAudioInput(isBuss, channel);

            if (isBuss) {
                int recordIns = m_studio->getRecordIns().size();
                if (stereo) {
                    return recordIns + input;
                } else {
                    return recordIns * 2 + input * 2 + channel;
                }
            } else {
                if (stereo) {
                    return input;
                } else {
                    return input * 2 + channel;
                }
            }

            break;
        }

    case Out:
        return m_instrument->getAudioOutput();
    }

    return 0;
}

QString
AudioRouteMenu::getEntryText(int entry)
{
    if (!m_instrument)
        { return tr("none");}

    switch (m_direction) {

    case In: {
            bool stereo = (m_instrument->getAudioChannels() > 1);
            int recordIns = m_studio->getRecordIns().size();

            if (stereo) {
                if (entry < recordIns) {
                    return tr("In %1").arg(entry + 1);
                } else if (entry == recordIns) {
                    return tr("Master");
                } else {
                    return tr("Sub %1").arg(entry - recordIns);
                }
            } else {
                int channel = entry % 2;
                entry /= 2;
                if (entry < recordIns) {
                    return (channel ? tr("In %1 R") :
                            tr("In %1 L")).arg(entry + 1);
                } else if (entry == recordIns) {
                    return (channel ? tr("Master R") :
                            tr("Master L"));
                } else {
                    return (channel ? tr("Sub %1 R") :
                            tr("Sub %1 L")).arg(entry - recordIns);
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
    slotEntrySelected(a->objectName().toInt());
}

void
AudioRouteMenu::slotEntrySelected(int i)
{
    if (!m_instrument)
        { return; }
    
    switch (m_direction) {

    case In: {
            bool stereo = (m_instrument->getAudioChannels() > 1);

            bool oldIsBuss;
            int oldChannel;
            int oldInput = m_instrument->getAudioInput(oldIsBuss, oldChannel);

            bool newIsBuss;
            int newChannel = 0;
            int newInput;

            int recordIns = m_studio->getRecordIns().size();

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

            MappedObjectId oldMappedId = 0, newMappedId = 0;

            if (oldIsBuss) {
                Buss *buss = m_studio->getBussById(oldInput);
                if (buss)
                    oldMappedId = buss->getMappedId();
            } else {
                RecordIn *in = m_studio->getRecordIn(oldInput);
                if (in)
                    oldMappedId = in->getMappedId();
            }

            if (newIsBuss) {
                Buss *buss = m_studio->getBussById(newInput);
                if (!buss)
                    return ;
                newMappedId = buss->getMappedId();
            } else {
                RecordIn *in = m_studio->getRecordIn(newInput);
                if (!in)
                    return ;
                newMappedId = in->getMappedId();
            }

            if (oldMappedId != 0) {
                StudioControl::disconnectStudioObjects
                (oldMappedId, m_instrument->getMappedId());
            } else {
                StudioControl::disconnectStudioObject
                (m_instrument->getMappedId());
            }

            StudioControl::setStudioObjectProperty
            (m_instrument->getMappedId(),
             MappedAudioFader::InputChannel,
             MappedObjectValue(newChannel));

            if (newMappedId != 0) {
                StudioControl::connectStudioObjects
                (newMappedId, m_instrument->getMappedId());
            }

            if (newIsBuss) {
                m_instrument->setAudioInputToBuss(newInput, newChannel);
            } else {
                m_instrument->setAudioInputToRecord(newInput, newChannel);
            }

            m_instrument->changed();

            break;
        }

    case Out: {
            BussId bussId = m_instrument->getAudioOutput();
            Buss *oldBuss = m_studio->getBussById(bussId);
            Buss *newBuss = m_studio->getBussById(i);
            if (!newBuss)
                return ;

            if (oldBuss) {
                StudioControl::disconnectStudioObjects
                (m_instrument->getMappedId(), oldBuss->getMappedId());
            } else {
                StudioControl::disconnectStudioObject
                (m_instrument->getMappedId());
            }

            StudioControl::connectStudioObjects
            (m_instrument->getMappedId(), newBuss->getMappedId());

            m_instrument->setAudioOutput(i);
            m_instrument->changed();

            break;
        }
    }

    slotRepopulate();
    emit changed();
}

void
AudioRouteMenu::
slotInstrumentGone(void)
{
    m_instrument = 0;
}

}
