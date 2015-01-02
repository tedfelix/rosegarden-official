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


#include "RescaleDialog.h"

#include "misc/ConfigGroups.h"
#include "base/Composition.h"
#include "gui/widgets/TimeWidget.h"
#include "misc/Strings.h"

#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QApplication>
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


    m_newDuration = new TimeWidget(tr("Duration of selection"), vbox,
                     composition, startTime, originalDuration, 
                     minimumDuration, true,
                     constrainToCompositionDuration);
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
        m_closeGap = 0;
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    vboxLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QPushButton *resetButton = buttonBox->button(QDialogButtonBox::Reset);
    connect(resetButton, SIGNAL(clicked()),
            m_newDuration, SLOT(slotResetToDefault()));

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

/*
int
RescaleDialog::getMultiplier()
{
    return m_to;
}

int
RescaleDialog::getDivisor()
{
    return m_from;
}

void
RescaleDialog::slotFromChanged(int i)
{
    m_from = i + 1;
    int perTenThou = m_to * 10000 / m_from;
    m_percent->setText(QString("%1.%2%").
                       arg(perTenThou / 100).
                       arg(perTenThou % 100));
}

void
RescaleDialog::slotToChanged(int i)
{
    m_to = i + 1;
    int perTenThou = m_to * 10000 / m_from;
    m_percent->setText(QString("%1.%2%").
                       arg(perTenThou / 100).
                       arg(perTenThou % 100));
}
*/

}
#include "RescaleDialog.moc"
