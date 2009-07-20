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


FileDialog::FileDialog(QWidget *parent,
                       const QString &caption,
                       const QString &dir,
                       const QString &filter) :
        QFileDialog(parent,
                    caption,
                    dir,
                    filter)
{
    // Since we're here anyway, there may be a way to style the directory
    // navigation arrows from inside here.  It never worked from the external
    // stylesheet, and I can't even remember what I tried unsuccessfully in the
    // past.
   
    // set up the sidebar stuff; the entire purpose of this class 
    QList<QUrl> urls;

    urls << QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation))
         << QUrl::fromLocalFile("~/.local/share/rosegarden/examples")
         << QUrl::fromLocalFile("~/.local/share/rosegarden/templates")
         << QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::MusicLocation))
         << QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));

    setSidebarUrls(urls);

}


FileDialog::~FileDialog()
{
}


QString
FileDialog::getOpenFileName(QWidget *parent,
                            const QString &caption,
                            const QString &dir,
                            const QString &filter,
                            QString *selectedFilter,
                            QFileDialog::Options options)
{
    FileDialog dialog(parent, caption, dir, filter);

    if (options)
        dialog.setOptions(options);

    // (code borrowed straight out of Qt 4.5.0 Copyright 2009 Nokia)
    if (selectedFilter)
        dialog.selectNameFilter(*selectedFilter);

    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedFilter();
        return dialog.selectedFiles().value(0);
    }

    return QString();
}


QStringList
FileDialog::getOpenFileNames(QWidget *parent,
                             const QString &caption,
                             const QString &dir,
                             const QString &filter,
                             QString *selectedFilter,
                             QFileDialog::Options options)
{
    FileDialog dialog(parent, caption, dir, filter);

    if (options)
        dialog.setOptions(options);

    // (code borrowed straight out of Qt 4.5.0 Copyright 2009 Nokia)
    if (selectedFilter)
        dialog.selectNameFilter(*selectedFilter);

    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedFilter();
        return dialog.selectedFiles();
    }

    return QStringList();
}


QString
FileDialog::getSaveFileName(QWidget *parent,
                            const QString &caption,
                            const QString &dir,
                            const QString &filter,
                            QString *selectedFilter,
                            QFileDialog::Options options)
{
    FileDialog dialog(parent, caption, dir, filter);

    if (options)
        dialog.setOptions(options);

    // (code borrowed straight out of Qt 4.5.0 Copyright 2009 Nokia)
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (selectedFilter)
        dialog.selectNameFilter(*selectedFilter);

    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedFilter();
        return dialog.selectedFiles().value(0);
    }

    return QString();
}


}

#include "FileDialog.moc"
