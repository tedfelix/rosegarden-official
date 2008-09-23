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


#include "NameSetEditor.h"
#include "BankEditorDialog.h"
#include <kcompletion.h>
#include <kglobalsettings.h>
#include <klineedit.h>
#include <klocale.h>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <iostream>

namespace Rosegarden
{

NameSetEditor::NameSetEditor(BankEditorDialog* bankEditor,
                             QString title,
                             QWidget* parent,
                             const char* name,
                             QString headingPrefix,
                             bool showEntryButtons)
        : QGroupBox(title, parent),
        m_bankEditor(bankEditor),
        m_mainFrame(new QFrame(this))
{
    QVBoxLayout *layout = new QVBoxLayout;

    m_mainFrame->setContentsMargins(2, 2, 2, 2);
    m_mainLayout = new QGridLayout(m_mainFrame);
    m_mainFrame->setLayout(m_mainLayout);
    layout->addWidget(m_mainFrame);

    // Librarian
    //
    QGroupBox *groupBox = new QGroupBox(i18n("Librarian"), m_mainFrame);
    QHBoxLayout *groupBoxLayout = new QHBoxLayout;

    m_mainLayout->addWidget(groupBox, 0, 3, 2- 1, 5- 4);

    groupBoxLayout->addWidget(new QLabel(i18n("Name")));
    m_librarian = new QLabel(groupBox);
    groupBoxLayout->addWidget(m_librarian);

    groupBoxLayout->addWidget(new QLabel(i18n("Email")));
    m_librarianEmail = new QLabel(groupBox);
    groupBoxLayout->addWidget(m_librarianEmail);

    groupBox->setLayout(groupBoxLayout);
    groupBox->setToolTip(i18n("The librarian maintains the Rosegarden device data for this device.\nIf you've made modifications to suit your own device, it might be worth\nliaising with the librarian in order to publish your information for the benefit\nof others."));

    QTabWidget* tabw = new QTabWidget(this);
    layout->addWidget(tabw);

    setLayout(layout);
    m_mainLayout->setSpacing(layout->spacing());

    tabw->setMargin(10);

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
                QString numberText = QString("%1").arg(labelId + 1);

                if (tab == 0 && col == 0 && row == 0) {
                    // Initial label; button to adjust whether labels start at 0 or 1
                    m_initialLabel = new QPushButton(numberText, numBox);
                    numBoxLayout->addWidget(m_initialLabel);
                    connect(m_initialLabel,
                            SIGNAL(clicked()),
                            this,
                            SLOT(slotToggleInitialLabel()));
                } else {
                    QLabel *label = new QLabel(numberText, numBox);
                    numBoxLayout->addWidget(label);
                    label->setFixedWidth(40);
                    label->setAlignment(Qt::AlignCenter);
                    m_labels.push_back(label);
                }



                if (showEntryButtons) {
                    QPushButton *button = new QPushButton("", numBox, numberText);
                    numBoxLayout->addWidget(button);
                    button->setMaximumWidth(40);
                    button->setMaximumHeight(20);
                    button->setFlat(true);
                    connect(button, SIGNAL(clicked()),
                            this, SLOT(slotEntryButtonPressed()));
                    m_entryButtons.push_back(button);
                }

                KLineEdit* lineEdit = new KLineEdit(numBox, numberText);
                numBoxLayout->addWidget(lineEdit);
                numBox->setLayout(numBoxLayout);
                lineEdit->setMinimumWidth(110);
                lineEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
                lineEdit->setCompletionObject(&m_completion);
                m_names.push_back(lineEdit);

                connect(m_names[labelId],
                        SIGNAL(textChanged(const QString&)),
                        this,
                        SLOT(slotNameChanged(const QString&)));

                ++labelId;
            }
            v->setLayout(vLayout);
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
        << initial.ascii()
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
#include "NameSetEditor.moc"
