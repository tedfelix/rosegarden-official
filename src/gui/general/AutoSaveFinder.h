/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2007 Queen
    Mary, University of London.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUTO_SAVE_FINDER_H
#define RG_AUTO_SAVE_FINDER_H

#include <QString>

namespace Rosegarden {
	
class AutoSaveFinder
{
public:
    AutoSaveFinder() { }
    virtual ~AutoSaveFinder() { }

    /**
     * Return the auto-save path for the given filename.
     */
    QString getAutoSavePath(QString filename);

    /**
     * Return the auto-save path for the given filename, if and only
     * if there is a non-empty file there which may merit recovery.
     */
    QString checkAutoSaveFile(QString filename);

protected:
    QString getAutoSaveDir();
};

}

#endif

    
