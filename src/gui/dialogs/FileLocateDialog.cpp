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
#include <QGridLayout>
#include <QPushButton>


namespace Rosegarden
{


FileLocateDialog::FileLocateDialog(QWidget *parent,
                                   const QString &file,
                                   const QString &path):
    QDialog(parent),
    m_result(Cancel),
    m_path(path),
    m_fileName(file)
{
    setWindowTitle(tr("Locate audio file"));
    setModal(true);
    setContentsMargins(10, 10, 10, 10);

    QGridLayout *gridLayout = new QGridLayout;
    setLayout(gridLayout);

    int row = 0;

    QLabel *label = new QLabel(
            tr("<p>Could not find audio file:</p><p>&nbsp;&nbsp;%1</p><p>at expected audio file location:</p><p>&nbsp;&nbsp;%2</p><p>You can either cancel the file open and move the files yourself or locate the missing file and adjust the audio file location to match.</p><p>Which would you like to do?</p>").
                    arg(m_fileName).
                    arg(m_path));

    gridLayout->addWidget(label, row, 0);
    gridLayout->setRowStretch(row, 10);

    ++row;

    // Spacer
    gridLayout->setRowMinimumHeight(row, 20);

    ++row;

    QDialogButtonBox *buttonBox = new QDialogButtonBox;

    QPushButton *locate = new QPushButton(tr("&Locate Missing File"));
    locate->setProperty("Action", static_cast<int>(Locate));
    buttonBox->addButton(locate, QDialogButtonBox::ActionRole);

    QPushButton *skip = new QPushButton(tr("&Skip This Audio File"));
    skip->setProperty("Action", static_cast<int>(Skip));
    buttonBox->addButton(skip, QDialogButtonBox::ActionRole);

    QPushButton *cancel = new QPushButton(tr("&Cancel File Open"));
    cancel->setProperty("Action", static_cast<int>(Cancel));
    buttonBox->addButton(cancel, QDialogButtonBox::ActionRole);

    connect(buttonBox, &QDialogButtonBox::clicked,
            this, &FileLocateDialog::slotButtonClicked);

    gridLayout->addWidget(buttonBox, row, 0);
}

void
FileLocateDialog::slotButtonClicked(QAbstractButton *button)
{
    m_result = static_cast<Result>(button->property("Action").toInt());

    if (m_result == Locate) {
        // ??? We really just need a directory.  Is there a directory open
        //     dialog that would show the files to help confirm we are in
        //     the right place?
        m_fileName = FileDialog::getOpenFileName(
                this,  // parent
                tr("Select an Audio File"),  // caption
                m_path,  // dir
                tr("Requested file") +
                    QString(" (%1)").arg(QFileInfo(m_fileName).fileName()) +
                    ";;" +
                tr("WAV files") + " (*.wav *.WAV)" + ";;" +
                tr("All files") + " (*)");  // filter

        if (!m_fileName.isEmpty()) {
            QFileInfo fileInfo(m_fileName);
            m_path = fileInfo.path();
        }
    }

    // Always accept.  The buttons are all actions.  There isn't really a
    // concept of "reject" at this level.  Call getResult() to find out which
    // button was pressed.
    accept();
}


}
