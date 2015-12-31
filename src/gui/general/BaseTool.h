
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

#ifndef RG_BASETOOL_H
#define RG_BASETOOL_H

#include <QObject>
#include <QString>


class QMenu;


namespace Rosegarden
{

/**
 * BaseTool : base tool class, just handles RMB menu creation and
 * handling by a BaseToolBox
 * 
 */
class BaseTool : public QObject
{
    Q_OBJECT
    
    friend class BaseToolBox;

public:

    virtual ~BaseTool();

    /**
     * Is called by the parent View (EditView or SegmentCanvas) when
     * the tool is set as current.
     * Add any setup here (e.g. setting the mouse cursor shape)
     */
    virtual void ready();

    /**
     * Is called by the parent View (EditView or SegmentCanvas) after
     * the tool is put away.
     * Add any cleanup here
     */
    virtual void stow();

    /**
     * Show the menu if there is one
     */
    virtual void showMenu();

    /**
     * Retrieve current status-line type help for the tool, if any
     */
    virtual QString getCurrentContextHelp() const;

signals:
    void showContextHelp(QString);

protected:
    /**
     * Create a new BaseTool
     *
     * \a menuName : the name of the menu defined in the XML rc file
     */
    BaseTool(const QString &menuName, QObject *parent);

    virtual void createMenu() = 0;
    virtual bool hasMenu() { return false; }

    virtual void setContextHelp(const QString &help);
    virtual void clearContextHelp() { setContextHelp(""); }

    //--------------- Data members ---------------------------------

    QString m_menuName;
    QMenu *m_menu;

    QString m_contextHelp;
};



}

#endif
