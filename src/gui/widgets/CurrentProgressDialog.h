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

#ifndef RG_CURRENTPROGRESSDIALOG_H
#define RG_CURRENTPROGRESSDIALOG_H

#include <QObject>

#include <rosegardenprivate_export.h>


namespace Rosegarden
{

class ProgressDialog;

/// Global freeze() and thaw() for the progress dialog that is up.
/**
 * DEPRECATED
 * Since the freeze() and thaw() functionality does nothing now, this
 * class can probably just be removed without any effect.
 */
class ROSEGARDENPRIVATE_EXPORT CurrentProgressDialog : public QObject
{
    Q_OBJECT
public:
    static CurrentProgressDialog* getInstance();

    static void set(ProgressDialog*);

    /**
     * Block the current progress dialog so that it won't appear
     * regardless of passing time and occurring events.
     * This is useful when you want to show another dialog
     * and you want to make sure the value() dialog is out of the way
     */
    static void freeze();

    /**
     * Restores the progress dialog to its normal state after a freeze()
     */
    static void thaw();

public slots:
    /// Called then the current progress dialog is being destroyed
    void slotCurrentProgressDialogDestroyed();

private:
    // Global Singleton.  Instance is created when needed by getInstance().
    CurrentProgressDialog(QObject* parent) : QObject(parent) {}

    //static ProgressDialog* get()  { return m_currentProgressDialog; }

    //--------------- Data members ---------------------------------
    static CurrentProgressDialog* m_instance;
    static ProgressDialog* m_currentProgressDialog;
};


}

#endif
