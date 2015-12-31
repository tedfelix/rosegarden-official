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

#ifndef RG_PROGRESSDIALOG_H
#define RG_PROGRESSDIALOG_H

#include "ProgressBar.h"

#include <QDateTime>
#include <QProgressDialog>
#include <QLabel>
#include <QTimer>

class QHideEvent;


namespace Rosegarden
{

/** A simple dialog for reporting progress.  This was originally a subclass of
 * KProgressDialog from KDE 3.  It implements the API our existing progress
 * reporting scheme needed in order to function again, and is simply a subclass
 * of QDialog now, managing everything from scratch with a new implementation.
 */
class ProgressDialog : public QProgressDialog
{
    Q_OBJECT
public:
    ProgressDialog(const QString &labelText ,/*
                   int totalSteps,
                   int showAfter = 500, */
                   QWidget *parent = 0 /*,
                   bool modal = true*/);

    ~ProgressDialog();

    /**
     * A "safe" way to process events without worrying about user
     * input during the process.  If there is a modal progress dialog
     * visible, then this will permit user input so as to allow the
     * user to hit Cancel; otherwise it will prevent all user input
     */
//    static void processEvents();

    /** Sets whether the dialog should hide itself once its value has been set
     * to be greater than or equal to totalSteps as passed in the ctor.
     *
     * The default is false.
     */
//    void setAutoClose(bool state);

    /** Sets whether the dialog should reset its progress bar back to 0% upon
     * reaching 100% complete.
     *
     * The default is false.
     */
//    void setAutoReset(bool state);

    /** Sets the dialog's operation text, eg. "Calculating notation..."
     * "Generating audio previews..." &c. by calling slotSetOperationName
     */
//    void setLabelText(QString text);

    /** Sets indeterminate state (Knight Rider mode) on the progress bar.
     */
    void setIndeterminate(bool ind);

// signals:
    /** The user pressed the cancel button.  (In practice, I have yet to see
     * this work usefully either in the current or the Classic codebase, but we
     * emit the signal on cue.)
     */
//    void canceled();

 public slots:
    /** Sets the dialog to 100% and makes it linger there for 500 ms, after
     * which completeOperationChange() is called.  This ensures the dialog will
     * always appear to finish whatever it is chewing on before moving on to the
     * next step.
     */
//    void slotSetOperationName(QString);

    /** Connected to the cancel button.  Causes canceled() to be emitted.
     */
//    void cancel();

    /** Stop and hide if we're shown
     */
    void slotFreeze();

    /** Restore to our normal state after freezing
     */
    void slotThaw();

    /** Set the value for this dialog's progress bar.  This replaces setValue()
     * and advance() with the same function, and any calls or connections to
     * advance() must be switched over to setValue().  All management of the
     * progress bar is done by this dialog now, and it does not expose its
     * internally-managed progress bar to the outside.
     */
    void setValue(int value);

    /** To allow parallel connections between ProgressReporter and
     * ProgressDialog, connect the setProgress() signal to this slot, so this
     * dialog can report what signal it caught.  Simply calls setValue().
     */
//    void setProgress(int value);

    /** To allow parallel connections between ProgressReporter and
     * ProgressDialog, connect the incrementProgress() signal to this slot, so
     * this dialog can report what signal it caught.  Simply calls setValue().
     */
//    void incrementProgress(int value);

    /** Convenience legacy support slot.  Simply calls setValue().
     */
//    void advance(int value);

// protected slots:
    /** Called when the showAfter time has elapsed.  The dialog will not be
     * visible until then.
     */
//    void slotShowNow();
    
    /** Called when the minimum duration timer has counted down
     */
//    void slotMinimumTimeElapsed();

    /** Completes the process of changing an operation, after a suitable delay
     * has been put in motion by slotSetOperationText()
     */
//    void completeOperationChange();

//      void show();
      

 protected:
    /** Intercept hideEvent() and determine whether we should honor it yet or
     * not, in order to remain visible for a minimum specified time.
     */
//    virtual void hideEvent(QHideEvent*);

    /** Intercept close() and determine whether we should honor it yet or not,
     * in order to remain visible for a minimum specified time.  If we defer a
     * close(), set a flag so that slotMinimumTimeElapsed() will call close()
     * after the timer has expired.
     */
//    virtual void closeEvent(QCloseEvent*);

    //--------------- Data members ---------------------------------

    QTimer      *m_showAfterTimer;
//    QTimer      *m_timer;
//    QTimer      *m_operationTimer;
//    bool         m_wasVisible;
//    bool         m_frozen;
//    bool         m_modal;
//    static bool  m_modalVisible;
//    bool         m_minimumTimeHasExpired;
//    bool         m_Thorn;

//    ProgressBar *m_progressBar;
//    QLabel      *m_label;
//    int          m_minimumDuration;
//    bool         m_autoReset;
//    bool         m_autoClose;
//    bool         m_sleepingBetweenOperations;
//    QString      m_operationText;
    int          m_totalSteps;
    bool         m_indeterminate;
//    bool         m_deferredClose;
};


}

#endif
