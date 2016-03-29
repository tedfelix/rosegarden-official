/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef TMPSTATUSMSG_H
#define TMPSTATUSMSG_H

class QMainWindow;
class QString;

/**
 * A class to create a temporary message on QMainWindow's status bar
 *
 * Use as follows :
 * { // some block of code starts here
 *  TmpStatusMsg tmpMsg("doing something...", mainWindow);
 *
 *  // do something
 *
 * } // the message goes away
 *
 */
class TmpStatusMsg
{
public:

    /**
     * Creates a new temporary status message on the status bar
     * of the specified KMainWindow.
     */
    TmpStatusMsg(const QString& msg, QMainWindow* window);

    ~TmpStatusMsg();

private:

    //--------------- Data members ---------------------------------

    QMainWindow* m_mainWindow;
};

#endif

