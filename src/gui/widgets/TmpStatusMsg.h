/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
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

/// Change status bar message on construction and clear on destruction.
/**
 * A class to create a temporary message on QMainWindow's status bar
 *
 * Use as follows:
 *
 *     { // some block of code starts here
 *
 *         TmpStatusMsg tmpMsg("doing something...", mainWindow);
 *
 *         // do something
 *
 *     } // the message goes away
 *
 * ??? In most cases where this is used, it is used incorrectly.  It is being
 *     used in places where the status message will immediately be cleared.
 *     Need to audit and remove ineffective usage of this class.  It's also
 *     likely that this class will not work if we don't yield to the message
 *     loop.  Check all usage and make sure this is actually doing something.
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

