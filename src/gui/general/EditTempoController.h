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

#ifndef EDITTEMPOCONTROLLER_H
#define EDITTEMPOCONTROLLER_H

#include <QObject>
#include "gui/dialogs/TempoDialog.h"

namespace Rosegarden
{

class RosegardenDocument;
class Composition;

/**
 * @brief The EditTempoController class centralizes code for tempo edition
 * It is called both by the Tempo Ruler instances, by the main window (for shortcuts, and for the transport dialog)
 */
class EditTempoController : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor. Called by the mainwindow
     */
    EditTempoController(QObject *parent = nullptr);

    /// The user of this class *must* call this method.
    void setDocument(RosegardenDocument *doc);

    /**
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
    void editTempo(QWidget *parent, timeT atTime, bool timeEditable = false);
    void editTimeSignature(QWidget *parent, timeT time);
    void moveTempo(timeT oldTime, timeT newTime);
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
