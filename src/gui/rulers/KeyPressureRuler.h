/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_KEYPRESSURERULER_H
#define RG_KEYPRESSURERULER_H

#include "ControllerEventsRuler.h"

namespace Rosegarden
{

class ViewElement;

class KeyPressureRuler : public ControllerEventsRuler
{
public:
    KeyPressureRuler(ViewSegment*,
                     RulerScale*,
                     QWidget* parent=nullptr,
                     const ControlParameter *controller = nullptr,
                     const char* name=nullptr );

    ~KeyPressureRuler() override;

    void setElementSelection(const std::vector<ViewElement *> &elementList);

    int getPitch() override;

    void paintEvent(QPaintEvent *) override;
    void setSegment(Segment *) override;

    void getLimits(float& xmin, float& xmax) override;

private:
    int m_notePitch;
    timeT m_noteStart;
    timeT m_noteDuration;

};


}

#endif
