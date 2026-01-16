/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMPOSITIONPOSITION_H
#define RG_COMPOSITIONPOSITION_H

#include "base/TimeT.h"
#include "base/RealTime.h"

#include <QObject>

class QTimer;


namespace Rosegarden
{

class RosegardenDocument;

/// The Playback Position Pointer (PPP).
class ROSEGARDENPRIVATE_EXPORT CompositionPosition  : public QObject
{
    Q_OBJECT

public:
    /// Singleton
    static CompositionPosition* getInstance();

    timeT get() const;
    RealTime getElapsedTime() const;

public slots:
    void slotSet(timeT time);

signals:
    /// Emitted whenever the position is changed.
    void changed(timeT);

public slots:
    void documentAboutToChange();
    void documentLoaded(RosegardenDocument* doc);

private:
    CompositionPosition();

    QTimer* m_updateTimer;

    // cached position values
    timeT m_position;
    RealTime m_positionAsElapsedTime;

private slots:
    void slotUpdate();
};


}

#endif // RG_COMPOSITIONPOSITION_H
