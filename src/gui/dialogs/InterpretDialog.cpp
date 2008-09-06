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


#include "InterpretDialog.h"
#include <kapplication.h>

#include <klocale.h>
#include "document/ConfigGroups.h"
#include "commands/notation/InterpretCommand.h"
#include <kconfig.h>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

InterpretDialog::InterpretDialog(QDialogButtonBox::QWidget *parent) :
        QDialog(parent)
{
    setHelp("nv-interpret");

    setModal(true);
    setWindowTitle(i18n("Interpret"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    QGroupBox *groupBox = new QGroupBox( i18n("Interpretations to apply"), vbox );
    vboxLayout->addWidget(groupBox);
    vbox->setLayout(vboxLayout);

    m_applyTextDynamics = new QCheckBox
                          (i18n("Apply text dynamics (p, mf, ff etc)"), groupBox);
    m_applyHairpins = new QCheckBox
                      (i18n("Apply hairpin dynamics"), groupBox);
    m_stressBeats = new QCheckBox
                    (i18n("Stress beats"), groupBox);
    m_articulate = new QCheckBox
                   (i18n("Articulate slurs, staccato, tenuto etc"), groupBox);
    m_allInterpretations = new QCheckBox
                           (i18n("All available interpretations"), groupBox);

    QSettings config ; // was: kapp->config()
    QSettings config;
    config.beginGroup( NotationViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( NotationViewConfigGroup );
    //  


    m_allInterpretations->setChecked
    ( qStrToBool( config.value("interpretall", "true" ) ) );
    m_applyTextDynamics->setChecked
    ( qStrToBool( config.value("interprettextdynamics", "true" ) ) );
    m_applyHairpins->setChecked
    ( qStrToBool( config.value("interprethairpins", "true" ) ) );
    m_stressBeats->setChecked
    ( qStrToBool( config.value("interpretstressbeats", "true" ) ) );
    m_articulate->setChecked
    ( qStrToBool( config.value("interpretarticulate", "true" ) ) );

    connect(m_allInterpretations,
            SIGNAL(clicked()), this, SLOT(slotAllBoxChanged()));

    slotAllBoxChanged();
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
InterpretDialog::slotAllBoxChanged()
{
    bool all = m_allInterpretations->isChecked();
    m_applyTextDynamics->setEnabled(!all);
    m_applyHairpins->setEnabled(!all);
    m_stressBeats->setEnabled(!all);
    m_articulate->setEnabled(!all);
}

int
InterpretDialog::getInterpretations()
{
    QSettings config ; // was: kapp->config()
    QSettings config;
    config.beginGroup( NotationViewConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( NotationViewConfigGroup );
    //  


    config->writeEntry("interpretall", m_allInterpretations->isChecked());
    config->writeEntry("interprettextdynamics", m_applyTextDynamics->isChecked());
    config->writeEntry("interprethairpins", m_applyHairpins->isChecked());
    config->writeEntry("interpretstressbeats", m_stressBeats->isChecked());
    config->writeEntry("interpretarticulate", m_articulate->isChecked());

    if (m_allInterpretations->isChecked()) {
        return InterpretCommand::AllInterpretations;
    } else {
        int in = 0;
        if (m_applyTextDynamics->isChecked())
            in |= InterpretCommand::ApplyTextDynamics;
        if (m_applyHairpins->isChecked())
            in |= InterpretCommand::ApplyHairpins;
        if (m_stressBeats->isChecked())
            in |= InterpretCommand::StressBeats;
        if (m_articulate->isChecked()) {
            in |= InterpretCommand::Articulate;
        }
        return in;
    }
}

}
#include "InterpretDialog.moc"
