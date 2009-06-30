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

#ifndef _RG_MATRIXELEMENT_H_
#define _RG_MATRIXELEMENT_H_

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
                  bool drum);
    virtual ~MatrixElement();

    /// Returns true if the wrapped event is a note
    bool isNote() const;

    double getWidth() const { return m_width; }
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

    /// Adjust the item to reflect the given values, not those of our event
    void reconfigure(timeT time, timeT duration, int pitch, int velocity);

    static MatrixElement *getMatrixElement(QGraphicsItem *);

protected:
    MatrixScene *m_scene;
    bool m_drum;
    bool m_current;
    QGraphicsItem *m_item;
    double m_width;
    double m_velocity;
};


typedef ViewElementList MatrixElementList;


}

#endif
