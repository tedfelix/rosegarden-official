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

#define RG_MODULE_STRING "[AudioPluginPresetDialog]"
//#define RG_NO_DEBUG_PRINT 1

#include "AudioPluginPresetDialog.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/studio/AudioPluginGUIManager.h"

#include "misc/Debug.h"

#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>

namespace Rosegarden
{

AudioPluginPresetDialog::AudioPluginPresetDialog
(QWidget *parent,
 InstrumentId instrument,
 int position) :
    QDialog(parent),
    m_instrument(instrument),
    m_position(position),
    m_pluginGUIManager(RosegardenMainWindow::self()->getPluginGUIManager())
{
    setWindowTitle(tr("Plugin Presets"));
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);
    QGroupBox *presetBox = new QGroupBox(tr("Plugin Presets"));
    layout->addWidget(presetBox);

    QVBoxLayout* boxLayout = new QVBoxLayout;
    presetBox->setLayout(boxLayout);
    QHBoxLayout * presetLayout = new QHBoxLayout;
    boxLayout->addLayout(presetLayout);
    QLabel* presetLabel = new QLabel(tr("Predefined presets:"));
    presetLayout->addWidget(presetLabel);
    m_presetCombo = new QComboBox;
    m_pluginGUIManager->getPresets(m_instrument, m_position, m_presets);
    for(auto& preset : m_presets) {
        m_presetCombo->addItem(preset.label);
    }
    presetLayout->addWidget(m_presetCombo);
    QPushButton* presetButton = new QPushButton(tr("Set Preset"));
    connect(presetButton, &QPushButton::clicked,
            this, &AudioPluginPresetDialog::slotSetPreset);
    presetLayout->addWidget(presetButton);

    QHBoxLayout* fileLayout = new QHBoxLayout;
    boxLayout->addLayout(fileLayout);
    QLabel* fileLabel = new QLabel(tr("Load/Save state from/to file"));
    fileLayout->addWidget(fileLabel);
    QPushButton* loadButton = new QPushButton(tr("Load"));
    connect(loadButton, &QPushButton::clicked,
            this, &AudioPluginPresetDialog::slotLoadPreset);
    fileLayout->addWidget(loadButton);
    QPushButton* saveButton = new QPushButton(tr("Save"));
    connect(saveButton, &QPushButton::clicked,
            this, &AudioPluginPresetDialog::slotSavePreset);
    fileLayout->addWidget(saveButton);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    // "Close" button has the RejectRole
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

void AudioPluginPresetDialog::slotSetPreset()
{
    int index = m_presetCombo->currentIndex();
    AudioPluginInstance::PluginPreset& pp = m_presets[index];
    RG_DEBUG << "slotSetPreset" << index << pp.uri;
    m_pluginGUIManager->setPreset(m_instrument, m_position, pp.uri);
}

void AudioPluginPresetDialog::slotLoadPreset()
{
    RG_DEBUG << "slotLoadPreset";
    QString file =
        QFileDialog::getOpenFileName(this,
                                     tr("Load preset"),
                                     "",
                                     tr("Preset files") + " (*.rgp)" + ";;" +
                                     tr("All files") + " (*)");
    RG_DEBUG << "file:" << file;
    if (file == "") return;
    m_pluginGUIManager->loadPreset(m_instrument, m_position, file);
}

void AudioPluginPresetDialog::slotSavePreset()
{
    RG_DEBUG << "slotSavePreset";
    QString file =
        QFileDialog::getSaveFileName(this,
                                     tr("Save preset"),
                                     "",
                                     tr("Preset files") + "(*.rgp)");
    RG_DEBUG << "file:" << file;
    if (file == "") return;
    m_pluginGUIManager->savePreset(m_instrument, m_position, file);
}

}
