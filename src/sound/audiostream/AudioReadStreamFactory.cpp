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


#include "AudioReadStreamFactory.h"
#include "AudioReadStream.h"

#include "base/ThingFactory.h"
#include "misc/Debug.h"

#include <QFileInfo>

#define DEBUG_AUDIO_READ_STREAM_FACTORY 1

namespace Rosegarden {

typedef ThingFactory<AudioReadStream, QString>
AudioReadStreamFactoryImpl;

template <>
AudioReadStreamFactoryImpl *
AudioReadStreamFactoryImpl::m_instance = 0;

AudioReadStream *
AudioReadStreamFactory::createReadStream(QString audioFileName)
{
    AudioReadStream *s = 0;

    QString extension = QFileInfo(audioFileName).suffix().toLower();

    AudioReadStreamFactoryImpl *f = AudioReadStreamFactoryImpl::getInstance();

    // Try to use a reader that has actually registered an interest in
    // this extension, first

    try {
        s = f->createFor(extension, audioFileName);
    } catch (...) {
    }

    if (s && s->isOK() && s->getError() == "") {
        return s;
    } else if (s) {
        std::cerr << "Error with recommended reader: \""
                  << s->getError().toStdString() << "\""
                  << std::endl;
    }

    delete s;
    s = 0;

    // If that fails, try all readers in arbitrary order

    AudioReadStreamFactoryImpl::URISet uris = f->getURIs();

    for (AudioReadStreamFactoryImpl::URISet::const_iterator i = uris.begin();
         i != uris.end(); ++i) {

        try {
            s = f->create(*i, audioFileName);
        } catch (UnknownThingException) { }

        if (s && s->isOK() && s->getError() == "") {
            return s;
        }

        delete s;
        s = 0;
    }

    return 0;
}

QStringList
AudioReadStreamFactory::getSupportedFileExtensions()
{
    return AudioReadStreamFactoryImpl::getInstance()->getTags();
}

}

