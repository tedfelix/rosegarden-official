/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "PlayListDialog.h"

#include "document/ConfigGroups.h"
#include "PlayList.h"
#include <kdialogbase.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

PlayListDialog::PlayListDialog(QString caption,
                               QWidget* parent, const char* name)
        : KDialogBase(parent, name, false, caption,
                      KDialogBase::Close,  // standard buttons
                      KDialogBase::Close,  // default button
                      true),
        m_playList(new PlayList(this))
{
    setWFlags(WDestructiveClose);
    setMainWidget(m_playList);
    restore();
}

void PlayListDialog::save()
{
    saveDialogSize(PlayListConfigGroup);
}

void PlayListDialog::restore()
{
    setInitialSize(configDialogSize(PlayListConfigGroup));
}

void PlayListDialog::closeEvent(QCloseEvent *e)
{
    save();
    emit closing();
    KDialogBase::closeEvent(e);
}

void PlayListDialog::slotClose()
{
    save();
    emit closing();
    KDialogBase::slotClose();
}

}
#include "PlayListDialog.moc"
