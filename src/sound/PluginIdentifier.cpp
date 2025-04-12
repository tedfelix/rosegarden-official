/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PluginIdentifier.h"

namespace Rosegarden
{

QString
PluginIdentifier::createIdentifier(QString type,
                                   QString soName,
                                   QString label)
{
    QString identifier = type + ":" + soName + ":" + label;
    return identifier;
}

void
PluginIdentifier::parseIdentifier(QString identifier,
                                  QString &type,
                                  QString &soName,
                                  QString &label,
                                  QString &architecture)
{
    type = identifier.section(':', 0, 0);
    soName = identifier.section(':', 1, 1);
    label = identifier.section(':', 2);
    architecture = type;
    // lv2 does not have a fixed type. Assume anything that is not
    // ladspa or dssi and is not empty is lv2
    if (type != "ladspa" && type != "dssi" && type != "") architecture = "lv2";
}

bool
PluginIdentifier::areIdentifiersSimilar(QString id1, QString id2)
{
    QString type1, type2, soName1, soName2, label1, label2, arch1, arch2;

    parseIdentifier(id1, type1, soName1, label1, arch1);
    parseIdentifier(id2, type2, soName2, label2, arch2);

    if (type1 != type2 || label1 != label2)
        return false;

    bool similar = (soName1.section('/', -1).section('.', 0, 0) ==
                    soName2.section('/', -1).section('.', 0, 0));

    return similar;
}

// The prefix of this key is also used as a literal in base/AudioPluginInstance.C.
// If you change one, change the other.
// Better still, don't change one.
QString
PluginIdentifier::RESERVED_PROJECT_DIRECTORY_KEY = "__ROSEGARDEN__:__RESERVED__:ProjectDirectoryKey";

}
