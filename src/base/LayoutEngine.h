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

#ifndef RG_LAYOUT_ENGINE_H
#define RG_LAYOUT_ENGINE_H

#include "RulerScale.h"

namespace Rosegarden {

class ViewSegment;
class TimeSignature;


/**
 * Base classes for layout engines.  The intention is that
 * different sorts of renderers (piano-roll, score etc) can be
 * implemented by simply plugging different implementations
 * of ViewSegment and LayoutEngine into a single view class.
 */
class LayoutEngine
{
public: 
    LayoutEngine() { }
    virtual ~LayoutEngine() { }

    /**
     * Resets internal data stores for all segments
     */
    virtual void reset() = 0;

    /**
     * Precomputes layout data for a single segment, updating any
     * internal data stores associated with that segment and updating
     * any layout-related properties in the events on the segment's
     * segment.
     * 
     * If full == true, a complete rescan of the entire composition is
     * in progress and the segment is known to be contained completely
     * within the start to end time range (which may extend beyond the
     * segment at either end).  Otherwise, the start to end time
     * represents the modified range of the segment.
     */
    virtual void scanViewSegment(ViewSegment &viewSegment,
				 timeT startTime,
				 timeT endTime,
                                 bool full) = 0;

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one segment.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     * 
     * If full == true, a complete relayout of the entire composition
     * is in progress and the segment is known to be contained
     * completely within the start to end time range (which may extend
     * beyond the segment at either end).  Otherwise, the start to end
     * time represents the modified range of the segment.
     */
    virtual void finishLayout(timeT startTime,
                              timeT endTime,
                              bool full) = 0;

    unsigned int getStatus() const { return m_status; }

protected:
    unsigned int m_status;
};


class HorizontalLayoutEngine : public LayoutEngine,
				public RulerScale
{
public:
    HorizontalLayoutEngine(Composition *c) : LayoutEngine(), RulerScale(c) { }
    ~HorizontalLayoutEngine() override { }

    /**
     * Sets a page width for the layout.
     *
     * A layout implementation does not have to use this.  Some might
     * use it (for example) to ensure that bar lines fall precisely at
     * the right-hand margin of each page.  The computed x-coordinates
     * will still require to be wrapped into lines by the segment or
     * view implementation, however.
     *
     * A width of zero indicates no requirement for division into
     * pages.
     */
    virtual void setPageWidth(double) { /* default: ignore it */ }

    /**
     * Returns the number of the first visible bar line on the given
     * segment
     */
    virtual int getFirstVisibleBarOnViewSegment(ViewSegment &) const {
        return  getFirstVisibleBar();
    }

    /**
     * Returns the number of the last visible bar line on the given
     * segment
     */
    virtual int getLastVisibleBarOnViewSegment(ViewSegment &) const {
        return  getLastVisibleBar();
    }

    /**
     * Returns true if the specified bar has the correct length
     */
    virtual bool isBarCorrectOnViewSegment(ViewSegment &, int/* barNo */) const {
        return true;
    }

    /**
     * Returns true if there is a new time signature in the given bar,
     * setting timeSignature appropriately and setting timeSigX to its
     * x-coord
     */
    virtual bool getTimeSignaturePosition
    (ViewSegment &, int /* barNo */, TimeSignature &, double &/* timeSigX */) const {
        return 0;
    }
};



class VerticalLayoutEngine : public LayoutEngine
{
public:
    VerticalLayoutEngine() { }
    ~VerticalLayoutEngine() override { }

    // I don't think we need to add anything here for now
};

}


#endif
