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


#include "CompositionLengthDialog.h"

#include "base/Composition.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>


namespace Rosegarden
{

CompositionLengthDialog::CompositionLengthDialog(
    QWidget *parent,
    Composition *composition):
        QDialog(parent),
        m_composition(composition)
{
    setModal(true);
    setWindowTitle(tr("Change Composition Length"));

    //###
    // This note applies in many other places, but since this is the nth time
    // I've gotten rid of the QWidget + metagrid + QWidget + layout paradigm, I
    // thought I'd explain.  We have a QWidget coming in here, the QDialog.  We
    // can set its layout.  If we create something like a QFrame or a QGroupBox,
    // we have a widget there, too, and can set that widget's layout.  It won't
    // be that often we really have to make a random QWidget just to hold a
    // layout, and also there's no real need to have an overall grid layout just
    // to contain sublayouts.  We used to lay out most simple dialogs on an HBox
    // or VBox, and we can still do that directly, without the extra
    // complication.
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    vboxLayout->addWidget(new QLabel(tr("Change the start and end markers for the composition:")));

    // GROUP BOX BEGIN

    QGroupBox *gbox = new QGroupBox(this);
    vboxLayout->addWidget(gbox);

    QGridLayout *gboxLayout = new QGridLayout;
    gboxLayout->setVerticalSpacing(10);
    gbox->setLayout(gboxLayout);

    // ROW 0, Start Bar

    gboxLayout->addWidget(new QLabel(tr("Start Bar")), 0, 0);

    m_startMarkerSpinBox = new QSpinBox(gbox);
    m_startMarkerSpinBox->setMinimum( -10);
    m_startMarkerSpinBox->setMaximum(10000);
    m_startMarkerSpinBox->setValue(m_composition->getBarNumber(m_composition->getStartMarker()) + 1);
    gboxLayout->addWidget(m_startMarkerSpinBox, 0, 1);

    // ROW 1, End Bar

    gboxLayout->addWidget(new QLabel(tr("End Bar")), 1, 0);

    m_endMarkerSpinBox = new QSpinBox(gbox);
    m_endMarkerSpinBox->setMinimum( -10);
    m_endMarkerSpinBox->setMaximum(10000);
    m_endMarkerSpinBox->setValue(m_composition->getBarNumber(m_composition->getEndMarker()));
    gboxLayout->addWidget(m_endMarkerSpinBox, 1, 1);

    // ROW 2, Auto-Expand when Editing

    gboxLayout->addWidget(new QLabel(tr("Auto-Expand when Editing")), 2, 0);

    m_autoExpandCheckBox = new QCheckBox(gbox);
    m_autoExpandCheckBox->setChecked(m_composition->autoExpandEnabled());
    gboxLayout->addWidget(m_autoExpandCheckBox, 2, 1);
    
    // GROUP BOX END

    // Now the button box on the bottom, outside the group box and any of the
    // previous layouts, just gets added to the vbox under the QDialog widget
    // we're coming in with.  No overall grid layout required.
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vboxLayout->addWidget(buttonBox);

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

bool
CompositionLengthDialog::autoExpandEnabled()
{
    return m_autoExpandCheckBox->isChecked();
}

}
#include "CompositionLengthDialog.moc"
