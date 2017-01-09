/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2017 the Rosegarden development team.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/


#include "AudioFile.h"
#include "QDateTime"

namespace Rosegarden

{

AudioFile::AudioFile(unsigned int id,
                     const std::string &label,
                     const QString &fileName):
        SoundFile(fileName),
        m_type(UNKNOWN),
        m_id(id),
        m_label(label),
        m_bitsPerSample(0),
        m_sampleRate(0),
        m_channels(0),
        m_dataChunkIndex( -1)
{
    m_fileInfo = new QFileInfo(fileName);
}

AudioFile::AudioFile(const QString &fileName,
                     unsigned int channels = 1,
                     unsigned int sampleRate = 48000,
                     unsigned int bitsPerSample = 16):
        SoundFile(fileName),
        m_type(UNKNOWN),
        m_id(0),
        m_label(""),
        m_bitsPerSample(bitsPerSample),
        m_sampleRate(sampleRate),
        m_channels(channels),
        m_dataChunkIndex( -1)
{
    m_fileInfo = new QFileInfo(fileName);
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

