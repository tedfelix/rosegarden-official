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

#include <QVBoxLayout>
#include <QTabBar>
#include <QStackedWidget>

namespace Rosegarden
{

ControlRulerWidget::ControlRulerWidget() :
m_controlList(0),
m_segment(0),
m_scene(0),
m_scale(0)
{
    m_tabBar = new QTabBar;
    m_tabBar->setDrawBase(false);
    m_tabBar->setShape(QTabBar::RoundedSouth);

    m_stackedWidget = new QStackedWidget;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addWidget(m_stackedWidget);
    layout->addWidget(m_tabBar);

    this->setLayout(layout);
    
    connect(m_tabBar,SIGNAL(currentChanged(int)),
            m_stackedWidget,SLOT(setCurrentIndex(int)));
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

    if (instr) {

        MidiDevice *device = dynamic_cast <MidiDevice*> (instr->getDevice());

        if (device) {
            m_controlList = &(device->getControlParameters());
        }
    }

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

void ControlRulerWidget::slotTogglePropertyRuler(const PropertyName &propertyName)
{
    PropertyControlRuler *propruler;
    std::list<ControlRuler*>::iterator it;
    for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); it++) {
        propruler = dynamic_cast <PropertyControlRuler*> (*it);
        if (propruler) {
            if (propruler->getPropertyName() == propertyName)
            {
                // We already have a ruler for this property
                if (m_stackedWidget->currentWidget() != (*it)) {
                    // It was not on show, so show it
                    m_tabBar->setCurrentTab(m_stackedWidget->indexOf(*it));
//                    m_stackedWidget->setCurrentWidget(*it);
                }
                else {
                    // It was on show, so delete it
                    removeRuler(it);
                }
                break;
            }
        }
    }
    if (it==m_controlRulerList.end()) slotAddPropertyRuler(propertyName);
}

void ControlRulerWidget::slotToggleControlRuler(std::string controlName)
{
    if (!m_controlList) return;

    ControlList::const_iterator it;
    // Check that the device supports a control parameter of this name
    for (it = m_controlList->begin();
        it != m_controlList->end(); it++) {
        if ((*it).getName() == controlName) {
            break;
        }
    }

    // If we found this control name in the list for this device
    if (it != m_controlList->end()) {
        // Check whether we already have a control ruler for a control parameter of this name
        ControllerEventsRuler *eventruler;
        std::list<ControlRuler*>::iterator jt;
        for (jt = m_controlRulerList.begin(); jt != m_controlRulerList.end(); jt++) {
            eventruler = dynamic_cast <ControllerEventsRuler*> (*jt);
            if (eventruler) {
                if (eventruler->getControlParameter()->getName() == controlName)
                {
                    // We already have a ruler for this control
                    if (m_stackedWidget->currentWidget() != (*jt)) {
                        // It was not on show, so show it
                        m_tabBar->setCurrentTab(m_stackedWidget->indexOf(*jt));
//                        m_stackedWidget->setCurrentWidget(*jt);
                    }
                    else {
                        // It was not on show, so delete it
                        removeRuler(jt);
                    }
                    break;
                }
            }
        }
        // If we don't have a control ruler, make one now
        if (jt==m_controlRulerList.end()) slotAddControlRuler(*it);
    }
}

void ControlRulerWidget::removeRuler(std::list<ControlRuler*>::iterator it)
{
    int index = m_stackedWidget->indexOf(*it);
    m_stackedWidget->removeWidget(*it);
    m_tabBar->removeTab(index);
    delete (*it);
    m_controlRulerList.erase(it);
}

void ControlRulerWidget::addRuler(ControlRuler *controlruler, QString name)
{
    m_stackedWidget->addWidget(controlruler);
    int index = m_tabBar->addTab(name.toUpper());
    m_tabBar->setCurrentTab(index);
    m_controlRulerList.push_back(controlruler);
    controlruler->slotSetPannedRect(m_pannedRect);
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
    connect(controlruler,SIGNAL(dragScroll(timeT)),
            this,SLOT(slotDragScroll(timeT)));

    addRuler(controlruler,QString::fromStdString(controlParameter.getName()));
    
    slotSetToolName(m_currentToolName);
}

void ControlRulerWidget::slotAddPropertyRuler(const PropertyName &propertyName)
{
    if (!m_scene) return;

    MatrixViewSegment *viewSegment = m_scene->getCurrentViewSegment();
    if (!viewSegment) return;

    PropertyControlRuler *controlruler = new PropertyControlRuler(propertyName, viewSegment, m_scale, this);
    controlruler->updateSelection(&m_selectedElements);

    addRuler(controlruler,QString::fromStdString(propertyName.getName()));
}

void ControlRulerWidget::slotSetPannedRect(QRectF pr)
{
    // Current Panned.cpp code uses QGraphicsView::centreOn this point
    ///TODO Note these rectangles are currently wrong
    RG_DEBUG << "ControlRulerWidget::slotSetPannedRect - " << pr;

    // Ruler widgets should draw this region (using getTimeForX from the segment) so pass the rectangle on
    // Provided rectangle should be centered on current widget size
    m_pannedRect = pr;

    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->slotSetPannedRect(pr);
        }
    }

    update();
}

void ControlRulerWidget::slotDragScroll(timeT t)
{
    emit dragScroll(t);
}

void ControlRulerWidget::slotSelectionChanged(EventSelection *s)
{
//    ViewElementList *selectedElements = new ViewElementList();
    m_selectedElements.clear();
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
        m_selectedElements.push_back(element);
    }

    // Should be dispatched to all PropertyControlRulers
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            PropertyControlRuler *pr = dynamic_cast <PropertyControlRuler *> (*it);
            if (pr) {
                pr->updateSelection(&m_selectedElements);
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
    m_currentToolName = toolname;
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
