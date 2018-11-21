/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PROGRESSBAR_H
#define RG_PROGRESSBAR_H

#include <QProgressBar>


class QWidget;


namespace Rosegarden
{

/** An odd legacy.  This used to be a subclass of KProgress, but even then it
 * had a bool useDelay that was a no-op.  It seems the only useful thing we can
 * do with this is convert it to a subclass of QProgressBar, and do a
 * setRange(0, totalSteps) in the ctor to mimic the old behavior.
 */
class ProgressBar : public QProgressBar
{
    Q_OBJECT

public:
    ProgressBar(QWidget *parent = nullptr);

    ProgressBar(int totalSteps,
                QWidget *parent = nullptr);

public slots:
    void WTF(int);

};


}

#endif
