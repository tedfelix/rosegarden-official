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


#include "PlayListDialog.h"

#include "document/ConfigGroups.h"
#include "PlayList.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QDialog>

namespace Rosegarden
{

PlayListDialog::PlayListDialog( QString caption,
                               QWidget* parent, const char* name)
	: KDialogBase(parent, name, false, caption,
	  KDialogBase::Close,  // standard buttons
	  KDialogBase::Close,  // default button
   true),
   /*
	: KDialogBase(parent, name, false, caption,
	  KDialogBase::Close,  // standard buttons
	  KDialogBase::Close,  // default button
   true),
   */
   m_playList(new PlayList(this))
{
//     setWFlags(WDestructiveClose);
	this->setAttribute( Qt::WA_DeleteOnClose );
	
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
	close();
//     KDialogBase::closeEvent(e);
}

void PlayListDialog::slotClose()
{
    save();
    emit closing();
	close();
//     KDialogBase::slotClose();
}

}
#include "PlayListDialog.moc"
