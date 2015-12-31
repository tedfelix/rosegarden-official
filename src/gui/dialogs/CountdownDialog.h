
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

#ifndef RG_COUNTDOWNDIALOG_H
#define RG_COUNTDOWNDIALOG_H

#include <QDialog>


class QWidget;
class QString;
class QPushButton;
class QLabel;
class QShortcut;


namespace Rosegarden
{

class CountdownBar;


class CountdownDialog : public QDialog
{
    Q_OBJECT

public:
    CountdownDialog(QWidget *parent, int seconds = 300);

    void setLabel(const QString &label);
    void setElapsedTime(int seconds);

    int getTotalTime() const { return m_totalTime; }
    void setTotalTime(int seconds);

    QShortcut* getShortcuts() { return m_shortcuts; }

signals:
    void completed(); // m_totalTime has elapsed
    void stopped();   // someone pushed the stop button

protected:
    void setPastEndMode();

    bool          m_pastEndMode;

    int           m_totalTime;

    QLabel       *m_label;
    QLabel       *m_time;
    CountdownBar *m_progressBar;

    QPushButton  *m_stopButton;

    int           m_progressBarWidth;
    int           m_progressBarHeight;

    QShortcut       *m_shortcuts;
};


}

#endif
