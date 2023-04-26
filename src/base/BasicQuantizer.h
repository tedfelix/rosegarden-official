/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
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


/// The "Grid quantizer"
class BasicQuantizer : public Quantizer
{
public:
    // unit == -1 => Note::Shortest
    // unit == 0 => No quantization, call setUnit() to change.
    BasicQuantizer(timeT unit = -1, bool doDurations = false,
                   int swingPercent = 0, int iteratePercent = 100);
    BasicQuantizer(std::string source, std::string target,
                   timeT unit, bool doDurations,
                   int swingPercent, int iteratePercent);
    ~BasicQuantizer() override  { }

    void setUnit(timeT unit)  { m_unit = unit; }
    timeT getUnit() const  { return m_unit; }

    void setRemoveSmaller(timeT unit)  { m_removeSmaller = unit; }

    bool getDoDurations() const  { return m_durations; }

    /**
     * Return the standard quantization units in descending order of
     * unit duration
     */
    static std::vector<timeT> getStandardQuantizations();

protected:
    /// Quantize a single Event.
    void quantizeSingle(Segment *,
                        Segment::iterator) const override;

private:
    // Hide copy ctor and op=
    // ??? Actually these are perfectly copyable.  There is no need to do this.
    BasicQuantizer(const BasicQuantizer &);
    BasicQuantizer &operator=(const BasicQuantizer &);

    // Quantization unit (e.g. 1/8 notes).  0 => No quantization.
    timeT m_unit;
    // Also quantize durations.
    bool m_durations;
    // Swing percentage.
    int m_swing;
    // Iterative (partial) quantization percentage.
    int m_iterate;

    timeT m_removeSmaller{0};

};


}

#endif
