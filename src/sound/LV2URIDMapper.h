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

#ifndef RG_LV2URIDMAPPER_H
#define RG_LV2URIDMAPPER_H

#include <lv2/urid/urid.h>


namespace Rosegarden
{


/// LV2 URI/URID mapper
/**
 * https://lv2plug.in/c/html/group__urid.html
 */
namespace LV2URIDMapper
{

    /// URID Map Feature.  Gets the URID for a URI.
    /**
     * If the URI hasn't been seen yet, a new URID is assigned.
     */
    LV2_URID uridMap(const char *uri);
    /// Get the function pointer to uridMap().  Suitable for lilv calls.
    LV2_URID_Map *getURIDMapFeature();

    /// URID Unmap Feature.  Gets the URI for a URID.
    /**
     * The uridUnmap function is called for debug output and is passed to
     * the plugins which may call it.
     */
    const char *uridUnmap(LV2_URID urid);
    /// Get the function pointer to uridUnmap().  Suitable for lilv calls.
    LV2_URID_Unmap *getURIDUnmapFeature();

}


}

#endif // RG_LV2UTILS_H
