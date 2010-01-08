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

#ifndef _RG_ROSEGARDENPROGRESSDIALOG_H_
#define _RG_ROSEGARDENPROGRESSDIALOG_H_

#include "ProgressBar.h"

#include <QDateTime>
#include <QDialog>
#include <QLabel>
#include <QTimer>

class QHideEvent;


namespace Rosegarden
{

/** A simple dialog for reporting progress.  This implementation is designed to
 * give all our existing legacy code what it expects in order to function,
 * rather than going to some considerable lengths to rewrite all of the
 * dependent code.  This class was originally a subclass of KProgressDialog from
 * KDE 3.  This implementation does not aim to reproduce KProgressDialog from
 * KDE 3 so much as it aims to satisfy the minimum requirements of what existing
 * Rosegarden code actually needed a ProgressDialog and related classes (eg.
 * ProgressBar) to do in practice.  No more, no less.
 */
class ProgressDialog : public QDialog
{
    Q_OBJECT
public:
    ProgressDialog(const QString &labelText,
                   int totalSteps,
                   QWidget *parent = 0,
                   bool modal = true);

    ~ProgressDialog();

    /**
     * A "safe" way to process events without worrying about user
     * input during the process.  If there is a modal progress dialog
     * visible, then this will permit user input so as to allow the
     * user to hit Cancel; otherwise it will prevent all user input
     */
    static void processEvents();

    /**
     * Return a pointer to the dialog's ProgressBar.  Create a ProgressBar if
     * one does not already exist.
     */
    ProgressBar *progressBar();

    void setAutoClose(bool state);
    void setAutoReset(bool state);
    void setMinimumDuration(int duration);
    int minimumDuration();
    void setLabelText(QString text);

signals:
    void canceled();

public slots:
    void slotSetOperationName(QString);
    void cancel();

    /// Stop and hide (if it's shown) the progress dialog
    void slotFreeze();

    /// Restore the dialog to its normal state
    void slotThaw();

    void setValue(int value);

protected slots:
    void slotCheckShow(int);
    
    /// Called when the minimum duration timer has counted down
    void slotMinimumTimeElapsed();

protected:
    virtual void hideEvent(QHideEvent*);

    //--------------- Data members ---------------------------------

    QTime        m_chrono;
    QTimer      *m_timer;
    bool         m_wasVisible;
    bool         m_frozen;
    bool         m_modal;
    static bool  m_modalVisible;
    bool         m_minimumTimeHasExpired;
    bool         m_Thorn;

    ProgressBar *m_progressBar;
    QLabel      *m_label;
    int          m_minimumDuration;
};


}

#endif
