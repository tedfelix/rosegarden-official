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

#define RG_MODULE_STRING "[FileUtil]"
#define RG_NO_DEBUG_PRINT

#include "FileUtil.h"

#include <QDir>


namespace Rosegarden
{

namespace FileUtil
{

QString toAbsolute(const QString &relativePath, const QString &docAbsFilePath)
{
    if (relativePath.isEmpty())
        return relativePath;

    QString absolutePath = relativePath;

    // Convert tilde to home dir.
    if (absolutePath.left(1) == "~") {
        absolutePath.remove(0, 1);
        absolutePath = QDir::homePath() + absolutePath;
    }

    // Handle double-dot.  A bit messy, but should work.
    if (absolutePath.left(2) == "..")
        absolutePath = "./" + absolutePath;

    // Convert dot to .rg file location.
    if (absolutePath.left(1) == ".") {
        absolutePath.remove(0, 1);
        QString absFilePath = docAbsFilePath;
        QFileInfo fileInfo(absFilePath);
        absolutePath = fileInfo.canonicalPath() + absolutePath;
    }

    return absolutePath;
}

QString addTrailingSlash(const QString &path)
{
    if (path.isEmpty())
        return "/";

    QString path2 = path;

    // Add a trailing "/" if needed.
    if (!path2.endsWith('/'))
        path2 += "/";

    return path2;
}

}


}
