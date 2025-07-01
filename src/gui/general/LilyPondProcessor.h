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

#ifndef RG_LILYPONDPROCESSOR_H
#define RG_LILYPONDPROCESSOR_H

#include <QDialog>
#include <QProcess>
#include <QString>

class QLabel;
class QWidget;


namespace Rosegarden
{


class ProgressBar;


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
     * filename: Temporary file path.  E.g. "/tmp/rosegarden_tmp_T73123.ly".
     */
    LilyPondProcessor(QWidget *parent,
                      Mode mode,
                      const QString &filename);

private:

    Mode m_mode;
    QString m_filename;
    QString m_workingDirectory;
    ProgressBar *m_progress;
    QLabel *m_info;
    QProcess *m_process;

    void print();
    void preview();

    /// Display an explanatory failure message and terminate processing.
    void fail(const QString &error, const QString &details = QString());

    /// Try to run convert-ly and call runLilyPond() if successful.
    void runConvertLy();

private slots:

    /// Try to run lilypond and call runFinalStage() if successful.
    void runLilyPond(int exitCode, QProcess::ExitStatus);

    /// Launch PDF viewer or file printer.
    void runFinalStage(int exitCode, QProcess::ExitStatus);

    /// Clean up when printing is done.
    void finishedPrinting(int exitCode, QProcess::ExitStatus);

};


}

#endif
