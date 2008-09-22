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

#ifndef _RG_AUDIOLISTVIEW_H_
#define _RG_AUDIOLISTVIEW_H_

#include <QTreeWidget>

#include <Q3DragObject>

namespace Rosegarden {
        
class AudioListView : public QTreeWidget
{
public:
    AudioListView(QWidget *parent = 0, const char *name = 0);

protected:
    bool acceptDrag(QDropEvent* e) const;
    virtual Q3DragObject* dragObject();
};

}

#endif /*AUDIOLISTVIEW_H_*/
