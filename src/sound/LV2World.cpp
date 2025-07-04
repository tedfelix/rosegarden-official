/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LV2World]"
#define RG_NO_DEBUG_PRINT 1

#include "LV2World.h"

#include "misc/Debug.h"

namespace
{

    /// Simple auto construct/destroy to make sure we call lilv_world_free().
    class LV2WorldAuto
    {
    public:
        LV2WorldAuto()
        {
            // Thread-Safe
            // Guaranteed in C++11 to be lazy initialized and thread-safe.
            // See ISO/IEC 14882:2011 6.7(4).
            // This object is static constructed in LV2World::get() below.

            RG_DEBUG << "LV2WorldAuto create world";
            m_world = lilv_world_new();
            lilv_world_load_all(m_world);
        }
        ~LV2WorldAuto()
        {
            // No need to lock here.  We are going down at static destruction
            // time.  See getInstance().  Everyone who ever talked to us should
            // be gone by now.  If there is thread contention at this point in
            // time, we've got Static Destruction Order Fiasco (some other dtor
            // is trying to talk to us at static destruction time) and that won't
            // be solved with a lock.

            lilv_world_free(m_world);
        }
        LilvWorld *get()
        {
            return m_world;
        }
    private:
        LilvWorld *m_world;
    };

}

namespace Rosegarden
{
    namespace LV2World
    {

        LilvWorld *get()
        {
            // Guaranteed in C++11 to be lazy initialized and thread-safe.
            // See ISO/IEC 14882:2011 6.7(4).
            static LV2WorldAuto lv2WorldAuto;
            return lv2WorldAuto.get();
        }

    }
}
