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

#include "AudioFileManager.h"

// This dialog presents and allows editing of the Audio files that
// are in the Composition.  We edit the AudioFileManager directly
// and in this dialog we show previews and other pertinent file
// information.  This dialog also allows the user to preview the
// sounds and (eventually) to perform some operations of the audio
// files (split silence etc).
//
//

#ifndef _AUDIOMANAGERDIALOG_H_
#define _AUDIOMANAGERDIALOG_H_

namespace Rosegarden
{

class AudioFileManager;

class AudioManagerDialog : public KDialogBase
{
    Q_OBJECT

public:
    AudioManagerDialog(QWidget *parent,
                       AudioFileManager *aFM);

    // Populate the file list from the AudioFileManager
    //
    void populateFileList();

    // Return a pointer to the currently selected AudioFile -
    // returns 0 if nothing is selected
    //
    AudioFile* getCurrentSelection();

public slots:
    void slotDeleteSelected();
    void slotPlayPreview();
    void slotAdd();

protected:
    virtual void closeEvent(QCloseEvent *e);

    QListBox         *m_fileList;
    QPushButton      *m_addButton;
    QPushButton      *m_deleteButton;
    QPushButton      *m_playButton;

    AudioFileManager *m_audioFileManager;

};

}

#endif // _AUDIOAMANAGERDIALOG_H_
