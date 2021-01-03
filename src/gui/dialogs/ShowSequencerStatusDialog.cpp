/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ShowSequencerStatusDialog.h"

#include "sound/Audit.h"
#include "misc/Strings.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QByteArray>
#include <QDataStream>
#include <QLabel>
#include <QString>
#include <QTextEdit>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

ShowSequencerStatusDialog::ShowSequencerStatusDialog(QWidget *parent) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Sequencer status"));

    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    int row = 0;

    layout->addWidget(new QLabel(tr("Sequencer status:")), row, 0);

    QString status = strtoqstr(AUDIT.str());

    QTextEdit *text = new QTextEdit;
    text->setReadOnly(true);
    text->setMinimumWidth(500);
    text->setMinimumHeight(200);
    text->setPlainText(status);

    layout->addWidget(text, ++row, 0);
    layout->setRowStretch(row, 20);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    layout->addWidget(buttonBox, ++row, 0);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

}
