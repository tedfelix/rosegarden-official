/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "VelocityColour.h"

#include <QColor>
// #include <QtGlobal>

namespace Rosegarden
{

VelocityColour::VelocityColour(const QColor &loud,
                               const QColor &medium,
                               const QColor &quiet,
                               int maximum,
                               int loudKnee,
                               int mediumKnee,
                               int quietKnee):
        m_loudColour(loud),
        m_mediumColour(medium),
        m_quietColour(quiet),
        m_loudKnee(loudKnee),
        m_mediumKnee(mediumKnee),
        m_quietKnee(quietKnee),
        m_maximum(maximum),
        m_mixedColour(QColor(0, 0, 0)),  // black as default
        m_multiplyFactor(1000)
{

//    Q_ASSERT(maximum > loudKnee);
//    Q_ASSERT(loudKnee > mediumKnee);
//    Q_ASSERT(mediumKnee > quietKnee);

    // These are the colours for the first band from Quiet to Medium.
    // We use a multiplication factor to keep ourselves in the realms
    // of integer arithmetic as we can potentially be doing a lot of
    // these calculations when playing.
    //
    //
    m_loStartRed = m_quietColour.red() * m_multiplyFactor;
    m_loStartGreen = m_quietColour.green() * m_multiplyFactor;
    m_loStartBlue = m_quietColour.blue() * m_multiplyFactor;

    m_loStepRed = ( m_mediumColour.red() * m_multiplyFactor
                    - m_loStartRed ) / m_mediumKnee;
    m_loStepGreen = ( m_mediumColour.green() * m_multiplyFactor
                      - m_loStartGreen ) / m_mediumKnee;
    m_loStepBlue = ( m_mediumColour.blue() * m_multiplyFactor
                     - m_loStartBlue ) / m_mediumKnee;

    m_hiStartRed = m_mediumColour.red() * m_multiplyFactor;
    m_hiStartGreen = m_mediumColour.green() * m_multiplyFactor;
    m_hiStartBlue = m_mediumColour.blue() * m_multiplyFactor;

    m_hiStepRed = ( m_loudColour.red() * m_multiplyFactor
                    - m_hiStartRed ) / m_mediumKnee;
    m_hiStepGreen = ( m_loudColour.green() * m_multiplyFactor
                      - m_hiStartGreen ) / m_mediumKnee;
    m_hiStepBlue = ( m_loudColour.blue() * m_multiplyFactor
                     - m_hiStartBlue ) / m_mediumKnee;

}

VelocityColour::~VelocityColour()
{}

const QColor&
VelocityColour::getColour(int value)
{
    if (value > m_maximum)
        value = m_maximum;

    if (value < m_quietKnee) {
        return m_quietColour;
    } else if (value < m_mediumKnee) {
        m_mixedColour.setRgb(
            ( m_loStartRed + m_loStepRed * value ) / m_multiplyFactor,
            ( m_loStartGreen + m_loStepGreen * value ) / m_multiplyFactor,
            ( m_loStartBlue + m_loStepBlue * value ) / m_multiplyFactor);

    } else if (value >= m_mediumKnee && value < m_loudKnee) {
        int mixFactor = value - m_mediumKnee;

        m_mixedColour.setRgb(
            ( m_hiStartRed + m_hiStepRed * mixFactor ) / m_multiplyFactor,
            ( m_hiStartGreen + m_hiStepGreen * mixFactor ) / m_multiplyFactor,
            ( m_hiStartBlue + m_hiStepBlue * mixFactor ) / m_multiplyFactor);
    } else {
        return m_loudColour;
        return m_loudColour;
    }

    return m_mixedColour;
}

}
