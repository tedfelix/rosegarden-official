// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef MATRIXSTAFF_H
#define MATRIXSTAFF_H

#include "linedstaff.h"
#include "matrixelement.h"

class VelocityColour;

class MatrixStaff : public LinedStaff<MatrixElement>
{
public:
    MatrixStaff(QCanvas *, Rosegarden::Segment *, int id, int vResolution,
                const Rosegarden::PropertyName &selectedProperty);
    virtual ~MatrixStaff();

protected:
    virtual int getLineCount() const;
    virtual int getLegerLineCount() const;
    virtual int getBottomLineHeight() const;
    virtual int getHeightPerLine() const;
    virtual bool elementsInSpaces() const;
    virtual bool showBeatLines() const;

    /**
     * Override from Rosegarden::Staff<T>
     * Wrap only notes 
     */
    virtual bool wrapEvent(Rosegarden::Event*);

public:
    LinedStaff<MatrixElement>::setResolution;

    double getTimeScaleFactor() const { return m_scaleFactor; }
    void setTimeScaleFactor(double f) { m_scaleFactor = f; }

    int getElementHeight() { return m_resolution; }

    virtual void positionElements(Rosegarden::timeT from,
				  Rosegarden::timeT to);

    virtual void positionElement(MatrixElement*);

    QString getNoteNameForPitch(unsigned int pitch);

    // Return this so that the tools can use it for recolouring
    // unselected elements.
    //
    VelocityColour* getVelocityColour() { return m_elementColour; }

private:
    double m_scaleFactor;

    VelocityColour *m_elementColour;
    Rosegarden::PropertyName m_selectedProperty;

};

#endif
