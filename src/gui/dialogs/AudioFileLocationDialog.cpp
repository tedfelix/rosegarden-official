/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
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
#include "gui/widgets/LineEdit.h"
#include "misc/Preferences.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>


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
    layout->addWidget(
            new QLabel(tr("Audio files have been introduced in this session.  Where would you like to save them?")),
            row, 0, 1, 3);

    ++row;

    // Spacer
    layout->setRowMinimumHeight(row, 10);

    ++row;

    // Audio directory.  Recommended.
    m_audioDir = new QRadioButton(tr("To an \"audio\" directory where the document is saved.  (%1) (Recommended)").arg("./audio"));
    layout->addWidget(m_audioDir, row, 1, 1, 2);

    ++row;

    // Document name directory.
    // ??? <br /> isn't working here.  Going with \n.
    m_documentNameDir = new QRadioButton(
            tr("To a directory named after the document where the document is saved.\n(%1)").arg(m_documentNameDirStr));
    layout->addWidget(m_documentNameDir, row, 1, 1, 2);

    ++row;

    // Same directory.
    m_documentDir = new QRadioButton(tr("To the same directory where the document is saved.  (.)"));
    layout->addWidget(m_documentDir, row, 1, 1, 2);

    ++row;

    // Central repo.
    m_centralDir = new QRadioButton(tr("To a central audio file repository.  (%1)").arg("~/rosegarden-audio"));
    layout->addWidget(m_centralDir, row, 1, 1, 2);

    ++row;

    // Custom location.
    m_customDir = new QRadioButton(tr("To a custom audio file location:"));
    layout->addWidget(m_customDir, row, 1);

    m_customDirText = new LineEdit(Preferences::getCustomAudioLocation());
    layout->addWidget(m_customDirText, row, 2);

    ++row;

    // Spacer
    layout->setRowMinimumHeight(row, 10);

    ++row;
    // Note
    layout->addWidget(
            new QLabel(tr("Note: You can always move the audio files later by setting the audio location in the document properties.")),
            row, 0, 1, 3);

    ++row;

    // Spacer
    layout->setRowMinimumHeight(row, 10);

    ++row;

    // Don't show this again.
    m_dontShow = new QCheckBox(tr("Use the above selection for all new files and don't display this dialog again."));
    // ??? Actually, if this dialog becomes the way to set this in the
    //     document properties, we might want to get this from the settings
    //     instead.  Or maybe hide this.
    m_dontShow->setChecked(false);
    layout->addWidget(m_dontShow, row, 1, 1, 2);

    // Spacer
    layout->setRowMinimumHeight(row, 10);

    ++row;

    // Button Box
    QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    layout->addWidget(buttonBox, row, 0, 1, 3);

    updateWidgets();
}

void
AudioFileLocationDialog::updateWidgets()
{
    const Preferences::Location location =
            static_cast<Preferences::Location>(
                    Preferences::getDefaultAudioLocation());

    switch (location) {
    case Preferences::AudioDir:
        m_audioDir->setChecked(true);
        break;
    case Preferences::DocumentNameDir:
        m_documentNameDir->setChecked(true);
        break;
    case Preferences::DocumentDir:
        m_documentDir->setChecked(true);
        break;
    case Preferences::CentralDir:
        m_centralDir->setChecked(true);
        break;
    case Preferences::CustomDir:
        m_customDir->setChecked(true);
        break;
    }

    m_customDirText->setText(Preferences::getCustomAudioLocation());
}

void AudioFileLocationDialog::accept()
{
    // Copy the user's choices to the Preferences.

    Preferences::Location location = Preferences::AudioDir;

    if (m_audioDir->isChecked())
        location = Preferences::AudioDir;
    if (m_documentNameDir->isChecked())
        location = Preferences::DocumentNameDir;
    if (m_documentDir->isChecked())
        location = Preferences::DocumentDir;
    if (m_centralDir->isChecked())
        location = Preferences::CentralDir;
    if (m_customDir->isChecked())
        location = Preferences::CustomDir;

    Preferences::setDefaultAudioLocation(location);
    Preferences::setCustomAudioLocation(m_customDirText->text());

    Preferences::setAudioFileLocationDlgDontShow(m_dontShow->isChecked());

    QDialog::accept();
}


}
