// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "velocitycolour.h"
#include "rosestrings.h"
#include "rosedebug.h"


VelocityColour::VelocityColour(const QColor &loud,
                               const QColor &medium,
                               const QColor &quiet,
                               int maxValue,
                               int loudKnee,
                               int mediumKnee,
                               int quietKnee):
        m_loudColour(loud),
        m_mediumColour(medium),
        m_quietColor(quiet),
        m_loudKnee(loudKnee),
        m_mediumKnee(mediumKnee),
        m_quietKnee(quietKnee),
        m_maxValue(maxValue),
        m_mixedColour(QColor(0, 0, 0)), // black as default
        m_multiplyFactor(1000)
{

    assert(maxValue > loudKnee);
    assert(loudKnee > mediumKnee);
    assert(mediumKnee > quietKnee);

    // These are the colours for the first band from Quiet to Medium.
    // We use a multiplication factor to keep ourselves in the realms
    // of integer arithmetic as we can potentially be doing a lot of
    // these calculations when playing.
    //
    //
    m_loStartRed   = m_quietColor.red()   * m_multiplyFactor;
    m_loStartGreen = m_quietColor.green() * m_multiplyFactor;
    m_loStartBlue  = m_quietColor.blue()  * m_multiplyFactor;

    m_loStepRed    = ( m_mediumColour.red() * m_multiplyFactor
                       - m_loStartRed ) / m_mediumKnee;
    m_loStepGreen  = ( m_mediumColour.green() * m_multiplyFactor
                       - m_loStartGreen ) / m_mediumKnee;
    m_loStepBlue   = ( m_mediumColour.blue() * m_multiplyFactor
                       - m_loStartBlue  ) / m_mediumKnee;

    m_hiStartRed   = m_mediumColour.red()   * m_multiplyFactor;
    m_hiStartGreen = m_mediumColour.green() * m_multiplyFactor;
    m_hiStartBlue  = m_mediumColour.blue()  * m_multiplyFactor;

    m_hiStepRed    = ( m_loudColour.red()   * m_multiplyFactor 
                       - m_hiStartRed ) / m_mediumKnee;
    m_hiStepGreen  = ( m_loudColour.green() * m_multiplyFactor
                       - m_hiStartGreen ) / m_mediumKnee;
    m_hiStepBlue   = ( m_loudColour.blue()  * m_multiplyFactor
                       - m_hiStartBlue ) / m_mediumKnee;

}

VelocityColour::~VelocityColour()
{
}


const QColor&
VelocityColour::getColour(int value)
{
    if (value > m_maxValue) value = m_maxValue;

    if (value < m_quietKnee)
    {
        return m_quietColor;
    }
    else if (value < m_mediumKnee)
    {
        m_mixedColour.setRgb(
                ( m_loStartRed   + m_loStepRed   * value ) / m_multiplyFactor,
                ( m_loStartGreen + m_loStepGreen * value ) / m_multiplyFactor,
                ( m_loStartBlue  + m_loStepBlue *  value ) / m_multiplyFactor);

    }
    else if (value >= m_mediumKnee < m_loudKnee)
    {
        int mixFactor = value - m_mediumKnee;

        m_mixedColour.setRgb(
            ( m_hiStartRed   + m_hiStepRed   * mixFactor ) / m_multiplyFactor,
            ( m_hiStartGreen + m_hiStepGreen * mixFactor ) / m_multiplyFactor,
            ( m_hiStartBlue  + m_hiStepBlue  * mixFactor ) / m_multiplyFactor);
    }
    else
    {
        return m_loudColour;
        return m_loudColour;
    }

    return m_mixedColour;
}


DefaultVelocityColour* DefaultVelocityColour::getInstance()
{
    if (!m_instance) m_instance = new DefaultVelocityColour;
    
    return m_instance;
}

DefaultVelocityColour* DefaultVelocityColour::m_instance = 0;
