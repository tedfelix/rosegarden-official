/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2000-2024 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LV2URIDMapper]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2URIDMapper.h"

#include "misc/Debug.h"

#include <QMutex>
#include <QMutexLocker>
#include <QThread>

#include <map>
#include <string>

#include <unistd.h>  // gettid()

#define LOCKED QMutexLocker rgMapperLocker(&mutex)

namespace
{
    // Wrapper to provide the unused handle.
    LV2_URID LV2UridMap(LV2_URID_Map_Handle /*handle*/,
                        const char *uri)
    {
        return Rosegarden::LV2URIDMapper::uridMap(uri);
    }
    LV2_URID_Map lv2URIDMapFeature {NULL, &LV2UridMap};

    // Wrapper to provide the unused handle.
    const char *LV2UridUnmap(LV2_URID_Unmap_Handle /*handle*/,
                             LV2_URID urid)
    {
        return Rosegarden::LV2URIDMapper::uridUnmap(urid);
    }
    LV2_URID_Unmap lv2URIDUnmapFeature{NULL, &LV2UridUnmap};

    // This is likely not needed as we are only seeing calls from the
    // UI thread.  See comments below in uridMap().
    QMutex mutex;

    // Next URID.
    LV2_URID nextId{1};

    // URI -> URID
    typedef std::map<std::string /* URI */, LV2_URID> URIToURIDMap;
    URIToURIDMap uriToURIDMap;

    // URID -> URI
    typedef std::map<LV2_URID, std::string /* URI */> URIDToURIMap;
    URIDToURIMap uridToURIMap;

}


namespace Rosegarden
{


LV2_URID_Map *LV2URIDMapper::getURIDMapFeature()
{
    return &lv2URIDMapFeature;
}

LV2_URID_Unmap *LV2URIDMapper::getURIDUnmapFeature()
{
    return &lv2URIDUnmapFeature;
}

LV2_URID
LV2URIDMapper::uridMap(const char *uri)
{
    // The UI thread is the main user of this.  However, since we send
    // this to each plugin as a feature, it's possible that a plugin might
    // call this either in its worker thread or its real-time processing
    // thread (seems dangerous).  The worker thread is really our UI
    // thread, so not an issue.  The real-time processing thread is likely
    // the JACK process callback thread (jack_set_process_callback()).  If
    // there is even the slightest chance this might be called from that,
    // then we need a mutex.  Unfortunately the LV2 docs are silent on this.
    // Might want to check Carla.
    //
    // In (limited) testing, I've only seen the UI thread calling into
    // this routine.

#ifdef THREAD_DEBUG
    RG_WARNING << "uridMap(): gettid(): " << gettid();
#endif

    // In case we are called by the real-time processing thread?
    LOCKED;

    URIToURIDMap::const_iterator it = uriToURIDMap.find(uri);
    // Not found?  Create a new URID and entry in the map.
    if (it == uriToURIDMap.end()) {
        const int id = nextId++;
        uriToURIDMap[uri] = id;
        uridToURIMap[id] = uri;
        return id;
    }

    return it->second;
}

const char *
LV2URIDMapper::uridUnmap(const LV2_URID urid)
{
    // ??? I never see this called.  Ever.
#ifdef THREAD_DEBUG
    RG_WARNING << "uridUnmap(): gettid(): " << gettid();
#endif

    // In case we are called by the real-time processing thread?
    LOCKED;

    URIDToURIMap::const_iterator it = uridToURIMap.find(urid);
    // Not found?
    if (it == uridToURIMap.end())
        return "";

    return it->second.c_str();
}


}
