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

namespace Rosegarden
{


NameSetEditor::NameSetEditor(BankEditorDialog *bankEditor,
                             QString title,
                             QWidget *parent,
                             QString headingPrefix,
                             bool showKeyMapButtons) :
    QGroupBox(title, parent),
    m_bankEditor(bankEditor),
    m_topFrame(new QFrame(this)),
    m_topLayout(new QGridLayout(m_topFrame)),
    m_librarian(NULL),
    m_librarianEmail(NULL),
    m_names(),
    m_completions(),
    m_numberingBaseButton(NULL),
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

    // Tabbed widget.

    QTabWidget *tabWidget = new QTabWidget(this);
    mainLayout->addWidget(tabWidget);

    setLayout(mainLayout);

    // Note: For smaller displays, this can be increased to 8.
    const unsigned tabs = 4;
    const unsigned cols = 2;
    const unsigned rows = 128 / (tabs * cols);
    const unsigned entriesPerTab = 128 / tabs;

    unsigned int index = 0;

    // For each tab
    for (unsigned int tab = 0; tab < tabs; ++tab) {
        // Widget and layout to hold each of the columns.
        QWidget *pageWidget = new QWidget(tabWidget);
        QHBoxLayout *pageLayout = new QHBoxLayout;

        // For each column
        for (unsigned int col = 0; col < cols; ++col) {
            // Widget and layout to hold each of the rows.
            QWidget *columnWidget = new QWidget(pageWidget);
            QVBoxLayout *columnLayout = new QVBoxLayout;
            columnLayout->setSpacing(2);

            pageLayout->addWidget(columnWidget);

            // For each row
            for (unsigned int row = 0; row < rows; ++row) {
                // Widget and layout to hold the row.  A row consists of the
                // number label, optional keymap button, and name line edit.
                QWidget *rowWidget = new QWidget(columnWidget);
                QHBoxLayout *rowLayout = new QHBoxLayout;
                // take out the excess vertical space that was making this
                // dialog two screens tall
                rowLayout->setMargin(0);

                columnLayout->addWidget(rowWidget);

                // If this is the very first number label, make it a button.
                if (tab == 0  &&  col == 0  &&  row == 0) {
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
                    // 1-based
                    button->setProperty("index", index + 1);
                    connect(button, SIGNAL(clicked()),
                            this, SLOT(slotKeyMapButtonPressed()));
                    m_keyMapButtons.push_back(button);

                    rowLayout->addWidget(button);
                }

                // Note: ThornStyle::sizeFromContents() reduces the size
                //       of these so they will fit on smaller displays.
                LineEdit *lineEdit = new LineEdit("", rowWidget);
                // 1-based
                lineEdit->setProperty("index", index + 1);
                lineEdit->setMinimumWidth(110);
                lineEdit->setCompleter(new QCompleter(m_completions));

                m_names.push_back(lineEdit);
                connect(m_names[index],
                        SIGNAL(textChanged(const QString &)),
                        SLOT(slotNameChanged(const QString &)));

                rowLayout->addWidget(lineEdit);
                rowWidget->setLayout(rowLayout);

                ++index;
            }

            columnWidget->setLayout(columnLayout);
        }

        pageWidget->setLayout(pageLayout);

        const unsigned from = tab * entriesPerTab + 1;
        const unsigned to = (tab + 1) * entriesPerTab;
        QString range = QString("%1 - %2").arg(from).arg(to);

        QString tabLabel;
        if (tab == 0)
            tabLabel = headingPrefix + " " + range;
        else
            tabLabel = range;

        tabWidget->addTab(pageWidget, tabLabel);
    }

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
