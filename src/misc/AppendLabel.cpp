// -*- c-basic-offset: 4 -*-
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <string>

#include "document/ConfigGroups.h"
#include "kconfig.h"
#include "kapp.h"

namespace Rosegarden 
{

std::string
appendLabel(const std::string &label, const std::string &suffix)
{
    using std::string;

    QSettings config ; // was: kapp->config()
    QSettings config;
    config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
    //  


    if (! qStrToBool( config.value("appendlabel", "true" ) ) ) {
       return string(label);
    }
    if (label.length() >= suffix.length()) {
        string::size_type loc = label.find(suffix, label.length() - suffix.length());
        if (loc != string::npos) {
            return string(label);    
        }
    }
    return string(label + " " + suffix);    
}    
 
}
