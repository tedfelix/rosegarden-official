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


#include "PitchPickerDialog.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QLayout>
#include <QFrame>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

// We keep the parent parameter here, because parents control the location at
// which a dialog pops up, and they influence style inheritance.
PitchPickerDialog::PitchPickerDialog(QWidget *parent, int initialPitch, QString text) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Pitch Selector"));

    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    setLayout(vBoxLayout);

    QFrame *frame = new QFrame;
    vBoxLayout->addWidget(frame);

    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *frameLayout = new QGridLayout;
    frameLayout->setSpacing(5);
    frame->setLayout(frameLayout);

    m_pitch = new PitchChooser(text, frame, initialPitch);         // internal class still needs a parent
    frameLayout->addWidget(m_pitch, 0, 0, 1, 3, Qt::AlignHCenter); // simplified conversion legacy 0-0+1 == 1; 2-0+1 == 3

    // Since we're stacking this in a VBox, we can just stack the button box on
    // the bottom layer of the main layout, without any top level grid.
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vBoxLayout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

PitchPickerDialog::~PitchPickerDialog()
{
    // Nothing here...
}

}
