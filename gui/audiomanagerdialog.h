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

#include <qapplication.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qpixmap.h>

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


// Add an Id to a QListViewItem
//
class AudioListItem : public QListViewItem
{

public:

    AudioListItem(QListView * parent, QString label, unsigned int id):
        QListViewItem(parent, label, "", "", "", "", "", "", ""), m_id(id) {;}

    unsigned int getId() { return m_id; }

protected:
    unsigned int m_id;

};



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
    void slotAdd();
    void slotDeleteSelected();
    void slotPlayPreview();
    void slotRenameSelected();

    void slotEnableButtons();

signals:

    // Control signals so we can tell the sequencer about our changes
    // or actions.
    //
    void addAudioFile(unsigned int);
    void deleteAudioFile(unsigned int);
    void playAudioFile(unsigned int);

protected:
    virtual void closeEvent(QCloseEvent *e);
    void generateEnvelopePixmap(QPixmap *pixmap, AudioFile *aF);

    QListView        *m_fileList;
    QPushButton      *m_addButton;
    QPushButton      *m_deleteButton;
    QPushButton      *m_playButton;
    QPushButton      *m_renameButton;
    QPushButton      *m_insertButton;

    AudioFileManager *m_audioFileManager;

    RealTime          m_maxLength;
    QApplication     *m_app;

};

}

#endif // _AUDIOAMANAGERDIALOG_H_
