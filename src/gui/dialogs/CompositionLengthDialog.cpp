/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
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

    QGroupBox *gbox = new QGroupBox(this);
    vboxLayout->addWidget(gbox);

    QVBoxLayout *gboxLayout = new QVBoxLayout;
    gbox->setLayout(gboxLayout);

    // Since we want to stack two hbox layouts in a vbox layout (inside a group
    // box inside another vbox layout), we do have to create a QWidget here.
    // This is the widget/hbox combo for the top set of controls:
    QWidget *startBox = new QWidget(gbox);
    QHBoxLayout *startBoxLayout = new QHBoxLayout;
    startBox->setLayout(startBoxLayout);

    gboxLayout->addWidget(startBox);
    
    startBoxLayout->addWidget(new QLabel(tr("Start Bar")), Qt::AlignLeft);

    m_startMarkerSpinBox = new QSpinBox(startBox);
    m_startMarkerSpinBox->setMinimum( -10);
    m_startMarkerSpinBox->setMaximum(10000);
    m_startMarkerSpinBox->setValue(m_composition->getBarNumber(m_composition->getStartMarker()) + 1);
    startBoxLayout->addWidget(m_startMarkerSpinBox);

    // Now we need another widget/hbox for the bottom set of controls (or I
    // guess we could have laid everything out on a grid layout inside the combo
    // box, in retrospect, but whatever.  This works.  This is the bottom set of
    // controls:
    QWidget *endBox = new QWidget(gbox);
    QHBoxLayout *endBoxLayout = new QHBoxLayout;
    endBox->setLayout(endBoxLayout);

    gboxLayout->addWidget(endBox);

    endBoxLayout->addWidget(new QLabel(tr("End Bar")), Qt::AlignLeft);

    m_endMarkerSpinBox = new QSpinBox(endBox);
    m_endMarkerSpinBox->setMinimum( -10);
    m_endMarkerSpinBox->setMaximum(10000);
    m_endMarkerSpinBox->setValue(m_composition->getBarNumber(m_composition->getEndMarker()));
    endBoxLayout->addWidget(m_endMarkerSpinBox);

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

}
#include "CompositionLengthDialog.moc"
