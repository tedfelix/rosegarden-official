/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIXTOOL_H
#define RG_MATRIXTOOL_H

#include "gui/general/BaseTool.h"

#include "gui/general/ActionFileClient.h"

class QAction;

namespace Rosegarden
{

class MatrixView;
class SnapGrid;
class MatrixMouseEvent;
class MatrixWidget;
class MatrixScene;
class Event;

class MatrixTool : public BaseTool, public ActionFileClient
{
    Q_OBJECT

    friend class MatrixToolBox;

public:
    ~MatrixTool();

    //!!! todo: hoist common bits of this & NotationTool into a new
    // version of EditTool? (only if there is enough to be worth it)

    enum FollowMode {
        NoFollow = 0x0,
        FollowHorizontal = 0x1,
        FollowVertical = 0x2
    };

    virtual void handleLeftButtonPress(const MatrixMouseEvent *);
    virtual void handleMidButtonPress(const MatrixMouseEvent *);
    virtual void handleRightButtonPress(const MatrixMouseEvent *);
    virtual void handleMouseRelease(const MatrixMouseEvent *);
    virtual void handleMouseDoubleClick(const MatrixMouseEvent *);
    virtual FollowMode handleMouseMove(const MatrixMouseEvent *);

public slots:
    /**
     * Respond to an event being deleted -- it may be one that the
     * tool is remembering as the current event.
     */
    virtual void handleEventRemoved(Event *event);

protected slots:
    // For switching between tools on RMB
    //
    void slotSelectSelected();
    void slotMoveSelected();
    void slotEraseSelected();
    void slotResizeSelected();
    void slotVelocityChangeSelected();
    void slotDrawSelected();

protected:
    MatrixTool(QString rcFileName, QString menuName, MatrixWidget *);

    const SnapGrid *getSnapGrid() const;

    virtual void createMenu();
    virtual bool hasMenu() { return m_menuName != ""; }

    void setScene(MatrixScene *scene) { m_scene = scene; }

    virtual void invokeInParentView(QString actionName);
    virtual QAction *findActionInParentView(QString actionName);

    MatrixWidget *m_widget;
    MatrixScene *m_scene;
    QString m_rcFileName;
};


}

#endif
