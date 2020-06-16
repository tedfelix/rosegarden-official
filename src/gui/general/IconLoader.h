/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ICON_LOADER_H
#define RG_ICON_LOADER_H

#include <QIcon>
#include <QPixmap>
#include <QString>

// Needed because loadPixmap() is called by main().
#include <rosegardenprivate_export.h>

namespace Rosegarden {


namespace IconLoader
{
    QIcon load(QString name);
    QPixmap ROSEGARDENPRIVATE_EXPORT loadPixmap(QString name);

    /// Invert pixmap for dark backgrounds.
    /**
     * This is only used by NotationView::setCurrentNotePixmap() to
     * display the small inverted note symbol in the status bar to the
     * far right.  You have to change note value to get it to appear at
     * first.
     *
     * ??? This should be moved to NotationView.
     */
    QPixmap invert(QPixmap);
}


}

#endif

	
