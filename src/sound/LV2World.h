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

#ifndef RG_LV2WORLD_H
#define RG_LV2WORLD_H

#include <lilv/lilv.h>


namespace Rosegarden
{

    namespace LV2World
    {

        /// Get the LilvWorld instance.  Create if needed.
        LilvWorld *get();

    }

}


#endif
