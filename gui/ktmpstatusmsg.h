// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#ifndef KTMPSTATUSMSG_H
#define KTMPSTATUSMSG_H

class KStatusBar;

/**
 * A class to create a temporary message on the status bar
 *
 * Use as follows :
 * { // some block of code starts here
 *  KTmpStatusMsg tmpMsg("doing something...", statusBar);
 *
 *  // do something
 *
 * } // the message goes away
 *
 */
class KTmpStatusMsg
{
public:
    /**
     * Creates a new temporary status message on the specified
     * status bar.
     * The id of the text widget in the status bar can be specified
     */
    KTmpStatusMsg(const QString& msg, KStatusBar*, int id = m_defaultId);
    ~KTmpStatusMsg();

    /**
     * Sets the message which will replace the temporary one in the
     * status bar
     */
    static void setDefaultMsg(const QString&);

    /**
     * Returns the default message which will replace the temporary
     * one in the status bar
     */
    static const QString& getDefaultMsg();

    /**
     * Sets the default id which will be used as the id of the text
     * widget in the status bar
     */
    static void setDefaultId(int);

    /**
     * Returns the default id which will be used as id of the text
     * widget in the status bar
     */
    static int getDefaultId();
    
protected:
    KStatusBar* m_statusBar;
    int m_id;

    static int m_defaultId;
    static QString m_defaultMsg;
};

#endif

