/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SynthPluginManagerDialog.h"
#include <QLayout>

#include <klocale.h>
#include "misc/Debug.h"
#include "AudioPlugin.h"
#include "AudioPluginManager.h"
#include "AudioPluginOSCGUIManager.h"
#include "base/AudioPluginInstance.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include <QAction>
#include <QComboBox>
#include <QMainWindow>
#include <kstandardaction.h>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>


namespace Rosegarden
{

SynthPluginManagerDialog::SynthPluginManagerDialog(QWidget *parent,
            RosegardenGUIDoc *doc
#ifdef HAVE_LIBLO
            , AudioPluginOSCGUIManager *guiManager
#endif
                                                      ) :
            QMainWindow(parent),
            m_document(doc),
            m_studio(&doc->getStudio()),
            m_pluginManager(doc->getPluginManager())
#ifdef HAVE_LIBLO
            , m_guiManager(guiManager)
#endif
    {
        setWindowTitle(i18n("Manage Synth Plugins"));

        QFrame *mainBox = new QFrame(this);
        setCentralWidget(mainBox);

        mainBox->setContentsMargins(10, 10, 10, 10);
        QVBoxLayout *mainLayout = new QVBoxLayout(mainBox);
        mainLayout->setSpacing(10);

        QGroupBox *pluginFrame = new QGroupBox(i18n("Synth plugins"), mainBox);
        pluginFrame->setContentsMargins(3, 3, 3, 3);
        QGridLayout *pluginLayout = new QGridLayout(pluginFrame);
        pluginLayout->setSpacing(3);

        mainLayout->addWidget(pluginFrame);


        m_synthPlugins.clear();
        m_synthPlugins.push_back( -1);

        int count = 0;

        for (PluginIterator itr = m_pluginManager->begin();
                itr != m_pluginManager->end(); ++itr) {

            if ((*itr)->isSynth()) {
                m_synthPlugins.push_back(count);
            }

            ++count;
        }

        for (unsigned int i = 0; i < SoftSynthInstrumentCount; ++i) {

            InstrumentId id = SoftSynthInstrumentBase + i;
            Instrument *instrument = m_studio->getInstrumentById(id);
            if (!instrument)
                continue;

            //	pluginLayout->addWidget(new QLabel(instrument->getPresentationName().c_str(),
            //					   pluginFrame), i, 0);
            pluginLayout->addWidget(new QLabel(QString("%1").arg(i + 1),
                                               pluginFrame), i, 0);

            AudioPluginInstance *plugin = instrument->getPlugin
                                          (Instrument::SYNTH_PLUGIN_POSITION);

            std::string identifier;
            if (plugin)
                identifier = plugin->getIdentifier();

            int currentIndex = 0;

            QComboBox *pluginCombo = new QComboBox(pluginFrame);
            pluginCombo->addItem(i18n("<none>"));

            for (size_t j = 0; j < m_synthPlugins.size(); ++j) {

                if (m_synthPlugins[j] == -1)
                    continue;

                AudioPlugin *plugin =
                    m_pluginManager->getPlugin(m_synthPlugins[j]);

                pluginCombo->addItem(plugin->getName());

                if (plugin->getIdentifier() == identifier.c_str()) {
                    pluginCombo->setCurrentIndex(pluginCombo->count() - 1);
                }
            }

            connect(pluginCombo, SIGNAL(activated(int)),
                    this, SLOT(slotPluginChanged(int)));

            pluginLayout->addWidget(pluginCombo, i, 1);

            m_synthCombos.push_back(pluginCombo);

            QPushButton *controlsButton = new QPushButton(i18n("Controls"), pluginFrame);
            pluginLayout->addWidget(controlsButton, i, 2);
            connect(controlsButton, SIGNAL(clicked()), this, SLOT(slotControlsButtonClicked()));
            m_controlsButtons.push_back(controlsButton);

#ifdef HAVE_LIBLO

            QPushButton *guiButton = new QPushButton(i18n("Editor >>"), pluginFrame);
            pluginLayout->addWidget(guiButton, i, 3);
            guiButton->setEnabled(m_guiManager->hasGUI
                                  (id, Instrument::SYNTH_PLUGIN_POSITION));
            connect(guiButton, SIGNAL(clicked()), this, SLOT(slotGUIButtonClicked()));
            m_guiButtons.push_back(guiButton);
#endif

        }

        pluginFrame->setLayout(pluginLayout);

        QFrame* btnBox = new QFrame(mainBox);

        btnBox->setSizePolicy(
            QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

        QPushButton *closeButton = new QPushButton(i18n("Close"), btnBox);

        btnBox->setContentsMargins(0,0,0,0);
        QHBoxLayout* layout = new QHBoxLayout(btnBox);
        layout->setSpacing(10);
        layout->addStretch(10);
        layout->addWidget(closeButton);
        layout->addSpacing(5);

        KAction* close = KStandardAction::close(this,
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
    SynthPluginManagerDialog::updatePlugin(InstrumentId id, int plugin)
    {
        if (id < SoftSynthInstrumentBase)
            return ;
        int row = id - SoftSynthInstrumentBase;
        if (row >= m_synthCombos.size())
            return ;

        QComboBox *comboBox = m_synthCombos[row];

        for (unsigned int i = 0; i < m_synthPlugins.size(); ++i) {
            if (m_synthPlugins[i] == plugin) {
                blockSignals(true);
                comboBox->setCurrentIndex(i);
                blockSignals(false);
                return ;
            }
        }

        blockSignals(true);
        comboBox->setCurrentIndex(0);
        blockSignals(false);
        return ;
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
        QMainWindow::closeEvent(e);
    }

    void
    SynthPluginManagerDialog::slotGUIButtonClicked()
    {
        const QObject *s = sender();

        int instrumentNo = -1;

        for (unsigned int i = 0; i < m_guiButtons.size(); ++i) {
            if (s == m_guiButtons[i])
                instrumentNo = i;
        }

        if (instrumentNo == -1) {
            RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotGUIButtonClicked: unknown sender" << endl;
            return ;
        }

        InstrumentId id = SoftSynthInstrumentBase + instrumentNo;

        emit showPluginGUI(id, Instrument::SYNTH_PLUGIN_POSITION);
    }

    void
    SynthPluginManagerDialog::slotControlsButtonClicked()
    {
        const QObject *s = sender();

        int instrumentNo = -1;

        for (unsigned int i = 0; i < m_controlsButtons.size(); ++i) {
            if (s == m_controlsButtons[i])
                instrumentNo = i;
        }

        if (instrumentNo == -1) {
            RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotControlsButtonClicked: unknown sender" << endl;
            return ;
        }

        InstrumentId id = SoftSynthInstrumentBase + instrumentNo;

        emit showPluginDialog(this, id, Instrument::SYNTH_PLUGIN_POSITION);
    }

    void
    SynthPluginManagerDialog::slotPluginChanged(int index)
    {
        const QObject *s = sender();

        RG_DEBUG << "SynthPluginManagerDialog::slotPluginChanged(" << index
        << ")" << endl;

        int instrumentNo = -1;

        for (unsigned int i = 0; i < m_synthCombos.size(); ++i) {
            if (s == m_synthCombos[i])
                instrumentNo = i;
        }

        if (instrumentNo == -1) {
            RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotValueChanged: unknown sender" << endl;
            return ;
        }

        InstrumentId id = SoftSynthInstrumentBase + instrumentNo;

        if (index >= int(m_synthPlugins.size())) {
            RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotValueChanged: synth "
            << index << " out of range" << endl;
            return ;
        }

        // NB m_synthPlugins[0] is -1 to represent the <none> item

        AudioPlugin *plugin = m_pluginManager->getPlugin(m_synthPlugins[index]);
        Instrument *instrument = m_studio->getInstrumentById(id);

        if (instrument) {

            AudioPluginInstance *pluginInstance = instrument->getPlugin
                                                  (Instrument::SYNTH_PLUGIN_POSITION);

            if (pluginInstance) {

                if (plugin) {
                    RG_DEBUG << "plugin is " << plugin->getIdentifier() << endl;
                    pluginInstance->setIdentifier(plugin->getIdentifier().data());

                    // set ports to defaults

                    AudioPlugin::PortIterator it = plugin->begin();
                    int count = 0;

                    for (; it != plugin->end(); ++it) {

                        if (((*it)->getType() & PluginPort::Control) &&
                                ((*it)->getType() & PluginPort::Input)) {

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

#ifdef HAVE_LIBLO
        if (instrumentNo < m_guiButtons.size()) {
            m_guiButtons[instrumentNo]->setEnabled
            (m_guiManager->hasGUI
             (id, Instrument::SYNTH_PLUGIN_POSITION));
        }
#endif

        emit pluginSelected(id, Instrument::SYNTH_PLUGIN_POSITION,
                            m_synthPlugins[index]);
    }

    }
#include "SynthPluginManagerDialog.moc"
