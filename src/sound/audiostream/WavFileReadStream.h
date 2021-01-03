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


#ifndef RG_WAV_FILE_READ_STREAM_H
#define RG_WAV_FILE_READ_STREAM_H

#include "AudioReadStream.h"

#include <rosegardenprivate_export.h>

#ifdef HAVE_LIBSNDFILE

#include <sndfile.h>

namespace Rosegarden
{
    
class ROSEGARDENPRIVATE_EXPORT WavFileReadStream : public AudioReadStream
{
public:
    WavFileReadStream(QString path);
    ~WavFileReadStream() override;
    
    static void initStaticObjects();

    QString getError() const override { return m_error; }

protected:
    size_t getFrames(size_t count, float *frames) override;
    
    SF_INFO m_fileInfo;
    SNDFILE *m_file;

    QString m_path;
    QString m_error;

    size_t m_offset;
};

}

#endif

#endif
