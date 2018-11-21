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

#ifndef RG_PITCH_GRAPH_WIDGET_H
#define RG_PITCH_GRAPH_WIDGET_H

#include "PitchHistory.h"

#include "base/Event.h"

#include <QWidget>
#include <QList>

namespace Rosegarden {

namespace Accidentals { class Tuning; }

struct RealTime;

/**
 * \addtogroup Codicil
 * \@{
 * \brief Graphical display of pitch tracker results
 *
 * This is part of the network for Interdisciplinary research in
 * Science and Music's "Rosegarden Codicil" project.
 * http://www.n-ism.org/Projects/microtonalism.php
 *
 * \author Graham Percival
 * \date 2009
 */
class PitchGraphWidget : public QWidget
{
    Q_OBJECT

public:
    PitchGraphWidget(PitchHistory &history, QWidget *parent = nullptr);
    ~PitchGraphWidget() override;

    void setTuning(Accidentals::Tuning* tuning);

protected:
    void paintEvent(QPaintEvent *event) override;

    unsigned int    m_graphHeight;  // Height of graph (in cents)
    unsigned int    m_graphWidth;   // Width of graph (in milliseconds)
    bool            m_ignoreOctave; // Whether to ignore octave errors
    
    Accidentals::Tuning* m_tuning;  // Tuning in use in this widget
    PitchHistory&   m_history;      // structure of data to plot
};

} // End namespace Rosegarden

/**\@}*/

#endif

