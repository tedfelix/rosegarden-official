/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    This file originally from Sonic Visualiser, copyright 2006 Chris
    Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TEMP_DIRECTORY_H
#define RG_TEMP_DIRECTORY_H

#include <QString>
#include <QMutex>

#include <exception>

namespace Rosegarden
{

class DirectoryCreationFailed : virtual public std::exception
{
public:
    DirectoryCreationFailed(QString directory) throw();
    ~DirectoryCreationFailed() throw() override;
    const char *what() const throw() override;
    
protected:
    QString m_directory;
};

/**
 * A class that manages the creation and removal of a temporary
 * directory tree to store data during the program run.  There is one
 * root temporary directory for the program, created on demand and
 * deleted when the program exits.
 *
 * This class is thread safe.
 */

class TempDirectory
{
public:
    static TempDirectory *getInstance();
    
    virtual ~TempDirectory();

    /**
     * Create the root temporary directory if necessary, and return
     * its path.  Throw DirectoryCreationFailed if the directory
     * cannot be created.
     */
    QString getPath();

    /** 
     * Create an immediate subdirectory of the root temporary
     * directory of the given name, if it doesn't already exist, and
     * return its path.  Throw DirectoryCreationFailed if the
     * directory cannot be created.
     */
    QString getSubDirectoryPath(QString subdir);

    /**
     * Delete the temporary directory (before exiting).
     */
    void cleanup();

protected:
    TempDirectory();

    QString createTempDirectoryIn(QString inDir);
    void cleanupDirectory(QString tmpDir);
    void cleanupAbandonedDirectories(QString svDir);

    QString m_tmpdir;
    QMutex m_mutex;

    static TempDirectory *m_instance;
};

}

#endif
