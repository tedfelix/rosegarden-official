/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    This file is Copyright 2005-2011 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioWriteStreamFactory.h"
#include "AudioWriteStream.h"

#include "base/ThingFactory.h"

#include <QFileInfo>

namespace Rosegarden {

typedef ThingFactory<AudioWriteStream, AudioWriteStream::Target>
AudioWriteStreamFactoryImpl;

template <>
AudioWriteStreamFactoryImpl *
AudioWriteStreamFactoryImpl::m_instance = 0;

AudioWriteStream *
AudioWriteStreamFactory::createWriteStream(QString audioFileName,
                                           size_t channelCount,
                                           size_t sampleRate)
{
    AudioWriteStream *s = 0;

    QString extension = QFileInfo(audioFileName).suffix().toLower();

    AudioWriteStream::Target target(audioFileName, channelCount, sampleRate);

    AudioWriteStreamFactoryImpl *f = AudioWriteStreamFactoryImpl::getInstance();

    try {
        s = f->createFor(extension, target);
    } catch (...) {
    }

    if (s && s->isOK() && s->getError() == "") {
        return s;
    }

    delete s;
    return 0;
}

}

