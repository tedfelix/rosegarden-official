/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_FILEUTIL_H
#define RG_FILEUTIL_H

#include <QString>


namespace Rosegarden
{

/// A collection of general-use file-related utilities.
namespace FileUtil
{

    /// Convert a relative path or file path to absolute.
    /**
     * Expands "~" and "." to an absolute path.
     */
    QString toAbsolute(
            const QString &relativePath, const QString &docAbsFilePath);

    /// Add a trailing "/" if there isn't one.
    QString addTrailingSlash(const QString &path);

}

}


#endif
