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
#include "base/TimeT.h"  // timeT

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

/// Model layer between CompositionView and Composition.
/**
 * This class works together with CompositionView to provide the composition
 * user interface (the segment canvas).  TrackEditor creates and owns the
 * only instance of this class.
 *
 * This class is a segment rectangle and preview manager.  It augments
 * segments and events with a sense of position on a view.  The key member
 * objects are:
 *
 *   - m_segmentRectMap
 *   - m_notationPreviewCache
 *   - m_audioPeaksCache
 *   - m_audioPreviewImageCache
 *   - m_selectedSegments
 *
 * The Qt interpretation of the term "Model" is a layer of functionality
 * that sits between the data (Composition) and the view (CompositionView).
 * It's like Decorator Pattern.  It adds functionality to an object without
 * modifying that object.  It maintains the focus on the object.
 *
 * To me, Decorator Pattern is a cop-out.  It always results in a
 * disorganized pile of ideas dumped into a class.  Like this.
 * A better way to organize this code would be to look for big coherent
 * ideas.  For CompositionView and CompositionModelImpl, those would
 * be:
 *
 *   - Segments
 *   - Notation Previews
 *   - Audio Previews
 *   - Selection
 *   - Artifacts
 *
 * Now organize the code around those:
 *
 *   - CompositionView - thin layer over Qt to provide drawing help and
 *                       coordinate the other objects
 *   - SegmentRenderer - takes a Composition and draws segments on
 *                       the CompositionView
 *   - NotationPreviewRenderer - takes a Composition and draws previews
 *                       on the CompositionView for the MIDI segments
 *   - AudioPreviewRenderer - takes a Composition and draws previews on the
 *                       CompositionView for the Audio segments
 *   - SelectionManager - Handles the concept of selected segments.
 *   - ArtifactState - A data-only object that holds things like
 *                     the position of the guides and whether they are
 *                     visible.  Modified by the tools and notifies
 *                     ArtifactRenderer of changes.
 *   - ArtifactRenderer - Draws the Artifacts on the CompositionView.  Uses
 *                        the Composition and ArtifactState.
 */
class CompositionModelImpl :
        public QObject,
        public CompositionObserver,
        public SegmentObserver
{
    Q_OBJECT
public:
    CompositionModelImpl(QObject *parent,
                         Composition &,
                         Studio &,
                         RulerScale *,
                         int trackCellHeight);

    virtual ~CompositionModelImpl();

    Composition &getComposition()  { return m_composition; }
    Studio &getStudio()  { return m_studio; }
    SnapGrid &grid()  { return m_grid; }

    // --- Notation Previews ------------------------------

    /// A vector of QRect's.
    /**
     * Each QRect represents a note/event in the preview.
     *
     * See NotationPreviewCache.
     *
     * QRectVector was also considered as a name for this, but since it
     * is used in so many places, the more abstract name seemed more
     * helpful to readers of the code.
     */
    typedef std::vector<QRect> NotationPreview;

    /// The visible range of notation preview data for a segment.
    struct NotationPreviewRange {
        NotationPreview::iterator begin;
        NotationPreview::iterator end;

        // y coord in contents coords
        int segmentTop;
        // 0 when the segment isn't moving.
        int moveXOffset;

        QColor color;
    };

    /// A vector of NotationPreviewRange objects, one per segment.
    typedef std::vector<NotationPreviewRange> NotationPreviewRanges;

    // --- Audio Previews ---------------------------------

    /// Audio Preview Image (tiled)
    typedef std::vector<QImage> QImageVector;

    struct AudioPreview {
        AudioPreview(QImageVector i, QPoint bp, QRect r) :
            image(i), basePoint(bp), rect(r), resizeOffset(0)  { }

        // Vector of QImage tiles containing the preview graphics.
        QImageVector image;

        // Upper left corner of the segment in contents coords.
        // ??? Same as rect.topLeft().  Redundant.  Can be removed.
        QPoint basePoint;

        // Segment rect in contents coords.
        QRect rect;

        // While the user is resizing a segment from the beginning,
        // this contains the offset between the current
        // rect of the segment and the resized one.
        int resizeOffset;
    };

    typedef std::vector<AudioPreview> AudioPreviews;

    /**
     * Used by CompositionView's ctor to connect
     * RosegardenDocument::m_audioPreviewThread.
     */
    void setAudioPreviewThread(AudioPreviewThread *thread);
    //AudioPreviewThread* getAudioPreviewThread() { return m_audioPreviewThread; }

    struct AudioPeaks {
        AudioPeaks() :
            channels(0)
        { }

        unsigned int channels;

        // See AudioPreviewThread::getPreview()
        typedef std::vector<float> Values;
        Values values;
    };

    // --- Segments ---------------------------------------

    typedef std::vector<CompositionRect> SegmentRects;

    /// Get the segment rectangles and segment previews
    /**
     * ??? Audio and Notation Previews too.  Need an "ALL" category.
     */
    const SegmentRects &getSegmentRects(
            const QRect &clipRect,
            NotationPreviewRanges *notationPreviews,
            AudioPreviews *audioPreviews);

    /// Get the topmost item (segment) at the given position on the view.
    /**
     * This routine returns a pointer to a *copy* of the CompositionItem.
     * The caller is responsible for deleting the object that is returned.
     *
     * ??? Need to audit all callers.  Many of them do not delete this
     *     object.
     */
    CompositionItemPtr getFirstItemAt(const QPoint &pos);

    /// See CompositionView::clearSegmentRectsCache()
    /**
     * ??? Audio and Notation Previews too.  Need an "ALL" category.
     */
    void clearSegmentRectsCache(bool clearPreviews = false)
            { clearInCache(0, clearPreviews); }

    CompositionRect computeSegmentRect(const Segment &, bool computeZ = false);

    // --- Selection --------------------------------------

    void setSelected(Segment *, bool selected = true);
    void selectSegments(const SegmentSelection &segments);
    void clearSelected();

    /// Click and drag selection rect (rubber band).
    void setSelectionRect(const QRect &);
    /// Select segments based on the selection rect (rubber band).
    void finalizeSelectionRect();

    /// Emit selectionChanged() signal
    /**
     * Used by SegmentSelector and others to signal a change in the selection.
     * See RosegardenMainViewWidget::slotSelectedSegments()
     */
    void selectionHasChanged();

    bool isSelected(const Segment *) const;
    bool haveSelection() const  { return !m_selectedSegments.empty(); }
    bool haveMultipleSelection() const  { return m_selectedSegments.size() > 1; }
    SegmentSelection getSelectedSegments()  { return m_selectedSegments; }
    /// Bounding rect of the currently selected segments.
    QRect getSelectedSegmentsRect();

    // --- Recording --------------------------------------

    // ??? This category might make more sense combined with Segments.

    void addRecordingItem(CompositionItemPtr);
    /// Update the recording segments on the display.
    void pointerPosChanged(int x);
    void clearRecordingItems();

    // --- Changing (moving and resizing) -----------------

    // ??? This category might make more sense combined with Segments.

    enum ChangeType { ChangeMove, ChangeResizeFromStart, ChangeResizeFromEnd };

    /// Begin move/resize for a single segment.
    void startChange(CompositionItemPtr, ChangeType change);
    /// Begin move for all selected segments.
    void startChangeSelection(ChangeType change);

    /// Compare Segment pointers in a CompositionItem.
    /**
     * Used by ChangingSegmentSet to order the CompositionItem objects.
     *
     * All this indexing with pointers gives me the willies.  IDs are safer.
     */
    struct CompositionItemPtrCompare {
        bool operator()(CompositionItemPtr c1, CompositionItemPtr c2) const
        {
            // operator< on Segment *'s?  I guess order isn't too important.
            return c1->getSegment() < c2->getSegment();

            // ??? Is the above better than just comparing the
            //     CompositionItemPtr addresses?
            // Compare the QPointer addresses
            //return c1.data() < c2.data();
        }
    };

    /**
     * startChange() is the reason this is a std::set.  startChangeSelection()
     * calls startChange() repeatedly with each selected segment.
     * startChange() uses std::set::find() to determine whether that segment
     * is already in m_changingSegments.  With a std::vector, this would be an
     * exponential search.  In practice, what is the worst use-case?
     */
    typedef std::set<CompositionItemPtr, CompositionItemPtrCompare>
            ChangingSegmentSet;

    /// Get the segments that are moving or resizing.
    ChangingSegmentSet &getChangingSegments()  { return m_changingSegments; }

    /// Cleanup after move/resize.
    void endChange();

    // --- Misc -------------------------------------------

    typedef std::vector<int> YCoordVector;

    /// Get the Y coords of each track within clipRect.
    /**
     * CompositionView::drawSegments() uses this to draw the track dividers.
     */
    YCoordVector getTrackYCoords(const QRect &clipRect);

    /// Number of pixels needed vertically to render all tracks.
    int getCompositionHeight();

signals:
    /// Connected to CompositionView::slotUpdateAll()
    void needUpdate();
    /// Connected to CompositionView::slotAllNeedRefresh(rect)
    void needUpdate(const QRect &);

    /// Connected to CompositionView::slotUpdateArtifacts()
    void needArtifactsUpdate();

    /// Connected to CompositionView::slotUpdateSize()
    void needSizeUpdate();

    /// Connected to RosegardenMainViewWidget::slotSelectedSegments().
    void selectionChanged(const SegmentSelection &);

public slots:
    /// Connected to RosegardenDocument::audioFileFinalized()
    /**
     * Called when recording of an audio file is finished and the
     * preview is ready to display.
     */
    void slotAudioFileFinalized(Segment *);

    /// Connected to InstrumentStaticSignals::changed()
    void slotInstrumentChanged(Instrument *);

private slots:
    /// Connected to AudioPreviewUpdater::audioPreviewComplete()
    // ??? rename: slotAudioPeaksComplete()
    void slotAudioPreviewComplete(AudioPreviewUpdater *);

private:
    // --- Misc -------------------------------------------

    Composition &m_composition;
    Studio &m_studio;
    SnapGrid m_grid;

    // CompositionObserver Interface
    // ??? It's hard to pin these down to a category as they contribute
    //     to multiple categories.
    virtual void segmentAdded(const Composition *, Segment *);
    virtual void segmentRemoved(const Composition *, Segment *);
    virtual void segmentRepeatChanged(const Composition *, Segment *, bool);
    virtual void segmentStartChanged(const Composition *, Segment *, timeT);
    virtual void segmentEndMarkerChanged(const Composition *, Segment *, bool);
    virtual void segmentTrackChanged(const Composition *, Segment *, TrackId);
    virtual void endMarkerTimeChanged(const Composition *, bool shorten);

    QSharedPointer<InstrumentStaticSignals> m_instrumentStaticSignals;

    // --- Notation Previews ------------------------------

    // SegmentObserver Interface
    // ??? These primarily affect the notation previews, however,
    //     endMarkerTimeChanged() feels more like a Segment thing.
    virtual void eventAdded(const Segment *, Event *);
    virtual void eventRemoved(const Segment *, Event *);
    virtual void allEventsChanged(const Segment *);
    virtual void appearanceChanged(const Segment *);
    virtual void endMarkerTimeChanged(const Segment *, bool shorten);
    virtual void segmentDeleted(const Segment *)
            { /* nothing to do - handled by CompositionObserver::segmentRemoved() */ }

    /// Make a NotationPreviewRange for a Segment.
    /**
     * Calls getNotationPreview() to get the preview for the segment.
     * Scans the NotationPreview for the range within the clipRect.
     * Assembles a NotationPreviewRange and adds it to ranges.
     */
    void makeNotationPreviewRange(
            QPoint basePoint, const Segment *segment,
            const QRect &clipRect, NotationPreviewRanges *ranges);

    /// Make a NotationPreviewRange for a Changing Segment.
    /**
     * Calls getNotationPreview() to get the preview for the segment.
     * Scans the NotationPreview for the range within the clipRect.
     * Assembles a NotationPreviewRange and adds it to ranges.
     *
     * Differs from makeNotationPreviewRange() in that it takes into
     * account that the Segment is changing (moving, resizing, etc...).
     * currentRect is the Segment's modified QRect at the moment.
     */
    void makeNotationPreviewRangeCS(
            QPoint basePoint, const Segment *segment,
            const QRect &currentRect, NotationPreviewRanges *ranges);

    /// Get the NotationPreview for a Segment.
    NotationPreview *getNotationPreview(const Segment *);

    /// Refresh the NotationPreview in the cache for a Segment.
    NotationPreview *refreshNotationPreviewCache(const Segment *);

    /// Create a NotationPreview from a Segment.
    void makeNotationPreview(const Segment *, NotationPreview *);

    typedef std::map<const Segment *, NotationPreview *> NotationPreviewCache;
    NotationPreviewCache m_notationPreviewCache;

    // --- Audio Previews ---------------------------------

    // AudioPreview generation happens in three steps.
    //   1. The AudioPeaks are generated asynchronously for a segment.
    //      See AudioPreviewUpdater.
    //   2. The audio preview image is created from the AudioPeaks.
    //      See AudioPreviewPainter.
    //   3. An AudioPreview object is created using the audio preview image.
    //      See makeAudioPreview().

    /// Make an AudioPreview for a Segment and add it to audioPreviews.
    void makeAudioPreview(
            AudioPreviews *audioPreviews, const Segment *,
            const CompositionRect &segmentRect);

    /**
     * If it's cached, it's returned immediately.  Otherwise, the process
     * of generating the preview asynchronously is started by a call to
     * getAudioPeaks().
     */
    QImageVector getAudioPreviewImage(const Segment *);

    /// Also generates the preview image asynchronously.
    AudioPeaks *getAudioPeaks(const Segment *);

    /// Also generates the preview image asynchronously.
    AudioPeaks *refreshAudioPeaksCache(const Segment *);

    /// Create audio peaks for a segment asynchronously.
    /**
     * Uses an AudioPreviewUpdater.  When the AudioPreviewUpdater is done,
     * slotAudioPreviewComplete() is called, which in turn calls
     * refreshPreviewImageCache().
     *
     * Also generates the preview image asynchronously.
     */
    void makeAudioPeaksAsync(const Segment *);

    /// Convert AudioPeaks into a QImageVector and add to m_audioPreviewImageCache.
    QRect refreshPreviewImageCache(AudioPeaks *, const Segment *);

    /// More of an AudioPeaksThread.
    AudioPreviewThread *m_audioPreviewThread;

    /// More of an AudioPeaksGeneratorMap.
    typedef std::map<const Segment *, AudioPreviewUpdater *>
            AudioPreviewUpdaterMap;
    AudioPreviewUpdaterMap m_audioPreviewUpdaterMap;

    typedef std::map<const Segment *, AudioPeaks *> AudioPeaksCache;
    AudioPeaksCache m_audioPeaksCache;

    std::map<const Segment *, QImageVector> m_audioPreviewImageCache;

    // --- Notation and Audio Previews --------------------

    // Segment Previews
    /// Make and cache notation or audio preview for segment.
    void makePreviewCache(const Segment *s);
    /// Remove cached notation or audio preview for segment.
    void removePreviewCache(const Segment *s);

    /// Clear notation and audio preview caches.
    void clearPreviewCache();

    // --- Segments ---------------------------------------

    QPoint computeSegmentOrigin(const Segment &);

    std::map<TrackId, int> m_trackHeights;
    bool setTrackHeights(Segment *changed = 0); // true if something changed

    //void computeRepeatMarks(CompositionItemPtr);
    void computeRepeatMarks(CompositionRect &sr, const Segment *s);

    SegmentOrderer m_segmentOrderer;
    unsigned int computeZForSegment(const Segment *s);

    /// Segment Cache
    std::map<const Segment *, CompositionRect> m_segmentRectMap;
    std::map<const Segment *, timeT> m_segmentEndTimeMap;

    void putInCache(const Segment *, const CompositionRect &);
    const CompositionRect &getFromCache(const Segment *, timeT &endTime);
    bool isCachedRectCurrent(const Segment &s, const CompositionRect &r,
                             QPoint cachedSegmentOrigin,
                             timeT cachedSegmentEndTime);
    void clearInCache(const Segment *, bool clearPreviewCache = false);

    /// Get all the segments at a point.
    ChangingSegmentSet getItemsAt(const QPoint &);

    /// Used by getSegmentRects() to return a reference for speed.
    SegmentRects m_segmentRects;

    // --- Selection --------------------------------------

    SegmentSelection m_selectedSegments;

    SegmentSelection m_tmpSelectedSegments;
    bool isTmpSelected(const Segment *) const;

    SegmentSelection m_previousTmpSelectedSegments;
    bool wasTmpSelected(const Segment *) const;

    QRect m_selectionRect;
    QRect m_previousSelectionUpdateRect;

    // --- Recording --------------------------------------

    typedef std::set<Segment *> RecordingSegmentSet;
    RecordingSegmentSet m_recordingSegments;
    timeT m_pointerTimePos;

    bool isRecording(const Segment *) const;

    // --- Changing (moving and resizing) -----------------

    bool isMoving(const Segment *) const;

    ChangeType m_changeType;
    ChangingSegmentSet m_changingSegments;

    // ??? rename: ChangingSegmentGC/m_changingSegmentGC
    typedef std::vector<CompositionItemPtr> ItemGC;
    /// ChangingSegments waiting for garbage collection.
    // ??? Use QSharedPointer and get rid of this.
    ItemGC m_itemGC;

};



}

#endif
