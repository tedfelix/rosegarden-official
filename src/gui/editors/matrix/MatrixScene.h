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

#ifndef RG_MATRIXSCENE_H
#define RG_MATRIXSCENE_H

#include <QGraphicsScene>

#include "base/Composition.h"
#include "gui/general/SelectionManager.h"

class QGraphicsLineItem;

namespace Rosegarden
{

class MatrixWidget;
class Segment;
class RosegardenDocument;
class Event;
class EventSelection;
class MatrixElement;
class MatrixMouseEvent;
class MatrixViewSegment;
class ViewSegment;
class RulerScale;
class ZoomableRulerScale;
class SnapGrid;

/**
 * Specialised graphics scene for matrix elements.  The note blocks and
 * horizontal and vertical grid lines are all represented by graphics items
 * owned by this scene.  This scene also owns the MatrixViewSegment classes
 * which track segment contents in view objects.
 *
 * The scene works with MatrixViewSegment, MatrixViewElement, MatrixPainter,
 * and MatrixMover to support the new "concert pitch matrix" concept.  All
 * pitches on the grid as well as the key signature pitch highlights are
 * represented in concert pitch, presenting the pitches as they would look if
 * they were struck as notes on a conventional piano.  If the user draws a Bb in
 * a segment in -9 (Eb) transposition, the actual event is created with a pitch
 * +9 higher, and the resulting MatrixElement representing that pitch is then
 * drawn -9, so that it remains at the correct height.  If this note were viewed
 * in notation, it would appear to be a G.
 *
 * The scene can display events from any number of segments spanning any number
 * of tracks, and the concert pitch matrix is necessary in order to avoid the
 * chaos that would ensue when working with segments in different transpositions
 * on the same grid.  Note, however, the pitch highlights may come out
 * incorrectly if the key signatures in various segments do not resolve to the
 * same key after factoring the various segment transpositions into account.
 * This kind of situation would be flagged as an error in the notation view, via
 * the track/staff header warning icon, and it is not a problem that can be
 * resolved.  In this case, the user must intervene to ensure sanity of the
 * results.
 */
class MatrixScene : public QGraphicsScene,
                    public CompositionObserver,
                    public SelectionManager
{
    Q_OBJECT

public:
    MatrixScene();
    ~MatrixScene();

    void setMatrixWidget(MatrixWidget *w);
    void setSegments(RosegardenDocument *doc, std::vector<Segment *> segments);

    void handleEventAdded(Event *);
    void handleEventRemoved(Event *);

    RosegardenDocument *getDocument() { return m_document; }

    virtual EventSelection *getSelection() const { return m_selection; }
    virtual void setSelection(EventSelection* s, bool preview);
    void selectAll();
    int calculatePitchFromY(int y) const;

    void setSingleSelectedEvent(MatrixViewSegment *viewSegment,
                                MatrixElement *e,
                                bool preview);

    void setSingleSelectedEvent(Segment *segment,
                                Event *e,
                                bool preview);

    Segment *getCurrentSegment();
    void setCurrentSegment(Segment *);

    Segment *getPriorSegment();
    Segment *getNextSegment();

    MatrixViewSegment *getCurrentViewSegment();

    bool segmentsContainNotes() const;

    int getYResolution() const { return m_resolution; }

    const RulerScale *getRulerScale() const { return m_scale; }
    ZoomableRulerScale *getReferenceScale() { return m_referenceScale; }
    const SnapGrid *getSnapGrid() const { return m_snapGrid; }

    void setSnap(timeT);

    bool constrainToSegmentArea(QPointF &scenePos);

    void playNote(Segment &segment, int pitch, int velocity = -1);

    // SegmentObserver method forwarded from MatrixViewSegment
    void segmentEndMarkerTimeChanged(const Segment *s, bool shorten);

signals:
    void mousePressed(const MatrixMouseEvent *e);
    void mouseMoved(const MatrixMouseEvent *e);
    void mouseReleased(const MatrixMouseEvent *e);
    void mouseDoubleClicked(const MatrixMouseEvent *e);

    void eventRemoved(Event *e);

    void currentViewSegmentChanged(ViewSegment *);
    //void selectionChanged(); // already defined in QGraphicsScene
    void selectionChanged(EventSelection *s);
    void segmentDeleted(Segment *);
    void sceneDeleted(); // all segments have been removed

public slots:
    void slotRulerSelectionChanged(EventSelection *s);

protected slots:
    void slotCommandExecuted();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);

    void segmentRemoved(const Composition *, Segment *); // CompositionObserver
    void timeSignatureChanged(const Composition *); // CompositionObserver

private:
    MatrixWidget *m_widget; // I do not own this

    RosegardenDocument *m_document; // I do not own this

    std::vector<Segment *> m_segments; // I do not own these
    std::vector<MatrixViewSegment *> m_viewSegments; // I own these

    RulerScale *m_scale; // I own this (it maps between time and scene x)
    ZoomableRulerScale *m_referenceScale; // I own this (it refers to m_scale
                                  // and knows zoom level needed by the loop
                                  // rulers (which refer to m_snapGrid))
    SnapGrid *m_snapGrid; // I own this

    int m_resolution;
    EventSelection *m_selection; // I own this

    int m_currentSegmentIndex;

    // These are the background items -- the grid lines and the shadings
    // used to highlight the first, third and fifth in the current key
    std::vector<QGraphicsLineItem *> m_horizontals;
    std::vector<QGraphicsLineItem *> m_verticals;
    std::vector<QGraphicsRectItem *> m_highlights;

    void setupMouseEvent(QGraphicsSceneMouseEvent *, MatrixMouseEvent &) const;
    void recreateLines();
    void recreatePitchHighlights();
    void updateCurrentSegment();
    void setSelectionElementStatus(EventSelection *, bool set);
    void previewSelection(EventSelection *, EventSelection *oldSelection);
    void checkUpdate();
};

}

#endif
