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
#include <qlayout.h>
#include <qlabel.h>

#include <klocale.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kcombobox.h>

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
    setCaption(i18n("Manage Synth Plugins"));

    QFrame *mainBox = new QFrame(this);
    setCentralWidget(mainBox);

    QVBoxLayout *mainLayout = new QVBoxLayout(mainBox, 10, 10);

    QGroupBox *groupBox = new QGroupBox(1, Horizontal, i18n("Plugins"), mainBox);
    mainLayout->addWidget(groupBox);

    QFrame *pluginFrame = new QFrame(groupBox);
    QGridLayout *pluginLayout = new QGridLayout(pluginFrame, 1, 4, 3, 3);

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

	pluginLayout->addWidget(new QLabel(instrument->getPresentationName().c_str(),
					   pluginFrame), i, 0);

	Rosegarden::AudioPluginInstance *plugin = instrument->getPlugin
	    (Rosegarden::Instrument::SYNTH_PLUGIN_POSITION);

	std::string identifier;
	if (plugin) identifier = plugin->getIdentifier();

	int currentItem = 0;

	KComboBox *pluginCombo = new KComboBox(pluginFrame);
	pluginCombo->insertItem(i18n("<none>"));

	for (size_t j = 0; j < m_synthPlugins.size(); ++j) {

	    if (m_synthPlugins[j] == -1) continue;

	    Rosegarden::AudioPlugin *plugin =
		m_pluginManager->getPlugin(m_synthPlugins[j]);

	    pluginCombo->insertItem(plugin->getName());
	    
	    if (plugin->getIdentifier() == identifier.c_str()) {
		pluginCombo->setCurrentItem(pluginCombo->count() - 1);
	    }
	}

	connect(pluginCombo, SIGNAL(activated(int)),
		this, SLOT(slotPluginChanged(int)));

	pluginLayout->addWidget(pluginCombo, i, 1);

	m_synthCombos.push_back(pluginCombo);
    }

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

    setAutoSaveSettings(SynthPluginManagerConfigGroup, true);
}

SynthPluginManagerDialog::~SynthPluginManagerDialog()
{
    RG_DEBUG << "\n*** SynthPluginManagerDialog::~SynthPluginManagerDialog()"
	     << endl;
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
SynthPluginManagerDialog::slotPluginChanged(int index)
{
    const QObject *s = sender();

    RG_DEBUG << "SynthPluginManagerDialog::slotPluginChanged(" << index
	     << ")" << endl;

    int instrumentNo = -1;

    for (unsigned int i = 0; i < m_synthCombos.size(); ++i) {
	if (s == m_synthCombos[i]) instrumentNo = i;
    }

    if (instrumentNo == -1) {
	RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotValueChanged: unknown sender" << endl;
	return;
    }	

    Rosegarden::InstrumentId id = Rosegarden::SoftSynthInstrumentBase + instrumentNo;
    
    if (index >= int(m_synthPlugins.size())) {
	RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotValueChanged: synth "
		 << index << " out of range" << endl;
	return;
    }

    // NB m_synthPlugins[0] is -1 to represent the <none> item

    Rosegarden::AudioPlugin *plugin = m_pluginManager->getPlugin(m_synthPlugins[index]);
    Rosegarden::Instrument *instrument = m_studio->getInstrumentById(id);

    if (instrument) {

	Rosegarden::AudioPluginInstance *pluginInstance = instrument->getPlugin
	    (Rosegarden::Instrument::SYNTH_PLUGIN_POSITION);

	if (pluginInstance) {
	    
	    if (plugin) {
		RG_DEBUG << "plugin is " << plugin->getIdentifier() << endl;
		pluginInstance->setIdentifier(plugin->getIdentifier().data());

		// set ports to defaults

		Rosegarden::PortIterator it = plugin->begin();
		int count = 0;

		for (; it != plugin->end(); ++it) {

		    if (((*it)->getType() & Rosegarden::PluginPort::Control) &&
			((*it)->getType() & Rosegarden::PluginPort::Input)) {
			
			if (pluginInstance->getPort(count) == 0) {
			    pluginInstance->addPort(count, (float)(*it)->getDefaultValue());
			} else {
			    pluginInstance->getPort(count)->value = (*it)->getDefaultValue();
			}
		    }

		    ++count;
		}

	    } else {
		pluginInstance->setIdentifier("");
	    }
	}
    }
    
    emit pluginSelected(id, Rosegarden::Instrument::SYNTH_PLUGIN_POSITION,
			m_synthPlugins[index]);
}

