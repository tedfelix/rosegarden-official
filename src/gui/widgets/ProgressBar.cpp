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


#include "ProgressBar.h"

#include <QProgressBar>
#include <QProgressDialog>

#include <QWidget>


namespace Rosegarden
{

ProgressBar::ProgressBar(int totalSteps,
                         bool /*useDelay*/,
                         QWidget *creator,
                         const char *name,
                         WFlags f) :
        QProgressBar(creator)
//@@@ QProgressBar is quite different from KProgressWhateverWeReplaced, and its
// ctor takes only QProgressBar ( QWidget * parent = 0 ).  I think chucking
// "creator" in here is right, but who knows.
{
    // We need some totally rewritten guts here. There's a LOT more to this than
    // just swapping KWhateverWeReplaced for a QT4 method that has practically
    // nothing in common with either its QT3 or KDE3 ancestors.
}

}
#include "ProgressBar.moc"
