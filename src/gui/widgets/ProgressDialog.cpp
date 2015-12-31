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

#define RG_MODULE_STRING "[ProgressDialog]"

#include "ProgressDialog.h"
#include "CurrentProgressDialog.h"
#include "ProgressBar.h"

#include "misc/Debug.h"
#include "gui/application/RosegardenApplication.h"

#include <QCursor>
#include <QProgressDialog>
#include <QString>
#include <QTimer>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHideEvent>
#include <QCloseEvent>

#include <iostream>

namespace Rosegarden
{


// bool ProgressDialog::m_modalVisible = false;

ProgressDialog::ProgressDialog(const QString &labelText , /*
                               int totalSteps,
                               int showAfter,*/
                               QWidget *parent /*,
                               bool modal*/) :
    QProgressDialog(parent),
//    m_wasVisible(false),
//    m_frozen(false),
//    m_modal(modal),
//    m_minimumTimeHasExpired(false),
//    m_minimumDuration(1000),
//    m_sleepingBetweenOperations(false),
//    m_operationText(""),
//    m_totalSteps(totalSteps),
//    m_deferredClose(false),
    m_indeterminate(false)

{
    RG_DEBUG << "ProgressDialog::ProgressDialog - " << labelText << /* " - modal : " << modal << */ endl;

    setWindowTitle(tr("Rosegarden"));
    setBar(new ProgressBar(this));
    setLabelText(labelText);

    // Setting this to QString() causes the cancel button to be deleted.
    // The cancel buttons have never worked properly, and they are particularly
    // broken now, creating a very tricky plumbing problem.  Not being able to
    // cancel is not ideal, but at least not showing a cancel button is
    // truthful.
    setCancelButtonText(QString());

    setMinimumDuration(2000);
    setAttribute(Qt::WA_DeleteOnClose);
    hide();
//    setModal(modal);
/*    setWindowFlags(Qt::Dialog);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *info = new QLabel(tr("<qt><h3>Processing...</h3></qt>"));
    layout->addWidget(info);

    QGroupBox *box = new QGroupBox;
    layout->addWidget(box);

    QVBoxLayout *boxLayout = new QVBoxLayout;
    box->setLayout(boxLayout);

    m_label = new QLabel(labelText);
    boxLayout->addWidget(m_label);

    m_progressBar = new ProgressBar(totalSteps);
    boxLayout->addWidget(m_progressBar);

    connect(m_progressBar, SIGNAL(valueChanged(int)),
            this, SLOT(slotCheckShow(int)));

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Cancel);
    layout->addWidget(bb);
    bb->setCenterButtons(true);

    connect(bb, SIGNAL(rejected()), this, SLOT(cancel()));
*/
    // don't show before this timer has elapsed
    m_showAfterTimer = new QTimer;
    m_showAfterTimer->setSingleShot(true);
    m_showAfterTimer->start(2000);
    connect(m_showAfterTimer, SIGNAL(timeout()), this, SLOT(forceShow()));

/*
    m_timer = new QTimer;
    m_timer->setSingleShot(true);
    QWidget::hide(); */
}

ProgressDialog::~ProgressDialog()
{
    RG_DEBUG << "~ProgressDialog()" << endl;
/*    m_modalVisible = false; */
    delete m_showAfterTimer;
    m_showAfterTimer = 0;
}

void
ProgressDialog::setIndeterminate(bool ind)
{
    if (m_indeterminate == ind) return;
    if (ind) {
        m_totalSteps = maximum();
        setRange(0, 0);
    } else {
        setRange(0, m_totalSteps);
    }
    m_indeterminate = ind;
}
/*
void
ProgressDialog::slotShowNow()
{
    // the "don't show until" time has elapsed, so carry on with ensuring the
    // progress dialog remains visible for a reasonable amount of time (this is
    // meant to cure the flickery progress dialogs we used to have that would
    // often appear and then wink out almost instantly)
    m_timer->start(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotMinimumTimeElapsed()));

    CurrentProgressDialog::set(this);
    show();
}

void
ProgressDialog::hideEvent(QHideEvent *e)
{
    if (m_minimumTimeHasExpired) {
        RG_DEBUG << "ProgressDialog::hideEvent() - minimum time has elapsed.  Hiding..." << endl;
        e->accept();
        m_modalVisible = false;
    } else {
        RG_DEBUG << "ProgressDialog::hideEvent() - minimum time has not elapsed.  Ignoring hide event..." << endl;
        m_modalVisible = true;
        e->ignore();
    }
}

void
ProgressDialog::closeEvent(QCloseEvent *e)
{
    if (m_minimumTimeHasExpired) {
        RG_DEBUG << "ProgressDialog::hideEvent() - minimum time has elapsed.  Closing..." << endl;
        e->accept();
        m_modalVisible = false;
    } else {
        RG_DEBUG << "ProgressDialog::hideEvent() - minimum time has not elapsed.  Deferring close event..." << endl;
        m_modalVisible = true;
        m_deferredClose = true;
        e->ignore();
    }
}
*/

//void
//ProgressDialog::setLabelText(QString name)
//{
////    m_operationText = name;
//    setLabelText(name);
//}
/*
void
ProgressDialog::slotSetOperationName(QString name)
{
    RG_DEBUG << "ProgressDialog::slotSetOperationName(" << name << ") visible : " << isVisible() << endl;

    // save the passed text for the receiving slot
    m_operationText = name;

    // set a timer to ensure 100% remains visible for long enough to see
    m_operationTimer = new QTimer;
    m_operationTimer->start(500);
    connect(m_operationTimer, SIGNAL(timeout()), this, SLOT(completeOperationChange()));
    m_progressBar->setValue(m_totalSteps);
    m_progressBar->show();

    // a hack so external calls to setValue() are ignored during the pause
    m_sleepingBetweenOperations = true;

}

void
ProgressDialog::completeOperationChange()
{
    std::cout << "complete operation change" << std::endl;

    if (m_operationTimer) delete m_operationTimer;
    m_operationTimer = 0;

    m_sleepingBetweenOperations = false;

    // set the label to the text saved from setLabelText()
    m_label->setText(m_operationText);

    // increase resize only, never shrink
    int w = QMAX(isVisible() ? width() : 0, sizeHint().width());
    int h = QMAX(isVisible() ? height() : 0, sizeHint().height());
    resize(w, h);

    // call setValue() with whatever the current value is now that we've paused,
    // in order to trigger the auto show/close code that may have been blocked
    // while we were sleeping between operations
    setValue(m_progressBar->value());
}

void
ProgressDialog::cancel()
{
    RG_DEBUG << "ProgressDialog::slotCancel()\n";
    emit canceled();
    slotFreeze();
    reject();
}

void
ProgressDialog::slotMinimumTimeElapsed()
{
    RG_DEBUG << "ProgressDialog::slotMinimumTimeElapsed() - the QTimer has reached the minimum duration set in the ctor." << endl;
    m_minimumTimeHasExpired = true;

    // if we intercepted a closeEvent() before it was time, close now
    if (m_deferredClose) close();
}
*/
void
ProgressDialog::slotFreeze()
{
    RG_DEBUG << "ProgressDialog::slotFreeze()\n";

    // if we're frozen, consider that the minimum time has elapsed, even if it
    // hasn't, because it is very possible we will never be un-thawed, and will
    // get stuck in limbo otherwise
//    m_minimumTimeHasExpired = true;

    // if we're in the 100% hang time between operation changes, go ahead and
    // complete the operation change
//    if (m_sleepingBetweenOperations) completeOperationChange();

/*    m_wasVisible = isVisible();
    if (isVisible()) {
        m_modalVisible = false;
        hide();
    }
    setModal(false);

//    m_timer->stop();
    m_frozen = true;
*/
}

void
ProgressDialog::slotThaw()
{
/*    RG_DEBUG << "ProgressDialog::slotThaw()\n";

    if (m_wasVisible) {
        if (m_modal) m_modalVisible = true;
        show();
    }

    // Restart timer
//    m_timer->start(1000);
//    m_minimumTimeHasExpired = false;
    m_frozen = false;
*/
}

/* void
ProgressDialog::processEvents()
{
    RG_DEBUG << "ProgressDialog::processEvents: modalVisible is "
             << m_modalVisible << endl;
    if (m_modalVisible) {
        qApp->processEvents(QEventLoop::AllEvents, 50);
    } else {
        rosegardenApplication->refreshGUI(50);
    }
}

void
ProgressDialog::setAutoClose(bool state)
{
    m_autoClose = state;
}

void
ProgressDialog::setAutoReset(bool state)
{
    m_autoReset = state;
}

*/
void
ProgressDialog::setValue(int value)
{

    if (m_indeterminate) {
        //Just ignore the call otherwise the dialog will flash
        return;
    }
    QProgressDialog::setValue(value);
    
    // Just ignore 
/*//    std::cout << "ProgressDialog::setValue(" << value << ")" << std::endl;

    // Try to get our text and whatnot repainted whenever this is called, to
    // solve the "progress bar in an empty black box" problem.
    update();

    // bail out if the value changes (usually to 0) if the value changes during
    // our little enforced nap beween operation changes (after the nap, the
    // setValue(0) will be called anyway)
    if (m_sleepingBetweenOperations) return;

    // ensure we are visible when this changes, although this probably needs to
    // be hooked into m_modal nonsense somehow or other
    if (value > 0) show();

    // see if we're 100% complete (ie. the value was just set to the highest
    // possible value it could have been set to, or higher; as determined by
    // totalSteps in the ctor)
    bool complete = (value >= m_totalSteps);
    
    // In KProgressDialog, autoClose "sets whether the dialog should close automagically
    // when all the steps in the QProgressBar have been completed."  We'll
    // interpret this to mean we should hide ourselves if we're complete, and
    // m_autoClose is true.
    //
    // In KProgressDialog, autoReset "sets whether the dialog should reset the
    // QProgressBar dialog back to 0 steps compelete when all steps have been
    // completed."  So we'll reset back to 0 if we've reached 100% complete.
    if (complete) {
//        if (m_autoReset) std::cout << "Auto resetting..." << std::endl;
//        if (m_autoClose) std::cout << "Auto closing..." << std::endl;

        if (m_autoClose) close();
        if (m_autoReset) value = 0;

    }

    m_progressBar->setValue(value); */
}
/*
void
ProgressDialog::setProgress(int value)
{
    std::cout << "ProgressDialog::setProgress(" << value << ") calling setValue()" << std::endl;
    setValue(value);
}

void
ProgressDialog::incrementProgress(int value)
{
    std::cout << "ProgressDialog::incrementProgress(" << value << ") calling setValue()" << std::endl;
    setValue(value);
}

void
ProgressDialog::advance(int value)
{
    std::cout << "ProgressDialog::advance(" << value << ") calling setValue()" << std::endl;
    setValue(value);
}

void
ProgressDialog::setLabelText(QString text)
{
    setLabelText(text);
}

void
ProgressDialog::show() {
} */

}

