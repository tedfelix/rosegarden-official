/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

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

#include "document/RosegardenDocument.h"
#include "base/Composition.h"
#include "base/ControlParameter.h"
#include "base/Controllable.h"
#include "base/Device.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"  // for PitchBend::EventType
#include "base/PropertyName.h"
#include "gui/application/RosegardenMainWindow.h"
#include "base/Selection.h"  // for EventSelection
#include "base/parameterpattern/SelectionSituation.h"
#include "base/SoftSynthDevice.h"
#include "base/Track.h"

#include "misc/Debug.h"

#include <QVBoxLayout>
#include <QStackedWidget>


namespace Rosegarden
{


ControlRulerWidget::ControlRulerWidget() :
    m_viewSegment(nullptr),
    m_controlRulerList(),
    m_scale(nullptr),
    m_gutter(0),
    m_currentToolName(),
    m_pannedRect(),
    m_selectedElements()
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);

    // Stacked Widget
    m_stackedWidget = new QStackedWidget;
    layout->addWidget(m_stackedWidget);

    // Tab Bar
    m_tabBar = new ControlRulerTabBar;

    // sizeHint() is the maximum allowed, and the widget is still useful if made
    // smaller than this, but should never grow larger
    m_tabBar->setSizePolicy(
            QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    m_tabBar->setDrawBase(false);
    m_tabBar->setShape(QTabBar::RoundedSouth);

    layout->addWidget(m_tabBar);
    
    connect(m_tabBar, &QTabBar::currentChanged,
            m_stackedWidget, &QStackedWidget::setCurrentIndex);
    
    connect(m_tabBar, &ControlRulerTabBar::tabCloseRequest,
            this, &ControlRulerWidget::slotRemoveRuler);
}

void
ControlRulerWidget::setSegments(std::vector<Segment *> /*segments*/)
{
    // ??? This is needed for the Segment ruler list.  It
    //     will allow us to update the ruler list for each Segment when
    //     rulers are added or removed.
    //m_segments = segments;

    // ??? Need to launch all rulers from the Segments' ruler lists.
}

void
ControlRulerWidget::setViewSegment(ViewSegment *viewSegment)
{
    // No change?  Bail.
    if (viewSegment == m_viewSegment)
        return;

    // Disconnect the previous Segment from updates.
    // ??? Why doesn't the ruler's setViewSegment() do this for us?
    if (m_viewSegment) {
        Segment *segment = &(m_viewSegment->getSegment());
        if (segment) {
            disconnect(segment, &Segment::contentsChanged,
                       this, &ControlRulerWidget::slotUpdateRulers);
        }
    }

    m_viewSegment = viewSegment;

    // For each ruler, set the ViewSegment.
    for (ControlRuler *ruler : m_controlRulerList) {
        ruler->setViewSegment(viewSegment);
    }

    // Connect current Segment for updates.
    // ??? Why doesn't the ruler's setViewSegment() do this for us?
    if (viewSegment) {
        Segment *segment = &(viewSegment->getSegment());
        if (segment) {
            connect(segment, &Segment::contentsChanged,
                    this, &ControlRulerWidget::slotUpdateRulers);
        }
    }
}

void ControlRulerWidget::slotSetCurrentViewSegment(ViewSegment *viewSegment)
{
    // No change?  Bail.
    if (viewSegment == m_viewSegment)
        return;

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
        ControlRulerList::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->setRulerScale(m_scale);
        }
    }
}

void
ControlRulerWidget::togglePropertyRuler(const PropertyName &propertyName)
{
    PropertyControlRuler *propruler;
    ControlRulerList::iterator it;
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
    if (it==m_controlRulerList.end()) addPropertyRuler(propertyName);
}

bool hasPitchBend(Segment *segment)
{
    RosegardenDocument *document = RosegardenMainWindow::self()->getDocument();

    Track *track =
            document->getComposition().getTrackById(segment->getTrack());

    Instrument *instr = document->getStudio().
        getInstrumentById(track->getInstrument());

    if (!instr)
        return false;

    Controllable *c = instr->getDevice()->getControllable();

    if (!c)
        return false;

    // Check whether the device has a pitchbend controller
    for (const ControlParameter &cp : c->getControlParameters()) {
        if (cp.getType() == PitchBend::EventType)
            return true;
    }

    return false;
}

void
ControlRulerWidget::togglePitchBendRuler()
{
    // No pitch bend?  Bail.
    // ??? Rude.  We should gray the menu item instead of this.
    if (!hasPitchBend(&(m_viewSegment->getSegment())))
        return;

    // Check whether we already have a pitchbend ruler

    // For each ruler
    for (ControlRulerList::iterator rulerIter = m_controlRulerList.begin();
         rulerIter != m_controlRulerList.end();
         ++rulerIter) {
        ControllerEventsRuler *eventRuler =
                dynamic_cast<ControllerEventsRuler*>(*rulerIter);

        // Not a ControllerEventsRuler?  Try the next one.
        if (!eventRuler)
            continue;

        // If we already have a pitchbend ruler, remove it.
        if (eventRuler->getControlParameter()->getType() ==
                PitchBend::EventType)
        {
            removeRuler(rulerIter);
            return;
        }
    }

    // We don't already have a pitchbend ruler, make one now.
    addControlRuler(ControlParameter::getPitchBend());
}

void
ControlRulerWidget::removeRuler(ControlRulerList::iterator it)
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
ControlRulerWidget::addControlRuler(const ControlParameter &controlParameter)
{
    if (!m_viewSegment) return;

    ControlRuler *controlruler = new ControllerEventsRuler(m_viewSegment, m_scale, this, &controlParameter);
    controlruler->setXOffset(m_gutter);

    connect(controlruler, &ControlRuler::dragScroll,
            this, &ControlRulerWidget::slotDragScroll);

    // Mouse signals.  Forward them from the current ControlRuler.
    connect(controlruler, &ControlRuler::mousePress,
            this, &ControlRulerWidget::mousePress);
    connect(controlruler, &ControlRuler::mouseMove,
            this, &ControlRulerWidget::mouseMove);
    connect(controlruler, &ControlRuler::mouseRelease,
            this, &ControlRulerWidget::mouseRelease);

    connect(controlruler, &ControlRuler::rulerSelectionChanged,
            this, &ControlRulerWidget::slotChildRulerSelectionChanged);

    addRuler(controlruler,QString::fromStdString(controlParameter.getName()));
    controlruler->setViewSegment(m_viewSegment);
}

void
ControlRulerWidget::addPropertyRuler(const PropertyName &propertyName)
{
    if (!m_viewSegment) return;

    PropertyControlRuler *controlruler = new PropertyControlRuler(
            propertyName,
            m_viewSegment,  // viewSegment
            m_scale,  // scale
            this);  // parent

    connect(controlruler, &ControlRuler::rulerSelectionChanged,
            this, &ControlRulerWidget::slotChildRulerSelectionChanged);

    connect(controlruler, &ControlRuler::showContextHelp,
            this,  &ControlRulerWidget::showContextHelp);

    controlruler->setXOffset(m_gutter);
    controlruler->updateSelection(m_selectedElements);

    // little kludge here, we only have the one property ruler, and the string
    // "velocity" wasn't already in a context (any context) where it could be
    // translated, and "velocity" doesn't look good with "PitchBend" or "Reverb"
    // so we address a number of little problems thus:
    QString name = QString::fromStdString(propertyName.getName());
    if (name == "velocity") name = tr("Velocity");
    addRuler(controlruler, name);
    // Update selection drawing in matrix view.
    emit childRulerSelectionChanged(nullptr);
}

void
ControlRulerWidget::slotSetPannedRect(QRectF pr)
{
    // Current Panned.cpp code uses QGraphicsView::centreOn this point
    ///TODO Note these rectangles are currently wrong
    //RG_DEBUG << "slotSetPannedRect():" << pr;

    // Ruler widgets should draw this region (using getTimeForX from the segment) so pass the rectangle on
    // Provided rectangle should be centered on current widget size
    m_pannedRect = pr;

    if (m_controlRulerList.size()) {
        ControlRulerList::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            (*it)->slotSetPannedRect(pr);
        }
    }

    update();
}

void
ControlRulerWidget::slotDragScroll(timeT /*t*/)
{
    // ??? This is nothing.  Remove whoever was connecting to it.
    //     ControlRuler::dragScroll() was being connected to this.
    //     Get rid of the connection and see about getting rid of
    //     the signal.

    //emit dragScroll(t);
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
        ControlRulerList::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            PropertyControlRuler *pr = dynamic_cast <PropertyControlRuler *> (*it);
            if (pr) {
                pr->updateSelection(m_selectedElements);
            }
        }
    }
}

void
ControlRulerWidget::slotHoveredOverNoteChanged(int /* evPitch */, bool /* haveEvent */, timeT /* evTime */)
{
    // ??? Since all parameters are unused, can we change the signal we
    //     connect to (MatrixMover::hoveredOverNoteChanged) to have no
    //     parameters?  I think we can.

//    RG_DEBUG << "slotHoveredOverNoteChanged()";

    // ??? What does this routine even do.  At first I thought it made sure
    //     that the velocity bars would move as the notes are dragged around
    //     on the matrix.  But it does not.  And that makes sense since the
    //     dragging is a temporary change to the view that doesn't change
    //     the underlying Segment until the mouse is released.  So we
    //     wouldn't expect to see the velocity bars moving around during
    //     the drag.
    //
    //     I've removed the code below for now to see if anyone notices.

#if 0
    // ??? This code doesn't appear to do anything.  Removing to see if
    //     anyone notices.
    if (m_controlRulerList.size()) {
        ControlRulerList::iterator it;
        for (it = m_controlRulerList.begin(); it != m_controlRulerList.end(); ++it) {
            PropertyControlRuler *pr = dynamic_cast <PropertyControlRuler *> (*it);
            if (pr) pr->updateSelectedItems();
        }
    }
#endif
}

void
ControlRulerWidget::slotUpdateRulers(timeT startTime, timeT endTime)
{
    // No rulers to update?  Bail.
    if (m_controlRulerList.empty())
        return;

    // For each ruler, ask for an update.
    for (ControlRulerList::iterator it = m_controlRulerList.begin();
         it != m_controlRulerList.end();
         ++it) {
        (*it)->notationLayoutUpdated(startTime, endTime);
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
        ControlRulerList::iterator it;
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
ControlRulerWidget::getActiveRuler()
{
    QWidget * widget = m_stackedWidget->currentWidget ();
    if (!widget) { return nullptr; }
    return dynamic_cast <ControllerEventsRuler *> (widget);
}

PropertyControlRuler *
ControlRulerWidget::getActivePropertyRuler()
{
    QWidget * widget = m_stackedWidget->currentWidget ();
    if (!widget) { return nullptr; }
    return dynamic_cast <PropertyControlRuler *> (widget);
}

bool
ControlRulerWidget::hasSelection()
{
    ControllerEventsRuler *ruler = getActiveRuler();
    if (!ruler) { return false; }
    return ruler->getEventSelection() ? true : false;
}

// Return the active ruler's event selection, or nullptr if none.
// @author Tom Breton (Tehom)
EventSelection *
ControlRulerWidget::getSelection()
{
    ControllerEventsRuler *ruler = getActiveRuler();
    if (!ruler) { return nullptr; }
    return ruler->getEventSelection();
}

ControlParameter *
ControlRulerWidget::getControlParameter()
{
    ControllerEventsRuler *ruler = getActiveRuler();
    if (!ruler) { return nullptr; }
    return ruler->getControlParameter();
}

// @return the active ruler's parameter situation, or nullptr if none.
// Return is owned by caller.
// @author Tom Breton (Tehom)
SelectionSituation *
ControlRulerWidget::getSituation()
{
    ControllerEventsRuler *ruler = getActiveRuler();
    if (!ruler) { return nullptr; }
    EventSelection * selection = ruler->getEventSelection();
    if (!selection) { return nullptr; }
    ControlParameter * cp = ruler->getControlParameter();
    if (!cp) { return nullptr; }
    return
        new SelectionSituation(cp->getType(), selection);
}


}
