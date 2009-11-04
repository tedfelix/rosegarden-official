/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef LSCPPATCHEXTRACTOR_H
#define LSCPPATCHEXTRACTOR_H

#include "misc/Strings.h"

#include <QString>
#include <QStringList>

#include <map>
#include <string>

class QString;


namespace Rosegarden
{

class LSCPPatchExtractor
{

public:
    typedef std::map<int, std::string> Bank;
    typedef std::map<int, Bank> Device;

    static bool isLSCPFile(const QString& fileName);
    static Device extractContent(const QString& fileName);

protected:
};

}

#endif // LSCPPATCHEXTRACTOR_H
