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


#include "BeatsBarsDialog.h"

#include "base/Segment.h"

#include <QLayout>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QSpinBox>
#include <QWidget>


namespace Rosegarden
{

BeatsBarsDialog::BeatsBarsDialog(QWidget* parent) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Audio Segment Duration"));
    setObjectName("MinorDialog");
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);

    QGroupBox *gbox = new QGroupBox(tr("The selected audio segment contains:"));
    gbox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(5);

    metagrid->addWidget(gbox, 0, 0);

    m_spinBox = new QSpinBox;
    m_spinBox->setMinimum(1);
    m_spinBox->setMaximum(INT_MAX);
    m_spinBox->setSingleStep(1);
    layout->addWidget(m_spinBox, 0, 0);

    m_comboBox = new QComboBox;
    m_comboBox->setEditable(false);
    m_comboBox->addItem(tr("beat(s)"));
    m_comboBox->addItem(tr("bar(s)"));
    m_comboBox->setCurrentIndex(0);
    layout->addWidget(m_comboBox, 0, 1);

    gbox->setLayout(layout);

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

}
