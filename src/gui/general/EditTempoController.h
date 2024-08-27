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

#ifndef EDITTEMPOCONTROLLER_H
#define EDITTEMPOCONTROLLER_H

#include <QObject>
#include "gui/dialogs/TempoDialog.h"


namespace Rosegarden
{


class RosegardenDocument;
class Composition;


/// A collection of tempo and time signature dialogs and commands.
/**
 * @brief The EditTempoController class centralizes code for tempo edition
 * It is called both by the Tempo Ruler instances, by the main window (for shortcuts, and for the transport dialog)
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
    /**
     * Constructor. Called by the mainwindow
     */
    explicit EditTempoController(QObject *parent = nullptr);

    /// Make sure the current document is used.
    /**
     * ??? Should be able to safely switch to RosegardenDocument::currentDocument
     *     and get rid of this.
     */
    void setDocument(RosegardenDocument *doc);

    /**
     * ??? Make this a proper Singleton so that we don't have to jump
     *     through so many hoops.  Create on first call to Self.  Maintain
     *     the instance internally.  Private ctor.  Etc...
     *
     * Returns the unique instance of EditTempoController, created by the mainwindow.
     * This is only an acceptable hack because the mainwindow itself is a singleton.
     * The alternative is to pass this down from the mainwindow to
     * MatrixView -> MatrixWidget -> TempoRuler
     * NotationView -> NotationWidget -> TempoRuler
     * RosegardeMainViewWidget -> TrackEditor -> TempoRuler
     * so we might as well first collect more controllers into a single object to pass down to all these.
     */
    static EditTempoController *self();

    void emitEditTempos(timeT time) { emit editTempos(time); }

    /// Launches a TempoDialog.
    void editTempo(QWidget *parent, timeT atTime, bool timeEditable = false);
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
    RosegardenDocument *m_doc;
    Composition *m_composition;
};

}

#endif // EDITTEMPOCONTROLLER_H
