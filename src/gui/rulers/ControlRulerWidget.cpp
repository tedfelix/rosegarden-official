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
#include "ControlRulerWidget.h"

#include "ControlRuler.h"
#include "ControllerEventsRuler.h"
#include "PropertyControlRuler.h"

#include "gui/editors/matrix/MatrixElement.h"
#include "gui/editors/matrix/MatrixViewSegment.h"
#include "gui/editors/matrix/MatrixScene.h"

#include "document/RosegardenDocument.h"
#include "base/Controllable.h"
#include "base/ControlParameter.h"
#include "base/MidiDevice.h"
#include "base/PropertyName.h"
#include "base/BaseProperties.h"
#include "base/Event.h"
#include "base/RulerScale.h"
#include "base/Selection.h"

#include "misc/Debug.h"

#include <QStackedWidget>

namespace Rosegarden
{

ControlRulerWidget::ControlRulerWidget() :
m_segment(0),
m_scene(0),
m_scale(0)
{
}

ControlRulerWidget::~ControlRulerWidget()
{
}

void ControlRulerWidget::setSegments(RosegardenDocument *document, std::vector<Segment *> segments)
{
    m_document = document;
//    m_segments = segments;

//    connect(m_document, SIGNAL(pointerPositionChanged(timeT)),
//            this, SLOT(slotPointerPositionChanged(timeT)));

    Composition &comp = document->getComposition();

    Track *track =
        comp.getTrackById(segments[0]->getTrack());

    Instrument *instr = document->getStudio().
                        getInstrumentById(track->getInstrument());

    SegmentSelection selection;
    selection.insert(segments.begin(), segments.end());

    delete m_scale;
    m_scale = new SegmentsRulerScale(&m_document->getComposition(),
                                     selection,
                                     0,
                                     Note(Note::Shortest).getDuration() / 2.0);

    // This is single segment code
    m_segment = segments[0];

    RG_DEBUG << "ControlRulerWidget::setSegments Widget contains " << m_controlRulerList.size() << " rulers.";

    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->setSegment(m_segment);
            (*it)->setRulerScale(m_scale);
        }
    }
}

void ControlRulerWidget::setScene(MatrixScene *scene)
{
    m_scene = scene;

///TEMP CODE
//    slotAddPropertyRuler(BaseProperties::VELOCITY);
    //        Controllable *c = dynamic_cast<MidiDevice *>(instr->getDevice());
    //        const ControlList &list = c->getControlParameters();
    //        RG_DEBUG << "ControlRulerWidget::setSegments - Device control parameters:";
    //        for (ControlList::const_iterator it = list.begin();it != list.end(); ++it) {
    //            if (it->getName() == "Volume")
    //                slotAddControlRuler(*it);
    //        }
///TEMP CODE END

    PropertyControlRuler *propertyruler;
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            propertyruler = dynamic_cast<PropertyControlRuler *> (*it);
            if (propertyruler) {
                propertyruler->setViewSegment(m_scene->getCurrentViewSegment());
            }
        }
    }
}

void ControlRulerWidget::slotAddRuler()
{
    ///TODO Temp code
    slotAddPropertyRuler(BaseProperties::VELOCITY);
    update();
}

void ControlRulerWidget::slotAddControlRuler(const ControlParameter &controlParameter)
{
    if (!m_scene) return;

    MatrixViewSegment *viewSegment = m_scene->getCurrentViewSegment();
    if (!viewSegment) return;

    ControlRuler *controlruler = new ControllerEventsRuler(viewSegment, m_scale, this, &controlParameter);
    addWidget(controlruler);
    m_controlRulerList.push_back(controlruler);
    controlruler->slotSetPannedRect(m_pannedRect);
}

void ControlRulerWidget::slotAddPropertyRuler(const PropertyName &propertyName)
{
    if (!m_scene) return;

    MatrixViewSegment *viewSegment = m_scene->getCurrentViewSegment();
    if (!viewSegment) return;

    ControlRuler *controlruler = new PropertyControlRuler(propertyName, viewSegment, m_scale, this);
    addWidget(controlruler);
    m_controlRulerList.push_back(controlruler);
    controlruler->slotSetPannedRect(m_pannedRect);
}

void ControlRulerWidget::slotSetPannedRect(QRectF pr)
{
    // Current Panned.cpp code uses QGraphicsView::centreOn this point
    ///TODO Note these rectangles are currently wrong
    RG_DEBUG << "ControlRulerWidget::slotSetPannedRect - " << pr;

    // Ruler widgets should draw this region (using getTimeForX from the segment) so pass the rectangle on
    // Provided rectangle should be centered on current widget size
    // No zooming yet. This will confuse things somewhat
    m_pannedRect = pr;

    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->slotSetPannedRect(pr);
        }
    }

    update();
}

void ControlRulerWidget::slotSelectionChanged(EventSelection *s)
{
    ViewElementList *selectedElements = new ViewElementList();
    ViewSegment *viewSegment = m_scene->getCurrentViewSegment();

    for (EventSelection::eventcontainer::iterator it =
            s->getSegmentEvents().begin();
            it != s->getSegmentEvents().end(); ++it) {
        MatrixElement *element = 0;
        ViewElementList::iterator vi = viewSegment->findEvent(*it);
        if (vi != viewSegment->getViewElementList()->end()) {
            element = static_cast<MatrixElement *>(*vi);
        }
        if (!element) continue;
        selectedElements->insert(element);
    }

    // Should be dispatched to all PropertyControlRulers
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            PropertyControlRuler *pr = dynamic_cast <PropertyControlRuler *> (*it);
            if (pr) {
                pr->updateSelection(selectedElements);
            }
        }
    }
}

void ControlRulerWidget::slotHoveredOverNoteChanged(int evPitch, bool haveEvent, timeT evTime)
{
    slotHoveredOverNoteChanged();
}

void ControlRulerWidget::slotHoveredOverNoteChanged()
{
    // Should be dispatched to all PropertyControlRulers
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            PropertyControlRuler *pr = dynamic_cast <PropertyControlRuler *> (*it);
            if (pr) {
                pr->updateControlItems();
            }
        }
    }
    // This is called twice for a simple note move. First time with the original position then with the new position
}

void ControlRulerWidget::slotSetToolName(const QString &toolname)
{
    // Should be dispatched to all PropertyControlRulers
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->slotSetTool(toolname);
        }
    }
}

}

#include "ControlRulerWidget.moc"
