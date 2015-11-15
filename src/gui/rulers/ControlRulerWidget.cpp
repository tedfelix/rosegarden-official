/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ControlRulerWidget]"

#include "ControlRulerWidget.h"

#include "ControlRuler.h"
#include "ControlRulerTabBar.h"
#include "ControllerEventsRuler.h"
#include "PropertyControlRuler.h"

#include "gui/editors/matrix/MatrixElement.h"
#include "gui/editors/matrix/MatrixViewSegment.h"
#include "gui/editors/matrix/MatrixScene.h"

#include "document/RosegardenDocument.h"
#include "base/BaseProperties.h"
#include "base/ControlParameter.h"
#include "base/Controllable.h"
#include "base/Event.h"
#include "base/MidiDevice.h"
#include "base/PropertyName.h"
#include "base/RulerScale.h"
#include "base/Selection.h"
#include "base/SoftSynthDevice.h"
#include "base/parameterpattern/SelectionSituation.h"

#include "misc/Debug.h"

#include <QVBoxLayout>
//#include <QTabBar>
#include <QStackedWidget>
#include <QIcon>

namespace Rosegarden
{

ControlRulerWidget::ControlRulerWidget() :
m_controlList(0),
m_segment(0),
m_viewSegment(0),
m_scale(0)
{
    m_tabBar = new ControlRulerTabBar;

    // sizeHint() is the maximum allowed, and the widget is still useful if made
    // smaller than this, but should never grow larger
    m_tabBar->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

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
    
    connect(m_tabBar,SIGNAL(tabCloseRequest(int)),
            this,SLOT(slotRemoveRuler(int)));
}

ControlRulerWidget::~ControlRulerWidget()
{
}

void
ControlRulerWidget::setSegments(RosegardenDocument *document, std::vector<Segment *> segments)
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
        Device *device = instr->getDevice();

        // Cast to a Controllable if possible, otherwise leave c NULL.
        Controllable *c =
            dynamic_cast<MidiDevice *>(device);
        if (!c)
            { c = dynamic_cast<SoftSynthDevice *>(device); }

        if (c) {
            m_controlList = &(c->getControlParameters());
        }
    }

    SegmentSelection selection;
    selection.insert(segments.begin(), segments.end());

    delete m_scale;

    setRulerScale(new SegmentsRulerScale(&m_document->getComposition(),
            selection,
            0,
            Note(Note::Shortest).getDuration() / 2.0));
    
    // This is single segment code
    setSegment(segments[0]);
}

void
ControlRulerWidget::setSegment(Segment *segment)
{
    if (m_segment) {
        disconnect(m_segment, SIGNAL(contentsChanged(timeT, timeT)),
                this, SLOT(slotUpdateRulers(timeT, timeT)));
    }
    m_segment = segment;

    RG_DEBUG << "ControlRulerWidget::setSegments Widget contains " << m_controlRulerList.size() << " rulers.";

    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->setSegment(m_segment);
        }
    }
    if (m_segment) {
        connect(m_segment, SIGNAL(contentsChanged(timeT, timeT)),
                   this, SLOT(slotUpdateRulers(timeT, timeT)));
    }
}

void
ControlRulerWidget::setViewSegment(ViewSegment *viewSegment)
{
    m_viewSegment = viewSegment;

//    PropertyControlRuler *propertyruler;
//    if (m_controlRulerList.size()) {
    for (std::list<ControlRuler *>::iterator it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
        (*it)->setViewSegment(viewSegment);
    }
//            propertyruler = dynamic_cast<PropertyControlRuler *> (*it);
//            if (propertyruler) {
//                propertyruler->setViewSegment(m_viewSegment);
//            }
//    }
//    
//    slotTogglePropertyRuler(BaseProperties::VELOCITY);
}

void ControlRulerWidget::slotSetCurrentViewSegment(ViewSegment *viewSegment)
{
    if (viewSegment == m_viewSegment) return;
    
    setViewSegment(viewSegment);
}

void
ControlRulerWidget::setRulerScale(RulerScale *scale)
{
    setRulerScale(scale, 0);
}

void
ControlRulerWidget::setRulerScale(RulerScale *scale, int gutter)
{
    m_scale = scale;
    m_gutter = gutter;
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->setRulerScale(m_scale);
        }
    }
}

void
ControlRulerWidget::slotTogglePropertyRuler(const PropertyName &propertyName)
{
    PropertyControlRuler *propruler;
    std::list<ControlRuler*>::iterator it;
    for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
        propruler = dynamic_cast <PropertyControlRuler*> (*it);
        if (propruler) {
            if (propruler->getPropertyName() == propertyName)
            {
                // We already have a ruler for this property
                // Delete it
                removeRuler(it);
                break;
            }
        }
    }
    if (it==m_controlRulerList.end()) slotAddPropertyRuler(propertyName);
}

void
ControlRulerWidget::slotToggleControlRuler(std::string controlName)
{
    if (!m_controlList) return;

    ControlList::const_iterator it;
    // Check that the device supports a control parameter of this name
    for (it = m_controlList->begin();
        it != m_controlList->end(); ++it) {
        if ((*it).getName() == controlName) {
            break;
        }
    }

    // If we found this control name in the list for this device
    if (it != m_controlList->end()) {
        // Check whether we already have a control ruler for a control parameter of this name
        ControllerEventsRuler *eventruler;
        std::list<ControlRuler*>::iterator jt;
        for (jt = m_controlRulerList.begin(); jt != m_controlRulerList.end(); ++jt) {
            eventruler = dynamic_cast <ControllerEventsRuler*> (*jt);
            if (eventruler) {
                if (eventruler->getControlParameter()->getName() == controlName)
                {
                    // We already have a ruler for this control
                    // Delete it
                    removeRuler(jt);
                    break;
                }
            }
        }
        // If we don't have a control ruler, make one now
        if (jt == m_controlRulerList.end()) {
            slotAddControlRuler(*it);
        }
    }
}

void
ControlRulerWidget::removeRuler(std::list<ControlRuler*>::iterator it)
{
    int index = m_stackedWidget->indexOf(*it);
    m_stackedWidget->removeWidget(*it);
    m_tabBar->removeTab(index);
    delete (*it);
    m_controlRulerList.erase(it);
}

void
ControlRulerWidget::slotRemoveRuler(int index)
{
    ControlRuler *ruler = (ControlRuler*) m_stackedWidget->widget(index);
    m_stackedWidget->removeWidget(ruler);
    m_tabBar->removeTab(index);
    delete (ruler);
    m_controlRulerList.remove(ruler);
}

void
ControlRulerWidget::addRuler(ControlRuler *controlruler, QString name)
{
    m_stackedWidget->addWidget(controlruler);
    // controller names (if translatable) come from AutoLoadStrings.cpp and are
    // in the QObject context/namespace/whatever
    int index = m_tabBar->addTab(QObject::tr(name.toStdString().c_str()));
    m_tabBar->setCurrentIndex(index);
    m_controlRulerList.push_back(controlruler);
    controlruler->slotSetPannedRect(m_pannedRect);
    slotSetToolName(m_currentToolName);
}

void
ControlRulerWidget::slotAddControlRuler(const ControlParameter &controlParameter)
{
    if (!m_viewSegment) return;

    ControlRuler *controlruler = new ControllerEventsRuler(m_viewSegment, m_scale, this, &controlParameter);
    controlruler->setXOffset(m_gutter);

    connect(controlruler, SIGNAL(dragScroll(timeT)),
            this, SLOT(slotDragScroll(timeT)));

    connect(controlruler, SIGNAL(rulerSelectionChanged(EventSelection *)),
            this, SLOT(slotChildRulerSelectionChanged(EventSelection *)));

    addRuler(controlruler,QString::fromStdString(controlParameter.getName()));
    controlruler->setViewSegment(m_viewSegment);
}

void
ControlRulerWidget::slotAddPropertyRuler(const PropertyName &propertyName)
{
    if (!m_viewSegment) return;

    PropertyControlRuler *controlruler = new PropertyControlRuler(propertyName, m_viewSegment, m_scale, this);

    connect(controlruler, SIGNAL(rulerSelectionChanged(EventSelection *)),
            this, SLOT(slotChildRulerSelectionChanged(EventSelection *)));

    controlruler->setXOffset(m_gutter);
    controlruler->updateSelection(&m_selectedElements);

    // little kludge here, we only have the one property ruler, and the string
    // "velocity" wasn't already in a context (any context) where it could be
    // translated, and "velocity" doesn't look good with "PitchBend" or "Reverb"
    // so we address a number of little problems thus:
    QString name = QString::fromStdString(propertyName.getName());
    if (name == "velocity") name = tr("Velocity");
    addRuler(controlruler, name);
    // Update selection drawing in matrix view.
    emit childRulerSelectionChanged(0);
}

void
ControlRulerWidget::slotSetPannedRect(QRectF pr)
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

void
ControlRulerWidget::slotDragScroll(timeT t)
{
    emit dragScroll(t);
}

// comes from the view indicating the view's selection changed, we do NOT emit
// childRulerSelectionChanged() here
void
ControlRulerWidget::slotSelectionChanged(EventSelection *s)
{
    m_selectedElements.clear();

    // If empty selection then we will also clean the selection for control ruler
    if (s) {
    //    ViewElementList *selectedElements = new ViewElementList();


        for (EventSelection::eventcontainer::iterator it =
                s->getSegmentEvents().begin();
                it != s->getSegmentEvents().end(); ++it) {
    //        ViewElement *element = 0;
                    // TODO check if this code is necessary for some reason
                    // It seems there abundant work done here
            ViewElementList::iterator vi = m_viewSegment->findEvent(*it);
    //        if (vi != m_viewSegment->getViewElementList()->end()) {
    //            element = dynamic_cast<ViewElement *>(*vi);
    //        }
    //        if (!element) continue;
            m_selectedElements.push_back(*vi);
        }
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

void
ControlRulerWidget::slotHoveredOverNoteChanged(int /* evPitch */, bool /* haveEvent */, timeT /* evTime */)
{
    slotHoveredOverNoteChanged();
}

void
ControlRulerWidget::slotHoveredOverNoteChanged()
{
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            PropertyControlRuler *pr = dynamic_cast <PropertyControlRuler *> (*it);
            if (pr) pr->updateSelectedItems();
        }
    }
}

void
ControlRulerWidget::slotUpdateRulers(timeT startTime, timeT endTime)
{
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->notationLayoutUpdated(startTime,endTime);
        }
    }
}

void
ControlRulerWidget::slotSetToolName(const QString &toolname)
{
    QString rulertoolname = toolname;
    // Translate Notation tool names
    if (toolname == "notationselector") rulertoolname = "selector";
    if (toolname == "notationselectornoties") rulertoolname = "selector";
    if (toolname == "noterestinserter") rulertoolname = "painter";
    if (toolname == "notationeraser") rulertoolname = "eraser";
    
    m_currentToolName = rulertoolname;
    // Should be dispatched to all PropertyControlRulers
    if (m_controlRulerList.size()) {
        std::list<ControlRuler *>::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->slotSetTool(rulertoolname);
        }
    }
}

void
ControlRulerWidget::slotChildRulerSelectionChanged(EventSelection *s)
{
    emit childRulerSelectionChanged(s);
}

bool
ControlRulerWidget::isAnyRulerVisible()
{
    return m_controlRulerList.size();
}

ControllerEventsRuler *
ControlRulerWidget::getActiveRuler(void)
{
    QWidget * widget = m_stackedWidget->currentWidget ();
    if (!widget) { return 0; }
    return dynamic_cast <ControllerEventsRuler *> (widget);
}

bool
ControlRulerWidget::hasSelection(void)
{
    ControllerEventsRuler *ruler = getActiveRuler();
    if (!ruler) { return false; }
    return ruler->getEventSelection() ? true : false;
}

// Return the active ruler's event selection, or NULL if none.
// @author Tom Breton (Tehom)
EventSelection *
ControlRulerWidget::getSelection(void)
{
    ControllerEventsRuler *ruler = getActiveRuler();
    if (!ruler) { return 0; }
    return ruler->getEventSelection();
}

ControlParameter *
ControlRulerWidget::getControlParameter(void)
{
    ControllerEventsRuler *ruler = getActiveRuler();
    if (!ruler) { return 0; }
    return ruler->getControlParameter();
}

// @return the active ruler's parameter situation, or NULL if none.
// Return is owned by caller.
// @author Tom Breton (Tehom)
SelectionSituation *
ControlRulerWidget::getSituation(void)
{
    ControllerEventsRuler *ruler = getActiveRuler();
    if (!ruler) { return 0; }
    EventSelection * selection = ruler->getEventSelection();
    if (!selection) { return 0; }
    ControlParameter * cp = ruler->getControlParameter();
    if (!cp) { return 0; }
    return
        new SelectionSituation(cp->getType(), selection);
}

}

