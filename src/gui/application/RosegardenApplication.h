/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENAPPLICATION_H
#define RG_ROSEGARDENAPPLICATION_H

#include <QApplication>
#include <QByteArray>
#include <QString>


class QSessionManager;
class QProcess;


namespace Rosegarden
{



/**
 * RosegardenApplication
 *
 * Handles RosegardenMainWindow's perceived uniqueness for us.
 *
 */
class RosegardenApplication : public QApplication
{
    Q_OBJECT
public:
    RosegardenApplication(int &argc, char **argv) :
        QApplication(argc, argv), m_noSequencerMode(false) {}

    /**
     * Handle the attempt at creation of a new instance - 
     * only accept new file names which we attempt to load
     * into the existing instance (if it exists)
     */
//&&&    virtual int newInstance();

    void refreshGUI(int maxTime);

    static RosegardenApplication* ApplicationObject();

    static QByteArray Empty;

    void setNoSequencerMode(bool m=true) { m_noSequencerMode = m; }
    bool noSequencerMode() { return m_noSequencerMode; }

    virtual void saveState(QSessionManager&);
    
    //!!!
    //@@@ Need session manager commitData() call

signals:
    // connect this to RosegardenMainWindow
    void aboutToSaveState();
    
public slots:
    void sfxLoadExited(QProcess *proc);
    void slotSetStatusMessage(QString txt);

protected:

    virtual bool notify(QObject * receiver, QEvent * event);
    
    //--------------- Data members ---------------------------------
    
    bool m_noSequencerMode;
};

#define rosegardenApplication RosegardenApplication::ApplicationObject()


}

#endif
