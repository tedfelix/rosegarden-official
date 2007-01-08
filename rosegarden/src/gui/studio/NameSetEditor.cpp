/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
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
#include <qframe.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwidget.h>
#include <iostream>

namespace Rosegarden
{

NameSetEditor::NameSetEditor(BankEditorDialog* bankEditor,
                             QString title,
                             QWidget* parent,
                             const char* name,
                             QString headingPrefix,
                             bool showEntryButtons)
        : QVGroupBox(title, parent, name),
        m_bankEditor(bankEditor),
        m_mainFrame(new QFrame(this))
{
    m_mainLayout = new QGridLayout(m_mainFrame,
                                   4,   // rows
                                   6,   // cols
                                   2); // margin

    // Librarian
    //
    QGroupBox *groupBox = new QGroupBox(2,
                                        Qt::Horizontal,
                                        i18n("Librarian"),
                                        m_mainFrame);
    m_mainLayout->addMultiCellWidget(groupBox, 0, 2, 3, 5);

    new QLabel(i18n("Name"), groupBox);
    m_librarian = new QLabel(groupBox);

    new QLabel(i18n("Email"), groupBox);
    m_librarianEmail = new QLabel(groupBox);

    QToolTip::add
        (groupBox,
                i18n("The librarian maintains the Rosegarden device data for this device.\nIf you've made modifications to suit your own device, it might be worth\nliaising with the librarian in order to publish your information for the benefit\nof others."));

    QTabWidget* tabw = new QTabWidget(this);

    tabw->setMargin(10);

    QHBox *h;
    QVBox *v;
    QHBox *numBox;

    unsigned int tabs = 4;
    unsigned int cols = 2;
    unsigned int labelId = 0;

    for (unsigned int tab = 0; tab < tabs; ++tab) {
        h = new QHBox(tabw);

        for (unsigned int col = 0; col < cols; ++col) {
            v = new QVBox(h);

            for (unsigned int row = 0; row < 128 / (tabs*cols); ++row) {
                numBox = new QHBox(v);
                QString numberText = QString("%1").arg(labelId + 1);

                if (tab == 0 && col == 0 && row == 0) {
                    // Initial label; button to adjust whether labels start at 0 or 1
                    m_initialLabel = new QPushButton(numberText, numBox);
                    connect(m_initialLabel,
                            SIGNAL(clicked()),
                            this,
                            SLOT(slotToggleInitialLabel()));
                } else {
                    QLabel *label = new QLabel(numberText, numBox);
                    label->setFixedWidth(40);
                    label->setAlignment(AlignCenter);
                    m_labels.push_back(label);
                }



                if (showEntryButtons) {
                    QPushButton *button = new QPushButton("", numBox, numberText);
                    button->setMaximumWidth(40);
                    button->setMaximumHeight(20);
                    button->setFlat(true);
                    connect(button, SIGNAL(clicked()),
                            this, SLOT(slotEntryButtonPressed()));
                    m_entryButtons.push_back(button);
                }

                KLineEdit* lineEdit = new KLineEdit(numBox, numberText);
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
        }

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
