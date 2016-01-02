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

#include "ProgressBar.h"
#include "misc/Debug.h"

#include <QString>
#include <QTimer>


namespace Rosegarden
{


ProgressDialog::ProgressDialog(const QString &labelText, QWidget *parent) :
    QProgressDialog(parent),
    m_showAfterTimer(new QTimer),
    m_totalSteps(0),
    m_indeterminate(false)
{
    //RG_DEBUG << "ctor - " << labelText;

    setWindowTitle(tr("Rosegarden"));
    setBar(new ProgressBar(this));
    setLabelText(labelText);

    // Setting this to QString() causes the cancel button to be deleted.
    // The cancel buttons have never worked properly, and they are particularly
    // broken now, creating a very tricky plumbing problem.  Not being able to
    // cancel is not ideal, but at least not showing a cancel button is
    // truthful.
    setCancelButtonText(QString());

    // ??? Then why do we need m_showAfterTimer?
    setMinimumDuration(2000);

    setAttribute(Qt::WA_DeleteOnClose);

    hide();

    // Modality is needed to prevent users from editing the Composition
    // while we are working.  In the case of File > Open, this could be
    // catastrophic.
    //setWindowModality(Qt::WindowModal);

    // don't show before this timer has elapsed
    // ??? QProgressDialog already has delay behavior.  See the docs for
    //     the minimumDuration property.  This may not be needed.
    //     The call to setMinimumDuration() might be all we need.
    //     I've tried removing these lines and it appears to work fine.
    //     My test case might be faulty, though, as I think File Open
    //     forces the progress dialog to be displayed at some point.
    m_showAfterTimer->setSingleShot(true);
    m_showAfterTimer->start(2000);
    connect(m_showAfterTimer, SIGNAL(timeout()), this, SLOT(forceShow()));
}

ProgressDialog::~ProgressDialog()
{
    //RG_DEBUG << "dtor...";

    delete m_showAfterTimer;
    m_showAfterTimer = 0;
}

void
ProgressDialog::setIndeterminate(bool ind)
{
    if (m_indeterminate == ind)
        return;

    if (ind) {
        // Keep track of maximum for when we go out of indeterminate mode.
        m_totalSteps = maximum();
        setRange(0, 0);
    } else {
        setRange(0, m_totalSteps);
    }

    m_indeterminate = ind;
}

void
ProgressDialog::slotFreeze()
{
    //RG_DEBUG << "slotFreeze()";
}

void
ProgressDialog::slotThaw()
{
    //RG_DEBUG << "slotThaw()";
}

void
ProgressDialog::setValue(int value)
{
    // If we're in indeterminate mode, bail.  Otherwise the dialog will flash.
    if (m_indeterminate)
        return;

    // Let the baseclass handle it.
    QProgressDialog::setValue(value);
}


}
