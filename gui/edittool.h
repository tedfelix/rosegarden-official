// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4
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

#ifndef EDITIONTOOL_H
#define EDITIONTOOL_H

#include <qevent.h>
#include <qobject.h>
#include <qdict.h>

#include <kxmlguiclient.h>

#include "Event.h"

class BaseTool;
class EditView;
class QPopupMenu;

namespace Rosegarden { class ViewElement; }

/**
 * BaseToolBox : maintains a single instance of each registered tool
 *
 * Tools are fetched from a name
 */
class BaseToolBox : public QObject
{
public:
    BaseToolBox(QWidget* parent);

    virtual BaseTool* getTool(const QString& toolName);

protected:
    virtual BaseTool* createTool(const QString& toolName) = 0;

    QDict<BaseTool> m_tools;
};

/**
 * BaseTool : base tool class, just handles RMB menu creation and
 * handling by a BaseToolBox
 * 
 */
class BaseTool : public QObject
{
    friend class BaseToolBox;

public:

    /**
     * handleMouseMove() will return a OR-ed combination of these
     */
    enum {
        NoFollow = 0x0,
        FollowHorizontal = 0x1,
        FollowVertical = 0x2
    };

    virtual ~BaseTool();

    /**
     * Is called by the parent View (EditView or SegmentCanvas) when
     * the tool is set as current.
     * Add any setup here
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

protected:
    /**
     * Create a new BaseTool
     *
     * \a menuName : the name of the menu defined in the XML rc file
     */
    BaseTool(const QString& menuName, KXMLGUIFactory*, QObject* parent);

    virtual void createMenu() = 0;

    //--------------- Data members ---------------------------------

    QString m_menuName;
    QPopupMenu* m_menu;

    KXMLGUIFactory* m_parentFactory;
};


/**
 * Edit tool base class.
 *
 * A EditTool represents one of the items on an edition view
 * toolbar. It handles mouse click events for the EditView ('State'
 * design pattern).
 *
 * A EditTool can have a menu, normally activated through a right
 * mouse button click. This menu is defined in an XML file, see
 * NoteInserter and noteinserter.rc for an example.
 *
 * This class is a "semi-singleton", that is, only one instance per
 * EditView window is created. This is because menu creation is
 * slow, and the fact that a tool can trigger the setting of another
 * tool through a menu choice). This is maintained with the
 * EditToolBox class This means we can't rely on the ctor/dtor to
 * perform setting up, like mouse cursor changes for instance. Use the
 * ready() and stow() method for this.
 *
 * @see EditView#setTool()
 * @see EditToolBox
 */
class EditTool : public BaseTool, public KXMLGUIClient
{
    friend class EditToolBox;

public:

    /**
     * Dispatch the event to Left/Middle/Right MousePress
     */
    virtual void handleMousePress(Rosegarden::timeT time,
                                  int height,                                  
                                  int staffNo,
                                  QMouseEvent *event,
                                  Rosegarden::ViewElement*);

    /**
     * Main operation of the tool
     */
    virtual void handleLeftButtonPress(Rosegarden::timeT time,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       Rosegarden::ViewElement*) = 0;

    /**
     * Do nothing
     */
    virtual void handleMidButtonPress(Rosegarden::timeT time,
                                      int height,
                                      int staffNo,
                                      QMouseEvent*,
                                      Rosegarden::ViewElement*);

    /**
     * Show option menu
     */
    virtual void handleRightButtonPress(Rosegarden::timeT time,
                                        int height,
                                        int staffNo,
                                        QMouseEvent*,
                                        Rosegarden::ViewElement*);

    /**
     * Do nothing
     */
    virtual void handleMouseDoubleClick(Rosegarden::timeT time,
					int height,
					int staffNo,
					QMouseEvent*,
					Rosegarden::ViewElement*);

    /**
     * Do nothing.
     * Implementations of handleMouseMove should return true if
     * they want the canvas to scroll to the position the mouse
     * moved to following the method's return.
     */
    virtual int handleMouseMove(Rosegarden::timeT time,
                                int height,
                                QMouseEvent*);

    /**
     * Do nothing
     */
    virtual void handleMouseRelease(Rosegarden::timeT time,
                                    int height,
                                    QMouseEvent*);

protected:
    /**
     * Create a new EditTool
     *
     * \a menuName : the name of the menu defined in the XML rc file
     */
    EditTool(const QString& menuName, EditView*);

    void setRCFileName(QString rcfilename) { m_rcFileName = rcfilename; }

    virtual void createMenu();
    virtual void createMenu(QString rcFileName);

    //--------------- Data members ---------------------------------
    QString m_rcFileName;

    EditView* m_parentView;
};

/**
 * EditToolBox : specialized toolbox for EditViews (notation, matrix...)
 *
 */
class EditToolBox : public BaseToolBox
{
public:
    EditToolBox(EditView* parent);

    virtual EditTool* getTool(const QString& toolName);

protected:
    virtual EditTool* createTool(const QString& toolName);

    //--------------- Data members ---------------------------------

    EditView* m_parentView;
};

#endif
