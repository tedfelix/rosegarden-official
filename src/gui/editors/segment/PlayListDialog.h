
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PLAYLISTDIALOG_H
#define RG_PLAYLISTDIALOG_H

#include <QDialog>
#include <QString>


class QWidget;
class QCloseEvent;


namespace Rosegarden
{

class PlayList;


class PlayListDialog : public QDialog
{
    Q_OBJECT

public:
    PlayListDialog( QString caption, QWidget* parent = 0, const char* name = 0);

    PlayList* getPlayList() { return m_playList; }

public slots:
    void slotClose();

signals:
    void closing();

protected:    
    virtual void closeEvent(QCloseEvent *e);

    void save();
    void restore();

    //--------------- Data members ---------------------------------
    PlayList* m_playList;
};


}

#endif
