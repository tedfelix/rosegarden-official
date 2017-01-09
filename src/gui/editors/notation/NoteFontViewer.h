
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

#ifndef RG_NOTEFONTVIEWER_H
#define RG_NOTEFONTVIEWER_H

#include <QDialog>
#include <QString>
#include <QStringList>


class QWidget;
class QComboBox;


namespace Rosegarden
{

class FontViewFrame;


class NoteFontViewer : public QDialog
{
    Q_OBJECT

public:
    NoteFontViewer(QWidget *parent, QString noteFontName,
                   QStringList systemFontNames, int pixelSize);

protected slots:
    void slotFontChanged(const QString &);
    void slotViewChanged(int);
    void slotRowChanged(const QString &);

private:
    QComboBox *m_font;
    QComboBox *m_view;
    QComboBox *m_rows;
    FontViewFrame *m_frame;
};



}

#endif
