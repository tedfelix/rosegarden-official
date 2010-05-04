/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Idea obtained from an article by David Boddie in Qt Quarterly.
*/
#include <qaction.h>
#include <qpushbutton.h>
#include <q3table.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>

#include "misc/Debug.h"
#include "gui/dialogs/ShortcutsDialog.h"

namespace Rosegarden
{

ShortcutsDialog::ShortcutsDialog(QObjectList *shortcutsList, QWidget *parent)
    : QDialog(parent)
{
    /* how many rows do we need, skip all empty stuff */
    int maxRows=0;
    for (QObjectList::iterator i=shortcutsList->begin(); i!=shortcutsList->end();++i) {
        QAction *shortcut = static_cast<QAction*>((*i));
        QString tmpAccelText = QString(shortcut->accel());
        QString tmpText = shortcut->text();
        if ( !tmpAccelText.isEmpty() || !tmpText.isEmpty() ) {
          ++maxRows;
        }
    }

    Q3Table *dummyTable = new Q3Table(1, 1, this);
    dummyTable->setText(0, 0, "                                                                                                             ");
    dummyTable->verticalHeader()->hide();
    dummyTable->horizontalHeader()->hide();
    dummyTable->setShowGrid(false);
    dummyTable->setColumnReadOnly(0, true);
    dummyTable->adjustColumn(0);

    m_shortcutsTable = new Q3Table(maxRows, 2, this);
    m_shortcutsTable->horizontalHeader()->setLabel(0, tr("     Description     "));
    m_shortcutsTable->horizontalHeader()->setLabel(1, tr("     Shortcut     "));
    m_shortcutsTable->verticalHeader()->hide();
    m_shortcutsTable->setLeftMargin(0);
    m_shortcutsTable->setColumnReadOnly(0, true);

    int row = 0;
    RG_DEBUG << "ShortcutsDialog::ShortcutsDialog()" << endl;

    for (QObjectList::iterator i=shortcutsList->begin(); i!=shortcutsList->end();++i) {
        QAction *shortcut = static_cast<QAction*>((*i));
        RG_DEBUG << "ShortcutsDialog::ShortcutsDialog():" << shortcut->text() << " / "  << QString(shortcut->accel()) << endl;

        QString tmpAccelText = QString(shortcut->accel());
        QString tmpText = shortcut->text();

        if ( !tmpAccelText.isEmpty() || !tmpText.isEmpty() ) {
          m_shortcutsTable->setText(row, 0, shortcut->text());
          m_shortcutsTable->setText(row, 1, QString(shortcut->accel()));

          m_shortcutsList.append(shortcut);
          ++row;
        }
    }
    m_shortcutsTable->adjustColumn(0);
    m_shortcutsTable->adjustColumn(1);

    QPushButton *okButton = new QPushButton(tr("&OK"), this);
    QPushButton *cancelButton = new QPushButton(tr("&Cancel"), this);

    connect(m_shortcutsTable, SIGNAL(currentChanged(int, int)),
            this, SLOT(recordShortcut(int, int)));
    connect(m_shortcutsTable, SIGNAL(valueChanged(int, int)),
            this, SLOT(validateShortcut(int, int)));
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    Q3HBoxLayout *buttonLayout = new Q3HBoxLayout;
    buttonLayout->setSpacing(18);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    Q3VBoxLayout *mainLayout = new Q3VBoxLayout(this);
    mainLayout->setMargin(10);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(m_shortcutsTable);
    mainLayout->addWidget(dummyTable);
    mainLayout->addLayout(buttonLayout);

    setCaption(tr("Edit Shortcuts"));
}

void ShortcutsDialog::accept()
{
    for (int row = 0; row < (int)m_shortcutsList.size(); ++row) {
        QAction *shortcut = m_shortcutsList[row];
        RG_DEBUG << "ShortcutsDialog: accept " << row << " = " << m_shortcutsTable->text(row, 1) << endl;

        shortcut->setAccel(QKeySequence(m_shortcutsTable->text(row, 1)));
    }

    QDialog::accept();
}

void ShortcutsDialog::recordShortcut(int row, int column)
{
    m_oldAccelText = m_shortcutsTable->item(row, column)->text();
    RG_DEBUG << "ShortcutsDialog: recordShortcut " << m_oldAccelText << endl;
}

void ShortcutsDialog::validateShortcut(int row, int column)
{
    Q3TableItem *item = m_shortcutsTable->item(row, column);
    QString accelText = QString(QKeySequence(item->text()));

    RG_DEBUG << "ShortcutsDialog: validateShortcut " << accelText << " / " <<  m_oldAccelText << endl;

    if (accelText.isEmpty() && !item->text().isEmpty())
        item->setText(m_oldAccelText);
    else
        item->setText(accelText);
}

}

#include "ShortcutsDialog.moc"

