/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[FileDialog]"

#include "FileDialog.h"

#include <QFileDialog>
#include <QList>
#include <QUrl>
#include <QDesktopServices>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif
#include <QApplication>

#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "gui/general/ThornStyle.h"

namespace Rosegarden
{


FileDialog::FileDialog(QWidget *parent,
                       const QString &caption,
                       const QString &dir,
                       const QString &filter,
                       QFileDialog::Options options) :
        QFileDialog(parent,
                    caption,
                    dir,
                    filter)
{
    setOptions(options);

    // set up the sidebar stuff; the entire purpose of this class 
    QList<QUrl> urls;

#if QT_VERSION >= 0x050000
    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#else
    QString home = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
#endif
    QString examples = home + "/.local/share/rosegarden/examples";
    QString templates = home + "/.local/share/rosegarden/templates";
    QString rosegarden = home + "/rosegarden";

    RG_DEBUG  << "FileDialog::FileDialog(...)"
              << "     using paths:  examples: " << examples << endl
              << "                  templates: " << templates << endl
              << "                 rosegarden: " << rosegarden << endl;

    urls << QUrl::fromLocalFile(home)
         << QUrl::fromLocalFile(examples)
         << QUrl::fromLocalFile(templates)
#if QT_VERSION >= 0x050000
         << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
         << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MusicLocation))
#else
         << QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation))
         << QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::MusicLocation))
#endif
         << QUrl::fromLocalFile(rosegarden)
         ; // closing ; on this line to allow the lines above to be shuffled easily

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
    if (!ThornStyle::isEnabled()) {
        return QFileDialog::getOpenFileName(parent, caption, dir, filter,
                                            selectedFilter, options);
    }

    FileDialog dialog(parent, caption, dir, filter, options);

    // (code borrowed straight out of Qt 4.5.0 Copyright 2009 Nokia)
    if (selectedFilter)
        dialog.selectNameFilter(*selectedFilter);

    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedNameFilter();
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
    if (!ThornStyle::isEnabled()) {
        return QFileDialog::getOpenFileNames(parent, caption, dir, filter,
                                             selectedFilter, options);
    }

    FileDialog dialog(parent, caption, dir, filter, options);

    // (code borrowed straight out of Qt 4.5.0 Copyright 2009 Nokia)
    if (selectedFilter)
        dialog.selectNameFilter(*selectedFilter);

    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedNameFilter();
        return dialog.selectedFiles();
    }

    return QStringList();
}


QString
FileDialog::getSaveFileName(QWidget *parent,
                            const QString &caption,
                            const QString &dir,
                            const QString &defaultName,
                            const QString &filter,
                            QString *selectedFilter,
                            QFileDialog::Options options)
{
    if (!ThornStyle::isEnabled()) {
        return QFileDialog::getSaveFileName(parent, caption, dir, filter,
                                            selectedFilter, options);
    }

    FileDialog dialog(parent, caption, dir, filter, options);

    dialog.selectFile(defaultName);

    // (code borrowed straight out of Qt 4.5.0 Copyright 2009 Nokia)
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (selectedFilter)
        dialog.selectNameFilter(*selectedFilter);

    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedNameFilter();
        return dialog.selectedFiles().value(0);
    }

    return QString();
}

QString
FileDialog::getExistingDirectory(QWidget *parent,
                                 const QString &caption,
                                 const QString &dir)
{
    if (!ThornStyle::isEnabled()) {
        return QFileDialog::getExistingDirectory(parent, caption, dir, QFileDialog::ShowDirsOnly);
    }

    // (code adapted from Qt 4 Git repository (c) 2012 Digia)
    FileDialog dialog(parent, caption, dir);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);

    if (dialog.exec() == QDialog::Accepted) {
        return dialog.selectedFiles().value(0);
    }
    return QString();    
}

}

