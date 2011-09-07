/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_COMPOSITIONMODEL_H_
#define _RG_COMPOSITIONMODEL_H_

#include "base/Composition.h"
#include "base/Segment.h"
#include <set>
#include <QColor>
#include <QObject>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <utility>
#include <vector>
#include "base/Event.h"
#include "CompositionRect.h"
#include "CompositionItem.h"


class RectRanges;
//class CompositionItem;
class AudioPreviewDrawData;


namespace Rosegarden
{

class SnapGrid;
typedef std::vector<QImage> PixmapArray;

/// The interface for a composition model.
/**
 * This is primarily an interface (abstract base) class that defines the
 * interface for a composition model.
 *
 * See the deriver (CompositionModelImpl) for details.
 *
 * Given that there is only one deriver from this interface and probably has
 * been for quite some time, this class can probably be removed.
 */
class CompositionModel : public QObject, public CompositionObserver, public SegmentObserver
{
    Q_OBJECT
public:

    struct CompositionItemCompare {
        bool operator()(const CompositionItem &c1, const CompositionItem &c2) const;
    };

    typedef std::vector<QRect> rectlist;
    typedef std::vector<int> heightlist;
    typedef std::vector<CompositionRect> rectcontainer;
    typedef std::set<CompositionItem, CompositionItemCompare> itemcontainer;

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

    struct RectRange {
        std::pair<rectlist::iterator, rectlist::iterator> range;
        QPoint basePoint;
        QColor color;
    };

    typedef std::vector<RectRange> RectRanges;

    class AudioPreviewData {
    public:
        AudioPreviewData(bool showMinima, unsigned int channels) : m_showMinima(showMinima), m_channels(channels) {};
        // ~AudioPreviewData();

        bool showsMinima()              { return m_showMinima; }
        void setShowMinima(bool s)      { m_showMinima = s;    }

        unsigned int getChannels()       { return m_channels;   }
        void setChannels(unsigned int c) { m_channels = c;      }

        const std::vector<float> &getValues() const { return m_values;  }
        void setValues(const std::vector<float>&v) { m_values = v; }

        QRect getSegmentRect()              { return m_segmentRect; }
        void setSegmentRect(const QRect& r) { m_segmentRect = r; }

    protected:
        std::vector<float> m_values;
        bool               m_showMinima;
        unsigned int       m_channels;
        QRect              m_segmentRect;

    private:
        // no copy ctor
        AudioPreviewData(const AudioPreviewData&);
    };


    virtual ~CompositionModel() {};

    virtual unsigned int getNbRows() = 0;
    virtual const rectcontainer& getRectanglesIn(const QRect& rect,
                                                 RectRanges* notationRects, AudioPreviewDrawData* audioRects) = 0;

    virtual unsigned int getHeight() = 0;
    virtual heightlist getTrackDividersIn(const QRect& rect) = 0;

    virtual itemcontainer getItemsAt (const QPoint&) = 0;
    virtual timeT getRepeatTimeAt (const QPoint&, const CompositionItem&) = 0;

    virtual SnapGrid& grid() = 0;

    virtual void setPointerPos(int xPos) = 0;
    virtual void setSelected(const CompositionItem&, bool selected = true) = 0;
    virtual bool isSelected(const CompositionItem&) const = 0;
    virtual void setSelected(const itemcontainer&) = 0;
    virtual void setSelected(const Segment*, bool selected = true) = 0;
    virtual bool isSelected(const Segment*) const = 0;
    virtual void clearSelected() = 0;
    virtual bool haveSelection() const = 0;
    virtual bool haveMultipleSelection() const = 0;
    virtual void signalSelection() = 0;
    virtual void setSelectionRect(const QRect&) = 0;
    virtual void finalizeSelectionRect() = 0;
    virtual QRect getSelectionContentsRect() = 0;
    virtual void signalContentChange() = 0;

    virtual void addRecordingItem(const CompositionItem&) = 0;
    virtual void removeRecordingItem(const CompositionItem&) = 0;
    virtual void clearRecordingItems() = 0;
    virtual bool haveRecordingItems() = 0;

    enum ChangeType { ChangeMove, ChangeResizeFromStart, ChangeResizeFromEnd };

    virtual void startChange(const CompositionItem&, ChangeType change) = 0;
    virtual void startChangeSelection(ChangeType change) = 0;
    virtual itemcontainer& getChangingItems() = 0;
    virtual void endChange() = 0;
    virtual ChangeType getChangeType() = 0;

    virtual void setLength(int width) = 0;
    virtual int  getLength() = 0;

signals:
    void needContentUpdate();
    void needContentUpdate(const QRect&);
    void needArtifactsUpdate();

protected:
    CompositionItem* m_currentCompositionItem;
};

class AudioPreviewThread;
class AudioPreviewUpdater;


}

#endif
