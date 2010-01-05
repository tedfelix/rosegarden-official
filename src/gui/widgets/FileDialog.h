/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _FILE_DIALOG_H_
#define _FILE_DIALOG_H_

#include <QFileDialog>

/** We used to use static convenience functions to create file dialogs, like
 * fname = QFileDialog::getOpenFilename(...).  This was convenient, but
 * unfortunately did not permit us to do anything with the default sidebar.
 *
 * Rather than replace all of those occurrences with block copied boilerplate
 * code, I decided to put the boilerplate code into a new drop-in class.
 *
 * (All of this sure was a fat lot of work just to fix the stupid speedbar.  You
 * can imagine my pleasure upon realizing I really had no easier choice than
 * this.)
 *
 *
 * \author D. Michael McIntyre
 */
namespace Rosegarden
{

class FileDialog : public QFileDialog
{
    Q_OBJECT
public:
    explicit FileDialog(QWidget *parent = 0,
                        const QString &caption = QString(),
                        const QString &dir = QString(),
                        const QString &filter = QString());

    ~FileDialog();

    /** See documentation for QFileDialog::getOpenFilename()
     */
    static QString getOpenFileName(QWidget *parent = 0,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = 0,
                                   QFileDialog::Options options = 0);

    /** See documentation for QFileDialog::getOpenFileNames()
     */
    static QStringList getOpenFileNames(QWidget *parent = 0,
                                        const QString &caption = QString(),
                                        const QString &dir = QString(),
                                        const QString &filter = QString(),
                                        QString *selectedFilter = 0,
                                        QFileDialog::Options options = 0);

    /** See documentation for QFileDialog::getSaveFilename()
     */
    static QString getSaveFileName(QWidget *parent = 0,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = 0,
                                   QFileDialog::Options options = 0);
};

}

#endif
