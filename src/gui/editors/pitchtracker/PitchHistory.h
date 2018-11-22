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

#ifndef RG_PITCH_HISTORY_H
#define RG_PITCH_HISTORY_H

#include "QList"

/**
 * \addtogroup Codicil
 * \@{
 * \brief Structure of vectors representing the performance's pitch history
 *
 * This is part of the network for Interdisciplinary research in
 * Science and Music's "Rosegarden Codicil" project.
 * http://www.n-ism.org/Projects/microtonalism.php
 *
 * The History structure will be created and maintained by
 * the PitchTrackerView. A reference is passed to the
 * PitchTrackerWidget which uses it to draw the graph.
 *
 * \author Nick Bailey nick@n-ism.org
 * \date Apr 2010
 */

#include "base/Event.h"

namespace Rosegarden {

struct RealTime;

class PitchHistory {
  public:
    void clear();
    
    QList<double>   m_detectFreqs;
    QList<double>   m_detectErrorsCents;
    QList<bool>     m_detectErrorsValid;
    QList<timeT>    m_detectTimes;
    QList<RealTime> m_detectRealTimes;

    QList<double>   m_targetFreqs;
    QList<RealTime> m_targetChangeTimes;
};
/**\@}*/

}

#endif
