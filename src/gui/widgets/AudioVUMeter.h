
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

#ifndef RG_AUDIOVUMETER_H
#define RG_AUDIOVUMETER_H

#include <QWidget>
#include "VUMeter.h"


class QPaintEvent;


namespace Rosegarden
{



class AudioVUMeter : public QWidget
{
public:
    AudioVUMeter(QWidget *parent = 0,
                 VUMeter::VUMeterType type = VUMeter::AudioPeakHoldShort,
                 bool stereo = true,
                 bool hasRecord = false,
                 int width = 12,
                 int height = 140);

    void setLevel(double dB) {
        m_meter->setLevel(dB);
    }
    void setLevel(double dBleft, double dBright) {
        m_meter->setLevel(dBleft, dBright);
    }

    void setRecordLevel(double dB) {
        m_meter->setRecordLevel(dB);
    }
    void setRecordLevel(double dBleft, double dBright) {
        m_meter->setRecordLevel(dBleft, dBright);
    }

    virtual void paintEvent(QPaintEvent*);

protected:
    class AudioVUMeterImpl : public VUMeter
    {
    public:
        AudioVUMeterImpl(QWidget *parent,
                         VUMeterType type,
                         bool stereo,
                         bool hasRecord,
                         int width,
                         int height);
    protected:
        virtual void meterStart() { }
        virtual void meterStop() { }
    };
        
    AudioVUMeterImpl *m_meter;
    bool m_stereo;
    int m_yoff;
    int m_xoff;
};


// A push button that emits wheel events.  Used by AudioRouteMenu.
//

}

#endif
