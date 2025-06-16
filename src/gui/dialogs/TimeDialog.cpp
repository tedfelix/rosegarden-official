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


#include "TimeDialog.h"

#include "base/Composition.h"
#include "gui/widgets/TimeWidget.h"
#include "gui/widgets/TimeWidget2.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>

// Define for TimeWidget2 testing.
//#define TIMEWIDGET2


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

#ifdef TIMEWIDGET2
    m_timeWidget2 = new TimeWidget2(
            title,
            vbox,  // parent
            composition,
            defaultTime,  // initialTime
            constrainToCompositionDuration);
    vboxLayout->addWidget(m_timeWidget2);
#else
    m_timeWidget = new TimeWidget(
            title,
            vbox,  // parent
            composition,
            defaultTime,  // initialTime
            constrainToCompositionDuration);
    vboxLayout->addWidget(m_timeWidget);
#endif

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Reset | QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel);
    vboxLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Without a real slot to connect to, this is unused and misleads
    // the user.
    // QPushButton *resetButton =
    // buttonBox->button(QDialogButtonBox::Reset);
    // No such slot
    // connect(resetButton, &QPushButton::clicked,
    //         m_timeWidget, &TimeWidget::slotResetToDefault);
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

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);

#ifdef TIMEWIDGET2
    m_timeWidget2 = new TimeWidget2(
            title,
            this,  // parent
            composition,
            startTime,
            defaultDuration,  // initialDuration
            minimumDuration,
            constrainToCompositionDuration);
    vboxLayout->addWidget(m_timeWidget2);
#else
    m_timeWidget = new TimeWidget(
            title,
            this,  // parent
            composition,
            startTime,
            defaultDuration,  // initialDuration
            minimumDuration,
            constrainToCompositionDuration);
    vboxLayout->addWidget(m_timeWidget);
#endif

    // No such slot
    //connect(this, &TimeDialog::ResetClicked,
    //        m_timeWidget, &TimeWidget::slotResetToDefault);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Reset | QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel);
    vboxLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

timeT
TimeDialog::getTime() const
{
    if (m_timeWidget)
        return m_timeWidget->getTime();

    if (m_timeWidget2)
        return m_timeWidget2->getTime();

    return 0;
}


}
