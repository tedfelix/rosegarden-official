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


#include "PitchPickerDialog.h"
#include <klocale.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLayout>
#include <QFrame>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

PitchPickerDialog::PitchPickerDialog(QDialogButtonBox::QWidget *parent, int initialPitch, QString text) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Pitch Selector"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);


    QFrame *frame = new QFrame( vBox );
    vBoxLayout->addWidget(frame);
    vBox->setLayout(vBoxLayout);

    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    m_pitch = new PitchChooser(text, frame, initialPitch);
    layout->addWidget(m_pitch, 0, 0, 0- 0+1, 2- 1, Qt::AlignHCenter);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

PitchPickerDialog::~PitchPickerDialog()
{
    // Nothing here...
}

}
#include "PitchPickerDialog.moc"
