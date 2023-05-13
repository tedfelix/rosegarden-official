/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LV2URID_H
#define RG_LV2URID_H

#include <map>
#include <lv2/urid/urid.h>
#include <QMutex>

namespace Rosegarden
{

class LV2Urid
{
public:
    /// Singleton
    static LV2Urid *getInstance();

    LV2_URID uridMap(const char *uri);
    const char* uridUnmap(LV2_URID urid);

    LV2_URID_Map m_map;
    LV2_URID_Unmap m_unmap;

private:
    /// Singleton.  See getInstance().
    LV2Urid();

    QMutex m_mutex;

    //urid map
    std::map<std::string, int> m_uridMap;
    std::map<int, std::string> m_uridUnmap;
    int m_nextId;
};

}

#endif // RG_LV2URID_H
