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

#ifndef RG_TRACKVUMETER_H
#define RG_TRACKVUMETER_H

#include "gui/widgets/VUMeter.h"


class QWidget;


namespace Rosegarden
{



class TrackVUMeter: public VUMeter
{
public:
     TrackVUMeter(QWidget *parent = nullptr,
                  VUMeterType type = Plain,
                  int width = 0,
                  int height = 0,
                  int position = 0);

    int getPosition() const { return m_position; }

protected:
    void meterStart() override;
    void meterStop() override;

private:
    int m_position;
    int m_textHeight;

};


}

#endif
