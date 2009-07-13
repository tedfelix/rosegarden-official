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
#include "base/NotationTypes.h"
#include "gui/general/SelectionManager.h"

#include <QWidget>
#include <vector>

class QGridLayout;

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
class StandardRuler;
class TempoRuler;
class ChordNameRuler;
class RawNoteRuler;

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

    timeT getInsertionTime() const;

    bool isInChordMode() { return false; }
    bool isInTripletMode() { return false; }
    bool isInGraceMode() { return false; }

    bool getPlayTracking() const { return m_playTracking; }

    NotationToolBox *getToolBox() { return m_toolBox; }
    NotationTool *getCurrentTool() const;

    void setCanvasCursor(QCursor cursor);

    Segment *getCurrentSegment();

    void setTempoRulerVisible(bool visible);
    void setChordNameRulerVisible(bool visible);
    void setRawNoteRulerVisible(bool visible);

public slots:
    void slotSetTool(QString name);
    void slotSetSelectTool();
    void slotSetEraseTool();
    void slotSetNoteInserter();
    void slotSetRestInserter();
    void slotSetInsertedNote(Note::Type type, int dots);
    void slotSetAccidental(Accidental accidental, bool follow);
    void slotSetClefInserter();
    void slotSetInsertedClef(Clef type);
    void slotSetTextInserter();
    void slotSetGuitarChordInserter();
    void slotSetLinearMode();
    void slotSetContinuousPageMode();
    void slotSetMultiPageMode();
    void slotSetFontName(QString);
    void slotSetFontSize(int);
    void slotSetPlayTracking(bool);
    void slotTogglePlayTracking();

protected:
    virtual void showEvent(QShowEvent * event);

protected slots:
    void slotDispatchMousePress(const NotationMouseEvent *);
    void slotDispatchMouseRelease(const NotationMouseEvent *);
    void slotDispatchMouseMove(const NotationMouseEvent *);
    void slotDispatchMouseDoubleClick(const NotationMouseEvent *);

    void slotPointerPositionChanged(timeT);
    void slotEnsureLastMouseMoveVisible();

    void slotZoomInFromPanner();
    void slotZoomOutFromPanner();

    void slotHScroll();
    void slotHScrollBarRangeChanged(int min, int max);

private:
    RosegardenDocument *m_document; // I do not own this
    Panned *m_view; // I own this
    Panner *m_hpanner; // I own this
    NotationScene *m_scene; // I own this
    bool m_playTracking;
    double m_hZoomFactor;
    double m_vZoomFactor;
    ZoomableRulerScale *m_referenceScale; // I own this (refers to scene scale)
    NotationToolBox *m_toolBox;
    NotationTool *m_currentTool;
    bool m_inMove;
    QPointF m_lastMouseMoveScenePos;

    StandardRuler *m_topStandardRuler; // I own this
    StandardRuler *m_bottomStandardRuler; // I own this
    TempoRuler *m_tempoRuler; // I own this
    ChordNameRuler *m_chordNameRuler; // I own this
    RawNoteRuler *m_rawNoteRuler; // I own this

    QGridLayout *m_layout; // I own this

    /**
     * Widgets vertical positions inside the main QGridLayout
     */
    enum {
        CHORDNAMERULER_ROW,
        TEMPORULER_ROW,
        RAWNOTERULER_ROW,
        TOPRULER_ROW,
        PANNED_ROW,
        BOTTOMRULER_ROW,
        HSLIDER_ROW,
        PANNER_ROW
    };

    /**
     * Widgets horizontal positions inside the main QGridLayout
     */
    enum {
        HEADER_COL,
        MAIN_COL
    };

};

}

#endif

    
