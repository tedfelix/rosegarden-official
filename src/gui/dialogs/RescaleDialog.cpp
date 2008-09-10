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


#include "RescaleDialog.h"

#include <klocale.h>
#include "document/ConfigGroups.h"
#include "base/Composition.h"
#include "gui/widgets/TimeWidget.h"
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QApplication>


namespace Rosegarden
{

RescaleDialog::RescaleDialog(QDialogButtonBox::QWidget *parent,
                             Composition *composition,
                             timeT startTime,
                             timeT originalDuration,
                             bool showCloseGapOption,
                             bool constrainToCompositionDuration) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Rescale"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    m_newDuration = new TimeWidget(i18n("Duration of selection"), vbox ,
                     constrainToCompositionDuration);
    vboxLayout->addWidget(m_newDuration);

    if (showCloseGapOption) {
        QGroupBox *optionBox = new QGroupBox( i18n("Options"), vbox );
        vboxLayout->addWidget(optionBox);
        vbox->setLayout(vboxLayout);
        m_closeGap = new QCheckBox(i18n("Adjust times of following events accordingly"),
                                   optionBox);
        QSettings config;
        config.beginGroup( GeneralOptionsConfigGroup );
        // 
        // FIX-manually-(GW), add:
        // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
        //  

        m_closeGap->setChecked
        ( qStrToBool( config.value("rescaledialogadjusttimes", "true" ) ) );
    } else {
        m_closeGap = 0;
    }

    setButtonText(User1, i18n("Reset"));
    connect(this, SIGNAL(user1Clicked()),
            m_newDuration, SLOT(slotResetToDefault()));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::User1 | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
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
        QSettings config;
        config.beginGroup( GeneralOptionsConfigGroup );
        // 
        // FIX-manually-(GW), add:
        // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
        //  

        config.setValue("rescaledialogadjusttimes", m_closeGap->isChecked());
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
