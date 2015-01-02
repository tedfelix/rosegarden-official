/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef NOTATION_QUANTIZER_H_
#define NOTATION_QUANTIZER_H_

#include "Quantizer.h"

namespace Rosegarden {

class NotationQuantizer : public Quantizer
{
public:
    NotationQuantizer();
    NotationQuantizer(std::string source, std::string target);
    NotationQuantizer(const NotationQuantizer &);
    ~NotationQuantizer();

    /**
     * Set the absolute time minimum unit.  Default is demisemiquaver.
     */
    void  setUnit(timeT);
    timeT getUnit() const;

    /**
     * Set the simplicity factor.  This controls the relative "pull"
     * towards larger units and more obvious beats in placing notes.
     * The value 10 means no pull to larger units, lower values mean
     * an active pull away from them.  Default is 13.
     */
    void setSimplicityFactor(int);
    int  getSimplicityFactor() const;

    /**
     * Set the maximum size of tuplet group.  2 = two-in-the-time-of-three
     * groupings, 3 = triplets, etc.  Default is 3.  Set <2 to switch off
     * tuplets altogether.
     */
    void setMaxTuplet(int);
    int  getMaxTuplet() const;

    /**
     * Set whether we assume the music may be contrapuntal -- that is,
     * may have notes that overlap rather than simply a sequence of
     * individual notes and chords.
     */
    void setContrapuntal(bool);
    bool getContrapuntal() const;

    /**
     * Set whether to add articulations (staccato, tenuto, slurs).
     * Default is true.  Doesn't affect quantization, only the marks
     * that are added to quantized notes.
     */
    void setArticulate(bool);
    bool getArticulate() const;

protected:
    virtual void quantizeRange(Segment *,
                               Segment::iterator,
                               Segment::iterator) const;

protected:
    // avoid having to rebuild absolutely everything each time we
    // tweak the implementation
    class Impl;
    Impl *m_impl;

private:
    NotationQuantizer &operator=(const NotationQuantizer &); // not provided
};

}

#endif
