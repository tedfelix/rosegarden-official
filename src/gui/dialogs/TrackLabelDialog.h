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

#ifndef RG_TRACKLABELDIALOG_H
#define RG_TRACKLABELDIALOG_H

#include <QDialog>


namespace Rosegarden
{

class LineEdit;

/** Basically a double QInputDialog that has two text prompts and returns two
 * text results.  Originally built to allow entering two different types of
 * track labels in one dialog.
 */
class TrackLabelDialog : public QDialog
{
    Q_OBJECT

public:
    TrackLabelDialog(QWidget *parent,
                     const QString &title,
                     const QString &PrimaryLabel,
                     const QString &PrimaryContents,
                     const QString &PrimaryTooltip,
                     const QString &SecondaryLabel,
                     const QString &SecondaryContents,
                     const QString &SecondaryTooltip);

    QString getPrimaryText();
    QString getSecondaryText();


protected:

    LineEdit          *m_primaryText;
    LineEdit          *m_secondaryText;
};


}


#endif
