// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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


#include "AudioFile.h"

namespace Rosegarden

{

AudioFile::AudioFile(unsigned int id,
                     const std::string &name,
                     const std::string &fileName):
    SoundFile(fileName),
    m_type(UNKNOWN),
    m_id(id),
    m_name(name),
    m_bitsPerSample(0),
    m_sampleRate(0),
    m_channels(0),
    m_dataChunkIndex(-1)
{
    m_fileInfo = new QFileInfo(QString(fileName.c_str()));
}

AudioFile::AudioFile(const std::string &fileName,
                     unsigned int channels = 1,
                     unsigned int sampleRate = 48000,
                     unsigned int bitsPerSample = 16):
    SoundFile(fileName),
    m_type(UNKNOWN),
    m_id(0),
    m_name(""),
    m_bitsPerSample(bitsPerSample),
    m_sampleRate(sampleRate),
    m_channels(channels),
    m_dataChunkIndex(-1)
{
    m_fileInfo = new QFileInfo(QString(fileName.c_str()));
}

AudioFile::~AudioFile()
{
    delete m_fileInfo;
}

QDateTime
AudioFile::getModificationDateTime()
{
    if (m_fileInfo)
        return m_fileInfo->lastModified();
    else 
	return QDateTime();
}


}

