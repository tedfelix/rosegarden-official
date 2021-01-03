/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTATIONTOOL_H
#define RG_NOTATIONTOOL_H

#include <QObject>
#include <QString>

#include "gui/general/BaseTool.h"
#include "gui/general/ActionFileClient.h"
#include "gui/general/AutoScroller.h"  // For FollowMode

class QMenu;
class QAction;

namespace Rosegarden
{

class NotationWidget;
class NotationMouseEvent;
class NotationScene;

/**
 * Notation tool base class.
 *
 * A NotationTool represents one of the items on the notation toolbars
 * (notes, rests, clefs, eraser, etc...). It handle mouse click events
 * for the NotationView ('State' design pattern).
 *
 * A NotationTool can have a menu, normally activated through a right
 * mouse button click. This menu is defined in an XML file, see
 * NoteRestInserter and noterestinserter.rc for an example.
 *
 * This class is a "semi-singleton", that is, only one instance per
 * NotationView window is created. This is because menu creation is
 * slow, and the fact that a tool can trigger the setting of another
 * tool through a menu choice). This is maintained with the
 * NotationToolBox class. This means we can't rely on the ctor/dtor to
 * perform setting up, like mouse cursor changes for instance. Use the
 * ready() and stow() method for this.
 *
 * @see NotationView#setTool()
 * @see NotationToolBox
 */
class NotationTool : public BaseTool, public ActionFileClient
{
    Q_OBJECT

    friend class NotationToolBox;

public:
    ~NotationTool() override;

    /**
     * Is called by the view when the tool is set as current.
     * Add any setup here
     */
    void ready() override;

    /**
     * Is called by the view when the tool is put away.
     * Add any cleanup here
     */
    void stow() override;

    virtual void handleLeftButtonPress(const NotationMouseEvent *);
    virtual void handleMidButtonPress(const NotationMouseEvent *);
    virtual void handleRightButtonPress(const NotationMouseEvent *);
    virtual void handleMouseRelease(const NotationMouseEvent *);
    virtual void handleMouseDoubleClick(const NotationMouseEvent *);
    virtual FollowMode handleMouseMove(const NotationMouseEvent *);
    virtual void handleWheelTurned(int, const NotationMouseEvent *);


    virtual const QString getToolName() = 0;
    virtual bool needsWheelEvents() = 0;


protected:
    /**
     * Create a new NotationTool
     *
     * \a rcFileName : the name of the XML rc file
     * \a menuName : the name of the menu defined in the rc file
     */
    NotationTool(QString rcFileName, QString menuName, NotationWidget *);

    /**
     * Create a new NotationTool without a menu
     */
    NotationTool(NotationWidget *);

    void createMenu() override;
    bool hasMenu() override { return m_menuName != ""; }

    void setScene(NotationScene *scene) { m_scene = scene; }

    virtual void invokeInParentView(QString actionName);
    virtual QAction *findActionInParentView(QString actionName);

    //--------------- Data members ---------------------------------

    NotationWidget *m_widget;
    NotationScene *m_scene;
    QString m_rcFileName;
};


}

#endif
