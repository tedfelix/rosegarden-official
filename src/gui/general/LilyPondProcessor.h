/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LILYPOND_PROCESSOR_H
#define RG_LILYPOND_PROCESSOR_H

#include "gui/widgets/ProgressBar.h"

#include <QDialog>
#include <QLabel>
#include <QProcess>


namespace Rosegarden
{


/// The small pop-up that appears when previewing or printing in notation.
/**
 * This dialog handles running LilyPond to generate a pdf of the score and then
 * either a pdf viewer or a printing command like lpr on the generated pdf file.
 */
class LilyPondProcessor : public QDialog
{
    Q_OBJECT

public:

    enum class Mode { Preview, Print };

    /**
     * The filename parameter should be a temporary file set elsewhere and
     * passed in.  Unfortunately, there was a bit of a snag with that.  The
     * QProcess bits choke when start() is called with a filename that has an
     * absolute path in it, but the code in RosegardenMainWindow needs to have
     * an absolute path where the file is going to reside in order for
     * QTemporaryFile to ensure a safe, non-duplicate filename is created, so we
     * have to pass the absolute path in and then strip it back out inside here,
     * using QProcess::setWorkingDirectory() instead.  (This is pretty ugly, but
     * I'm stumped coming up with a cleaner solution, so damn the torpedoes.)
     */
    LilyPondProcessor(QWidget *parent,
                      Mode mode,
                      const QString &filename);

private:

    Mode m_mode;
    QString       m_filename;
    QString       m_dir;
    ProgressBar  *m_progress;
    QLabel       *m_info;
    QProcess     *m_process;

    void print();
    void preview();

private slots:
    /**
     * Display an explanatory failure message and terminate processing
     */
    void fail(const QString &error, const QString &details = QString());

    /**
     * Try to run convert-ly and call runLilyPond() if successful
     */
    void runConvertLy();

    /**
     * Try to run lilypond and call runFinalStage() if successful
     */
    void runLilyPond(int exitCode, QProcess::ExitStatus);

    /**
     * Try to launch an external PDF viewer or file printer on the successfully
     * processed .pdf file
     */
    void runFinalStage(int exitCode, QProcess::ExitStatus);

    /// Clean up when everything is done.
    /**
     * ??? Rename: finishedPrinting()
     */
    void finished2(int exitCode, QProcess::ExitStatus);
};


}

#endif
