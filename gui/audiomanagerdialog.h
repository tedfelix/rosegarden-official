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

#include <kdialogbase.h>
#include <qlistbox.h>
#include <qpushbutton.h>

#include <string>

#ifndef _AUDIOMANAGERDIALOG_H_
#define _AUDIOMANAGERDIALOG_H_

namespace Rosegarden
{


class AudioManagerDialog : public KDialogBase
{
    Q_OBJECT

public:
    AudioManagerDialog(QWidget *parent);


protected:
    virtual void closeEvent(QCloseEvent *e);

    QListBox    *m_fileList;
    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QPushButton *m_playButton;

};

}

#endif // _AUDIOAMANAGERDIALOG_H_
