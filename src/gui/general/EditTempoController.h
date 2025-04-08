/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EDITTEMPOCONTROLLER_H
#define RG_EDITTEMPOCONTROLLER_H

#include "gui/dialogs/TempoDialog.h"

#include <QObject>


namespace Rosegarden
{


class RosegardenDocument;
class Composition;


/// A collection of tempo and time signature dialogs and commands.
/**
 * The EditTempoController class centralizes code for tempo edition It is
 * called both by the Tempo Ruler instances, by the main window (for shortcuts,
 * and for the transport dialog)
 *
 * ??? It's hard to think of an appropriate name for this assorted collection of
 *     dialogs and commands.  Therefore it is difficult to justify its
 *     existence.  We should see if we can move these routines to the dialogs
 *     and commands that they create.  E.g. editTempo() might become
 *     TempoDialog::launch().  Or just inline these where they are used.  There
 *     isn't much to them.
 */
class EditTempoController : public QObject
{
    Q_OBJECT

public:

    /// Singleton instance.
    /**
     * Returns the unique instance of EditTempoController, created by
     * RosegardenMainWindow.  This is only an acceptable hack because
     * RosegardenMainWindow itself is a Singleton.  The alternative is to pass
     * this down from the RosegardenMainWindow to:
     *
     *   - MatrixView -> MatrixWidget -> TempoRuler
     *   - NotationView -> NotationWidget -> TempoRuler
     *   - RosegardeMainViewWidget -> TrackEditor -> TempoRuler
     *
     * So we might as well first collect more controllers into a single object
     * to pass down to all these.
     */
    static EditTempoController *self();

    /// Make sure the current document is used.
    /**
     * ??? Should be able to safely switch to
     *     RosegardenDocument::currentDocument and get rid of this.
     */
    void setDocument(RosegardenDocument *doc);

    void emitEditTempos(timeT time)  { emit editTempos(time); }

    /// Launches a TempoDialog.
    void editTempo(QWidget *parent, timeT atTime, bool timeEditable);
    /// Launches a TimeSignatureDialog.
    void editTimeSignature(QWidget *parent, timeT time);
    /// Assembles and executes a command to move the tempo at oldTime to newTime.
    void moveTempo(timeT oldTime, timeT newTime);

    /// Creates and executes a RemoveTempoChangeCommand.
    void deleteTempoChange(timeT time);

signals:

    void editTempos(timeT time);

public slots:

    void changeTempo(timeT time,
                     tempoT value,
                     tempoT target,
                     TempoDialog::TempoDialogAction action);

private:

    /// Singleton.
    EditTempoController()  { }

    RosegardenDocument *m_doc{nullptr};
    Composition *m_composition{nullptr};

};


}

#endif
