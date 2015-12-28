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


#include "CommentsPopupDialog.h"

#include "document/RosegardenDocument.h"
#include "document/MetadataHelper.h"

#include <QVBoxLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QPlainTextEdit>

namespace Rosegarden
{

CommentsPopupDialog::CommentsPopupDialog(RosegardenDocument *doc,
                                         QWidget *parent):
    QDialog(parent, 0),
    m_doc(doc)
{
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose);

    MetadataHelper mh(doc);

    // Only create the dialog if asked for
    if (!mh.popupWanted()) return;

    QString fileName = doc->getAbsFilePath();
    setWindowTitle(tr("Notes about %1").arg(fileName));
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    QPlainTextEdit *textEdit = new QPlainTextEdit(this);
    mainLayout->addWidget(textEdit);
    QString localStyle("QPlainTextEdit {background-color: #D0D0D0; color: #000000;}");
    textEdit->setMinimumSize(600, 500);  // About the size of the edit widget
    textEdit->setPlainText(mh.getComments());
    textEdit->setReadOnly(true);
    textEdit->setBackgroundVisible(true);
    textEdit->setStyleSheet(localStyle);
    textEdit->setToolTip(tr("<qt>This is a short description of the current "
                            "composition</qt>"));

    QWidget *hb = new QWidget;
    mainLayout->addWidget(hb);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    hb->setLayout(bottomLayout);

    QCheckBox *checkBox = new QCheckBox;
    bottomLayout->addWidget(checkBox);
    checkBox->setText(tr("Show next time"));
    checkBox->setToolTip(tr("<qt>If checked, these notes will pop up the next time the document is loaded</qt>"));
    checkBox->setChecked(true);
    connect(checkBox, SIGNAL(stateChanged(int)),
            this, SLOT(slotCheckChanged(int)));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    bottomLayout->addWidget(buttonBox);

    QPushButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    connect(parent, SIGNAL(documentAboutToChange()), this, SLOT(close()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    show();
}

void
CommentsPopupDialog::slotCheckChanged(int state)
{
    MetadataHelper mh(m_doc);
    mh.setPopupWanted(state != Qt::Unchecked);
}

}

