/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
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
#include <vector>

class QString;


namespace Rosegarden
{

class LSCPPatchExtractor
{

public:

    typedef struct lscp_bank_program_data {
        int         bankNumber;
        std::string bankName;
        int         programNumber;
        std::string programName;
    } Bank_Prog;

    typedef std::vector<Bank_Prog> Device;

    static bool isLSCPFile(const QString& fileName);
    static Device extractContent(const QString& fileName);

protected:
};

}

#endif // LSCPPATCHEXTRACTOR_H
