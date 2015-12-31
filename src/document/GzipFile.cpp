/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "GzipFile.h"
#include <QString>
#include <string>
#include <zlib.h>

namespace Rosegarden
{

bool
GzipFile::writeToFile(QString file, QString text)
{
    std::string stext = std::string(text.toUtf8().data());
    const char *ctext = stext.c_str();
    size_t csize = stext.length();

    gzFile fd = gzopen(file.toLocal8Bit().data(), "wb");
    if (!fd) return false;

    int actual = gzwrite(fd, (void *)ctext, csize);
    gzclose(fd);

    return ((size_t)actual == csize);
}

bool
GzipFile::readFromFile(QString file, QString &text)
{
    text = "";
    gzFile fd = gzopen(file.toLocal8Bit().data(), "rb");
    if (!fd) return false;

    QByteArray ba;
    char buffer[100000];
    int got = 0;

    while ((got = gzread(fd, buffer, 100000)) > 0) {
        ba.append(QByteArray(buffer, got));
    }

    bool ok = gzeof(fd);
    gzclose(fd);
    text = QString::fromUtf8(ba);
    return ok;
}    

}


