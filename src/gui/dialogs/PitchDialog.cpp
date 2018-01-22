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


#include "PitchDialog.h"

#include "gui/widgets/PitchChooser.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>


namespace Rosegarden
{

PitchDialog::PitchDialog(QWidget *parent, QString title, int defaultPitch) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(title);

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    m_pitchChooser = new PitchChooser(title, vbox , defaultPitch);
    vboxLayout->addWidget(m_pitchChooser);
    vbox->setLayout(vboxLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    QPushButton *user1
        = buttonBox->addButton(tr("Reset"), QDialogButtonBox::ActionRole);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(user1, SIGNAL(clicked(bool)),
            m_pitchChooser, SLOT(slotResetToDefault()));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

int
PitchDialog::getPitch() const
{
    return m_pitchChooser->getPitch();
}

}
