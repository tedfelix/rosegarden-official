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

#ifndef RG_WARNINGDIALOG_H
#define RG_WARNINGDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QIcon>

namespace Rosegarden
{

/** A WarningDialog is a place to present queued warnings from various
 * sub-optimal runtime conditions (eg. bad timer, no JACK) so they can be
 * displayed and flipped through at the user's convenience.
 *
 * \author D. Michael McIntyre
 */
class WarningDialog : public QDialog
{
    Q_OBJECT
public:
    WarningDialog(QWidget *parent = 0);
    ~WarningDialog();

    /** We'll build the message queue out of these for convenience, so both the
     * text and informative text can be tossed about as one unit
     *
     * (copied typedef from WarningWidget because I got bored with trying to
     * figure out whatever was wrong with the way I tried to share it between
     * these classes...)
     */
    typedef std::pair<std::pair<QString, QString>, int> Message;

    // also copied from WarningWidget.h, which is lame, but I can't be bothered
    // to do anything about that just now
    typedef enum { Midi, Audio, Timer, Other, Info } WarningType;

    void addWarning(Message message);

private:
    QTabWidget *m_tabWidget;

};

}

#endif
