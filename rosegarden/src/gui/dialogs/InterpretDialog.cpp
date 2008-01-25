/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
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
#include <kdialogbase.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

InterpretDialog::InterpretDialog(QWidget *parent) :
        KDialogBase(parent, 0, true, i18n("Interpret"), Ok | Cancel | Help)
{
    setHelp("nv-interpret");

    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *groupBox = new QGroupBox
                          (1, Horizontal, i18n("Interpretations to apply"), vbox);

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

    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    m_allInterpretations->setChecked
    (config->readBoolEntry("interpretall", true));
    m_applyTextDynamics->setChecked
    (config->readBoolEntry("interprettextdynamics", true));
    m_applyHairpins->setChecked
    (config->readBoolEntry("interprethairpins", true));
    m_stressBeats->setChecked
    (config->readBoolEntry("interpretstressbeats", true));
    m_articulate->setChecked
    (config->readBoolEntry("interpretarticulate", true));

    connect(m_allInterpretations,
            SIGNAL(clicked()), this, SLOT(slotAllBoxChanged()));

    slotAllBoxChanged();
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
    KConfig *config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

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
