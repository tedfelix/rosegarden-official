/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMMENTSPOPUPDIALOG_H
#define RG_COMMENTSPOPUPDIALOG_H

#include <QDialog>


namespace Rosegarden
{

class RosegardenDocument;

/**
 * This class create a pop up dialog which displays notes about the current
 * composition.
 * 
 * This dialog is only opened if the appropriate box has been checked in the
 * notes page of the composition properties dialog.
 */

class CommentsPopupDialog : public QDialog
{
    Q_OBJECT

public:
    CommentsPopupDialog(RosegardenDocument *doc, QWidget *parent = 0);
    
public slots:
    void slotCheckChanged(int state);

protected:
    RosegardenDocument *m_doc;
};

}

#endif



