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

#include <qcanvas.h>

#include "matrixstaff.h"
#include "matrixvlayout.h"
#include "velocitycolour.h"
#include "matrixview.h"

#include "Segment.h"
#include "BaseProperties.h"
#include "Composition.h"
#include "Selection.h"


using Rosegarden::Segment;
using Rosegarden::SnapGrid;
using Rosegarden::timeT;

MatrixStaff::MatrixStaff(QCanvas *canvas,
                         Segment *segment,
                         SnapGrid *snapGrid,
			 int id,
                         int vResolution,
                         MatrixView *view) :
    LinedStaff<MatrixElement>(canvas, segment, snapGrid, id, vResolution, 1),
    m_scaleFactor
        (1.5 / Rosegarden::Note(Rosegarden::Note::Shortest).getDuration()),
    m_elementColour(0),
    m_view(view)
{

    // Create a velocity colouring object
    //
    m_elementColour = new VelocityColour(
                            RosegardenGUIColours::LevelMeterRed,
                            RosegardenGUIColours::LevelMeterOrange,
                            RosegardenGUIColours::LevelMeterGreen,
                            127, // max knee
                            115, // red knee
                            75,  // orange knee
                            25); // green knee

 
}

MatrixStaff::~MatrixStaff()
{
    // nothing
}

int  MatrixStaff::getLineCount()        const { return MatrixVLayout::
						      maxMIDIPitch + 1; }
int  MatrixStaff::getLegerLineCount()   const { return 0; }
int  MatrixStaff::getBottomLineHeight() const { return 0; }
int  MatrixStaff::getHeightPerLine()    const { return 1; }
bool MatrixStaff::elementsInSpaces()    const { return true; }
bool MatrixStaff::showBeatLines()       const { return true; }

bool MatrixStaff::wrapEvent(Rosegarden::Event* e)
{
    // Changed from "Note or Time signature" to just "Note" because 
    // there should be no time signature events in any ordinary
    // segments, they're only in the composition's ref segment

    return e->isa(Rosegarden::Note::EventType) &&
	Rosegarden::Staff<MatrixElement>::wrapEvent(e);
}

void
MatrixStaff::positionElements(timeT from, timeT to)
{
    MatrixElementList *mel = getViewElementList();

    MatrixElementList::iterator beginAt = mel->findTime(from);
    if (beginAt != mel->begin()) --beginAt;

    MatrixElementList::iterator endAt = mel->findTime(to);

    for (MatrixElementList::iterator i = beginAt; i != endAt; ++i) {
	positionElement(*i);
    }
}

void MatrixStaff::positionElement(MatrixElement* el)
{
    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
	(el->getLayoutX(), int(el->getLayoutY()));

    // Get velocity for colouring
    //
    using Rosegarden::BaseProperties::VELOCITY;
    long velocity = 127;
    if (el->event()->has(VELOCITY))
        el->event()->get<Rosegarden::Int>(VELOCITY, velocity);

    el->setCanvas(m_canvas);

    // Is the event currently selected?  Colour accordingly.
    //

/*!!! This property is no longer set -- need to get hold of the
      current EventSelection from somewhere and test whether
      selection->contains(el->event())

    if (el->event()->has(m_selectedProperty))
        el->setColour(RosegardenGUIColours::SelectedElement);
    else*/

    Rosegarden::EventSelection *selection = m_view->getCurrentSelection();

    if (selection && selection->contains(el->event()))
        el->setColour(RosegardenGUIColours::SelectedElement);
    else
        el->setColour(m_elementColour->getColour(velocity));

    el->setCanvasX(coords.first);
    el->setCanvasY((double)coords.second);
}


QString MatrixStaff::getNoteNameForPitch(unsigned int pitch)
{
    static const char* noteNamesSharps[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    int octave = pitch / 12;
    pitch  = pitch % 12;

    return QString("%1%2").arg(noteNamesSharps[pitch]).arg(octave - 2);
}

MatrixElement*
MatrixStaff::getElement(Rosegarden::Event *event)
{
    return dynamic_cast<MatrixElement*>((*findEvent(event)));
}


