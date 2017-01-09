/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_AUDIO_READ_STREAM_FACTORY_H
#define RG_AUDIO_READ_STREAM_FACTORY_H

#include <QString>
#include <QStringList>

namespace Rosegarden {

class AudioReadStream;

class AudioReadStreamFactory
{
public:
    // May throw FileOpenFailed or AudioReadStream::FileDRMProtected,
    // or simply return NULL, for failure
    static AudioReadStream *createReadStream(QString fileName);

    static QStringList getSupportedFileExtensions();
};

}

#endif
