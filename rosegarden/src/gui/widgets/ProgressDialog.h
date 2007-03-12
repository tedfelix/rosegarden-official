
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

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

#define private protected // fugly
#include <kprogress.h>
#undef private
#include <qdatetime.h>

class QWidget;
class QString;
class QHideEvent;


namespace Rosegarden
{



class ProgressDialog : public KProgressDialog
{
    Q_OBJECT
public:
    ProgressDialog(QWidget * creator = 0,
                             const char * name = 0,
                             bool modal = true);

    ProgressDialog(const QString &labelText,
                             int totalSteps,
                             QWidget *creator = 0,
                             const char *name = 0,
                             bool modal = true);

    ~ProgressDialog();

    /**
     * A "safe" way to process events without worrying about user
     * input during the process.  If there is a modal progress dialog
     * visible, then this will permit user input so as to allow the
     * user to hit Cancel; otherwise it will prevent all user input
     */
    static void processEvents();

    virtual void polish();

public slots:
    void slotSetOperationName(QString);
    void slotCancel();

    /// Stop and hide (if it's shown) the progress dialog
    void slotFreeze();

    /// Restore the dialog to its normal state
    void slotThaw();

protected slots:
    void slotCheckShow(int);

protected:
    virtual void hideEvent(QHideEvent*);

    //--------------- Data members ---------------------------------

    QTime m_chrono;
    bool m_wasVisible;
    bool m_frozen;
    bool m_modal;
    static bool m_modalVisible;
};


}

#endif
