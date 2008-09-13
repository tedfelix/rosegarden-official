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


#include "FileLocateDialog.h"

#include <klocale.h>
#include "misc/Debug.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <kfiledialog.h>
#include <QFileInfo>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>


namespace Rosegarden
{

FileLocateDialog::FileLocateDialog(QDialogButtonBox::QWidget *parent,
                                   const QString &file,
                                   const QString & /*path*/):
        KDialogBase(parent, 0, true,
                    i18n("Locate audio file"),
                    User1 | User2 | User3,
                    Ok,
                    false,
                    i18n("&Skip"),
                    i18n("Skip &All"),
                    i18n("&Locate")),
        m_file(file)
{
    QHBox *w = makeHBoxMainWidget();
    QString label =
        i18n("Can't find file \"%1\".\n"
             "Would you like to try and locate this file or skip it?", m_file);

    QLabel *labelW = new QLabel(label, w);
    labelW->setAlignment(Qt::Qt::AlignCenter);
    labelW->setMinimumHeight(60);
}

void
FileLocateDialog::slotUser3()
{
    if (!m_file.isEmpty()) {
        m_file = KFileDialog::getOpenFileName
                 (":WAVS",
                  i18n("%1|Requested file (%2)\n*.wav|WAV files (*.wav)",
                   QFileInfo(m_file).fileName(),
                   QFileInfo(m_file).fileName()),
                  this, i18n("Select an Audio File"));

        RG_DEBUG << "FileLocateDialog::slotUser3() : m_file = " << m_file << endl;

        if (m_file.isEmpty()) {
            RG_DEBUG << "FileLocateDialog::slotUser3() : reject\n";
            reject();
        } else {
            QFileInfo fileInfo(m_file);
            m_path = fileInfo.dirPath();
            accept();
        }

    } else
        reject();
}

void
FileLocateDialog::slotUser1()
{
    reject();
}

void
FileLocateDialog::slotUser2()
{
    done( -1);
}

}
#include "FileLocateDialog.moc"
