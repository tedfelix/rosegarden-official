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

#ifndef RG_PROJECT_PACKAGER_H
#define RG_PROJECT_PACKAGER_H

#include "document/RosegardenDocument.h"
#include "gui/widgets/ProgressBar.h"

#include <QDialog>
#include <QLabel>
#include <QProcess>
#include <QStringList>


namespace Rosegarden
{


/** Implement functionality equivalent to the old external
 *  rosegarden-project-package script.  The script used the external dcop and
 *  kdialog command line utlities to provide a user interface.  We'll do the
 *  user interface in real code, though we still have to use external helper
 *  applications for various purposes to make the thing run.
 *
 *  \author D. Michael McIntyre
 *  \author Ilan Tal
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
     * The calling code creates an .rg file with an .rgd extension and passes
     * this filename to us via the \arg filename parameter.
     */
    static const int ConfTest  = 0;
    static const int Pack      = 1;
    static const int Unpack    = 2;

    ProjectPackager(QWidget *parent,
                    RosegardenDocument *document,
                    int mode,
                    QString filename);
    ~ProjectPackager() { };

    /** Return the true filename as discovered when analyzing the contents of
     * the .rgp file.  foo.rgp might contain bar.rg and directory bar/
     */
    QString getTrueFilename();

protected:
    RosegardenDocument *m_doc;
    int                 m_mode;
    QString             m_filename;
    ProgressBar        *m_progress;
    QLabel             *m_info;
    QProcess           *m_process;
    
    /// The backend script has to be accessed from multiple locations
    QFile               m_script;

    /** The real filename contained within the project package.  It is necessary
     * to discover and transmit this because foo.rgp might really contain bar.rg
     * and data files in bar/ if the user ever renamed the file for some reason
     */
    QString             m_trueFilename;

    QString             m_packTmpDirName;
    QString             m_packDataDirName;


    /** Returns a QStringList containing a sorted|uniqed list of audio files
     * used by m_doc
     *
     * Problems to solve: how do we |sort|uniq a QStringList?  The same audio
     * file might be used by hundreds of audio segments (eg. Emergence) and
     * while we could pull it out and overwrite the same file 100 times to
     * result in only one final copy, that's a big waste.
     */
    QStringList getAudioFiles();

    /** Returns a QStringList containing a sorted|uniqued list of extra files
     * used by plugins
     *
     * While assembling that list, rewrites the static .rg file stored on disk
     * to update the path component of these various file references, as well as
     * the document audio path
     *
     * \p fileToModify  the .rg file we need to work on
     * \p newPath       the new path component we need to switch in
     */
    QStringList getPluginFilesAndRewriteXML(const QString fileToModify, const QString newPath);

    // to avoid troubles, check that flac is available. It is fast and could
    // possibly avoid troubles.
    void sanityCheck();

    QString m_abortText;

protected slots:
    /**
     * Display an explanatory failure message and terminate processing
     */
    void puke(QString error);

    /**
     * Recursively remove a directory and all of its contents.  Returns
     * true if successful.
     *
     * Candidate for promotion to a utility class, or perhaps Qt itself.
     */
    static bool rmdirRecursive(QString dirName);

    /** If user cancels, clean up tmp dir
     */
    void reject();

    /**
     * Begin the packing process.
     *
     *   - discover audio files used by the composition
     *   - remove old tmp directory (if exists)
     *   - create tmp directory
     *   - copy .rg file from the main window save operation into tmp dir
     *   - copy audio files into tmp dir
     *   - prompt for extra files
     *   - copy extra files
     *   - hand off to the decoder backend non-blocking QProcess slot
     */
     // run this after the sanity check
    void runPackUnpack(int exitCode, QProcess::ExitStatus);

    void runPack();

    /** Assemble the various encoding operations into one backend script to
     * operate from a single QProcess, so this chewing can take place without
     * blocking the GUI.  Since it's possible to track the total number of
     * files, and what file out of n we're on, we should work out some nice way
     * to hook this up to m_status and give some indication of progress,
     * instead of just leaving it in "busy" mode.
     *
     * And you know what?  It worked for unpack, do it here too.  Just tar the
     * fucker at the end of the encoder script and cut out a million lines of
     * extra code.
     */
    void startAudioEncoder(QStringList files);

    /** Final pack stage
     *
     * 1. Correct audio and plugin data paths
     *
     * 2. Clean up
     */
    void finishPack(int exitCode, QProcess::ExitStatus);

    /** The first stage of unpacking an .rgp file:
     *
     * 1. QProcess out a tar tf to obtain a list of files from the .rgp tarball
     *
     * 2. Comb this assembled list for .flac files
     *
     * 3. Pass this list to the decoder backend
     */
    void runUnpack();

    /** Assemble a script to:
      *
      * 1. Actually unpack the tarball (tar xzf)
      *
      * 2. Decode the .flac files and remove them
      *
      * 3. Decode the .wv files and remove them
      *
      * (Files coming in via two separate variables because of the organic way
      * all of this was assembled.  We could figure out the file type in
      * startAudioDecoder() itself, but the code was already there to make this
      * determination upstream, so just use that going in)
      */
    void startAudioDecoder(QStringList flacFiles, QStringList wavpackFiles);

    /** Final unpack stage
     *
     * 1. Correct audio and plugin data paths
     *
     * 2. Clean up
     */
    void finishUnpack(int exitCode, QProcess::ExitStatus);
};


}

#endif
