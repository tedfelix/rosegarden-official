/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef TMPSTATUSMSG_H
#define TMPSTATUSMSG_H

#include <QUrl>
#include <QLabel>

class QMainWindow;
class QWidget;
class QString;
//class QLabel;


/**
 * A class to create a temporary message on QMainWindow's status bar
 *
 * Use as follows :
 * { // some block of code starts here
 *  TmpStatusMsg tmpMsg("doing something...", rosegardenApplication);
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
     * The id of the text widget in the status bar can be specified
     */
    TmpStatusMsg(const QString& msg, QMainWindow*, int id = m_defaultId);

    ~TmpStatusMsg();

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

    //--------------- Data members ---------------------------------

    QMainWindow* m_mainWindow;
    int m_id;

    static int m_defaultId;
    static QString m_defaultMsg;
};

#endif

