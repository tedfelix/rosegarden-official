
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TIMEDIALOG_H
#define RG_TIMEDIALOG_H

#include <QDialog>
#include <QString>
#include "base/Event.h"


class QWidget;


namespace Rosegarden
{

class TimeWidget;
class Composition;


class TimeDialog : public QDialog
{
    Q_OBJECT
public:
    /// for absolute times
    TimeDialog(QWidget *parent, QString title, Composition *composition,
               timeT defaultTime, bool constrainToCompositionDuration);

    /// for durations
    TimeDialog(QWidget *parent, QString title, Composition *composition,
               timeT startTime, timeT defaultDuration, timeT minimumDuration,
               bool constrainToCompositionDuration);

    timeT getTime() const;

protected:
    TimeWidget *m_timeWidget;
};
                     


}

#endif
