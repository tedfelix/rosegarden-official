// -*- c-basic-offset: 4 -*-

/*
  Rosegarden-4 v0.2
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

class EditTool;
class EditView;
class QPopupMenu;

namespace Rosegarden { class ViewElement; }

/**
 * NotationToolBox : maintains a single instance of each registered tool
 *
 * Tools are fetched from a name
 */
class EditToolBox : public QObject
{
public:
    EditToolBox(EditView* parent);

    virtual EditTool* getTool(const QString& toolName);

protected:
    virtual EditTool* createTool(const QString& toolName) = 0;

    //--------------- Data members ---------------------------------

    EditView* m_parentView;

    QDict<EditTool> m_tools;
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
class EditTool : public QObject, public KXMLGUIClient
{
    friend class EditToolBox;

public:
    virtual ~EditTool();

    /**
     * Is called by EditView when the tool is set as current
     * Add any setup here
     */
    virtual void ready();

    /**
     * Is called by EditView after the tool is not used
     * Add any cleanup here
     */
    virtual void stow();

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
    virtual void handleMouseDblClick(Rosegarden::timeT time,
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
    virtual bool handleMouseMove(Rosegarden::timeT time,
                                 int height,
                                 QMouseEvent*);

    /**
     * Do nothing
     */
    virtual void handleMouseRelease(Rosegarden::timeT time,
                                    int height,
                                    QMouseEvent*);

    /**
     * Show the menu if there is one
     */
    virtual void showMenu();

    void setParentView(EditView*);

protected:
    /**
     * Create a new EditTool
     *
     * \a menuName : the name of the menu defined in the XML rc file
     */
    EditTool(const QString& menuName, EditView*);

    void createMenu(const QString& rcFileName);

    //--------------- Data members ---------------------------------

    const QString m_menuName;

    EditView* m_parentView;

    QPopupMenu* m_menu;
};


#endif
