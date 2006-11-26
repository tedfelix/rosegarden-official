/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MatrixStaff.h"
#include "misc/Debug.h"

#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Event.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SnapGrid.h"
#include "base/Staff.h"
#include "base/Track.h"
#include "base/ViewElement.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/LinedStaff.h"
#include "gui/rulers/DefaultVelocityColour.h"
#include "MatrixElement.h"
#include "MatrixView.h"
#include "MatrixVLayout.h"
#include <qcanvas.h>


namespace Rosegarden
{

MatrixStaff::MatrixStaff(QCanvas *canvas,
                         Segment *segment,
                         SnapGrid *snapGrid,
                         int id,
                         int vResolution,
                         MatrixView *view) :
        LinedStaff(canvas, segment, snapGrid, id, vResolution, 1),
        m_scaleFactor(2.0 /
                      Note(Note::Shortest).getDuration()),
        m_view(view)
{}

MatrixStaff::~MatrixStaff()
{
    // nothing
}

int MatrixStaff::getLineCount() const
{
    //    MATRIX_DEBUG << "MatrixStaff::getLineCount: isDrumMode " << m_view->isDrumMode() << ", key mapping " << (getKeyMapping() ? getKeyMapping()->getName() : "<none>") << endl;

    if (m_view->isDrumMode()) {
        const MidiKeyMapping *km = getKeyMapping();
        if (km)
            return km->getPitchExtent() + 1;
    }
    return MatrixVLayout::maxMIDIPitch + 2;
}

int MatrixStaff::getLegerLineCount() const
{
    return 0;
}

int MatrixStaff::getBottomLineHeight() const
{
    if (m_view->isDrumMode()) {
        const MidiKeyMapping *km = getKeyMapping();
        if (km)
            return km->getPitchForOffset(0);
    }
    return 0;
}

int MatrixStaff::getHeightPerLine() const
{
    return 1;
}

bool MatrixStaff::elementsInSpaces() const
{
    return true;
}

bool MatrixStaff::showBeatLines() const
{
    return true;
}

bool MatrixStaff::wrapEvent(Event* e)
{
    // Changed from "Note or Time signature" to just "Note" because
    // there should be no time signature events in any ordinary
    // segments, they're only in the composition's ref segment

    return e->isa(Note::EventType) &&
           Staff::wrapEvent(e);
}

void
MatrixStaff::positionElements(timeT from, timeT to)
{
    MatrixElementList *mel = getViewElementList();

    MatrixElementList::iterator beginAt = mel->findTime(from);
    if (beginAt != mel->begin())
        --beginAt;

    MatrixElementList::iterator endAt = mel->findTime(to);

    for (MatrixElementList::iterator i = beginAt; i != endAt; ++i) {
        positionElement(*i);
    }
}

void MatrixStaff::positionElement(ViewElement* vel)
{
    MatrixElement* el = dynamic_cast<MatrixElement*>(vel);

    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords
                              (el->getLayoutX(), int(el->getLayoutY()));

    // Get velocity for colouring
    //
    using BaseProperties::VELOCITY;
    long velocity = 127;
    if (el->event()->has(VELOCITY))
        el->event()->get
        <Int>(VELOCITY, velocity);

    el->setCanvas(m_canvas);

    // Is the event currently selected?  Colour accordingly.
    //
    EventSelection *selection = m_view->getCurrentSelection();

    if (selection && selection->contains(el->event()))
        el->setColour(GUIPalette::getColour(GUIPalette::SelectedElement));
    else if (el->event()->has(BaseProperties::TRIGGER_SEGMENT_ID))
        el->setColour(Qt::gray);
    else
        el->setColour(DefaultVelocityColour::getInstance()->getColour(velocity));

    el->setCanvasX(coords.first);
    el->setCanvasY((double)coords.second);
}

MatrixElement*
MatrixStaff::getElement(Event *event)
{
    ViewElementList::iterator i = findEvent(event);
    if (i == m_viewElementList->end())
        return 0;
    return dynamic_cast<MatrixElement*>(*i);
}

void
MatrixStaff::eventRemoved(const Segment *segment,
                          Event *event)
{
    LinedStaff::eventRemoved(segment, event);
    m_view->handleEventRemoved(event);
}

ViewElement*
MatrixStaff::makeViewElement(Event* e)
{
    return new MatrixElement(e, m_view->isDrumMode());
}

const MidiKeyMapping*
MatrixStaff::getKeyMapping() const
{
    Composition *comp = getSegment().getComposition();
    if (!comp)
        return 0;
    TrackId trackId = getSegment().getTrack();
    Track *track = comp->getTrackById(trackId);
    Instrument *instr = m_view->getDocument()->getStudio().
                        getInstrumentById(track->getInstrument());
    if (!instr)
        return 0;
    return m_view->getKeyMapping();
}

}
