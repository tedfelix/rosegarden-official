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

#include "LibrarianDialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

namespace Rosegarden
{

LibrarianDialog::LibrarianDialog(QWidget *parent,
                                 const QString& librarianName,
                                 const QString& librarianEmail) :
    QDialog(parent)
{
    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    QString labelText = tr("If a librarian is already set please try to "
                           "contact them at the given email address "
                           "or on the development mailing list. To "
                           "publish the device please make a merge "
                           "request or contact the development mailing list");

    QLabel* textLabel = new QLabel(labelText);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel, 0, 0, 1, 2);

    QLabel* nameLabel = new QLabel(tr("Name:"));
    nameLabel->setAlignment(Qt::AlignRight);
    m_nameEdit = new QLineEdit;
    m_nameEdit->setMinimumSize(250, 10);
    m_nameEdit->setText(librarianName);
    layout->addWidget(nameLabel, 1, 0);
    layout->addWidget(m_nameEdit, 1, 1);

    QLabel* mailLabel = new QLabel(tr("Email:"));
    mailLabel->setAlignment(Qt::AlignRight);
    m_mailEdit = new QLineEdit;
    m_mailEdit->setMinimumSize(250, 10);
    m_mailEdit->setText(librarianEmail);
    layout->addWidget(mailLabel, 2, 0);
    layout->addWidget(m_mailEdit, 2, 1);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
    layout->addWidget(buttonBox);
    resize(350, 250);
}

void LibrarianDialog::getLibrarian(QString& name, QString& email)
{
    name = m_nameEdit->text();
    email = m_mailEdit->text();
}

}
