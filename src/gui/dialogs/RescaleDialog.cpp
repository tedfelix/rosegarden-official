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

#define RG_MODULE_STRING "[RescaleDialog]"
#define RG_NO_DEBUG_PRINT

#include "RescaleDialog.h"

#include "misc/ConfigGroups.h"
#include "base/Composition.h"
#include "gui/widgets/TimeWidget2.h"
#include "misc/Strings.h"

#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>


namespace Rosegarden
{


RescaleDialog::RescaleDialog(QWidget *parent,
                             Composition *composition,
                             timeT startTime,
                             timeT originalDuration,
                             timeT minimumDuration,
                             bool showCloseGapOption,
                             bool constrainToCompositionDuration) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Stretch or Squash"));

    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);


    m_newDuration = new TimeWidget2(
            tr("Duration of selection"),  // title
            vbox,  // parent
            composition,
            startTime,
            originalDuration,  // initialDuration
            minimumDuration,
            constrainToCompositionDuration);
    connect(m_newDuration, &TimeWidget2::signalIsValid,
            this, &RescaleDialog::slotIsValid);
    vboxLayout->addWidget(m_newDuration);

    if (showCloseGapOption) {
        QGroupBox *optionBox = new QGroupBox( tr("Options"), vbox );
        QVBoxLayout *optionBoxLayout = new QVBoxLayout;
        optionBox->setLayout(optionBoxLayout);
        vboxLayout->addWidget(optionBox);
        m_closeGap = new QCheckBox(tr("Adjust times of following events accordingly"),
                                   optionBox);
        optionBoxLayout->addWidget(m_closeGap);

        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        m_closeGap->setChecked(qStrToBool(settings.value("rescaledialogadjusttimes", "true")));

        settings.endGroup();
    } else {
        m_closeGap = nullptr;
    }

    m_buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Reset | QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    const QPushButton *resetButton = m_buttonBox->button(QDialogButtonBox::Reset);
    connect(resetButton, &QAbstractButton::clicked,
            m_newDuration, &TimeWidget2::slotResetToDefault);
    vboxLayout->addWidget(m_buttonBox);

    updateGeometry();
}

timeT
RescaleDialog::getNewDuration()
{
    return m_newDuration->getTime();
}

bool
RescaleDialog::shouldCloseGap()
{
    if (m_closeGap) {
        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        settings.setValue("rescaledialogadjusttimes", m_closeGap->isChecked());
        settings.endGroup();

        return m_closeGap->isChecked();
    } else {
        return true;
    }
}

void
RescaleDialog::slotIsValid(bool valid)
{
    QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(valid);
}


}
