/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[NameSetEditor]"

#include "NameSetEditor.h"

#include "BankEditorDialog.h"
#include "misc/Debug.h"
#include "gui/widgets/LineEdit.h"

#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QCompleter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QScrollArea>

namespace Rosegarden
{


NameSetEditor::NameSetEditor(BankEditorDialog *bankEditor,
                             QString title,
                             QWidget *parent,
                             bool showKeyMapButtons) :
    QGroupBox(title, parent),
    m_bankEditor(bankEditor),
    m_topFrame(new QFrame(this)),
    m_topLayout(new QGridLayout(m_topFrame)),
    m_librarian(nullptr),
    m_librarianEmail(nullptr),
    m_names(),
    m_completions(),
    m_numberingBaseButton(nullptr),
    m_numberingBase(1),
    m_labels(),
    m_keyMapButtons()
{
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(6, 2, 6, 6);

    // Area above the tabbed widget.

    m_topFrame->setContentsMargins(0, 0, 0, 5);
    m_topLayout->setSpacing(0);
    m_topLayout->setMargin(0);
    m_topFrame->setLayout(m_topLayout);
    mainLayout->addWidget(m_topFrame);

    // "Provided by" groupbox

    QGroupBox *groupBox = new QGroupBox(tr("Provided by"), m_topFrame);
    QGridLayout *groupBoxLayout = new QGridLayout;

    m_topLayout->addWidget(groupBox, 0, 3, 3, 3);

    // Librarian
    // ??? Would be nice if we could edit this.
    m_librarian = new QLabel(groupBox);
    groupBoxLayout->addWidget(m_librarian, 0, 1);

    // Librarian email
    // ??? Would be nice if we could edit this.
    m_librarianEmail = new QLabel(groupBox);
    groupBoxLayout->addWidget(m_librarianEmail, 1, 1);

    groupBox->setLayout(groupBoxLayout);

    // QScrollArea

    QScrollArea *scrollArea = new QScrollArea(this);
    // Make sure widget is expanded to fill the scroll area.
    scrollArea->setWidgetResizable(true);

    mainLayout->addWidget(scrollArea);

    setLayout(mainLayout);

    // Widget and layout to hold each of the rows.
    QWidget *listWidget = new QWidget;
    QVBoxLayout *listLayout = new QVBoxLayout;
    listLayout->setSpacing(2);

    unsigned index = 0;

    // For each row
    for (unsigned int row = 0; row < 128; ++row) {
        // Widget and layout to hold the row.  A row consists of the
        // number label, optional keymap button, and name line edit.
        QWidget *rowWidget = new QWidget;
        QHBoxLayout *rowLayout = new QHBoxLayout;
        // take out the excess vertical space that was making this
        // dialog two screens tall
        rowLayout->setMargin(0);

        // If this is the very first number label, make it a button.
        if (index == 0) {
            m_numberingBaseButton = new QPushButton("", rowWidget);
            m_numberingBaseButton->setFixedWidth(25);
            connect(m_numberingBaseButton,
                    SIGNAL(clicked()),
                    SLOT(slotToggleNumberingBase()));

            rowLayout->addWidget(m_numberingBaseButton);

        } else {  // All other numbers are QLabels.
            QLabel *label = new QLabel("", rowWidget);
            label->setFixedWidth(30);
            label->setAlignment(Qt::AlignCenter);
            m_labels.push_back(label);

            rowLayout->addWidget(label);
        }

        if (showKeyMapButtons) {
            QToolButton *button = new QToolButton;
            // Object name is required or else serious data loss occurs.
            // ??? Actually, this seems to work fine without this.  Leaving
            //     it here for safety.
            button->setObjectName(QString("Key Map Button %1").arg(index));
            button->setProperty("index", index);
            connect(button, SIGNAL(clicked()),
                    this, SLOT(slotKeyMapButtonPressed()));
            m_keyMapButtons.push_back(button);

            rowLayout->addWidget(button);
        }

        // Note: ThornStyle::sizeFromContents() reduces the size
        //       of these so they will fit on smaller displays.
        LineEdit *lineEdit = new LineEdit("", rowWidget);
        // A unique object name is required for each widget or else serious
        // data loss occurs when switching between nodes on the tree.  Not
        // sure why.
        lineEdit->setObjectName(QString("Line Edit %1").arg(index));
        lineEdit->setProperty("index", index);
        lineEdit->setCompleter(new QCompleter(m_completions));

        m_names.push_back(lineEdit);
        connect(m_names[index],
                SIGNAL(textChanged(const QString &)),
                SLOT(slotNameChanged(const QString &)));

        rowLayout->addWidget(lineEdit, 1);

        rowWidget->setLayout(rowLayout);

        listLayout->addWidget(rowWidget);

        ++index;
    }

    listWidget->setLayout(listLayout);

    scrollArea->setWidget(listWidget);

    m_numberingBaseButton->setMaximumSize(m_labels.front()->size());

    updateLabels();
}

void
NameSetEditor::slotToggleNumberingBase()
{
    m_numberingBase ^= 1;
    updateLabels();
}

void
NameSetEditor::updateLabels()
{
    unsigned index = m_numberingBase;

    m_numberingBaseButton->setText(QString("%1").arg(index++));

    // For each subsequent label.
    for (size_t i = 0; i < m_labels.size(); ++i) {
        m_labels[i]->setText(QString("%1").arg(index++));
    }
}


}
