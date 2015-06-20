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

#ifndef RG_COMPOSITIONMODELIMPL_H
#define RG_COMPOSITIONMODELIMPL_H

#include "base/SnapGrid.h"
#include "CompositionRect.h"
#include "CompositionItem.h"
#include "SegmentOrderer.h"

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QSharedPointer>

#include <vector>
#include <map>
#include <set>


namespace Rosegarden
{


class Studio;
class Segment;
class RulerScale;
class InstrumentStaticSignals;
class Instrument;
class Event;
class Composition;
class AudioPreviewUpdater;
class AudioPreviewThread;

/// For Audio Previews
typedef std::vector<QImage> PixmapArray;

/// Model layer between CompositionView and Composition.
/**
 * The Qt interpretation of the term "Model" is a layer of functionality
 * that sits between the data (Composition) and the view (CompositionView).
 * It's like Decorator Pattern.  It adds functionality to an object without
 * modifying that object.  It maintains the focus on the object.
 *
 * To me, Decorator Pattern is a cop-out.  It always results in a
 * disorganized pile of ideas dumped into a class.  Like this.
 * A better way to organize this code would be to look for big coherent
 * ideas (like selection and previews) and break those off into their own
 * classes.  What remains can be moved to CompositionView.  There may be
 * other big ideas hiding in CompositionView that are worthy of classes of
 * their own as well.
 *
 * This class works together with CompositionView to provide the composition
 * user interface (the segment canvas).  TrackEditor creates and owns the
 * only instance of this class.
 */
class CompositionModelImpl : public QObject, public CompositionObserver, public SegmentObserver
{
    Q_OBJECT
public:
    CompositionModelImpl(QObject *parent,
                         Composition& compo,
                         Studio& studio,
                         RulerScale *rulerScale,
                         int vStep);

    virtual ~CompositionModelImpl();

    typedef std::vector<QRect> RectList;

    struct RectRange {
        std::pair<RectList::iterator, RectList::iterator> range;
        QPoint basePoint;
        QColor color;
    };

    typedef std::vector<RectRange> RectRanges;

    struct AudioPreviewDrawDataItem {
        AudioPreviewDrawDataItem(PixmapArray p, QPoint bp, QRect r) :
            pixmap(p), basePoint(bp), rect(r), resizeOffset(0) {};
        PixmapArray pixmap;
        QPoint basePoint;
        QRect rect;

        // when showing a segment that is being resized from the
        // beginning, this contains the offset between the current
        // rect of the segment and the resized one
        int resizeOffset;
    };

    typedef std::vector<AudioPreviewDrawDataItem> AudioPreviewDrawData;

    typedef std::vector<CompositionRect> RectContainer;

    /// Get the segment rectangles and segment previews
    const RectContainer& getSegmentRects(const QRect &clipRect,
                                         RectRanges *notationPreview,
                                         AudioPreviewDrawData *audioPreview);

    typedef std::vector<int> YCoordList;

    /// Get the Y coords of each track within clipRect.
    /**
     * CompositionView::drawSegments() uses this to draw the track dividers.
     */
    YCoordList getTrackDividersIn(const QRect &clipRect);

    /// Compares Segment pointers in a CompositionItem.
    /**
     * Is this really better than just comparing the CompositionItemPtr
     * addresses?
     *
     *    // Compare the QPointer addresses
     *    return c1.data() < c2.data();
     *
     * All this indexing with pointers gives me the willies.  IDs are safer.
     */
    struct CompositionItemCompare {
        bool operator()(CompositionItemPtr c1, CompositionItemPtr c2) const
        {
            // This strikes me as odd.  I think the one below is better.
            //return CompositionItemHelper::getSegment(c1) < CompositionItemHelper::getSegment(c2);

            // operator< on Segment *'s?  I guess order isn't too important.
            return c1->getSegment() < c2->getSegment();
        }
    };

    typedef std::set<CompositionItemPtr, CompositionItemCompare> ItemContainer;

    /// Used by CompositionView on mouse double-click.
    ItemContainer getItemsAt(const QPoint &);
    /// Get the start time of the repeat nearest the point.
    /**
     * Used by CompositionView to determine the time at which to edit a repeat.
     *
     * Looking closely at the implementation of this, we find that this is a
     * function that brings together CompositionItem and SnapGrid.  It mainly
     * uses CompositionItem, so it likely belongs there.  Perhaps more as a
     *
     *   CompositionItem::getRepeatTimeAt(const SnapGrid &, const QPoint &)
     */
    timeT getRepeatTimeAt(const QPoint &, CompositionItemPtr);

    /// Used in many places for various coordinate/time purposes.
    SnapGrid& grid()  { return m_grid; }

    // *** Selection

    void setSelected(CompositionItemPtr, bool selected = true);
    void setSelected(Segment *, bool selected = true);
    //void setSelected(const ItemContainer&);
    void clearSelected();
    bool isSelected(CompositionItemPtr) const;
    bool isSelected(const Segment *) const;
    bool haveSelection() const  { return !m_selectedSegments.empty(); }
    bool haveMultipleSelection() const  { return m_selectedSegments.size() > 1; }
    SegmentSelection getSelectedSegments()  { return m_selectedSegments; }
    /// Bounding rect of the currently selected segments.
    QRect getSelectedSegmentsRect();

    /// Emit selectedSegments() signal
    /**
     * Used by SegmentSelector and others to signal a change in the selection.
     */
    void signalSelection();

    /// Click and drag selection rect.
    void setSelectionRect(const QRect &);
    /// Click and drag selection complete.
    void finalizeSelectionRect();

    /// Emit needContentUpdate() signal
    /**
     * rename: emitRefreshView()
     */
    void signalContentChange();

    // *** Recording

    /// Refresh the recording segments.
    void pointerPosChanged(int x);

    void addRecordingItem(CompositionItemPtr);
    //void removeRecordingItem(CompositionItemPtr);
    void clearRecordingItems();
    //bool haveRecordingItems()  { return !m_recordingSegments.empty(); }

    // *** Changing (moving and resizing)

    enum ChangeType { ChangeMove, ChangeResizeFromStart, ChangeResizeFromEnd };

    /// Begin move/resize for a single segment.
    void startChange(CompositionItemPtr, ChangeType change);
    /// Begin move for all selected segments.
    void startChangeSelection(ChangeType change);
    //ChangeType getChangeType() const  { return m_changeType; }
    /// Get the segments that are moving or resizing.
    ItemContainer &getChangingItems()  { return m_changingItems; }
    /// Cleanup after move/resize.
    void endChange();

    // *** Misc

    /// In pixels
    /**
     * Used to expand the composition when we go past the end.
     */
    void setCompositionLength(int width);
    /// In pixels
    /**
     * Used to expand the composition when we go past the end.
     */
    int getCompositionLength();
    /// Number of pixels needed vertically to render all tracks.
    unsigned int getCompositionHeight();
    
    /**
     * Used by CompositionView's ctor to connect
     * RosegardenDocument::m_audioPreviewThread.
     */
    void setAudioPreviewThread(AudioPreviewThread *thread);
    //AudioPreviewThread* getAudioPreviewThread() { return m_audioPreviewThread; }

    /// See CompositionView::clearSegmentRectsCache()
    void clearSegmentRectsCache(bool clearPreviews = false)
            { clearInCache(0, clearPreviews); }

    CompositionRect computeSegmentRect(const Segment &, bool computeZ = false);

    //void computeRepeatMarks(CompositionItemPtr);

    Composition &getComposition()  { return m_composition; }
    Studio &getStudio()  { return m_studio; }

    /**
     * rename: AudioPreview
     */
    struct AudioPreviewData {
        AudioPreviewData() :
            channels(0)
        { }

        unsigned int channels;

        typedef std::vector<float> Values;
        Values values;
    };

signals:
    /// rename: refreshView
    void needContentUpdate();
    /// rename: refreshView
    void needContentUpdate(const QRect&);
    void needArtifactsUpdate();
    void selectedSegments(const SegmentSelection &);
    void needSizeUpdate();

public slots:
    void slotAudioFileFinalized(Segment*);
    void slotInstrumentChanged(Instrument *);

private slots:
    void slotAudioPreviewComplete(AudioPreviewUpdater*);

private:
    // CompositionObserver Interface
    virtual void segmentAdded(const Composition *, Segment *);
    virtual void segmentRemoved(const Composition *, Segment *);
    virtual void segmentRepeatChanged(const Composition *, Segment *, bool);
    virtual void segmentStartChanged(const Composition *, Segment *, timeT);
    virtual void segmentEndMarkerChanged(const Composition *, Segment *, bool);
    virtual void segmentTrackChanged(const Composition *, Segment *, TrackId);
    virtual void endMarkerTimeChanged(const Composition *, bool /*shorten*/);

    // SegmentObserver Interface
    virtual void eventAdded(const Segment *, Event *);
    virtual void eventRemoved(const Segment *, Event *);
    virtual void AllEventsChanged(const Segment *s);
    virtual void appearanceChanged(const Segment *);
    virtual void endMarkerTimeChanged(const Segment *, bool /*shorten*/);
    virtual void segmentDeleted(const Segment*) { /* nothing to do - handled by CompositionObserver::segmentRemoved() */ };

    QPoint computeSegmentOrigin(const Segment &);

    bool setTrackHeights(Segment *changed = 0); // true if something changed

    bool isTmpSelected(const Segment*) const;
    bool wasTmpSelected(const Segment*) const;
    bool isMoving(const Segment*) const;
    bool isRecording(const Segment*) const;

    void computeRepeatMarks(CompositionRect& sr, const Segment* s);
    unsigned int computeZForSegment(const Segment* s);
        
    // Segment Previews
    /// Make and cache notation or audio preview for segment.
    void makePreviewCache(const Segment* s);
    /// Remove cached notation or audio preview for segment.
    void removePreviewCache(const Segment* s);

    // Notation Previews
    /// rename: getNotationPreviewStatic()?
    void makeNotationPreviewRects(QPoint basePoint, const Segment* segment,
                                  const QRect& clipRect, RectRanges* npData);
    /// rename: getNotationPreviewMoving()?
    void makeNotationPreviewRectsMovingSegment(
            QPoint basePoint, const Segment* segment,
            const QRect& currentSR, RectRanges* npData);
    /// rename: getNotationPreview()
    RectList* getNotationPreviewData(const Segment* s);
    /// rename: cacheNotationPreview()
    RectList* makeNotationPreviewDataCache(const Segment *s);
    /// For notation preview
    void createEventRects(const Segment* segment, RectList*);

    // Audio Previews
    void makeAudioPreviewRects(AudioPreviewDrawData* apRects, const Segment*,
                               const CompositionRect& segRect, const QRect& clipRect);
    PixmapArray getAudioPreviewPixmap(const Segment* s);
    AudioPreviewData* getAudioPreviewData(const Segment* s);
    QRect postProcessAudioPreview(AudioPreviewData* apData, const Segment*);
    /// rename: cacheAudioPreview()
    AudioPreviewData* makeAudioPreviewDataCache(const Segment *s);
    /// rename: makeAudioPreview()
    void updatePreviewCacheForAudioSegment(const Segment* s);

    /// Clear notation and audio preview caches.
    void clearPreviewCache();

    // Segment Rects (m_segmentRectMap)
    void putInCache(const Segment*, const CompositionRect&);
    const CompositionRect& getFromCache(const Segment*, timeT& endTime);
    bool isCachedRectCurrent(const Segment& s, const CompositionRect& r,
                             QPoint segmentOrigin, timeT segmentEndTime);
    void clearInCache(const Segment*, bool clearPreviewCache = false);

    //--------------- Data members ---------------------------------
    Composition&     m_composition;
    Studio&          m_studio;
    SnapGrid         m_grid;

    SegmentSelection m_selectedSegments;
    SegmentSelection m_tmpSelectedSegments;
    SegmentSelection m_previousTmpSelectedSegments;

    timeT            m_pointerTimePos;

    typedef std::set<Segment *>  RecordingSegmentSet;
    RecordingSegmentSet          m_recordingSegments;

    AudioPreviewThread*          m_audioPreviewThread;

    // Notation Preview
    typedef std::map<const Segment *, RectList *> NotationPreviewDataCache;
    NotationPreviewDataCache     m_notationPreviewDataCache;

    // Audio Preview
    typedef std::map<const Segment *, AudioPreviewData *> AudioPreviewDataCache;
    AudioPreviewDataCache        m_audioPreviewDataCache;

    /// Used by getSegmentRects() to return a reference for speed.
    RectContainer m_segmentRects;

    ChangeType    m_changeType;
    ItemContainer m_changingItems;
    typedef std::vector<CompositionItemPtr> ItemGC;
    ItemGC m_itemGC;

    QRect m_selectionRect;
    QRect m_previousSelectionUpdateRect;

    std::map<const Segment*, CompositionRect> m_segmentRectMap;
    std::map<const Segment*, timeT> m_segmentEndTimeMap;
    std::map<const Segment*, PixmapArray> m_audioSegmentPreviewMap;
    std::map<TrackId, int> m_trackHeights;

    typedef std::map<const Segment*, AudioPreviewUpdater *>
        AudioPreviewUpdaterMap;
    AudioPreviewUpdaterMap m_audioPreviewUpdaterMap;

    SegmentOrderer m_segmentOrderer;

    QSharedPointer<InstrumentStaticSignals> m_instrumentStaticSignals;
};



}

#endif
