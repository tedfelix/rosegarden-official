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


#include "CompositionLengthDialog.h"

#include <klocale.h>
#include "base/Composition.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

CompositionLengthDialog::CompositionLengthDialog(
    QDialogButtonBox::QWidget *parent,
    Composition *composition):
        QDialog(parent),
        m_composition(composition)
{
    setModal(true);
    setWindowTitle(i18n("Change Composition Length"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
	QWidget *startBox = new QWidget(this);
	QVBoxLayout *startBoxLayout = new QVBoxLayout;
	metagrid->addWidget(startBox, 0, 0);


    new QLabel(i18n("Set the Start and End bar markers for this Composition"), startBox );

    QLabel *child_7 = new QLabel(i18n("Start Bar"), startBox );
    startBoxLayout->addWidget(child_7);
    m_startMarkerSpinBox = new QSpinBox( startBox );
    startBoxLayout->addWidget(m_startMarkerSpinBox);
    startBox->setLayout(startBoxLayout);
    m_startMarkerSpinBox->setMinimum( -10);
    m_startMarkerSpinBox->setMaximum(10000);
    m_startMarkerSpinBox->setValue(
        m_composition->getBarNumber(m_composition->getStartMarker()) + 1);

	QWidget *endBox = new QWidget( startBox );
	startBoxLayout->addWidget(endBox);
	startBox->setLayout(startBoxLayout);
    QHBoxLayout *endBoxLayout = new QHBoxLayout;
    QLabel *child_4 = new QLabel(i18n("End Bar"), endBox );
    endBoxLayout->addWidget(child_4);
    m_endMarkerSpinBox = new QSpinBox( endBox );
    endBoxLayout->addWidget(m_endMarkerSpinBox);
    endBox->setLayout(endBoxLayout);
    m_endMarkerSpinBox->setMinimum( -10);
    m_endMarkerSpinBox->setMaximum(10000);
    m_endMarkerSpinBox->setValue(
        m_composition->getBarNumber(m_composition->getEndMarker()));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

timeT
CompositionLengthDialog::getStartMarker()
{
    return m_composition->getBarStart(m_startMarkerSpinBox->value() - 1);
}

timeT
CompositionLengthDialog::getEndMarker()
{
    return m_composition->getBarStart(m_endMarkerSpinBox->value());
}

}
#include "CompositionLengthDialog.moc"
