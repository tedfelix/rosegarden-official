
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_EDITTOOL_H_
#define _RG_EDITTOOL_H_

#include "BaseTool.h"
#include <kxmlguiclient.h>
#include <QString>
#include "base/Event.h"


class QMouseEvent;


namespace Rosegarden
{

class ViewElement;
class Event;
class EditView;


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
    virtual void handleMousePress(timeT time,
                                  int height,                                  
                                  int staffNo,
                                  QMouseEvent *event,
                                  ViewElement*);

    /**
     * Main operation of the tool
     */
    virtual void handleLeftButtonPress(timeT time,
                                       int height,
                                       int staffNo,
                                       QMouseEvent *event,
                                       ViewElement*) = 0;

    /**
     * Do nothing
     */
    virtual void handleMidButtonPress(timeT time,
                                      int height,
                                      int staffNo,
                                      QMouseEvent*,
                                      ViewElement*);

    /**
     * Show option menu
     */
    virtual void handleRightButtonPress(timeT time,
                                        int height,
                                        int staffNo,
                                        QMouseEvent*,
                                        ViewElement*);

    /**
     * Do nothing
     */
    virtual void handleMouseDoubleClick(timeT time,
                                        int height,
                                        int staffNo,
                                        QMouseEvent*,
                                        ViewElement*);

    /**
     * Do nothing.
     * Implementations of handleMouseMove should return true if
     * they want the canvas to scroll to the position the mouse
     * moved to following the method's return.
     */
    virtual int handleMouseMove(timeT time,
                                int height,
                                QMouseEvent*);

    /**
     * Do nothing
     */
    virtual void handleMouseRelease(timeT time,
                                    int height,
                                    QMouseEvent*);

    /**
     * Respond to an event being deleted -- it may be the one the tool
     * is remembering as the current event.
     */
    virtual void handleEventRemoved(Event *) { }
    

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
    virtual bool hasMenu();

    //--------------- Data members ---------------------------------
    QString m_rcFileName;

    EditView* m_parentView;
};


}

#endif
