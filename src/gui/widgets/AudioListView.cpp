/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "AudioListView.h"

#include "misc/Debug.h"
#include "gui/widgets/AudioListItem.h"

#include <Q3DragObject>
#include <Q3UriDrag>
#include <Q3TextDrag>

// #include <QMimeData>	// qt4: replaces Q3DragObject and Q3UriDrag


namespace Rosegarden {
        
AudioListView::AudioListView(QWidget *parent, const char *name)
    : QTreeWidget(parent)
{
	setObjectName( name );
    setDragEnabled(true);
    setAcceptDrops(true);
//&&&    setDropVisualizer(false);
}

bool AudioListView::acceptDrag(QDropEvent* e) const
{
    return false;
//&&&    return Q3UriDrag::canDecode(e) || QTreeWidget::acceptDrag(e);
}

Q3DragObject* AudioListView::dragObject()
{
    AudioListItem* item = dynamic_cast<AudioListItem*>(currentItem());

    QString audioData;
    QTextOStream ts(&audioData);
    ts << "AudioFileManager\n"
       << item->getId() << '\n'
       << item->getStartTime().sec << '\n'
       << item->getStartTime().nsec << '\n'
       << item->getDuration().sec << '\n'
       << item->getDuration().nsec << '\n';

    RG_DEBUG << "AudioListView::dragObject - "
             << "file id = " << item->getId()
             << ", start time = " << item->getStartTime() << endl;
    
    return new Q3TextDrag(audioData, this);
}

}
