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


#include "TimeDialog.h"

#include "base/Composition.h"
#include "gui/widgets/TimeWidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>


namespace Rosegarden
{

TimeDialog::TimeDialog(QWidget *parent, QString title,
                       Composition *composition,
                       timeT defaultTime,
                       bool constrainToCompositionDuration) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(title);
    setObjectName("MinorDialog");

    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    m_timeWidget = new TimeWidget(title, vbox, composition,
                defaultTime, true, constrainToCompositionDuration);
    vboxLayout->addWidget(m_timeWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vboxLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Without a real slot to connect to, this is unused and misleads
    // the user.
    // QPushButton *resetButton =
    // buttonBox->button(QDialogButtonBox::Reset);
    // No such slot
    // connect(resetButton, SIGNAL(clicked()),
    //         m_timeWidget, SLOT(slotResetToDefault()));
}

TimeDialog::TimeDialog(QWidget *parent, QString title,
                       Composition *composition,
                       timeT startTime,
                       timeT defaultDuration,
                       timeT minimumDuration,
                       bool constrainToCompositionDuration) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(title);
    setObjectName("MinorDialog");

    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    m_timeWidget = new TimeWidget(title, vbox, composition, startTime,
                defaultDuration, minimumDuration, true, 
                constrainToCompositionDuration);
    vboxLayout->addWidget(m_timeWidget);

    // No such slot
    // connect(this, SIGNAL(ResetClicked()),
    //         m_timeWidget, SLOT(slotResetToDefault()));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vboxLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

timeT
TimeDialog::getTime() const
{
    return m_timeWidget->getTime();
}

}
