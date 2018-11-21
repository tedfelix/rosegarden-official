/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_FILE_DIALOG_H
#define RG_FILE_DIALOG_H

#include <QFileDialog>

/**
 * We used to use static convenience functions to create file dialogs, like
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
 * NOTE: FileDialog worked rather well until both Rosegarden and KDE migrated to
 * Qt5.  New File->Open Example... and Open Template... have been provided, and
 * there may not be any remaining use for this class.  We can probably switch
 * everything back over to QFileDialog in all cases, but I don't feel like doing
 * that much work to find out what breaks.  I decided to leave well enough
 * alone, and I left this code and the hundreds of things referring to it in
 * place.
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
    ~FileDialog() override;

    /** See documentation for QFileDialog::getOpenFilename()
     */
    static QString getOpenFileName(QWidget *parent = nullptr,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr,
                                   QFileDialog::Options options = nullptr);

    /** See documentation for QFileDialog::getOpenFileNames()
     */
    static QStringList getOpenFileNames(QWidget *parent = nullptr,
                                        const QString &caption = QString(),
                                        const QString &dir = QString(),
                                        const QString &filter = QString(),
                                        QString *selectedFilter = nullptr,
                                        QFileDialog::Options options = nullptr);

    /**
     * Based on QFileDialog::getSaveFileName().
     * This version allows specification of a default filename (defaultName)
     * to save the user some typing.
     */
    static QString getSaveFileName(QWidget *parent = nullptr,
                                   const QString &caption = QString(),
                                   const QString &dir = QString(),
                                   const QString &defaultName = QString(),
                                   const QString &filter = QString(),
                                   QString *selectedFilter = nullptr,
                                   QFileDialog::Options options = nullptr);

    /**
     * Subclass of QFileDialog::getExistingDirectory() specifically to get an
     * existing directory, eg. an audio path.  This version has several details
     * hard coded in the implementation, and is less flexible than the full
     * QFileDialog version.
     */
    static QString getExistingDirectory(QWidget *parent = nullptr,
                                        const QString &caption = QString(),
                                        const QString &dir = QString());

protected:
    explicit FileDialog(QWidget *parent = nullptr,
                        const QString &caption = QString(),
                        const QString &dir = QString(),
                        const QString &filter = QString(), Options options = Options());

};

}

#endif
