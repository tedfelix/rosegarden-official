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

#ifndef RG_AUDIOSTRIP_H
#define RG_AUDIOSTRIP_H

#include <QWidget>


namespace Rosegarden
{


/// The strips on the "Audio Mixer" window.
class AudioStrip : public QWidget
{
    Q_OBJECT

public:
    AudioStrip(QWidget *parent, unsigned i_id);
    virtual ~AudioStrip();

    /// Buss/Instrument ID.
    unsigned id;

    void updateWidgets();

private:
};


}

#endif
