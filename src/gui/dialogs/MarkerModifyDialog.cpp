/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MarkerModifyDialog]"
#define RG_NO_DEBUG_PRINT

#include "MarkerModifyDialog.h"

#include "misc/Strings.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{


MarkerModifyDialog::MarkerModifyDialog(QWidget *parent,
                                       int time,
                                       const QString &text,
                                       const QString &comment):
    QDialog(parent)
{
    initialise(time, text, comment);
}

MarkerModifyDialog::MarkerModifyDialog(QWidget *parent,
                                       Marker *marker) :
    QDialog(parent)
{
    initialise(marker->getTime(),
               strtoqstr(marker->getName()),
               strtoqstr(marker->getDescription()));
}

void
MarkerModifyDialog::initialise(int time,
                               const QString &text,
                               const QString &comment)
{

    m_originalTime = time;

    setModal(true);
    setWindowTitle(tr("Edit Marker"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    // Marker Time
    m_timeWidget = new TimeWidget2(tr("Marker Time"),  // title
                                   vbox,  // parent
                                   time,  // initialTime
                                   true);  // constrainToCompositionDuration
    connect(m_timeWidget, &TimeWidget2::signalIsValid,
            this, &MarkerModifyDialog::slotIsValid);
    vboxLayout->addWidget(m_timeWidget);

    // Marker Properties
    QGroupBox *groupBox = new QGroupBox(tr("Marker Properties"));
    QHBoxLayout *groupBoxLayout = new QHBoxLayout;
    vboxLayout->addWidget(groupBox);

    QFrame *frame = new QFrame(groupBox);
    frame->setContentsMargins(5, 5, 5, 5);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);
    groupBoxLayout->addWidget(frame);

    // Text
    layout->addWidget(new QLabel(tr("Text:"), frame), 0, 0);
    m_text = new LineEdit(text, frame);
    layout->addWidget(m_text, 0, 1);

    // Comment
    layout->addWidget(new QLabel(tr("Comment:"), frame), 1, 0);
    m_comment = new LineEdit(comment, frame);
    layout->addWidget(m_comment, 1, 1);

    m_text->selectAll();
    m_text->setFocus();

    frame->setLayout(layout);
    groupBox->setLayout(groupBoxLayout);
    vbox->setLayout(vboxLayout);

    m_buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    metagrid->addWidget(m_buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
}

void
MarkerModifyDialog::slotIsValid(bool valid)
{
    QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(valid);
}


}
