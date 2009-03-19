/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MATRIX_WIDGET_H_
#define _RG_MATRIX_WIDGET_H_

#include <QWidget>

#include "base/Event.h" // for timeT
#include "gui/general/SelectionManager.h"

#include <vector>


namespace Rosegarden
{

class RosegardenDocument;
class Segment;
class MatrixScene;
class MatrixToolBox;
class MatrixTool;
class MatrixMouseEvent;
class RulerScale;
class SnapGrid;
class ZoomableRulerScale;
class Panner;
class Panned;
class EventSelection;

class MatrixWidget : public QWidget,
                     public SelectionManager
{
    Q_OBJECT

public:
    MatrixWidget(bool drumMode);
    virtual ~MatrixWidget();

    void setSegments(RosegardenDocument *document, 
                     std::vector<Segment *> segments);

    MatrixScene *getScene() { return m_scene; }

    void setZoomFactor(double factor);
    double getZoomFactor() const;

    int getCurrentVelocity() const { return m_currentVelocity; }

    bool isDrumMode() const { return m_drumMode; }

    MatrixToolBox *getToolBox() { return m_toolBox; }

    void setCanvasCursor(QCursor cursor);

    // These delegate to MatrixScene, which possesses the selection
    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection *s, bool preview);

    // This delegates to MatrixScene
    const SnapGrid *getSnapGrid() const;

    Segment *getCurrentSegment();
    bool segmentsContainNotes() const;

signals:
    void editTriggerSegment(int);

public slots:
    void slotSelectAll();
    void slotClearSelection();
    
    void slotSetTool(QString name);
    void slotSetPaintTool();
    void slotSetEraseTool();
    void slotSetSelectTool();
    void slotSetMoveTool();
    void slotSetResizeTool();
    void slotSetVelocityTool();

    void slotSetCurrentVelocity(int velocity) { m_currentVelocity = velocity; }
    void slotSetSnap(timeT);

protected slots:
    void slotDispatchMousePress(const MatrixMouseEvent *);
    void slotDispatchMouseRelease(const MatrixMouseEvent *);
    void slotDispatchMouseMove(const MatrixMouseEvent *);
    void slotDispatchMouseDoubleClick(const MatrixMouseEvent *);

    void slotEnsureLastMouseMoveVisible();

private:
    RosegardenDocument *m_document; // I do not own this
    Panned *m_view; // I own this
    Panner *m_hpanner; // I own this
    MatrixScene *m_scene; // I own this
    MatrixToolBox *m_toolBox; // I own this
    MatrixTool *m_currentTool; // Toolbox owns this
    bool m_drumMode;
    double m_zoomFactor;
    int m_currentVelocity;
    ZoomableRulerScale *m_referenceScale; // I own this (refers to scene scale)
    bool m_inMove;
    QPointF m_lastMouseMoveScenePos;
};

}

#endif
