
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

#ifndef RG_VELOCITYCOLOUR_H
#define RG_VELOCITYCOLOUR_H

#include <QColor>




namespace Rosegarden
{



/**
 * Returns a QColour according to a formula.  We provide three colours
 * to mix, a maximum value and three knees at which points the
 * intermediate colours max out.  Play around to your satisfaction.
 */
class VelocityColour
{

public:
    VelocityColour(const QColor &loud,
                   const QColor &medium,
                   const QColor &quiet,
                   int maximum,
                   int loudKnee,
                   int mediumKnee,
                   int quietKnee);
    ~VelocityColour();

    const QColor& getColour(int value);

    int getLoudKnee() const { return m_loudKnee; }
    int getMediumKnee() const { return m_mediumKnee; }
    int getQuietKnee() const { return m_quietKnee; }

    QColor getLoudColour() const { return m_loudColour; }
    QColor getMediumColour() const { return m_mediumColour; }
    QColor getQuietColour() const { return m_quietColour; }

    int getMaxValue() const { return m_maximum; }

private:

    QColor m_loudColour;
    QColor m_mediumColour;
    QColor m_quietColour;
    int    m_loudKnee;
    int    m_mediumKnee;
    int    m_quietKnee;
    int    m_maximum;

    // the mixed colour that we can return
    QColor m_mixedColour;


    int m_loStartRed;
    int m_loStartGreen;
    int m_loStartBlue;

    int m_loStepRed;
    int m_loStepGreen;
    int m_loStepBlue;

    int m_hiStartRed;
    int m_hiStartGreen;
    int m_hiStartBlue;

    int m_hiStepRed;
    int m_hiStepGreen;
    int m_hiStepBlue;


    int m_multiplyFactor;
};


}

#endif
