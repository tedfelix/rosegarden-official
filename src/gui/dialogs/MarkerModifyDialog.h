
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MARKERMODIFYDIALOG_H
#define RG_MARKERMODIFYDIALOG_H

#include "base/Marker.h"
#include "gui/widgets/TimeWidget2.h"
#include "gui/widgets/LineEdit.h"

#include <QDialog>
#include <QString>

class QDialogButtonBox;


namespace Rosegarden
{


class MarkerModifyDialog : public QDialog
{
    Q_OBJECT

public:
    MarkerModifyDialog(QWidget *parent,
                       int time,
                       const QString &text,
                       const QString &comment);

    MarkerModifyDialog(QWidget *parent,
                       Marker *marker);

    QString getText() const  { return m_text->text(); }
    QString getComment() const  { return m_comment->text(); }
    int getTime() const  { return m_timeWidget->getTime(); }
    int getOriginalTime() const  { return m_originalTime; }

private slots:

    void slotIsValid(bool valid);

private:

    void initialise(int time,
                    const QString &text,
                    const QString &comment);

    TimeWidget2 *m_timeWidget;
    LineEdit *m_text;
    LineEdit *m_comment;

    QDialogButtonBox *m_buttonBox;

    int m_originalTime;
};




}

#endif
