/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef LEGATO_QUANTIZER_H
#define LEGATO_QUANTIZER_H

#include "Quantizer.h"

namespace Rosegarden {

class BasicQuantizer;

class LegatoQuantizer : public Quantizer
{
public:
    // The default unit is the shortest note type.  A unit of
    // zero means do no quantization -- unlike for BasicQuantizer
    // this does have a purpose, as it still does the legato step
    LegatoQuantizer(timeT unit = -1);
    LegatoQuantizer(std::string source, std::string target, timeT unit = -1);
    LegatoQuantizer(const LegatoQuantizer &);
    ~LegatoQuantizer() override;

    void setUnit(timeT unit) { m_unit = unit; }
    timeT getUnit() const { return m_unit; }

    void quantizeRange(Segment *,
                               Segment::iterator,
                               Segment::iterator) const override;

protected:
    virtual void quantizeLegatoSingle(Segment *,
                                      Segment::iterator,
                                      Segment::iterator &) const;

    timeT quantizeTime(timeT) const;

private:
    LegatoQuantizer &operator=(const BasicQuantizer &); // not provided

    timeT m_unit;
};

}

#endif

