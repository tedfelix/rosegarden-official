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


#ifndef RG_SIMPLE_WAV_FILE_WRITE_STREAM_H
#define RG_SIMPLE_WAV_FILE_WRITE_STREAM_H

#include "AudioWriteStream.h"

#include <rosegardenprivate_export.h>

// If we have libsndfile, we shouldn't be using this class
#ifndef HAVE_LIBSNDFILE

#include <fstream>
#include <string>

namespace Rosegarden
{

class ROSEGARDENPRIVATE_EXPORT SimpleWavFileWriteStream : public AudioWriteStream
{
public:
    explicit SimpleWavFileWriteStream(Target target);
    virtual ~SimpleWavFileWriteStream();

    static void initStaticObjects();

    virtual QString getError() const override { return m_error; }

    virtual bool putInterleavedFrames(size_t count, float *frames) override;

protected:
    int m_bitDepth;
    QString m_error;
    std::ofstream *m_file;

    void writeFormatChunk();
    static std::string int2le(unsigned int value, unsigned int length);
    void putBytes(std::string);
    void putBytes(const unsigned char *, size_t);
};

}

#endif

#endif
