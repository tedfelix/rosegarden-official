
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

    Parts of this file are from KDE Konqueror : KonqMainWindowIface
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2000 David Faure <faure@kde.org>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ROSEGARDENIFACE_H_
#define _RG_ROSEGARDENIFACE_H_

#include <dcopobject.h>
#include <dcopref.h>
#include <qmap.h>
#include <qstring.h>
#include <qvaluevector.h>

#include "base/Instrument.h"
#include "sound/MappedComposition.h"

class QCString;
class KMainWindow;
class KDCOPActionProxy;


namespace Rosegarden
{


/**
 * RosegardenGUI DCOP Interface
 */
class RosegardenIface : virtual public DCOPObject
{
    K_DCOP

public:
    RosegardenIface(KMainWindow*);
    void iFaceDelayedInit(KMainWindow*);

k_dcop:
    virtual void openFile(QString file)  = 0;
    virtual void openURL(QString url)    = 0;
    virtual void mergeFile(QString file) = 0;
    virtual void fileNew()               = 0;
    virtual void fileSave()              = 0;
    virtual void fileClose()             = 0;
    virtual void quit()                  = 0;

    virtual void play() = 0;
    virtual void stop() = 0;
    virtual void rewind() = 0;
    virtual void fastForward() = 0;
    virtual void record() = 0;
    virtual void rewindToBeginning() = 0;
    virtual void fastForwardToEnd() = 0;
    virtual void jumpToTime(int sec, int usec) = 0;
    virtual void startAtTime(int sec, int usec) = 0;
    
    // Extra functions used by Infrared Remotes
    virtual void trackDown() = 0;
    virtual void trackUp() = 0;
    virtual void toggleMutedCurrentTrack() = 0;
    virtual void toggleRecordCurrentTrack() = 0;

    // Sequencer updates GUI with status
    //
    virtual void notifySequencerStatus(int status) = 0;

    // Used to map unexpected (async) MIDI events to the user interface.
    // We can show these on the Transport or on a MIDI Mixer.
    //
    virtual void processAsynchronousMidi(const MappedComposition &mC) = 0;

    // The sequencer tries to call this action until it can - then
    // we can go on and retrive device information
    //
    virtual void alive() = 0;

    // The sequencer requests that a new audio file is created - the
    // gui does so and returns the path of the new file so that the
    // sequencer can use it.
    //
    virtual QString createNewAudioFile() = 0;
    virtual QValueVector<QString> createRecordAudioFiles
    (const QValueVector<InstrumentId> &recordInstruments) = 0;
    virtual QString getAudioFilePath() = 0;

    virtual QValueVector<InstrumentId> getArmedInstruments() = 0;

    virtual void showError(QString error) = 0;

    // Actions proxy
    //
    DCOPRef action( const QCString &name );
    QCStringList actions();
    QMap<QCString,DCOPRef> actionMap();

protected:
    //--------------- Data members ---------------------------------

    KDCOPActionProxy *m_dcopActionProxy;

};


}

#endif
