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

#define RG_MODULE_STRING "[FileLocateDialog]"

#include "FileLocateDialog.h"

#include "gui/widgets/FileDialog.h"
#include "misc/Debug.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLabel>
#include <QWidget>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>


namespace Rosegarden
{


FileLocateDialog::FileLocateDialog(QWidget *parent,
                                   const QString &file,
                                   const QString &path):
    QDialog(parent),
    m_path(path),
    m_fileName(file)
{
    setModal(true);
    setWindowTitle(tr("Locate audio file"));

    QGridLayout *gridLayout = new QGridLayout;
    setLayout(gridLayout);

    int row = 0;

    const QString label =
        tr("<p>Could not find audio file:</p><p>&nbsp;&nbsp;%1</p><p>at expected audio file path:</p><p>&nbsp;&nbsp;%2</p><p>Would you like to try and locate this file or cancel file open?</p>").
                arg(m_fileName).
                arg(m_path);

    QLabel *labelW = new QLabel(label);

    gridLayout->addWidget(labelW, row, 0);
    gridLayout->setRowStretch(row, 10);

    ++row;

    // Spacer
    gridLayout->setRowMinimumHeight(row, 20);

    ++row;

    QDialogButtonBox *buttonBox = new QDialogButtonBox;

    QPushButton *user1 = new QPushButton(tr("&Skip"));
    buttonBox->addButton(user1, QDialogButtonBox::ActionRole);
    connect(user1, &QAbstractButton::clicked, this, &FileLocateDialog::slotSkip);

    QPushButton *user2 = new QPushButton(tr("Skip &All"));
    buttonBox->addButton(user2, QDialogButtonBox::ActionRole);
    connect(user2, &QAbstractButton::clicked, this, &FileLocateDialog::slotSkipAll);

    QPushButton *user3 = new QPushButton(tr("&Locate"));
    buttonBox->addButton(user3, QDialogButtonBox::ActionRole);
    connect(user3, &QAbstractButton::clicked, this, &FileLocateDialog::slotLocate);

    gridLayout->addWidget(buttonBox, row, 0);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void
FileLocateDialog::slotLocate()
{
    if (!m_fileName.isEmpty()) {
        m_fileName = FileDialog::getOpenFileName
            (this,
             tr("Select an Audio File"),
             m_path,
             tr("Requested file") + QString(" (%1)").arg(QFileInfo(m_fileName).fileName()) + ";;" +
             tr("WAV files") + " (*.wav *.WAV)" + ";;" +
             tr("All files") + " (*)");

        RG_DEBUG << "FileLocateDialog::slotLocate() : m_file = " << m_fileName;

        if (m_fileName.isEmpty()) {
            RG_DEBUG << "FileLocateDialog::slotLocate() : reject\n";
            reject();
        } else {
            QFileInfo fileInfo(m_fileName);
            m_path = fileInfo.path();
            accept();
        }

    } else {
        reject();
    }
}

void
FileLocateDialog::slotSkip()
{
    reject();
}

void
FileLocateDialog::slotSkipAll()
{
    done( -1);
}


}
