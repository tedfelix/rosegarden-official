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


#include "TimeDialog.h"

#include <klocale.h>
#include "base/Composition.h"
#include "gui/widgets/TimeWidget.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

TimeDialog::TimeDialog(QDialogButtonBox::QWidget *parent, QString title,
                       Composition *composition,
                       timeT defaultTime,
                       bool constrainToCompositionDuration) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(title);

    QGridLayout *metagrid = new QGridLayout;
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(metagrid);
    metagrid->addWidget(vbox, 0, 0);

    m_timeWidget = new TimeWidget(title, vbox, composition,
                defaultTime, true, constrainToCompositionDuration);
    vboxLayout->addWidget(m_timeWidget);
    vboxLayout->addWidget(m_timeWidget);

    connect(this, SIGNAL(ResetClicked()),
            m_timeWidget, SLOT(slotResetToDefault()));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

TimeDialog::TimeDialog(QWidget *parent, QString title,
                       Composition *composition,
                       timeT startTime,
                       timeT defaultTime,
                       bool constrainToCompositionDuration) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(title);

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    m_timeWidget = new TimeWidget(title, vbox, composition, startTime,
                defaultTime, true, constrainToCompositionDuration);
    vboxLayout->addWidget(m_timeWidget);
    vbox->setLayout(vboxLayout);
    vboxLayout->addWidget(m_timeWidget);
    vbox->setLayout(vboxLayout);

    connect(this, SIGNAL(ResetClicked()),
            m_timeWidget, SLOT(slotResetToDefault()));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

timeT
TimeDialog::getTime() const
{
    return m_timeWidget->getTime();
}

}
#include "TimeDialog.moc"
