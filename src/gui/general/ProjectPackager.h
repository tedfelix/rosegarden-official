/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _PROJECT_PACKAGER_H_
#define _PROJECT_PACKAGER_H_

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QProcess>


namespace Rosegarden
{

/** Implement functionality equivalent to the old external
 *  rosegarden-project-package script.  The script used the external dcop and
 *  kdialog command line utlities to provide a user interface.  We'll do the
 *  user interface in real code, though we still have to use external helper
 *  applications for various purposes to make the thing run.
 *
 *  \author D. Michael McIntyre
 */

class ProjectPackager : public QDialog
{
    Q_OBJECT

public:
    /** The old command line arguments are replaced with an int passed into the
     * ctor, using the following named constants to replace them, and avoid a
     * bunch of string parsing nonsense.  We no longer need a startup ConfTest
     * target.  We'll do the conftest every time we run instead, and only
     * complain if there is a problem.  We no longer need a version target,
     * since the version is tied to Rosegarden itself.
     *
     * The filename parameter should be a temporary file set elsewhere and
     * passed in.
     */
    static const int ConfTest  = 0;
    static const int Pack      = 1;
    static const int Unpack    = 2;

    ProjectPackager(QWidget *parent,
                      int mode,
                      QString filename);
    ~ProjectPackager() { };

protected:
    int           m_mode;
    QString       m_filename;
    QProgressBar *m_progress;
    QLabel       *m_info;
    QProcess     *m_process;

protected slots:
    /**
     * Display an explanatory failure message and terminate processing
     */
    void puke(QString error);

    /**
     * Try to unpack an existing .rgd file
     */
    void runUnpack();

    /**
     * Try to run lilypond and call runFinalStage() if successful
     */
    void runPack();
};


}

#endif
