// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include "vumeter.h"

#ifndef _TRACKVUMETER_H_
#define _TRACKVUMETER_H_

// VUMeter is the wrong term really.  But I'm a thickhead that needs
// correcting.  So correct me.
//

class TrackVUMeter: public VUMeter
{
public:
     TrackVUMeter(QWidget *parent = 0,
                  VUMeterType type = Plain,
                  int width = 0,
                  int height = 0,
                  int position = 0,
                  const char *name = 0);

    int getPosition() const { return m_position; }

protected:
    virtual void meterStart();
    virtual void meterStop();

private:
    int m_position;
    int m_textHeight;

};

// AudioVUMeter - a vertical audio meter.  Default is stereo.
//
class AudioVUMeter : public VUMeter
{
public:
    AudioVUMeter(QWidget *parent = 0,
                 VUMeterType type = VUMeter::AudioPeakHold,
                 bool stereo = true,
                 int width = 12,
                 int height = 140,
                 const char *name = 0);
protected:
    virtual void meterStart();
    virtual void meterStop();

    bool m_stereo;
};



#endif // _TRACKVUMETER_H_
