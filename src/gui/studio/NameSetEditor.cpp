/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "NameSetEditor.h"
#include "BankEditorDialog.h"
#include "gui/widgets/LineEdit.h"


#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QCompleter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>

#include <iostream>

namespace Rosegarden
{

NameSetEditor::NameSetEditor(BankEditorDialog* bankEditor,
                             QString title,
                             QWidget* parent,
                             const char* name,
                             QString headingPrefix,
                             bool showKeyMapButtons) :
    QGroupBox(title, parent),
    m_bankEditor(bankEditor),
    m_mainFrame(new QFrame(this))
{
    setObjectName(name);  // probably not needed, but too lazy to research this time
    QVBoxLayout *layout = new QVBoxLayout;

    m_mainFrame->setContentsMargins(0, 1, 0, 1);
    m_mainLayout = new QGridLayout(m_mainFrame);
    m_mainLayout->setSpacing(0);
    m_mainFrame->setLayout(m_mainLayout);
    layout->addWidget(m_mainFrame);

    // Librarian
    //
    QGroupBox *groupBox = new QGroupBox(tr("Provided by"), m_mainFrame);
    QGridLayout *groupBoxLayout = new QGridLayout;

    m_mainLayout->addWidget(groupBox, 0, 3, 3, 3);

    //TODO convert to LineEdit or such, so end users can edit these fields (but
    // not at this stage of sorting out; save this for polish afterwards)
    m_librarian = new QLabel(groupBox);
    groupBoxLayout->addWidget(m_librarian, 0, 1);

    m_librarianEmail = new QLabel(groupBox);
    groupBoxLayout->addWidget(m_librarianEmail, 1, 1);

    groupBox->setLayout(groupBoxLayout);
    //TODO add some message box to come up from a suitable context and explain
    // where to send modified files, and that you can browse the latest
    // available library at:
    //
    // http://rosegarden.svn.sourceforge.net/viewvc/rosegarden/trunk/rosegarden/data/library/
    //
//  groupBox->setToolTip(tr("<qt><p>The librarian maintains the Rosegarden device data for this device.</p><p>If you've made modifications to suit your own device, it might be worth\nliaising with the librarian in order to publish your information for the benefit\nof others."));

    QTabWidget* tabw = new QTabWidget(this);
    layout->addWidget(tabw);

    setLayout(layout);

    QWidget *h;
    QHBoxLayout *hLayout;

    QWidget *v;
    QVBoxLayout *vLayout;

    QWidget *numBox;
    QHBoxLayout *numBoxLayout;

    unsigned int tabs = 4;
    unsigned int cols = 2;
    unsigned int labelId = 0;

    for (unsigned int tab = 0; tab < tabs; ++tab) {
        h = new QWidget(tabw);
        hLayout = new QHBoxLayout;

        for (unsigned int col = 0; col < cols; ++col) {
            v = new QWidget(h);
            vLayout = new QVBoxLayout;
            hLayout->addWidget(v);

            for (unsigned int row = 0; row < 128 / (tabs*cols); ++row) {
                numBox = new QWidget(v);
                numBoxLayout = new QHBoxLayout;
                vLayout->addWidget(numBox);
                // take out the excess vertical space that was making this
                // dialog two screens tall
                numBoxLayout->setMargin(2);
                QString numberText = QString("%1").arg(labelId + 1);

                if (tab == 0 && col == 0 && row == 0) {
                    // Initial label; button to adjust whether labels start at 0 or 1
                    m_initialLabel = new QPushButton(numberText, numBox);
                    m_initialLabel->setFixedWidth(25);
                    numBoxLayout->addWidget(m_initialLabel);
                    connect(m_initialLabel,
                            SIGNAL(clicked()),
                            this,
                            SLOT(slotToggleInitialLabel()));
                } else {
                    QLabel *label = new QLabel(numberText, numBox);
                    numBoxLayout->addWidget(label);
                    label->setFixedWidth(30);
                    label->setAlignment(Qt::AlignCenter);
                    m_labels.push_back(label);
                }


                if (showKeyMapButtons) {
                    QToolButton *button = new QToolButton;
                    button->setObjectName(numberText);
                    numBoxLayout->addWidget(button);
                    connect(button, SIGNAL(clicked()),
                            this, SLOT(slotKeyMapButtonPressed()));
                    m_keyMapButtons.push_back(button);
                }

                LineEdit* lineEdit = new LineEdit("", numBox);
                lineEdit->setObjectName(numberText);
                numBoxLayout->addWidget(lineEdit);
                numBox->setLayout(numBoxLayout);
                lineEdit->setMinimumWidth(110);
                
                lineEdit->setCompleter(new QCompleter(m_completions));
                
                m_names.push_back(lineEdit);

                connect(m_names[labelId],
                        SIGNAL(textChanged(const QString&)),
                        this,
                        SLOT(slotNameChanged(const QString&)));

                ++labelId;
            }
            v->setLayout(vLayout);
            vLayout->setSpacing(0);
        }
        h->setLayout(hLayout);

        tabw->addTab(h,
                     (tab == 0 ? headingPrefix + QString(" %1 - %2") :
                      QString("%1 - %2")).
                     arg(tab * (128 / tabs) + 1).
                     arg((tab + 1) * (128 / tabs)));
    }

    m_initialLabel->setMaximumSize(m_labels.front()->size());
}

void
NameSetEditor::slotToggleInitialLabel()
{
    QString initial = m_initialLabel->text();

    // strip some unrequested nice-ification.. urg!
    if (initial.startsWith("&")) {
        initial = initial.right(initial.length() - 1);
    }

    bool ok;
    unsigned index = initial.toUInt(&ok);

    if (!ok) {
        std::cerr << "conversion of '"
        << initial.toStdString().c_str()
        << "' to number failed"
        << std::endl;
        return ;
    }

    if (index == 0)
        index = 1;
    else
        index = 0;

    m_initialLabel->setText(QString("%1").arg(index++));
    for (std::vector<QLabel*>::iterator it( m_labels.begin() );
            it != m_labels.end();
            ++it) {
        (*it)->setText(QString("%1").arg(index++));
    }
}

}
