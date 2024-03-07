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

#define RG_MODULE_STRING "[AudioPluginParameterDialog]"
#define RG_NO_DEBUG_PRINT 1

#include "AudioPluginParameterDialog.h"

#include "gui/application/RosegardenMainWindow.h"
#include "gui/studio/AudioPluginGUIManager.h"
#include "misc/Debug.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTimer>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QComboBox>
#include <QFileDialog>

namespace Rosegarden
{

AudioPluginParameterDialog::AudioPluginParameterDialog
(QWidget* parent,
 InstrumentId instrument,
 int position) :
    QDialog(parent),
    m_instrument(instrument),
    m_position(position),
    m_pluginGUIManager(RosegardenMainWindow::self()->getPluginGUIManager()),
    m_openEditorType(AudioPluginInstance::ParameterType::UNKNOWN)
{
    int minRowHeight = 35;
    QHBoxLayout* layout = new QHBoxLayout;
    setLayout(layout);
    QGroupBox *paramBox = new QGroupBox(tr("Plugin Parameters"));
    layout->addWidget(paramBox);
    QGridLayout* gridLayout = new QGridLayout;
    paramBox->setLayout(gridLayout);
    m_pluginGUIManager->getParameters(m_instrument, m_position, m_params);
    int row = 0;
    QLabel* paramLabel = new QLabel(tr("<b>Parameter</b>"));
    gridLayout->addWidget(paramLabel, row, 0);
    QLabel* valueLabel = new QLabel(tr("<b>Value</b>"));
    gridLayout->addWidget(valueLabel, row, 1);
    QLabel* editLabel = new QLabel(tr("<b>Set value</b>"));
    gridLayout->addWidget(editLabel, row, 2);
    gridLayout->setRowMinimumHeight(row, minRowHeight);
    row++;
    for(auto& pair : m_params) {
        const QString& propertyId = pair.first;
        AudioPluginInstance::PluginParameter& pd = pair.second;
        QLabel* lLabel = new QLabel(pd.label);
        gridLayout->addWidget(lLabel, row, 0);
        QLabel* vLabel = new QLabel;
        m_valueLabels[propertyId] = vLabel;
        if (pd.value.type() != QVariant::Invalid) {
            RG_DEBUG << "got value" << pd.value.toString();
            vLabel->setText(pd.value.toString());
        } else {
            vLabel->setText(tr("<not set>"));
        }
        gridLayout->addWidget(vLabel, row, 1);
        if (pd.writable) {
            QPushButton* setPushButton = new QPushButton(tr("Set value"));
            setPushButton->setObjectName(propertyId);
            connect(setPushButton, &QPushButton::clicked,
                    this, &AudioPluginParameterDialog::slotSetValue);
            gridLayout->addWidget(setPushButton, row, 2);
        }
        gridLayout->setRowMinimumHeight(row, minRowHeight);
        row++;
    }

    // the parameter editor
    m_editorBox = new QGroupBox(tr("Edit Parameter"));
    m_editorBox->hide();
    layout->addWidget(m_editorBox);
    QVBoxLayout* editorBoxLayout = new QVBoxLayout;
    m_editorBox->setLayout(editorBoxLayout);
    m_editor = new QWidget(this);
    editorBoxLayout->addWidget(m_editor);
    QVBoxLayout* editorLayout = new QVBoxLayout(m_editor);
    m_editor->setLayout(editorLayout);

    m_editorHeader = new QLabel(m_editor);
    editorLayout->addWidget(m_editorHeader);

    m_paramLineEdit = new QLineEdit(m_editor);
    editorLayout->addWidget(m_paramLineEdit);
    m_paramLineEdit->hide();
    m_intValidator = new QIntValidator(m_editor);
    m_doubleValidator = new QDoubleValidator(m_editor);

    m_boolComboBox = new QComboBox(m_editor);
    m_boolComboBox->addItem(tr("false"));
    m_boolComboBox->addItem(tr("true"));
    editorLayout->addWidget(m_boolComboBox);
    m_boolComboBox->hide();

    m_pathButton = new QPushButton(tr("Select Path"), m_editor);
    connect(m_pathButton, &QPushButton::clicked,
            this, &AudioPluginParameterDialog::slotSelectPath);
    editorLayout->addWidget(m_pathButton);
    m_pathButton->hide();

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                       | QDialogButtonBox::Cancel, m_editor);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &AudioPluginParameterDialog::slotEditAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &AudioPluginParameterDialog::slotEditReject);
    editorLayout->addWidget(m_buttonBox);

    editorLayout->addStretch();

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout,
            this, &AudioPluginParameterDialog::slotUpdate);
    m_updateTimer->start(200);

    hideEditor();
}

void AudioPluginParameterDialog::slotSetValue()
{
    RG_DEBUG << "slotSetValue" << sender()->objectName();
    showEditor(sender()->objectName());
}

void AudioPluginParameterDialog::slotUpdate()
{
    //RG_DEBUG << "slotUpdate";

    m_pluginGUIManager->getParameters(m_instrument, m_position, m_params);
    for(auto& pair : m_params) {
        const QString& id = pair.first;
        AudioPluginInstance::PluginParameter& pd = pair.second;
        QLabel* vLabel = m_valueLabels[id];
        if (pd.value.type() != QVariant::Invalid) {
            RG_DEBUG << "got value" << pd.value.toString();
            vLabel->setText(pd.value.toString());
        } else {
            vLabel->setText(tr("<not set>"));
        }
    }
}

void AudioPluginParameterDialog::slotEditAccept()
{
    RG_DEBUG << "accept";
    // set value
    AudioPluginInstance::PluginParameter param = m_params[m_openEditorId];
    bool bval = (bool)(m_boolComboBox->currentIndex());
    QString tval = m_paramLineEdit->text();
    switch (m_openEditorType) {
    case AudioPluginInstance::ParameterType::BOOL:
        param.value = bval;
        break;
    case AudioPluginInstance::ParameterType::INT:
    case AudioPluginInstance::ParameterType::LONG:
        {
            int i = tval.toInt();
            param.value = i;
        }
        break;
    case AudioPluginInstance::ParameterType::FLOAT:
    case AudioPluginInstance::ParameterType::DOUBLE:
        {
            double d = tval.toDouble();
            param.value = d;
        }
        break;
    case AudioPluginInstance::ParameterType::STRING:
    case AudioPluginInstance::ParameterType::PATH:
        param.value = tval;
        break;
    case AudioPluginInstance::ParameterType::UNKNOWN:
    default:
        break;
    }

    m_pluginGUIManager->updatePluginParameter(m_instrument,
                                              m_position,
                                              m_openEditorId,
                                              param);
    clearEditor();
    hideEditor();
    m_openEditorType = AudioPluginInstance::ParameterType::UNKNOWN;
}

void AudioPluginParameterDialog::slotEditReject()
{
    RG_DEBUG << "reject";
    clearEditor();
    hideEditor();
    m_openEditorType = AudioPluginInstance::ParameterType::UNKNOWN;
}

void AudioPluginParameterDialog::showEditor(const QString& paramId)
{
    RG_DEBUG << "showEditor" << paramId;
    if (m_openEditorType != AudioPluginInstance::ParameterType::UNKNOWN)
        hideEditor();
    AudioPluginInstance::PluginParameter param = m_params[paramId];
    AudioPluginInstance::ParameterType ptype = param.type;

    if (param.value.type() != QVariant::Invalid) {
        if (ptype == AudioPluginInstance::ParameterType::BOOL) {
            bool bval = param.value.toBool();
            m_boolComboBox->setCurrentIndex(bval);
        } else {
            QString tval = param.value.toString();
            m_paramLineEdit->setText(tval);
        }
    }

    if (ptype == AudioPluginInstance::ParameterType::INT ||
        ptype == AudioPluginInstance::ParameterType::LONG) {
        m_paramLineEdit->setValidator(m_intValidator);
    }
    if (ptype == AudioPluginInstance::ParameterType::FLOAT ||
        ptype == AudioPluginInstance::ParameterType::DOUBLE) {
        m_paramLineEdit->setValidator(m_doubleValidator);
    }

    m_openEditorType = ptype;
    m_openEditorId = paramId;

    m_editorBox->show();
    m_editorHeader->setText
        (tr("<b>Editing parameter %1</b>").arg(param.label));

    if (ptype == AudioPluginInstance::ParameterType::BOOL) m_boolComboBox->show();
    else m_paramLineEdit->show();

    if (ptype == AudioPluginInstance::ParameterType::PATH) m_pathButton->show();

    adjustSize();
}

void AudioPluginParameterDialog::hideEditor()
{
    RG_DEBUG << "hideEditor";
    m_editorBox->hide();
    m_boolComboBox->hide();
    m_paramLineEdit->hide();
    m_paramLineEdit->setValidator(nullptr);
    m_pathButton->hide();
    m_openEditorType = AudioPluginInstance::ParameterType::UNKNOWN;
    m_openEditorId = "";
    adjustSize();
}

void AudioPluginParameterDialog::clearEditor()
{
    RG_DEBUG << "clearEditor";
    m_paramLineEdit->clear();
    m_boolComboBox->setCurrentIndex(0);
}

void AudioPluginParameterDialog::slotSelectPath()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select File"));
    m_paramLineEdit->setText(fileName);
}

}
