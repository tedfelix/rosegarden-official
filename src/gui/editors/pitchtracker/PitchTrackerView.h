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

#ifndef RG_PITCH_TRACKER_VIEW_H
#define RG_PITCH_TRACKER_VIEW_H

#include "PitchHistory.h"

#include "gui/editors/notation/NotationView.h"
#include "base/ViewSegment.h"
#include <lo/lo_osc_types.h>

namespace Rosegarden {


class JackCaptureClient;
class PitchDetector;
class PitchGraphWidget;
class RosegardenDocument;

namespace Accidentals { class Tuning; }


/**
 * \addtogroup Codicil
 * \@{
 * \brief Monophonic pitch tracker window (subclass of NotationView)
 *
 * This is part of the network for Interdisciplinary research in
 * Science and Music's "Rosegarden Codicil" project.
 * http://www.n-ism.org/Projects/microtonalism.php
 *
 * \author Graham Percival
 * \date 2009
 */
class PitchTrackerView : public NotationView
{
    Q_OBJECT

public:
    // basic Rosegarden infrastructure
    PitchTrackerView(RosegardenDocument *doc,
                     std::vector<Segment *> segments,
                     QWidget *parent = nullptr);
    ~PitchTrackerView() override;

    void setSegments(RosegardenDocument *document,
                     std::vector<Segment *> segments);

    bool getJackConnected() {
        return m_jackConnected;
    }

protected slots:
    // updates from GUI moving the vertical position line
    void slotUpdateValues(timeT time);
    // connected to start/stop Transport actions
    void slotStartTracker();
    void slotStopTracker();
    /** Ensure correct graphing state if user moves cursor during playback */
    void slotPlaybackJump();
    /** Set the current tuning and detection method from a menu action */
    void slotNewTuningFromAction(QAction *);
    void slotNewPitchEstimationMethod(QAction *);

protected:
    /** Record new note (history maintenance utility) */
    void addNoteBoundary(double freq, RealTime time);
    /** Record new pitch data (history maintenance utility) */
    void addPitchTime(double freq, timeT time, RealTime realTime);

    // doc for real-time/score-time conversion
    RosegardenDocument         *m_doc;
    // get audio
    JackCaptureClient          *m_jackCaptureClient;
    bool                        m_jackConnected;
    // get pitch from audio
    PitchDetector              *m_pitchDetector;
    // display pitch errors
    PitchGraphWidget           *m_pitchGraphWidget;

    bool                        m_running;

    // Pitch tracker Parameters
    int                         m_framesize;
    int                         m_stepsize;

    // notes in the Composition/Document -- what note should we be singing?
    ViewElementList            *m_notes;
    ViewElementList::iterator   m_notes_itr;
    
    // Choice of defined tunings
    QVector<Accidentals::Tuning*> m_availableTunings;
    QActionGroup               *m_tuningsActionGroup;
    // ...and of DSP method
    QActionGroup               *m_methodsActionGroup;
    
    // Tuning standard in use by this View
    Accidentals::Tuning        *m_tuning;
    
private:
    // Used to resync note iterator after user ff/rwind
    bool                        m_transport_posn_change;
    
    // Lists of detected and target frequencies 
    // These lists are maintained here and used by the Widget
    PitchHistory                m_history;
    
    // Override setupActions in NotationView so we can have
    // additional menu entries and so on.
    // Mark the initial tuning and method menu selections.
    void                        setupActions(int initialTuning,
                                             int initialMethod);
};

}

/**\@}*/

#endif

