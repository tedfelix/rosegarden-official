/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2022 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LV2Urid]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2Urid.h"

// LV2Urid is used in different threads
#define LOCKED QMutexLocker rg_uri_locker(&m_mutex)

namespace
{
    LV2_URID LV2UridMap(LV2_URID_Map_Handle handle,
                        const char *uri)
    {
        Rosegarden::LV2Urid* lv2i =
            static_cast<Rosegarden::LV2Urid*>(handle);
        return lv2i->uridMap(uri);
    }

    const char* LV2UridUnmap(LV2_URID_Unmap_Handle handle,
                             LV2_URID urid)
    {
        Rosegarden::LV2Urid* lv2i =
            static_cast<Rosegarden::LV2Urid*>(handle);
        return lv2i->uridUnmap(urid);
    }
}

namespace Rosegarden
{

LV2Urid *
LV2Urid::getInstance()
{
    static LV2Urid instance;
    return &instance;
}

LV2_URID
LV2Urid::uridMap(const char *uri)
{
    LOCKED;
    auto it = m_uridMap.find(uri);
    if (it == m_uridMap.end()) {
        int id = m_nextId;
        m_nextId++;
        m_uridMap[uri] = id;
        m_uridUnmap[id] = uri;
    }
    int ret = m_uridMap[uri];
    return ret;
}

const char*
LV2Urid::uridUnmap(LV2_URID urid)
{
    LOCKED;
    auto it = m_uridUnmap.find(urid);
    if (it == m_uridUnmap.end()) {
        return "";
    }
    return (*it).second.c_str();
}

LV2Urid::LV2Urid() :
    m_nextId(1)
{
    m_map = {this, &LV2UridMap};
    m_unmap = {this, &LV2UridUnmap};
}

}
