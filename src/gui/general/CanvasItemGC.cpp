/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "CanvasItemGC.h"

#include "misc/Debug.h"

#include <qcanvas.h>

namespace Rosegarden
{

void CanvasItemGC::mark(QCanvasItem* item)
{
    if (!item)
        return ;

    item->hide();
    //     RG_DEBUG << "CanvasItemGC::mark() : "
    //                          << item << std::endl;
    m_garbage.push_back(item);
}

void CanvasItemGC::gc()
{
    for (unsigned int i = 0; i < m_garbage.size(); ++i) {
        //         RG_DEBUG << "CanvasItemGC::gc() : delete "
        //                              << m_garbage[i] << "\n";
        delete m_garbage[i];
    }

    m_garbage.clear();
}

void CanvasItemGC::flush()
{
    m_garbage.clear();
}

std::vector<QCanvasItem*> CanvasItemGC::m_garbage;

}
