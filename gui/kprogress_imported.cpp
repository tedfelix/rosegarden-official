/* This file is part of the KDE libraries
   Copyright (C) 1996 Martynas Kunigelis

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kprogress_imported.h"

#include <qpainter.h>
#include <qpixmap.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qregexp.h>
#include <qstyle.h>
#include <qtimer.h>
#include <kapplication.h>
#include <kwin.h>
#include <kprogress.h>

/*
 * KProgressDialog implementation
 */
RGKProgressDialog::RGKProgressDialog(QWidget* parent, const char* name,
                                 const QString& caption, const QString& text,
                                 bool modal)
    : KDialogBase(KDialogBase::Plain, caption, KDialogBase::Cancel,
                  KDialogBase::Cancel, parent, name, modal),
      mAutoClose(true),
      mAutoReset(false),
      mCancelled(false),
      mAllowCancel(true),
      mShown(false),
      mMinDuration(2000)
{
    KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());
    mShowTimer = new QTimer(this);
    
    showButton(KDialogBase::Close, false);
    mCancelText = actionButton(KDialogBase::Cancel)->text();

    QFrame* mainWidget = plainPage();
    QVBoxLayout* layout = new QVBoxLayout(mainWidget, 10);

    mLabel = new QLabel(text, mainWidget);
    layout->addWidget(mLabel);

    mProgressBar = new KProgress(mainWidget);
    layout->addWidget(mProgressBar);

    connect(mProgressBar, SIGNAL(percentageChanged(int)),
            this, SLOT(slotAutoActions(int)));
    connect(mShowTimer, SIGNAL(timeout()), this, SLOT(slotAutoShow()));
    mShowTimer->start(mMinDuration, true);
}

RGKProgressDialog::~RGKProgressDialog()
{
}

void RGKProgressDialog::slotAutoShow()
{
    if (mShown || mCancelled)
    {
        return;
    }

    show();
    kapp->processEvents();
    mShown = true;
}

void RGKProgressDialog::slotCancel()
{
    mCancelled = true;

    if (mAllowCancel)
    {
        KDialogBase::slotCancel();
    }
}

bool RGKProgressDialog::wasCancelled()
{
    return mCancelled;
}

void RGKProgressDialog::setMinimumDuration(int ms)
{
    mMinDuration = ms;
    if (!mShown)
    {
        mShowTimer->stop();
        mShowTimer->start(mMinDuration, true);
    }
}

int RGKProgressDialog::minimumDuration()
{
    return mMinDuration;
}

void RGKProgressDialog::setAllowCancel(bool allowCancel)
{
    mAllowCancel = allowCancel;
    showCancelButton(allowCancel);
}

bool RGKProgressDialog::allowCancel()
{
    return mAllowCancel;
}

KProgress* RGKProgressDialog::progressBar()
{
    return mProgressBar;
}

void RGKProgressDialog::setLabel(const QString& text)
{
    mLabel->setText(text);
}

QString RGKProgressDialog::labelText()
{
    return mLabel->text();
}

void RGKProgressDialog::showCancelButton(bool show)
{
    showButtonCancel(show);
}

bool RGKProgressDialog::autoClose()
{
    return mAutoClose;
}

void RGKProgressDialog::setAutoClose(bool autoClose)
{
    mAutoClose = autoClose;
}

bool RGKProgressDialog::autoReset()
{
    return mAutoReset;
}

void RGKProgressDialog::setAutoReset(bool autoReset)
{
    mAutoReset = autoReset;
}

void RGKProgressDialog::setButtonText(const QString& text)
{
    mCancelText = text;
    setButtonCancelText(mCancelText);
}

QString RGKProgressDialog::buttonText()
{
    return mCancelText;
}

void RGKProgressDialog::slotAutoActions(int percentage)
{
    if (percentage < 100)
    {
        setButtonCancelText(mCancelText);
    }
    else
    {
        if (mAutoReset)
        {
            mProgressBar->setValue(0);
        }
        else
        {
            setAllowCancel(true);
            setButtonCancelText("&Close");
        }

        if (mAutoClose)
        {
            hide();
        }
    }
}
