// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "audiosynthmanager.h"

#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qtable.h>
#include <qlayout.h>

#include <klocale.h>
#include <kaction.h>
#include <kstdaction.h>

#include "audiopluginmanager.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"

#include "Studio.h"


const char *const SynthPluginManagerDialog::SynthPluginManagerConfigGroup =
"Synth Plugin Manager";

SynthPluginManagerDialog::SynthPluginManagerDialog(QWidget *parent,
						   RosegardenGUIDoc *doc) :
    KMainWindow(parent, "synthpluginmanagerdialog"),
    m_document(doc),
    m_studio(&doc->getStudio()),
    m_pluginManager(doc->getPluginManager())
{
    QFrame *mainBox = new QFrame(this);
    setCentralWidget(mainBox);

    QVBoxLayout *mainLayout = new QVBoxLayout(mainBox, 10, 10);

    setCaption(i18n("Manage Synth Plugins"));

    m_synthTable = new QTable(0, 2, mainBox);
    m_synthTable->setSorting(false);
    m_synthTable->setRowMovingEnabled(false);
    m_synthTable->setColumnMovingEnabled(false);
    m_synthTable->setShowGrid(false);
    m_synthTable->horizontalHeader()->setLabel(0, i18n("Instrument"));
    m_synthTable->horizontalHeader()->setLabel(1, i18n("Plugin"));
    m_synthTable->horizontalHeader()->show();
    m_synthTable->verticalHeader()->hide();
    m_synthTable->setLeftMargin(0);
    m_synthTable->setSelectionMode(QTable::SingleRow);

    mainLayout->addWidget(m_synthTable);

    QFrame* btnBox = new QFrame(mainBox);

    btnBox->setSizePolicy(
            QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QPushButton *closeButton = new QPushButton(i18n("Close"), btnBox);

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 0, 10);
    layout->addStretch(10);
    layout->addWidget(closeButton);
    layout->addSpacing(5);

    KAction* close = KStdAction::close(this,
                                       SLOT(slotClose()),
                                       actionCollection());

    closeButton->setText(close->text());
    connect(closeButton, SIGNAL(clicked()), this, SLOT(slotClose()));

    mainLayout->addWidget(btnBox);

    createGUI("synthpluginmanager.rc");

    m_synthTable->setCurrentCell(-1, 0);

    connect(m_synthTable, SIGNAL(valueChanged(int, int)),
	    this, SLOT(slotValueChanged (int, int)));

    setAutoSaveSettings(SynthPluginManagerConfigGroup, true);

    populate();
}

SynthPluginManagerDialog::~SynthPluginManagerDialog()
{
    RG_DEBUG << "\n*** SynthPluginManagerDialog::~SynthPluginManagerDialog()"
	     << endl;
}

void
SynthPluginManagerDialog::populate()
{
    m_synthTable->blockSignals(true);

    m_synthPlugins.clear();
    m_synthPlugins.push_back(-1);

    int count = 0;

    for (Rosegarden::PluginIterator itr = m_pluginManager->begin();
	 itr != m_pluginManager->end(); ++itr) {
	
	if ((*itr)->isSynth()) {
	    m_synthPlugins.push_back(count);
	}

	++count;
    }

    for (int i = 0; i < 16; ++i) {

	Rosegarden::InstrumentId id = Rosegarden::SoftSynthInstrumentBase + i;
	Rosegarden::Instrument *instrument = m_studio->getInstrumentById(id);
	if (!instrument) continue;

	m_synthTable->insertRows(i, 1);
	m_synthTable->setText(i, 0, instrument->getPresentationName().c_str());
	
	Rosegarden::AudioPluginInstance *plugin = instrument->getPlugin
	    (Rosegarden::Instrument::SYNTH_PLUGIN_POSITION);

	std::string identifier;
	if (plugin) identifier = plugin->getIdentifier();

	QStringList plugins;
	plugins.append(i18n("<none>"));
	int currentItem = 0;

	for (size_t j = 0; j < m_synthPlugins.size(); ++j) {

	    if (m_synthPlugins[j] == -1) continue;

	    Rosegarden::AudioPlugin *plugin =
		m_pluginManager->getPlugin(m_synthPlugins[j]);

	    plugins.append(plugin->getName());
	    if (plugin->getIdentifier() == identifier.c_str()) {
		currentItem = plugins.size() - 1;
	    }
	}

	QComboTableItem *item = new QComboTableItem(m_synthTable, plugins, false);
	item->setCurrentItem(currentItem);
	m_synthTable->setItem(i, 1, item);
    }

    m_synthTable->adjustColumn(0);
    m_synthTable->adjustColumn(1);
    if (m_synthTable->columnWidth(1) < 200) m_synthTable->setColumnWidth(1, 200);

    m_synthTable->blockSignals(false);
}


void
SynthPluginManagerDialog::slotClose()
{
    close();
}

void
SynthPluginManagerDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
    KMainWindow::closeEvent(e);
}

void
SynthPluginManagerDialog::slotValueChanged(int row, int col)
{
    RG_DEBUG << "SynthPluginManagerDialog::slotValueChanged(" << row << "," << col
	     << ")" << endl;

    if (col != 1) return;

    QComboTableItem *combo = dynamic_cast<QComboTableItem *>
	(m_synthTable->item(row, col));
    if (!combo) return;

    Rosegarden::InstrumentId id = Rosegarden::SoftSynthInstrumentBase + row;
    int item = combo->currentItem();

    RG_DEBUG << "SynthPluginManagerDialog::slotValueChanged(" << row << "," << col
	     << "): id " << id << ", item " << item << endl;

    
    if (item >= m_synthPlugins.size()) {
	RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotValueChanged("
		 << row << "," << col << "): synth " << item << " out of range"
		 << endl;
	return;
    }

    // NB m_synthPlugins[0] is -1 to represent the <none> item
    
    emit pluginSelected(id, Rosegarden::Instrument::SYNTH_PLUGIN_POSITION,
			m_synthPlugins[item]);

    Rosegarden::AudioPlugin *plugin = m_pluginManager->getPlugin(m_synthPlugins[item]);
    Rosegarden::Instrument *instrument = m_studio->getInstrumentById(id);

    if (instrument && plugin) {

	Rosegarden::AudioPluginInstance *pluginInstance = instrument->getPlugin
	    (Rosegarden::Instrument::SYNTH_PLUGIN_POSITION);

	if (pluginInstance) {
	    pluginInstance->setIdentifier(plugin->getIdentifier().data());
	}
    }
}

