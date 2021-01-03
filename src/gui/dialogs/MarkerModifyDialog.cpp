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


#include "MarkerModifyDialog.h"

#include "base/Composition.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/TimeWidget.h"
#include "misc/Strings.h"
#include "gui/widgets/LineEdit.h"

#include <QLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

MarkerModifyDialog::MarkerModifyDialog(QWidget *parent,
                                       Composition *composition,
                                       int time,
                                       const QString &name,
                                       const QString &des):
    QDialog(parent)
{
    initialise(composition, time, name, des);
}

MarkerModifyDialog::MarkerModifyDialog(QWidget *parent,
                                       Composition *composition,
                                       Marker *marker) :
    QDialog(parent)
{
    initialise(composition, marker->getTime(),
               strtoqstr(marker->getName()),
               strtoqstr(marker->getDescription()));
}

void
MarkerModifyDialog::initialise(Composition *composition,
                               int time,
                               const QString &name,
                               const QString &des)
{

    m_originalTime = time;

    setModal(true);
    setWindowTitle(tr("Edit Marker"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    m_timeEdit = new TimeWidget(tr("Marker Time"), vbox, composition,
                                time);
    vboxLayout->addWidget(m_timeEdit);

    /*!!!
     
        layout->addWidget(new QLabel(tr("Absolute Time:"), frame), 0, 0);
        m_timeEdit = new QSpinBox(frame);
        layout->addWidget(m_timeEdit, 0, 1);
     
        m_timeEdit->setMinimum(INT_MIN);
        m_timeEdit->setMaximum(INT_MAX);
        m_timeEdit->setSingleStep(
                Note(Note::Shortest).getDuration());
        m_timeEdit->setValue(time);
    */

    QGroupBox *groupBox = new QGroupBox(tr("Marker Properties"));
    QHBoxLayout *groupBoxLayout = new QHBoxLayout;
    vboxLayout->addWidget(groupBox);

    QFrame *frame = new QFrame(groupBox);
    frame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);
    groupBoxLayout->addWidget(frame);

    layout->addWidget(new QLabel(tr("Text:"), frame), 0, 0);
    m_nameEdit = new LineEdit(name, frame);
    layout->addWidget(m_nameEdit, 0, 1);

    layout->addWidget(new QLabel(tr("Comment:"), frame), 1, 0);
    m_desEdit = new LineEdit(des, frame);
    layout->addWidget(m_desEdit, 1, 1);

    m_nameEdit->selectAll();
    m_nameEdit->setFocus();

    frame->setLayout(layout);
    groupBox->setLayout(groupBoxLayout);
    vbox->setLayout(vboxLayout);

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

}
