/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioPluginConnectionDialog]"
#define RG_NO_DEBUG_PRINT 1

#include "AudioPluginConnectionDialog.h"

#include "document/RosegardenDocument.h"
#include "base/Composition.h"
#include "misc/Debug.h"

#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>


namespace Rosegarden
{


AudioPluginConnectionDialog::AudioPluginConnectionDialog
(QWidget *parent,
 const PluginPort::ConnectionList& connections) :
    QDialog(parent),
    m_connections(connections)
{
    setWindowTitle(tr("Audio Plugin Connections"));

    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Composition& comp = doc->getComposition();
    Studio& studio = doc->getStudio();

    int position = 0;
    while(Track* track = comp.getTrackByPosition(position)) {
        Instrument* instr = studio.getInstrumentFor(track);
        Instrument::InstrumentType itype = instr->getType();
        if (itype == Instrument::Audio) {
            m_iListAudio.push_back(instr);
            m_iListAudioSynth.push_back(instr);
        }
        if (itype == Instrument::SoftSynth) {
            m_iListAudioSynth.push_back(instr);
        }
        unsigned int nch = instr->getNumAudioChannels();
        RG_DEBUG << "track" << position << "instrument" << instr->getId() <<
            nch << itype;
        position++;
    }

    // the first connection is fixed to the plugin instrument
    InstrumentId pluginInstrumentId = m_connections.baseInstrument;
    QString pluginInstrumentName;
    if (pluginInstrumentId >= AudioInstrumentBase) {
        Instrument* pluginInstrument =
            studio.getInstrumentById(pluginInstrumentId);
        pluginInstrumentName =
            pluginInstrument->getLocalizedPresentationName();
    } else {
        // ist not an instrument its a buss
        Buss* pluginBuss = studio.getBussById(pluginInstrumentId);
        pluginInstrumentName =
            strtoqstr(pluginBuss->getPresentationName());
    }

    QGridLayout *mainLayout = new QGridLayout(this);
    setLayout(mainLayout);
    QLabel* pLabel = new QLabel(tr("Plugin port"), this);
    mainLayout->addWidget(pLabel, 0, 0);
    QLabel* iLabel = new QLabel(tr("Instrument"), this);
    mainLayout->addWidget(iLabel, 0, 1);
    QLabel* cLabel = new QLabel(tr("Channel"), this);
    mainLayout->addWidget(cLabel, 0, 2);

    int row = 1;
    bool firstInput = true;
    bool firstOutput = true;
    // For each connection...
    RG_DEBUG << "initial connection data" << m_connections.baseInstrument <<
        m_connections.numChannels;
    for (const PluginPort::Connection &connection : m_connections.connections) {
        RG_DEBUG << "process connection" << connection.isOutput <<
            connection.isAudio << connection.portIndex <<
            connection.pluginPort << connection.instrumentId <<
            connection.channel;
        m_pluginPorts.push_back(connection.pluginPort);

        QLabel* pl = new QLabel(connection.pluginPort, this);
        mainLayout->addWidget(pl, row, 0);

        // Instrument and channel ComboBox
        QComboBox* icb = new QComboBox(this);
        QComboBox* ccb = new QComboBox(this);
        m_instrumentCB.push_back(icb);
        m_channelCB.push_back(ccb);
        icb->setObjectName(QString::number(row));
        int selInstIndex = 0;
        m_fixedInstrument.push_back(false);
        if (firstInput && ! connection.isOutput) {
            firstInput = false;
            selInstIndex = 1;
            m_fixedInstrument[row - 1] = true;
            icb->setEnabled(false);
        } else if (firstOutput && connection.isOutput) {
            firstOutput = false;
            selInstIndex = 1;
            m_fixedInstrument[row - 1] = true;
            icb->setEnabled(false);
        }
        icb->addItem(tr("<none>"));
        icb->addItem(pluginInstrumentName);
        if (connection.instrumentId == pluginInstrumentId) selInstIndex = 1;
        int count = 2;
        // For each Instrument, add the Instrument to the
        // instrument ComboBox.
        if (connection.isOutput) {
            for (const Instrument *inst : m_iListAudio) {
                InstrumentId iid = inst->getId();
                if (connection.instrumentId == iid) selInstIndex = count;
                QString iname = inst->getLocalizedPresentationName();
                icb->addItem(iname);
                ++count;
            }
        } else {
            for (const Instrument *inst : m_iListAudioSynth) {
                InstrumentId iid = inst->getId();
                if (connection.instrumentId == iid) selInstIndex = count;
                QString iname = inst->getLocalizedPresentationName();
                icb->addItem(iname);
                ++count;
            }
        }
        icb->setCurrentIndex(selInstIndex);
        setupChannelCB(row - 1, selInstIndex);
        connect(icb, static_cast<void(QComboBox::*)(int)>
                (&QComboBox::currentIndexChanged),
                this, &AudioPluginConnectionDialog::slotInstrumentChanged);
        mainLayout->addWidget(icb, row, 1);
        mainLayout->addWidget(ccb, row, 2);

        ++row;
    }

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted,
                     this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected,
                     this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);
}

void AudioPluginConnectionDialog::getConnections
(PluginPort::ConnectionList& connections) const
{
    connections = m_connections;
    // widgetIndex
    int index = 0;

    // For each connection...
    for (PluginPort::Connection &c : connections.connections) {
        if (! m_fixedInstrument[index]) {
            c.instrumentId = 0;

            // Get the Instrument from the Instrument ComboBox
            QComboBox* icb = m_instrumentCB[index];
            int isel = icb->currentIndex();
            if (isel == 1) {
                c.instrumentId = connections.baseInstrument;
            }
            if (isel > 1) {
                Instrument* inst;
                if (c.isOutput) {
                    inst = m_iListAudio[isel - 2];
                } else {
                    inst = m_iListAudioSynth[isel - 2];
                }
                c.instrumentId = inst->getId();
            }
        }

        // Get the channel from the channel ComboBox
        QComboBox* ccb = m_channelCB[index];
        c.channel = ccb->currentIndex();
        if (c.channel == 2) c.channel = -1; // both channels

        ++index;

    }
    RG_DEBUG << "returning connection" << connections.baseInstrument <<
        connections.numChannels;
    for (const PluginPort::Connection &connection : connections.connections) {
        RG_DEBUG << "returning connection" << connection.isOutput <<
            connection.isAudio << connection.portIndex <<
            connection.pluginPort << connection.instrumentId <<
            connection.channel;
    }
}

void AudioPluginConnectionDialog::slotInstrumentChanged(int index)
{
    QString oName = sender()->objectName();
    int connectionIndex = oName.toInt() - 1;
    RG_DEBUG << "slotInstrumentChanged" << connectionIndex << index;
    setupChannelCB(connectionIndex, index);
}

void AudioPluginConnectionDialog::setupChannelCB(int connectionIndex,
                                                 int instrumentIndex)
{
    RG_DEBUG << "setupChannelCB" << connectionIndex << instrumentIndex;
    // setup the channel QComboBox
    QComboBox* channelComboBox = m_channelCB[connectionIndex];
    channelComboBox->clear();
    channelComboBox->setEnabled(false);
    if (instrumentIndex == 0) return;
    PluginPort::Connection connection =
        m_connections.connections[connectionIndex];
    Instrument* inst = nullptr;
    channelComboBox->setEnabled(true);
    unsigned int nch = m_connections.numChannels;
    if (instrumentIndex > 1) {
        if (connection.isOutput) {
            inst = m_iListAudio[instrumentIndex - 2];
        } else {
            inst = m_iListAudioSynth[instrumentIndex - 2];
        }
        if (! inst && ! m_fixedInstrument[connectionIndex]) {
            channelComboBox->setEnabled(false);
            return;
        }
        if (inst) nch = inst->getNumAudioChannels();
    }
    if (nch == 1) { // mono
        channelComboBox->addItem(tr("Mono"));
        channelComboBox->setEnabled(false);
        return;
    }
    // stereo channel
    channelComboBox->addItem(tr("Left"));
    channelComboBox->addItem(tr("Right"));
    channelComboBox->addItem(tr("Both"));
    channelComboBox->setEnabled(true);
    if (connection.channel == -1) { // both
        channelComboBox->setCurrentIndex(2);
    } else {
        channelComboBox->setCurrentIndex(connection.channel);
    }
}

}
