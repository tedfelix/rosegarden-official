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


#include "ProgressDialog.h"
#include "CurrentProgressDialog.h"
#include "ProgressBar.h"

#include "misc/Debug.h"
#include "gui/application/RosegardenApplication.h"

#include <QCursor>
#include <QDialog>
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


bool ProgressDialog::m_modalVisible = false;

ProgressDialog::ProgressDialog(const QString &labelText,
                               int totalSteps,
                               int showAfter,
                               QWidget *parent,
                               bool modal) :
    /*QDialog(parent),*/ QObject(parent),
    m_wasVisible(false),
    m_frozen(false),
    m_modal(modal),
    m_minimumTimeHasExpired(false),
    m_minimumDuration(1000),
    m_sleepingBetweenOperations(false),
    m_operationText(""),
    m_totalSteps(totalSteps),
    m_deferredClose(false),
    m_indeterminate(false)

{
}

ProgressDialog::~ProgressDialog()
{
}

void
ProgressDialog::setIndeterminate(bool ind)
{
}

void
ProgressDialog::slotShowNow()
{
}

void
ProgressDialog::hideEvent(QHideEvent *e)
{
}

void
ProgressDialog::closeEvent(QCloseEvent *e)
{
}

void
ProgressDialog::slotSetOperationName(QString name)
{
}

void
ProgressDialog::completeOperationChange()
{
}

void
ProgressDialog::cancel()
{
}

void
ProgressDialog::slotMinimumTimeElapsed()
{
}

void
ProgressDialog::slotFreeze()
{
}

void
ProgressDialog::slotThaw()
{
}

void
ProgressDialog::processEvents()
{
}

void
ProgressDialog::setAutoClose(bool state)
{
}

void
ProgressDialog::setAutoReset(bool state)
{
}

void
ProgressDialog::setValue(int value)
{
}

void
ProgressDialog::setProgress(int value)
{
}

void
ProgressDialog::incrementProgress(int value)
{
}

void
ProgressDialog::advance(int value)
{
}

void
ProgressDialog::setLabelText(QString text)
{
}


}
#include "ProgressDialog.moc"
