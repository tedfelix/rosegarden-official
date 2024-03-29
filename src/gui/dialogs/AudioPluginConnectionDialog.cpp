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
    QDialog(parent)
{
    setWindowTitle(tr("Audio Plugin Connections"));

    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Composition& comp = doc->getComposition();
    Studio& studio = doc->getStudio();

    Composition::trackcontainer& tracks = comp.getTracks();
    // For each track, add audio and softsynth tracks to the instrument list.
    for (const Composition::trackcontainer::value_type &pair : tracks) {
        Instrument* instr = studio.getInstrumentFor(pair.second);
        Instrument::InstrumentType itype = instr->getType();
        if (itype != Instrument::Audio && itype != Instrument::SoftSynth)
            continue;
        RG_DEBUG << "track" << pair.first << "instrument" << instr->getId();
        m_iList.push_back(instr);
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
    // For each connection...
    for (const PluginPort::Connection &connection : connections) {
        m_pluginPorts.push_back(connection.pluginPort);

        QLabel* pl = new QLabel(connection.pluginPort, this);
        mainLayout->addWidget(pl, row, 0);

        // Instrument ComboBox
        QComboBox* icb = new QComboBox(this);
        icb->addItem("<none>");
        int selInstIndex = 0;
        int count = 1;
        // For each Instrument, add the Instrument to the instrument ComboBox.
        for (const Instrument *inst : m_iList) {
            InstrumentId iid = inst->getId();
            if (connection.instrumentId == iid) selInstIndex = count;
            QString iname = inst->getLocalizedPresentationName();
            icb->addItem(iname);
            ++count;
        }
        icb->setCurrentIndex(selInstIndex);
        if (row == 1) icb->setEnabled(false);
        m_instrumentCB.push_back(icb);
        mainLayout->addWidget(icb, row, 1);

        // Channel ComboBox
        QComboBox* ccb = new QComboBox(this);
        ccb->addItem(tr("Left"));
        ccb->addItem(tr("Right"));
        ccb->setCurrentIndex(connection.channel);
        if (row == 1) ccb->setEnabled(false);
        m_channelCB.push_back(ccb);
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
    connections.clear();

    // widgetIndex
    int index = 0;

    // For each port, add to connections.
    for (const QString &port : m_pluginPorts) {
        PluginPort::Connection c;
        c.isOutput = false;
        c.isAudio = true;
        c.pluginPort = port;
        c.instrumentId = 0;

        // Get the Instrument from the Instrument ComboBox
        QComboBox* icb = m_instrumentCB[index];
        int isel = icb->currentIndex();
        if (isel != 0) {
            Instrument* inst = m_iList[isel - 1];
            c.instrumentId = inst->getId();
        }

        // Get the channel from the channel ComboBox
        QComboBox* ccb = m_channelCB[index];
        c.channel = ccb->currentIndex();

        ++index;

        connections.push_back(c);
    }
}


}
