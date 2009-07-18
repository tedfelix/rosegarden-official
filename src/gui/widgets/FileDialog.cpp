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

#include "FileDialog.h"

#include <QFileDialog>
#include <QList>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{


FileDialog::FileDialog(QWidget *parent) :
        QFileDialog(parent)
{
    // Since we're here anyway, there may be a way to style the directory
    // navigation arrows from inside here.  It never worked from the external
    // stylesheet, and I can't even remember what I tried unsuccessfully in the
    // past.
    
    QList<QUrl> urls;

    urls << QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation))
          << QUrl::fromLocalFile("~/.local/share/rosegarden/templates")
          << QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));

    setSidebarUrls(urls);

}


FileDialog::~FileDialog()
{
}


QString
getOpenFilename(QWidget *parent = 0,
                const QString &caption = QString(),
                const QString &dir = QString(),
                const QString &filter = QString(),
                QString *selectedFilter = 0,
                QFileDialog::Options options = 0)
{
//    setWindowTitle(caption);
//    return "foo";
}


QStringList
getOpenFileNames(QWidget *parent = 0,
                 const QString &caption = QString(),
                 const QString &dir = QString(),
                 const QString &filter = QString(),
                 QString *selectedFilter = 0,
                 QFileDialog::Options options = 0)
{
//    setWindowTitle(caption);
}

QString
getSaveFileName(QWidget *parent = 0,
                const QString &caption = QString(),
                const QString &dir = QString(),
                const QString &filter = QString(),
                QString *selectedFilter = 0,
                QFileDialog::Options options = 0)
{
}


}

#include "FileDialog.moc"
