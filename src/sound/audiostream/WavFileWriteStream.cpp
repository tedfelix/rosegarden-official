/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifdef HAVE_LIBSNDFILE

#include "WavFileWriteStream.h"

#include "misc/Debug.h"

#include <cstring>
#include <iostream>

namespace Rosegarden
{

static 
AudioWriteStreamBuilder<WavFileWriteStream>
wavbuilder(
    QUrl("http://breakfastquay.com/rdf/rosegarden/fileio/WavFileWriteStream"),
    QStringList() << "wav" << "aiff"
    );

WavFileWriteStream::WavFileWriteStream(Target target) :
    AudioWriteStream(target),
    m_file(0)
{
    memset(&m_fileInfo, 0, sizeof(SF_INFO));
    m_fileInfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    m_fileInfo.channels = getChannelCount();
    m_fileInfo.samplerate = getSampleRate();

    m_file = sf_open(getPath().toLocal8Bit().data(), SFM_WRITE, &m_fileInfo);

    if (!m_file) {
	std::cerr << "WavFileWriteStream::initialize: Failed to open output file for writing ("
		  << sf_strerror(m_file) << ")" << std::endl;

        m_error = QString("Failed to open audio file '") +
            getPath() + "' for writing";
        m_target.invalidate();
	return;
    } else {
        std::cerr << "WavFileWriteStream::initialize: Opened output file "
                  << getPath().toStdString() << " for writing" << std::endl;
    }
}

WavFileWriteStream::~WavFileWriteStream()
{
    if (m_file) sf_close(m_file);
}

bool
WavFileWriteStream::putInterleavedFrames(size_t count, float *frames)
{
    if (!m_file || !getChannelCount()) return false;
    if (count == 0) return false;

    sf_count_t written = sf_writef_float(m_file, frames, count);

    return (size_t(written) == count);
}

}

#endif
