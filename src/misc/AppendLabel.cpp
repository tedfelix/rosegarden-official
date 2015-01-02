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

#include <string>

#include "misc/ConfigGroups.h"
#include <QSettings>
#include "misc/Strings.h"

namespace Rosegarden 
{

std::string
appendLabel(const std::string &label, const std::string &suffix)
{
    using std::string;

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    if (! qStrToBool( settings.value("appendlabel", "true" ) ) ) {
       settings.endGroup();
       return string(label);
    }
    settings.endGroup();

    if (label.length() >= suffix.length()) {
        string::size_type loc = label.find(suffix, label.length() - suffix.length());
        if (loc != string::npos) {
            return string(label);    
        }
    }
    return string(label + " " + suffix);    
}    
 
}
