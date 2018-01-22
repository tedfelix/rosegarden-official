/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SEQUENCER_THREAD_H
#define RG_SEQUENCER_THREAD_H

#include <QThread>

namespace Rosegarden
{

/// The Sequencer Thread
/**
 * This class consists of a single processing loop, run(), which keeps
 * RosegardenSequencer processing incoming and outgoing MIDI events.
 *
 * A single instance of this is created and owned by RosegardenMainWindow.
 *
 * @see RosegardenMainWindow::m_sequencerThread
 * @see RosegardenMainWindow::launchSequencer()
 */
class SequencerThread : public QThread
{
protected:
    /// The sequencer thread's processing loop.
    virtual void run();
};

}

#endif
