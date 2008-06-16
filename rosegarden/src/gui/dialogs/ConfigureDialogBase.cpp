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


#include "ConfigureDialogBase.h"

#include <klocale.h>
#include "gui/configuration/ConfigurationPage.h"
#include <kdialogbase.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

ConfigureDialogBase::ConfigureDialogBase(QWidget *parent,
        QString label,
        const char *name):
        KDialogBase(IconList, label ? label : i18n("Configure"), Help | Apply | Ok | Cancel,
                    Ok, parent, name, true) // modal
{
    setWFlags(WDestructiveClose);
}

ConfigureDialogBase::~ConfigureDialogBase()
{}

void
ConfigureDialogBase::slotApply()
{
    for (configurationpages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i)
        (*i)->apply();
}

void
ConfigureDialogBase::slotActivateApply()
{
    //     ApplyButton->setDisabled(false);
}

void
ConfigureDialogBase::slotOk()
{
    slotApply();
    accept();
}

void
ConfigureDialogBase::slotCancelOrClose()
{}

}
#include "ConfigureDialogBase.moc"
