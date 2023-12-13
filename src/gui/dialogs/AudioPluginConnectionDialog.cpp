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

#include "misc/Debug.h"

#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>

namespace Rosegarden
{

AudioPluginConnectionDialog::AudioPluginConnectionDialog
(QWidget *parent,
 const PluginPortConnection::ConnectionList& connections) :
    QDialog(parent)
{
    setWindowTitle(tr("Audio Plugin Connections"));
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Studio &studio = doc->getStudio();
    m_iList = studio.getAllInstruments();

    QGridLayout *mainLayout = new QGridLayout(this);
    setLayout(mainLayout);
    QLabel* pLabel = new QLabel(tr("Plugin port"), this);
    mainLayout->addWidget(pLabel, 0, 0);
    QLabel* iLabel = new QLabel(tr("Instrument"), this);
    mainLayout->addWidget(iLabel, 0, 1);
    QLabel* cLabel = new QLabel(tr("Channel"), this);
    mainLayout->addWidget(cLabel, 0, 2);
    int row = 1;
    for(auto& connection : connections) {
        m_pluginPorts.push_back(connection.pluginPort);
        QLabel* pl = new QLabel(connection.pluginPort, this);
        mainLayout->addWidget(pl, row, 0);
        QComboBox* icb = new QComboBox(this);
        icb->addItem("<none>");
        int selInstIndex = 0;
        int count = 1;
        for(auto inst : m_iList) {
            InstrumentId iid = inst->getId();
            if (connection.instrumentId == iid) selInstIndex = count;
            Instrument::InstrumentType itype = inst->getType();
            if (itype != Instrument::Audio && itype != Instrument::SoftSynth)
                continue;
            QString iname = inst->getLocalizedPresentationName();
            icb->addItem(iname);
            count++;
        }
        icb->setCurrentIndex(selInstIndex);
        if (row == 1) icb->setEnabled(false);
        m_instrumentCB.push_back(icb);
        mainLayout->addWidget(icb, row, 1);
        QComboBox* ccb = new QComboBox(this);
        ccb->addItem(tr("Left"));
        ccb->addItem(tr("Right"));
        ccb->setCurrentIndex(connection.channel);
        if (row == 1) ccb->setEnabled(false);
        m_channelCB.push_back(ccb);
        mainLayout->addWidget(ccb, row, 2);
        row++;
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
(PluginPortConnection::ConnectionList& connections) const
{
    connections.clear();
    int index = 0;
    for (const auto& port : m_pluginPorts) {
        PluginPortConnection::Connection c;
        c.isOutput = false;
        c.isAudio = true;
        c.pluginPort = port;
        c.instrumentId = 0;
        QComboBox* icb = m_instrumentCB[index];
        int isel = icb->currentIndex();
        if (isel != 0) {
            Instrument* inst = m_iList[isel - 1];
            c.instrumentId = inst->getId();
        }
        QComboBox* ccb = m_channelCB[index];
        c.channel = ccb->currentIndex();
        index++;
        connections.push_back(c);
    }
}

}
