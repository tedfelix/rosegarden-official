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


#include "CurrentProgressDialog.h"

#include "ProgressDialog.h"
#include <QObject>


namespace Rosegarden
{

CurrentProgressDialog* CurrentProgressDialog::getInstance()
{
    if (!m_instance)
        m_instance = new CurrentProgressDialog(0);

    return m_instance;
}


ProgressDialog*
CurrentProgressDialog::get()
{
    return m_currentProgressDialog;
}

void
CurrentProgressDialog::set(ProgressDialog* d)
{
    if (m_currentProgressDialog)
        m_currentProgressDialog->disconnect(getInstance());

    m_currentProgressDialog = d;
    
    if (d) {
        // this lets the progress dialog deregister itself when it's deleted
        // 
        // SIGNAL(destroyed()) : emitted immediately before the object obj is destroyed
        connect(d, SIGNAL(destroyed()),
                getInstance(), SLOT(slotCurrentProgressDialogDestroyed()));
    }
}

void CurrentProgressDialog::freeze()
{
    if (m_currentProgressDialog) m_currentProgressDialog->slotFreeze();
}

void CurrentProgressDialog::thaw()
{
    if (m_currentProgressDialog) m_currentProgressDialog->slotThaw();
}

void CurrentProgressDialog::slotCurrentProgressDialogDestroyed()
{
    // (is static)
    m_currentProgressDialog = 0;
}

CurrentProgressDialog* CurrentProgressDialog::m_instance = 0;
ProgressDialog* CurrentProgressDialog::m_currentProgressDialog = 0;

}
#include "CurrentProgressDialog.moc"
