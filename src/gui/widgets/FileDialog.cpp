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
#include <QSettings>

#include "misc/Debug.h"
#include "misc/ConfigGroups.h"

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

#if QT_VERSION >= 0x050000
    QString home = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).path();
#else
    QString home = QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation)).path();
#endif
    QString examples = home + "/.local/share/rosegarden/examples";
    QString templates = home + "/.local/share/rosegarden/templates";
    QString rosegarden = home + "/rosegarden";

    RG_DEBUG  << "FileDialog::FileDialog(...)" << endl
              << "     using paths:  examples: " << examples << endl
              << "                  templates: " << templates << endl
              << "                 rosegarden: " << rosegarden << endl;

#if QT_VERSION >= 0x050000
    urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
#else
    urls << QUrl::fromLocalFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation))
#endif
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

    // NOTE: This code only executes if the Thorn style is in use, so there is
    // no need for conditional execution here.  Go straight to it, and hack the
    // stylesheet.  This fixes #1396, by ensuring that the file dialog has white
    // in the right places, even at deep levels of style inheritance.
    setStyleSheet("QAbstractScrollArea { background: #FFFFFF;} QLineEdit { background: #FFFFFF; }");
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
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();

    if (!Thorn) {
        return QFileDialog::getOpenFileName(parent, caption, dir, filter,
                                            selectedFilter, options);
    }

    FileDialog dialog(parent, caption, dir, filter);

#if QT_VERSION >= 0x040500
    if (options)
       dialog.setOptions(options);
#endif

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
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();

    if (!Thorn) {
        return QFileDialog::getOpenFileNames(parent, caption, dir, filter,
                                             selectedFilter, options);
    }

    FileDialog dialog(parent, caption, dir, filter);

#if QT_VERSION >= 0x040500
    if (options)
        dialog.setOptions(options);
#endif

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
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();

    if (!Thorn) {
        return QFileDialog::getSaveFileName(parent, caption, dir, filter,
                                            selectedFilter, options);
    }

    FileDialog dialog(parent, caption, dir, filter);

#if QT_VERSION >= 0x040500
    if (options)
        dialog.setOptions(options);
#endif

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
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();

    if (!Thorn) {
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

