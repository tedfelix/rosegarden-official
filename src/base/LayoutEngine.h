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

#ifndef _LAYOUT_ENGINE_2_H_
#define _LAYOUT_ENGINE_2_H_

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
     * Resets internal data stores for a specific segment.
     * 
     * If startTime == endTime, act on the whole segment; otherwise only
     * the given section.
     */
    virtual void resetViewSegment(ViewSegment &viewSegment,
				  timeT startTime = 0,
				  timeT endTime = 0) = 0;

    /**
     * Precomputes layout data for a single segment, updating any
     * internal data stores associated with that segment and updating
     * any layout-related properties in the events on the segment's
     * segment.
     * 
     * If startTime == endTime, act on the whole segment; otherwise only
     * the given section.
     */
    virtual void scanViewSegment(ViewSegment &viewSegment,
				 timeT startTime = 0,
				 timeT endTime = 0) = 0;

    /**
     * Computes any layout data that may depend on the results of
     * scanning more than one segment.  This may mean doing most of
     * the layout (likely for horizontal layout) or nothing at all
     * (likely for vertical layout).
     * 
     * If startTime == endTime, act on the whole segment; otherwise only
     * the given section.
     */
    virtual void finishLayout(timeT startTime = 0,
                              timeT endTime = 0) = 0;

    unsigned int getStatus() const { return m_status; }

protected:
    unsigned int m_status;
};


class HorizontalLayoutEngine : public LayoutEngine,
				public RulerScale
{
public:
    HorizontalLayoutEngine(Composition *c) : LayoutEngine(), RulerScale(c) { }
    virtual ~HorizontalLayoutEngine() { }

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
    virtual ~VerticalLayoutEngine() { }

    // I don't think we need to add anything here for now
};

}


#endif
