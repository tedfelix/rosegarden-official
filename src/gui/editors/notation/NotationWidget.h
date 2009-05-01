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

#ifndef _RG_NOTATION_WIDGET_H_
#define _RG_NOTATION_WIDGET_H_

#include "StaffLayout.h"
#include "gui/general/SelectionManager.h"

#include <QWidget>
#include <vector>

namespace Rosegarden
{

class RosegardenDocument;
class Segment;
class NotationScene;
class Note;
class NotationToolBox;
class NotationTool;
class NotationMouseEvent;
class Panner;
class Panned;
class ZoomableRulerScale;

class NotationWidget : public QWidget,
                       public SelectionManager
{
    Q_OBJECT

public:
    NotationWidget();
    virtual ~NotationWidget();

    void setSegments(RosegardenDocument *document, 
                     std::vector<Segment *> segments);

    NotationScene *getScene() { return m_scene; }

    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection* s, bool preview);

    //!!! to keep current NoteInserter implementation happy:
    void setSingleSelectedEvent(int staffNo,
                                Event *event,
                                bool preview = false,
                                bool redrawNow = false) { }

    void setSingleSelectedEvent(Segment &segment,
                                Event *event,
                                bool preview = false,
                                bool redrawNow = false) { }

    bool isInChordMode() { return false; }
    bool isInTripletMode() { return false; }
    bool isInGraceMode() { return false; }

    NotationToolBox *getToolBox() { return m_toolBox; }

    void setCanvasCursor(QCursor cursor);

    Segment *getCurrentSegment();

    //!!! to keep current staff implementation happy:
//    bool isInPrintMode() const { return false; }
//    NotationHLayout *getHLayout() { return m_hlayout; }
//    NotationVLayout *getVLayout() { return m_vlayout; }
//    NotationProperties &getProperties() { return *m_properties; }
//    RosegardenDocument *getDocument() { return m_document; }
//    EventSelection *getCurrentSelection() { return 0; }
//    void handleEventRemoved(Event *) { }
//    bool areAnnotationsVisible() { return true; }
//    bool areLilyPondDirectivesVisible() { return true; }

protected slots:
    void slotDispatchMousePress(const NotationMouseEvent *);
    void slotDispatchMouseRelease(const NotationMouseEvent *);
    void slotDispatchMouseMove(const NotationMouseEvent *);
    void slotDispatchMouseDoubleClick(const NotationMouseEvent *);

    void slotZoomInFromPanner();
    void slotZoomOutFromPanner();

private:
    RosegardenDocument *m_document; // I do not own this
    Panned *m_view; // I own this
    Panner *m_hpanner; // I own this
    NotationScene *m_scene; // I own this
    double m_hZoomFactor;
    double m_vZoomFactor;
    ZoomableRulerScale *m_referenceScale; // I own this (refers to scene scale)
    NotationToolBox *m_toolBox;
    NotationTool *m_currentTool;
};

}

#endif

    
