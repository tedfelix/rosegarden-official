/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <klocale.h>
#include <qhbox.h>
#include <qvbuttongroup.h>

#include "audiomanagerdialog.h"

namespace Rosegarden
{

AudioManagerDialog::AudioManagerDialog(QWidget *parent):
    KDialogBase(parent, 0, true, i18n("Audio Manager Dialog"), Ok | Cancel)
{
    QHBox *h = makeHBoxMainWidget();
    QVButtonGroup *v = new QVButtonGroup(h);

    m_addButton    = new QPushButton(i18n("Add"), v);
    m_deleteButton = new QPushButton(i18n("Delete"), v);
    m_playButton   = new QPushButton(i18n("Play"), v);

    m_fileList     = new QListBox(h);
}

void
AudioManagerDialog::closeEvent(QCloseEvent *e)
{
    e->accept();
}

}

