/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioFileLocationDialog]"

#include "AudioFileLocationDialog.h"

#include "misc/Debug.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSettings>


namespace
{
    const QString AudioFileLocationDialogGroup = "AudioFileLocationDialog";
}


namespace Rosegarden
{


AudioFileLocationDialog::AudioFileLocationDialog(
        QWidget *parent,
        QString documentNameDir) :
    QDialog(parent),
    m_documentNameDirStr(documentNameDir)
{
    setWindowTitle(tr("Audio File Location"));
    setModal(true);
    setContentsMargins(10, 10, 10, 10);

    QGridLayout *layout = new QGridLayout(this);
    layout->setVerticalSpacing(5);

    // Indent column
    layout->setColumnMinimumWidth(0, 20);

    int row = 0;

    // Label
    layout->addWidget(new QLabel(tr("Audio files have been introduced in this session.  Where would you like to save them?")), row, 0, 1, 2);

    ++row;

    // Spacer
    layout->setRowMinimumHeight(row, 10);

    ++row;

    m_audioDir = new QRadioButton(tr("To an \"audio\" directory where the document is saved.  (./audio) (Recommended)"));
    layout->addWidget(m_audioDir, row, 1);

    ++row;

    m_documentNameDir = new QRadioButton(
            tr("To a directory named after the document where the document is saved.") +
            "\n  (" + m_documentNameDirStr + ")");
    layout->addWidget(m_documentNameDir, row, 1);

    ++row;

    m_documentDir = new QRadioButton(tr("To the same directory where the document is saved.  (.)"));
    layout->addWidget(m_documentDir, row, 1);

    ++row;

    m_centralDir = new QRadioButton(tr("To a central audio file repository.  (~/rosegarden-audio)"));
    layout->addWidget(m_centralDir, row, 1);

    ++row;

    m_userDir = new QRadioButton(tr("Somewhere else?"));
    layout->addWidget(m_userDir, row, 1);

    ++row;

    // Button Box
    QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    //connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(buttonBox, row, 0, 1, 2);

    updateWidgets();
}

void
AudioFileLocationDialog::updateWidgets()
{
    QSettings settings;
    settings.beginGroup(AudioFileLocationDialogGroup);
    m_location = static_cast<Location>(
            settings.value("location", AudioDir).toInt());

    switch (m_location) {
    case AudioDir:
        m_audioDir->setChecked(true);
        break;
    case DocumentNameDir:
        m_documentNameDir->setChecked(true);
        break;
    case DocumentDir:
        m_documentDir->setChecked(true);
        break;
    case CentralDir:
        m_centralDir->setChecked(true);
        break;
    case UserDir:
        m_userDir->setChecked(true);
        break;
    }
}

void AudioFileLocationDialog::accept()
{
    if (m_audioDir->isChecked())
        m_location = AudioDir;
    if (m_documentNameDir->isChecked())
        m_location = DocumentNameDir;
    if (m_documentDir->isChecked())
        m_location = DocumentDir;
    if (m_centralDir->isChecked())
        m_location = CentralDir;
    if (m_userDir->isChecked())
        m_location = UserDir;

    QSettings settings;
    settings.beginGroup(AudioFileLocationDialogGroup);
    settings.setValue("location", m_location);

    QDialog::accept();
}


}
