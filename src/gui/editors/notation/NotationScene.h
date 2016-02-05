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

#ifndef RG_NOTATION_SCENE_H
#define RG_NOTATION_SCENE_H

#include <QGraphicsScene>

#include "base/NotationTypes.h"
#include "base/Composition.h"
#include "gui/general/SelectionManager.h"
#include "gui/general/GUIPalette.h"
#include "StaffLayout.h"
#include "NotePixmapFactory.h"
#include "ClefKeyContext.h"

class QGraphicsItem;
class QGraphicsTextItem;

namespace Rosegarden
{

class NotationStaff;
class NotationHLayout;
class NotationVLayout;
class NotePixmapFactory;
class NotationProperties;
class NotationMouseEvent;
class EventSelection;
class Event;
class ClefKeyContext;
class NotationElement;
class NotationWidget;
class RosegardenDocument;
class Segment;
class ViewSegment;
class RulerScale;

typedef std::map<int, int> TrackIntMap;

class NotationScene : public QGraphicsScene,
                      public CompositionObserver,
                      public SelectionManager
{
    Q_OBJECT

public:
    NotationScene();
    ~NotationScene();

    void setNotationWidget(NotationWidget *w);
    void setStaffs(RosegardenDocument *document, std::vector<Segment *> segments);

    void createClonesFromRepeatedSegments();

    std::vector<NotationStaff *> *getStaffs() { return &m_staffs; }

    /** Returns the total number of staffs irrespective of whether they are
      * visible individually or not.  This may return a number higher than the
      * apparent number of staffs, due to segment overlaps.
      *
      * If there are five total staffs spread across two apparent staffs, this
      * returns 5.
      */
    unsigned int getStaffCount() { return m_staffs.size(); }

    /** Returns the number of staffs that are visible individually as discrete
     * staffs, irrespective of how many real staffs might be represented by each
     * apparent staff.
     *
     * If there are five total staffs spread across two apparent staffs, this
     * returns 2.
     */
    unsigned int getVisibleStaffCount() { return m_visibleStaffs; }

    int getCurrentStaffNumber() { return m_currentStaff; }
    NotationStaff *getCurrentStaff();
    void setCurrentStaff(NotationStaff *);

    NotationStaff *getStaffAbove(timeT t);
    NotationStaff *getStaffBelow(timeT t);
    NotationStaff *getPriorStaffOnTrack();
    NotationStaff *getNextStaffOnTrack();

    NotationStaff *getStaffForSceneCoords(double x, int y) const;

    Segment *getCurrentSegment();

    bool segmentsContainNotes() const;

    //!!! to keep current staff implementation happy:
    bool isInPrintMode() const { return false; }
    NotationHLayout *getHLayout() { return m_hlayout; }
    NotationVLayout *getVLayout() { return m_vlayout; }
    NotationProperties &getProperties() { return *m_properties; }
    RosegardenDocument *getDocument() { return m_document; }
    NotePixmapFactory *getNotePixmapFactory() { return m_notePixmapFactory; }

    virtual EventSelection *getSelection() const { return m_selection; }
    virtual void setSelection(EventSelection* s, bool preview);

    timeT getInsertionTime() const;

    struct CursorCoordinates {
        QLineF currentStaff;
        QLineF allStaffs;
    };

    CursorCoordinates getCursorCoordinates(timeT t) const;
    timeT snapTimeToNoteBoundary(timeT t) const;

    void setSingleSelectedEvent(NotationStaff *staff,
                                NotationElement *e,
                                bool preview);

    void setSingleSelectedEvent(Segment *segment,
                                Event *e,
                                bool preview);

    StaffLayout::PageMode getPageMode() const { return m_pageMode; }
    void setPageMode(StaffLayout::PageMode mode);

    QString getFontName() const;
    void setFontName(QString);

    int getFontSize() const;
    void setFontSize(int);

    int getHSpacing() const;
    void setHSpacing(int);

    int getLeftGutter() const;
    void setLeftGutter(int);

    const RulerScale *getRulerScale() const;

    void suspendLayoutUpdates();
    void resumeLayoutUpdates();

    /**
     * Show and sound the given note.  The height is used for display,
     * the pitch for performance, so the two need not correspond (e.g.
     * under ottava there may be octave differences).
     */
    void showPreviewNote(NotationStaff *staff, double layoutX,
                         int pitch, int height,
                         const Note &note,
                         bool grace,
                         QColor color = GUIPalette::SelectionColor,
                         int velocity = -1,
                         bool play = true
                        );

    /// Remove any visible preview note
    void clearPreviewNote(NotationStaff *);

    void playNote(Segment &segment, int pitch, int velocity = -1);

    bool constrainToSegmentArea(QPointF &scenePos);

    // more dubious:
    void handleEventRemoved(Event *);
    bool areAnnotationsVisible() { return true; }
    bool areLilyPondDirectivesVisible() { return true; }

    int getMinTrack() { return m_minTrack; }
    int getMaxTrack() { return m_maxTrack; }
    TrackIntMap *getTrackHeights() { return &m_trackHeights; }
    TrackIntMap *getTrackCoords() { return &m_trackCoords; }

    ClefKeyContext * getClefKeyContext() { return m_clefKeyContext; }
    void updateClefKeyContext() { m_clefKeyContext->setSegments(this); }

    /// Return true if element is a clef or a key which already is in use
    bool isEventRedundant(Event *ev, Segment &seg) const;
    bool isEventRedundant(Clef &clef, timeT time, Segment &seg) const;
    bool isEventRedundant(Key &key, timeT time, Segment &seg) const;

    /// Return the segments about to be deleted if any
    std::vector<Segment *> * getSegmentsDeleted() { return &m_segmentsDeleted; }

    /// Return true if all segments in scene are about to be deleted
    /// (Editor needs to be closed)
    bool isSceneEmpty() { return m_sceneIsEmpty; }

    /**
     * Return true if another staff inside the scene than the given one
     * exists near the given time.
     */
    bool isAnotherStaffNearTime(NotationStaff *currentStaff, timeT t);

   /**
    * Update the refresh status off all segments on the given track and
    * for time greater than time.
    * This method is useful when a clef or key signature has changed as, since
    * the redundant clefs and keys may be hide, some changes may propagate
    * across the segments up to the end of the composition.
    */
    void updateRefreshStatuses(TrackId track, timeT time);

    /// YG: Only for debug
    void dumpVectors();
    void dumpBarDataMap();

signals:
    void mousePressed(const NotationMouseEvent *e);
    void mouseMoved(const NotationMouseEvent *e);
    void mouseReleased(const NotationMouseEvent *e);
    void mouseDoubleClicked(const NotationMouseEvent *e);
    void wheelTurned(int);

    void sceneNeedsRebuilding();

    void eventRemoved(Event *);

    void selectionChanged();
    void selectionChanged(EventSelection *);

    void layoutUpdated(timeT,timeT);
    void staffsPositionned();

    void currentStaffChanged();
    void currentViewSegmentChanged(ViewSegment *);

    /**
     * Emitted when the mouse cursor moves to a different height
     * on the staff
     *
     * \a noteName contains the MIDI name of the corresponding note
     */
    void hoveredOverNoteChanged(const QString &noteName);

    /**
     * Emitted when the mouse cursor moves to a note which is at a
     * different time
     *
     * \a time is set to the absolute time of the note the cursor is
     * hovering on
     */
    void hoveredOverAbsoluteTimeChanged(unsigned int time);

protected slots:
    void slotCommandExecuted();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
    void wheelEvent(QGraphicsSceneWheelEvent *);

    // CompositionObserver methods
    void segmentRemoved(const Composition *, Segment *);
    void timeSignatureChanged(const Composition *); // CompositionObserver
    void segmentRepeatChanged(const Composition *, Segment *, bool);
    void segmentRepeatEndChanged(const Composition *, Segment *, timeT);
    void segmentStartChanged(const Composition *, Segment *, timeT);
    void segmentEndMarkerChanged(const Composition *, Segment *, bool);
    void trackChanged(const Composition *, Track *);



private:
    void setNotePixmapFactories(QString fontName = "", int size = -1);
    NotationStaff *getNextStaffVertically(int direction, timeT t);
    NotationStaff *getNextStaffHorizontally(int direction, bool cycle);
    NotationStaff *getStaffbyTrackAndTime(const Track *track, timeT targetTime);
    void initCurrentStaffIndex(void);

    NotationWidget *m_widget; // I do not own this

    RosegardenDocument *m_document; // I do not own this
    NotationProperties *m_properties; // I own this

    NotePixmapFactory *m_notePixmapFactory; // I own this
    NotePixmapFactory *m_notePixmapFactorySmall; // I own this

    std::vector<Segment *> m_externalSegments; // I do not own these
    std::vector<Segment *> m_clones; // I own these
    std::vector<Segment *> m_segments; // The concatenation of m_clones 
                                       // and m_externalSegments
    std::vector<NotationStaff *> m_staffs; // I own these

    ClefKeyContext *m_clefKeyContext; // I own this

    EventSelection *m_selection; // I own this

    NotationHLayout *m_hlayout; // I own this
    NotationVLayout *m_vlayout; // I own this

    QGraphicsTextItem *m_title;
    QGraphicsTextItem *m_subtitle;
    QGraphicsTextItem *m_composer;
    QGraphicsTextItem *m_copyright;

    std::vector<QGraphicsItem *> m_pages;
    std::vector<QGraphicsItem *> m_pageNumbers;

    StaffLayout::PageMode m_pageMode;
    int m_printSize;
    int m_leftGutter;

    int m_currentStaff;
    int m_visibleStaffs;

    unsigned int m_compositionRefreshStatusId;
    bool m_timeSignatureChanged;

    bool m_updatesSuspended;

    /// Returns the page width according to the layout mode (page/linear)
    int getPageWidth();

    /// Returns the page height according to the layout mode (page/linear)
    int getPageHeight();

    /// Returns the margins within the page (zero if not in MultiPageMode)
    void getPageMargins(int &left, int &top);

    void setupMouseEvent(QGraphicsSceneMouseEvent *, NotationMouseEvent &);

    void checkUpdate();
    void positionStaffs();
    void layoutAll();
    void layout(NotationStaff *singleStaff, timeT start, timeT end);

    NotationStaff *setSelectionElementStatus(EventSelection *, bool set);
    void previewSelection(EventSelection *, EventSelection *oldSelection);

    int m_minTrack;
    int m_maxTrack;

    TrackIntMap m_trackHeights;
    TrackIntMap m_trackCoords;

    // Remember segments about to be deleted
    std::vector<Segment *> m_segmentsDeleted;

    bool m_finished;       // Waiting dtor : don't do too much now
    bool m_sceneIsEmpty;   // No more segment in scene
    bool m_showRepeated;   // Repeated segments are visible
    bool m_editRepeated;   // Direct edition of repeated segments is allowed
    bool m_haveInittedCurrentStaff;

    // Remember current labels of tracks
    std::map<int, std::string> m_trackLabels;
};

}

#endif
