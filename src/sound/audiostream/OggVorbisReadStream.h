/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_OGG_VORBIS_READ_STREAM_H
#define RG_OGG_VORBIS_READ_STREAM_H

#include "AudioReadStream.h"

#include <rosegardenprivate_export.h>

#ifdef HAVE_OGGZ
#ifdef HAVE_FISHSOUND

namespace Rosegarden
{
    
class ROSEGARDENPRIVATE_EXPORT OggVorbisReadStream : public AudioReadStream
{
public:
    OggVorbisReadStream(QString path);
    virtual ~OggVorbisReadStream();
    
    static void initStaticObjects();

    virtual QString getError() const { return m_error; }

protected:
    virtual size_t getFrames(size_t count, float *frames);

    QString m_path;
    QString m_error;

    class D;
    D *m_d;
};

}

#endif
#endif

#endif
