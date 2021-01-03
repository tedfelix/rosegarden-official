
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_FILELOCATEDIALOG_H
#define RG_FILELOCATEDIALOG_H

#include <QDialog>
#include <QString>


class QWidget;


namespace Rosegarden
{



class FileLocateDialog : public QDialog
{
    Q_OBJECT

public:
    FileLocateDialog(QWidget *parent,
                     const QString &file,
                     const QString &path);

    QString getDirectory() { return m_path; }
    QString getFilename() { return m_file; }

protected slots:
    virtual void slotUser1();
    virtual void slotUser2();
    virtual void slotUser3();

protected:
    QString m_file;
    QString m_path;
};
  

}

#endif
