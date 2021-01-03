/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#pragma once

#include <QObject>
#include <QTimer>

namespace Rosegarden
{


/// Class to manage button typematic (auto-repeat) behavior.
class Typematic : public QObject
{
    Q_OBJECT;

public:
    Typematic();

    /// Press (true) or release (false) the repeating button.
    void press(bool pressed);

signals:
    /// Sent repeatedly in response to a press.
    void click();

private slots:
    void slotTimeout();

private:
    QTimer timer;
};


}
