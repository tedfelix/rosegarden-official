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

#include "TrivialVelocityDialog.h"

#include <QSpinBox>
#include <QLabel>
#include <QWidget>
#include <QHBoxLayout>
#include <QDialogButtonBox>

namespace Rosegarden {
 
TrivialVelocityDialog::TrivialVelocityDialog(QWidget *parent, QString label, int deft) :
        QDialog(parent)
    {
        setModal(true);
    setWindowTitle(label);

    //QGridLayout *
	m_metagrid = new QGridLayout;
    setLayout(m_metagrid);
    QWidget *hbox = new QWidget(this);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    m_metagrid->addWidget(hbox, 0, 0);

        QLabel *child_3 = new QLabel(label, hbox );
        hboxLayout->addWidget(child_3);
        m_spin = new QSpinBox( hbox );
        hboxLayout->addWidget(m_spin);
        hbox->setLayout(hboxLayout);
        m_spin->setValue(deft);
    } 

int
TrivialVelocityDialog::getVelocity()
{
    return m_spin->value();
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_metagrid->addWidget(buttonBox, 1, 0);
    m_metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}
       
}
