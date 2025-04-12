/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PitchTrackerView]"

#include "PitchTrackerView.h"
#include "PitchGraphWidget.h"
#include "PitchHistory.h"

#ifdef HAVE_LIBJACK
#include "sound/JackCaptureClient.h"
#endif
#include "sound/PitchDetector.h"

// Need to find actions for <<< and >>> transport buttons
#include "gui/general/ActionFileClient.h"
// Might need the default pitch analysis frame size and overlap
#include "gui/configuration/PitchTrackerConfigurationPage.h"
#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
// to add PitchGraphWidget to display
#include "gui/editors/notation/NotationWidget.h"
// to get the notes in the composition
#include "gui/editors/notation/NotationScene.h"
#include "gui/editors/notation/NotationStaff.h"
#include "document/RosegardenDocument.h"
#include "base/ViewSegment.h"
#include "base/RealTime.h"

#include <QMessageBox>
#include <QSettings>
#include <QWidget>
#include <QAction>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>
#include <QVector>
#include <QActionGroup>

#include <QDebug>

#define DEBUG_PRINT_ALL 0

namespace Rosegarden
{


/*************************************
  CONSTRUCTOR, DESTRUCTOR, AND INIT
 *************************************/
PitchTrackerView::PitchTrackerView(RosegardenDocument *doc,
                                   const std::vector<Segment *>& segments) :
        NotationView(doc, segments),
        m_doc(doc),
        m_jackCaptureClient(nullptr),
        m_jackConnected(false),
        m_pitchDetector(nullptr),
        m_running(false)
{
    QSettings settings;
    settings.beginGroup(PitchTrackerConfigGroup);
    m_framesize = settings.value("framesize",
                                 PitchDetector::defaultFrameSize).toInt();
    m_stepsize = settings.value("stepsize",
                                PitchDetector::defaultStepSize).toInt();

    int tuning = settings.value("tuning", 0).toInt();
    int method = settings.value("method", 0).toInt();
    settings.endGroup();

    // The frame size and step size are set from the default properties
    // of PitchDetector or by reading the settings file. In principle,
    // the latter method is open to attack by someone editing the settings
    // file and entering some silly values to try and make the
    // JackCaptureClient's memory allocation fail. So we should check that
    // here and at least print a warning.
    if (m_framesize > 65536 || m_framesize < 64 ||
        m_stepsize > m_framesize || m_stepsize < m_framesize/16) {
        RG_WARNING << "PitchTrackerView: instantiation has these parameters: "
                     "Framesize:" << m_framesize <<
                     "Step size:" << m_stepsize <<
                     ". This seems rather unlikely; will continue anyway. "
                     "(fingers crossed!)";
    }

    // Find the current tuning index in use by the pitch tracker
    // The static method Tuning::getTunings() returns a pointer to a
    // std::vector of pointers to tunings. i.e.
    //     std::vector<Rosegarden::Accidentals::Tuning*>* getTunings(void);
    // The one we want is obtained be dereferencing the return value of
    // getTunings to obtain the std::vector, then indexing by the quantity
    // retreived above. So the following looks nasty, but that's C++ for you.
    const std::vector<std::shared_ptr<Accidentals::Tuning>> *availableTunings =
        Accidentals::Tuning::getTunings();

    if (availableTunings) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        m_availableTunings =
            QVector<std::shared_ptr<Accidentals::Tuning>>(
                    availableTunings->begin(),
                    availableTunings->end());
#else
        m_availableTunings =
            QVector<std::shared_ptr<Accidentals::Tuning>>::fromStdVector(*availableTunings);
#endif

        if (tuning < 0 ||
            static_cast<unsigned int>(tuning) >= availableTunings->size()) {
            // Illegal index into available tunings (how??) -> use default.
            tuning = 0;
        }
        m_tuning = m_availableTunings[tuning];
    } else {
        m_tuning = nullptr;
        RG_WARNING << "WARNING: No available tunings!";
    }

    m_pitchGraphWidget = new PitchGraphWidget(m_history);
    m_pitchGraphWidget->setTuning(m_tuning);
    // m_notationWidget is owned by our parent, NotationView
    m_notationWidget->addWidgetToBottom(m_pitchGraphWidget);

#ifdef HAVE_LIBJACK
    // jack capture client
    m_jackCaptureClient =
        new JackCaptureClient("Rosegarden PitchTracker",
                              m_framesize + m_stepsize);

    if (m_jackCaptureClient->isConnected()) {
        m_jackConnected = true;
    } else
#endif
    {
        QMessageBox::critical(this, "",
                              tr("Cannot connect to jack! "
                              "Ensure jack server is running and no other "
                              "tracker clients are open."));
        return;
    }

#ifdef HAVE_LIBJACK
    int sampleRate = m_jackCaptureClient->getSampleRate();

    // pitch detector
    if (method < 0 or method > PitchDetector::getMethods()->size()) method = 0;
    PitchDetector::Method pdMethod = (*PitchDetector::getMethods())[method];
    m_pitchDetector = new PitchDetector(m_framesize, m_stepsize, sampleRate);
    m_pitchDetector->setMethod(pdMethod);

    setSegments(doc, segments);

    setupActions(tuning, method);
#else
    Q_UNUSED(method);
#endif
}

PitchTrackerView::~PitchTrackerView()
{
    delete m_pitchDetector;
#ifdef HAVE_LIBJACK
    delete m_jackCaptureClient;
#endif
}

void PitchTrackerView::setupActions(int initialTuning, int initialMethod)
{
    // Add pitch-tracker-specific view menus
    QMenu *viewMenu = findMenu("View");

    QMenu *tuningsMenu = new QMenu(tr("Tunings"), viewMenu);
    m_tuningsActionGroup = new QActionGroup(this);

    {
        QVectorIterator<std::shared_ptr<Accidentals::Tuning>> it(m_availableTunings);
        while (it.hasNext()) {
            QAction *tuning =
              new QAction(QString::fromStdString(it.next()->getName()),
                          m_tuningsActionGroup);
            tuning->setCheckable(true);
            tuningsMenu->addAction(tuning);
        }
        m_tuningsActionGroup->actions().at(initialTuning)->setChecked(true);
    }

    connect(m_tuningsActionGroup, &QActionGroup::triggered,
            this, &PitchTrackerView::slotNewTuningFromAction);


    QMenu *methodsMenu = new QMenu(tr("Pitch estimate method"), viewMenu);
    m_methodsActionGroup = new QActionGroup(this);

    {
        QVectorIterator<PitchDetector::Method> it(*PitchDetector::getMethods());
        while (it.hasNext()) {
            QAction *method = new QAction(it.next(), m_methodsActionGroup);
            method->setCheckable(true);
            methodsMenu->addAction(method);
        }
        m_methodsActionGroup->actions().at(initialMethod)->setChecked(true);
    }

    connect(m_methodsActionGroup, &QActionGroup::triggered,
            this, &PitchTrackerView::slotNewPitchEstimationMethod);

    viewMenu->addSeparator();
    viewMenu->addMenu(tuningsMenu);
    viewMenu->addMenu(methodsMenu);
}

void
PitchTrackerView::slotNewTuningFromAction(QAction *a)
{
    const int whichTuning = m_tuningsActionGroup->actions().indexOf(a);
    m_tuning = m_availableTunings[whichTuning];
    m_pitchGraphWidget->setTuning(m_tuning);
    m_pitchGraphWidget->repaint();
}

void
PitchTrackerView::slotNewPitchEstimationMethod(QAction *a)
{
    const int whichMethod = m_methodsActionGroup->actions().indexOf(a);
    qDebug() << "Method " << whichMethod << " name: " << PitchDetector::getMethods()->at(whichMethod);
    m_pitchDetector->setMethod(PitchDetector::getMethods()->at(whichMethod));
    m_pitchGraphWidget->repaint();
}


void
PitchTrackerView::setSegments(RosegardenDocument *document,
                              const std::vector<Segment *>& /* segments */)
{
    // m_document is owned by our parent, NotationView
    if (m_document) {
        //disconnect(m_document, SIGNAL(pointerPositionChanged(timeT)),
        //           this, SLOT(slotPointerPositionChanged(timeT)));
    }
    m_document = document;

    // update GUI

    connect(m_document, &RosegardenDocument::pointerPositionChanged,
            this, &PitchTrackerView::slotUpdateValues);

    connect(this, &NotationView::play,
            this, &PitchTrackerView::slotStartTracker);
    connect(this, &NotationView::stop,
            this, &PitchTrackerView::slotStopTracker);

    // Any other jumping around in the score will invalidate the pitch
    // graph, so let's just erase it and let it start again.

    connect(this, &NotationView::stepBackward,
            this, &PitchTrackerView::slotPlaybackJump);
    connect(this, &NotationView::stepForward,
            this, &PitchTrackerView::slotPlaybackJump);
    connect(this, &NotationView::rewindPlayback,
            this, &PitchTrackerView::slotPlaybackJump);
    connect(this, &NotationView::fastForwardPlayback,
            this, &PitchTrackerView::slotPlaybackJump);
    connect(this, &NotationView::rewindPlaybackToBeginning,
            this, &PitchTrackerView::slotPlaybackJump);
    connect(this, &NotationView::fastForwardPlaybackToEnd,
            this, &PitchTrackerView::slotPlaybackJump);

}


/*************************************
      CONTROL REALTIME PROCESSING
 *************************************/

void
PitchTrackerView::slotPlaybackJump()
{
    m_transport_posn_change = true;
    RG_DEBUG << "PitchTrackerView: User changed playback posn\n";
}

void
PitchTrackerView::slotStartTracker()
{
    // The play transport button (which is what gets us here)
    // is really a play/pause button, so we need to toggle
    // the widget's behaviour rather than just starting it.
    if (m_running) {
        slotStopTracker();
    } else {
        m_history.clear();
#ifdef HAVE_LIBJACK
        m_jackCaptureClient->startProcessing();
#endif
        m_running = true;

        NotationStaff *currentStaff =
            m_notationWidget->getScene()->getCurrentStaff();
        if (!currentStaff) return;

        ViewSegment *vs = dynamic_cast<ViewSegment*>(currentStaff);
        m_notes = vs->getViewElementList();
        //m_notes_itr = m_notes->begin();
        // The tracker doesn't necessarily start at the beginning of the piece
        m_transport_posn_change = true;
    }
}

void
PitchTrackerView::slotStopTracker()
{
    m_running = false;
#ifdef HAVE_LIBJACK
    m_jackCaptureClient->stopProcessing();
#endif
}



/*************************************
          REALTIME PROCESSING
 *************************************/

// Pitch history maintenance functions

void
PitchTrackerView::addNoteBoundary(double freq, RealTime time)
{
    m_history.m_targetFreqs.append(freq);
    m_history.m_targetChangeTimes.append(time);
    m_pitchGraphWidget->update();
}

// Eventually, we'd like to be able to export a PML representation
// of the captured performance, so we maintain a QVector of score time
// (time) as well as the real performance time (realTime). Only the
// latter is used in the genertion of the pitch error graph, but the
// former will be useful in associating the performance part of the
// PML with the score part.
void
PitchTrackerView::addPitchTime(double freq, timeT time, RealTime realTime)
{
    m_history.m_detectFreqs.append(freq);
    m_history.m_detectTimes.append(time);
    m_history.m_detectRealTimes.append(realTime);

    if (freq == PitchDetector::NONE || freq == PitchDetector::NOSIGNAL) {
        m_history.m_detectErrorsCents.append(freq);
        m_history.m_detectErrorsValid.append(false);
    } else {
        double targetFreq = 0;
        if (m_history.m_targetFreqs.size() > 0) {
	    targetFreq = m_history.m_targetFreqs.last();
	}
        // TODO: isn't log(2) a constant defined in some header?
        // maybe move this out of this file?
        double cents = 1200.0 * log( freq / targetFreq ) / log (2);

        m_history.m_detectErrorsCents.append(cents);
        m_history.m_detectErrorsValid.append(true);
    }
#if DEBUG_PRINT_ALL
    for (int i = 0; i < m_history.m_detectErrorsCents.size(); i++) {
        RG_DEBUG << m_history.m_detectErrorsCents[i];
    }
#endif
    m_pitchGraphWidget->update();
}

//!!! deliberate design choice: this only gets the last few samples;
//    any samples between the last GUI event and the current GUI
//    event are lost.  (reduces processing)
void
PitchTrackerView::slotUpdateValues(timeT time)
{
    // Pitch tracker not running? then don't bother with any processing
    if (!m_running) return;

    // Find the nearest preceding or simultaneous event
    // in the our element list
    const ViewElementList::iterator score_event_itr =
        m_notes->findNearestTime(time);

    // Not found?  Bail.
    if (score_event_itr == m_notes->end())
        return;

    // Gracefully handle repositioning of the play cursor by the user
    if (m_transport_posn_change) {
        RG_DEBUG << "User changed transport position\n";
        m_transport_posn_change = false;
        m_history.clear();
        // Always record a note boundary if the transport posn's changed.
        // We'll find the previous Note event (not rest) because in then
        // case the new current event is a rest, we ignore pitch parameters
        // anyway. We just need m_notes_itr to be "wrong" to force the
        // boundary to be recorded.
        m_notes_itr =
            m_notes->findPrevious(Note::EventType, score_event_itr);
        //m_notes_itr = m_notes->findNearestTime(time);
    }

    const Event * const e = (*score_event_itr)->event();

    // Past the end of the last note? Nothing further to do.
    if (m_notes->findNext(Note::EventType, score_event_itr) == m_notes->end()
        && time > e->getAbsoluteTime()+e->getDuration()) {
        return;
    }

    const RealTime rt = m_doc->getComposition().getElapsedRealTime(time);

    // See whether the current note has changed since we last looked
    if (score_event_itr != m_notes_itr) {
        // if so, record the current note and issue record the note boundary
        RG_DEBUG << "***** PitchTrackerView: New " << e->getType()
                  << " at " << time;
        m_notes_itr = score_event_itr;
        // We can only register a note if this event isn't a rest
        // and there's a valid tuning
        if (e->isa(Note::EventType) && m_tuning) {
            const double noteFreq =
                m_tuning->getFrequency(Pitch(*e));
            addNoteBoundary(noteFreq, rt);
        }
    }

    // Record the actual pitch data. Easy case first
    if (e->isa(Note::EventRestType)) {
        addPitchTime(PitchDetector::NONE, time, rt);

    } else if (e->isa(Note::EventType)) {
#ifdef HAVE_LIBJACK
        if (m_jackCaptureClient->getFrame(m_pitchDetector->getInBuffer(),
                                          m_pitchDetector->getBufferSize())) {
            const double freq = m_pitchDetector->getPitch();
            addPitchTime(freq, time, rt);
        }
#endif

    } else {
        RG_WARNING << "PitchTrackerView: ummm, what's a \""
                  << e->getType() << "\"EventType?";
    }
}
} // Rosegarden namespace
