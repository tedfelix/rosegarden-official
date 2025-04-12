/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_AUDIO_WRITE_STREAM_FACTORY_H
#define RG_AUDIO_WRITE_STREAM_FACTORY_H

#include <QString>

namespace Rosegarden {

class AudioWriteStream;

class AudioWriteStreamFactory
{
public:
    static AudioWriteStream *createWriteStream(QString audioFileName,
                                               size_t channelCount,
                                               size_t sampleRate);
};

}

#endif
