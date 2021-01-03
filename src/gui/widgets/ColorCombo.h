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

#ifndef RG_COLORCOMBO_H
#define RG_COLORCOMBO_H

#include <QComboBox>


namespace Rosegarden
{


class ColorCombo : public QComboBox
{
    Q_OBJECT

public:
    explicit ColorCombo(QWidget *parent);

    /// Reload the combo with all the colors.
    /**
     * This is a pretty expensive routine.  Avoid calling unless necessary.
     * Given that the Segment color map (Composition::getSegmentColourMap())
     * cannot be changed, this only needs to be called once at the beginning.
     */
    void updateColors();

};


}

#endif
