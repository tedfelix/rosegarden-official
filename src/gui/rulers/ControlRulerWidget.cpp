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
    //     rulers are added or removed.  I believe the only other part
    //     of the system that knows this is the scene.  Probably not
    //     easier to get it from there given where this class is.
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

    // For each ruler, set the ruler scale.
    for (ControlRuler *ruler : m_controlRulerList) {
        ruler->setRulerScale(scale);
    }
}

void
ControlRulerWidget::togglePropertyRuler(const PropertyName &propertyName)
{
    // Toggle the *velocity* ruler.  As of 2021 there is only one property
    // ruler, the velocity ruler.

    // For each ruler...
    for (ControlRuler *ruler : m_controlRulerList) {
        PropertyControlRuler *propruler =
                dynamic_cast <PropertyControlRuler *> (ruler);
        // Not a property ruler?  Try the next one.
        if (!propruler)
            continue;

        // Found it?  Remove and bail.
        if (propruler->getPropertyName() == propertyName)
        {
            removeRuler(ruler);
            return;
        }
    }

    // Not found, add it.
    addPropertyRuler(propertyName);
}

bool hasPitchBend(Segment *segment)
{
    RosegardenDocument *document = RosegardenMainWindow::self()->getDocument();

    Track *track =
            document->getComposition().getTrackById(segment->getTrack());

    Instrument *instrument = document->getStudio().
        getInstrumentById(track->getInstrument());

    if (!instrument)
        return false;

    Controllable *controllable = instrument->getDevice()->getControllable();

    if (!controllable)
        return false;

    // Check whether the device has a pitchbend controller
    for (const ControlParameter &cp : controllable->getControlParameters()) {
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

    // For each ruler...
    for (ControlRuler *ruler : m_controlRulerList) {
        ControllerEventsRuler *eventRuler =
                dynamic_cast<ControllerEventsRuler*>(ruler);

        // Not a ControllerEventsRuler?  Try the next one.
        if (!eventRuler)
            continue;

        // If we already have a pitchbend ruler, remove it.
        if (eventRuler->getControlParameter()->getType() ==
                PitchBend::EventType)
        {
            removeRuler(ruler);
            return;
        }
    }

    // We don't already have a pitchbend ruler, make one now.
    addControlRuler(ControlParameter::getPitchBend());
}

void
ControlRulerWidget::removeRuler(ControlRuler *ruler)
{
    // Remove from the stacked widget.
    int index = m_stackedWidget->indexOf(ruler);
    m_stackedWidget->removeWidget(ruler);

    // Remove from the tabs.
    m_tabBar->removeTab(index);

    // Close the ruler window.
    delete ruler;

    // Remove from the list.
    m_controlRulerList.remove(ruler);
}

void
ControlRulerWidget::slotRemoveRuler(int index)
{
    ControlRuler *ruler =
            dynamic_cast<ControlRuler *>(m_stackedWidget->widget(index));

    removeRuler(ruler);
}

void
ControlRulerWidget::addRuler(ControlRuler *controlruler, QString name)
{
    // Add to the stacked widget.
    m_stackedWidget->addWidget(controlruler);

    // Add to tabs.
    // (Controller names, if translatable, come from AutoLoadStrings.cpp and are
    // in the QObject context/namespace/whatever.)
    const int index = m_tabBar->addTab(QObject::tr(name.toStdString().c_str()));
    m_tabBar->setCurrentIndex(index);

    // Add to ruler list.
    m_controlRulerList.push_back(controlruler);

    // Configure the ruler.
    controlruler->slotSetPannedRect(m_pannedRect);
    slotSetTool(m_currentToolName);
}

void
ControlRulerWidget::addControlRuler(const ControlParameter &controlParameter)
{
    // If we're not editing a ViewSegment, bail.
    if (!m_viewSegment)
        return;

    ControlRuler *controlRuler = new ControllerEventsRuler(
            m_viewSegment, m_scale, this, &controlParameter);

    controlRuler->setXOffset(m_gutter);

    // Mouse signals.  Forward them from the current ControlRuler.
    connect(controlRuler, &ControlRuler::mousePress,
            this, &ControlRulerWidget::mousePress);
    connect(controlRuler, &ControlRuler::mouseMove,
            this, &ControlRulerWidget::mouseMove);
    connect(controlRuler, &ControlRuler::mouseRelease,
            this, &ControlRulerWidget::mouseRelease);

    connect(controlRuler, &ControlRuler::rulerSelectionChanged,
            this, &ControlRulerWidget::slotChildRulerSelectionChanged);

    addRuler(controlRuler, QString::fromStdString(controlParameter.getName()));

    // ??? This is required or else we crash.  But we already passed this in
    //     in the ctor call.  Can we fix this so that we only pass it once?
    //     Preferably in the ctor call.  PropertyControlRuler appears to do
    //     this successfully.  See if we can follow its example.
    controlRuler->setViewSegment(m_viewSegment);
}

void
ControlRulerWidget::addPropertyRuler(const PropertyName &propertyName)
{
    // Note that as of 2021 there is only one property ruler, the
    // velocity ruler.

    // If we're not editing a ViewSegment, bail.
    if (!m_viewSegment)
        return;

    PropertyControlRuler *controlruler = new PropertyControlRuler(
            propertyName,
            m_viewSegment,  // viewSegment
            m_scale,  // scale
            this);  // parent

    // ??? The velocity ruler does not yet support selection, so this
    //     actually does nothing right now.
    connect(controlruler, &ControlRuler::rulerSelectionChanged,
            this, &ControlRulerWidget::slotChildRulerSelectionChanged);

    connect(controlruler, &ControlRuler::showContextHelp,
            this,  &ControlRulerWidget::showContextHelp);

    controlruler->setXOffset(m_gutter);
    controlruler->updateSelection(m_selectedElements);

    // Little kludge here.  We only have the one property ruler (velocity),
    // and the string "velocity" wasn't already in a context (any context)
    // where it could be translated, and "velocity" doesn't look good with
    // "PitchBend" or "Reverb", so we ask for an explicit tr() here.
    QString name = QString::fromStdString(propertyName.getName());
    if (name == "velocity")
        name = tr("Velocity");

    addRuler(controlruler, name);

    // Update selection drawing in matrix view.
    emit childRulerSelectionChanged(nullptr);
}

void
ControlRulerWidget::slotSetPannedRect(QRectF pannedRect)
{
    m_pannedRect = pannedRect;

    // For each ruler, pass on the panned rect.
    for (ControlRuler *ruler : m_controlRulerList) {
        ruler->slotSetPannedRect(pannedRect);
    }

    update();
}

void
ControlRulerWidget::slotSelectionChanged(EventSelection *eventSelection)
{
    m_selectedElements.clear();

    if (eventSelection) {
        // Convert the EventSelection into a vector of ViewElement *.

        // For each event in the new EventSelection...
        for (Event *event : eventSelection->getSegmentEvents()) {
            // Find the corresponding ViewElement.
            // TODO check if this code is necessary for some reason
            //      It seems there abundant work done here
            // ??? Performance: Search within for loop.
            ViewElementList::iterator viewElementIter =
                    m_viewSegment->findEvent(event);
            // Add it to m_selectedElements.
            m_selectedElements.push_back(*viewElementIter);
        }
    }

    // Send new selection to all PropertyControlRulers.  IOW the velocity ruler.
    // For each ruler...
    for (ControlRuler *ruler : m_controlRulerList) {
        PropertyControlRuler *propertyRuler =
                dynamic_cast<PropertyControlRuler *>(ruler);
        // Is this the velocity ruler?  Then pass on the selection.
        if (propertyRuler)
            propertyRuler->updateSelection(m_selectedElements);
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
    // For each ruler, ask for an update.
    for (ControlRuler *ruler : m_controlRulerList) {
        ruler->notationLayoutUpdated(startTime, endTime);
    }
}

void
ControlRulerWidget::slotSetTool(const QString &toolName)
{
    QString rulerToolName = toolName;

    // Translate Notation tool names to ruler tool names.
    if (toolName == "notationselector")
        rulerToolName = "selector";
    if (toolName == "notationselectornoties")
        rulerToolName = "selector";
    if (toolName == "noterestinserter")
        rulerToolName = "painter";
    if (toolName == "notationeraser")
        rulerToolName = "eraser";

    m_currentToolName = rulerToolName;

    // Dispatch to all rulers.
    for (ControlRuler *ruler : m_controlRulerList) {
        ruler->slotSetTool(rulerToolName);
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
