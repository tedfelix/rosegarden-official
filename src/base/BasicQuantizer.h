/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef BASIC_QUANTIZER_H
#define BASIC_QUANTIZER_H

#include "Quantizer.h"

namespace Rosegarden {

class BasicQuantizer : public Quantizer
{
public:
    // The default unit is the shortest note type.  A unit of
    // zero means do no quantization (rather pointlessly).
    BasicQuantizer(timeT unit = -1, bool doDurations = false,
                   int swingPercent = 0, int iteratePercent = 100);
    BasicQuantizer(std::string source, std::string target,
                   timeT unit = -1, bool doDurations = false,
                   int swingPercent = 0, int iteratePercent = 100);
    BasicQuantizer(const BasicQuantizer &);
    virtual ~BasicQuantizer();

    void setUnit(timeT unit) { m_unit = unit; }
    timeT getUnit() const { return m_unit; }

    void setDoDurations(bool doDurations) { m_durations = doDurations; }
    bool getDoDurations() const { return m_durations; }

    void setSwing(int percent) { m_swing = percent; }
    int getSwing() const { return m_swing; }

    void setIterative(int percent) { m_iterate = percent; }
    int getIterative() const { return m_iterate; }
    
    /**
     * Return the standard quantization units in descending order of
     * unit duration
     */
    static std::vector<timeT> getStandardQuantizations();

    /**
     * Study the given segment; if all the events in it have times
     * that match one or more of the standard quantizations, return
     * the longest standard quantization unit to match.  Otherwise
     * return 0.
     */
    static timeT getStandardQuantization(Segment *);

    /**
     * Study the given selection; if all the events in it have times
     * that match one or more of the standard quantizations, return
     * the longest standard quantization unit to match.  Otherwise
     * return 0.
     */
    static timeT getStandardQuantization(EventSelection *);

protected:
    virtual void quantizeSingle(Segment *,
                                Segment::iterator) const;

private:
    BasicQuantizer &operator=(const BasicQuantizer &); // not provided

    timeT m_unit;
    bool m_durations;
    int m_swing;
    int m_iterate;

    static std::vector<timeT> m_standardQuantizations;
    static void checkStandardQuantizations();
    static timeT getUnitFor(Event *);
};

}

#endif
