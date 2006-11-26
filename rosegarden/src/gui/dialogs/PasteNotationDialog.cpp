/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "PasteNotationDialog.h"

#include <klocale.h>
#include "commands/edit/PasteEventsCommand.h"
#include <kdialogbase.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qobject.h>
#include <qradiobutton.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

PasteNotationDialog::PasteNotationDialog(QWidget *parent,
        PasteEventsCommand::PasteType defaultType) :
        KDialogBase(parent, 0, true, i18n("Paste"), Ok | Cancel | Help ),
        m_defaultType(defaultType)
{
    setHelp("nv-paste-types");

    QVBox *vbox = makeVBoxMainWidget();

    QButtonGroup *pasteTypeGroup = new QButtonGroup
                                   (1, Horizontal, i18n("Paste type"), vbox);

    PasteEventsCommand::PasteTypeMap pasteTypes =
        PasteEventsCommand::getPasteTypes();

    for (PasteEventsCommand::PasteTypeMap::iterator i = pasteTypes.begin();
            i != pasteTypes.end(); ++i) {

        QRadioButton *button = new QRadioButton(i->second, pasteTypeGroup);
        button->setChecked(m_defaultType == i->first);
        QObject::connect(button, SIGNAL(clicked()),
                         this, SLOT(slotPasteTypeChanged()));

        m_pasteTypeButtons.push_back(button);
    }

    QButtonGroup *setAsDefaultGroup = new QButtonGroup
                                      (1, Horizontal, i18n("Options"), vbox);

    m_setAsDefaultButton = new QCheckBox
                           (i18n("Make this the default paste type"), setAsDefaultGroup);
    m_setAsDefaultButton->setChecked(true);
}

PasteEventsCommand::PasteType
PasteNotationDialog::getPasteType() const
{
    for (unsigned int i = 0; i < m_pasteTypeButtons.size(); ++i) {
        if (m_pasteTypeButtons[i]->isChecked()) {
            return (PasteEventsCommand::PasteType)i;
        }
    }

    return PasteEventsCommand::Restricted;
}

bool
PasteNotationDialog::setAsDefault() const
{
    return m_setAsDefaultButton->isChecked();
}

void
PasteNotationDialog::slotPasteTypeChanged()
{
    m_setAsDefaultButton->setChecked(m_defaultType == getPasteType());
}

}
#include "PasteNotationDialog.moc"
