/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MATRIXELEMENT_H
#define RG_MATRIXELEMENT_H

#include "base/ViewElement.h"

class QColor;
class QGraphicsItem;

namespace Rosegarden
{

class MatrixScene;
class Event;

class MatrixElement : public ViewElement
{
public:
    MatrixElement(MatrixScene *scene,
                  Event *event,
                  bool drum,
                  long pitchOffset);
    ~MatrixElement() override;

    /// Returns true if the wrapped event is a note
    bool isNote() const;

    // return at least 6.0 for width, addresses #1502, avoids drawing velocity
    // bars that are too small to see and manipulate for extremely short notes
    // (we have encountered MIDI gear in the field that generates durations as
    // short as 0, and durations less than about 60 generate too narrow a width
    // for human manipulation)
    double getWidth() const { return (m_width >= 6.0f ? m_width : 6.0f); }
    double getElementVelocity() { return m_velocity; }

    void setSelected(bool selected);

    void setCurrent(bool current);

    /// Adjust the item to reflect the values of our event
    void reconfigure();

    /// Adjust the item to reflect the given values, not those of our event
    void reconfigure(int velocity);

    /// Adjust the item to reflect the given values, not those of our event
    void reconfigure(timeT time, timeT duration);

    /// Adjust the item to reflect the given values, not those of our event
    void reconfigure(timeT time, timeT duration, int pitch);

    static MatrixElement *getMatrixElement(QGraphicsItem *);

protected:
    MatrixScene *m_scene;
    bool m_drum;
    bool m_current;
    QGraphicsItem *m_item;
    double m_width;
    double m_velocity;

    /** Events don't know anything about what segment owns them, so neither do
     * MatrixElements.  In order to handle transposing segments properly, we
     * have to adjust the pitch relative to the segment transpose, and this can
     * only be known from the outside.  We have to take it as a construction
     * parameter, and use it appropriately when doing height calculations
     */
    long m_pitchOffset;

private:
    /// Adjust the item to reflect the given values, not those of our event
    void reconfigure(timeT time, timeT duration, int pitch, int velocity);

};


typedef ViewElementList MatrixElementList;


}

#endif
