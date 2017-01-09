/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioPlayingDialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>


namespace Rosegarden
{

AudioPlayingDialog::AudioPlayingDialog(QWidget *parent,
                                       const QString &name):
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Playing audio file"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *w = new QWidget(this);
    QHBoxLayout *wLayout = new QHBoxLayout;
    metagrid->addWidget(w, 0, 0);

    QLabel *label = new QLabel(tr("Playing audio file \"%1\"").arg(name), w );
    wLayout->addWidget(label);
    w->setLayout(wLayout);

    label->setMinimumHeight(80);


    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

}
