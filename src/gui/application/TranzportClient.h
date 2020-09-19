/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This file is Copyright 2005
        Immanuel Litzroth         <immanuel203@gmail.com>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/
#ifndef RG_TRANZPORTCLIENT_H
#define RG_TRANZPORTCLIENT_H

#include "base/Composition.h"
#include "misc/Debug.h"

#include <QObject>

#include <queue>
#include <stdint.h>

class QSocketNotifier;

namespace Rosegarden
{
  
class RosegardenMainWindow;
class RosegardenDocument;

/// Support for the TranzPort(TM) wireless remote control.
/**
 * Frontier Design Group's TranzPort(TM) DAW Control:
 * http://www.frontierdesign.com/Products/TranzPort
 *
 * For the Rosegarden transport dialog, see TransportDialog.
 */
class TranzportClient : public QObject, public CompositionObserver
{
    Q_OBJECT
public:
    TranzportClient(RosegardenMainWindow* rgGUIApp);
      
    ~TranzportClient() override;

public slots:
    void readData();
      
    void slotDocumentLoaded(RosegardenDocument *doc);
      
    void writeCommandQueue();

    void pointerPositionChanged(timeT time);
      
    void loopChanged(timeT t1, timeT t2);

signals:
    
    void play();
    void stop();
    void record();
    void rewind();
    void rewindToBeginning();
    void fastForward();
    void fastForwardToEnd();
    void toggleRecord();
    void trackDown();
    void trackUp();
    void trackMute();
    void trackRecord();
    void solo(bool);
    void undo();
    void redo();
      
    void setPosition(timeT);
      
public:

    void stateUpdate();
      
public:
    enum ButtonMasks
    {
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    #define SWAP(x) ((((uint32_t)(x) & 0xff000000) >> 24) |     \
                     (((uint32_t)(x) & 0x00ff0000) >> 8)  |     \
                     (((uint32_t)(x) & 0x0000ff00) << 8)  |     \
                     (((uint32_t)(x) & 0x000000ff) << 24))
#elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    #define SWAP(x) x
#else
    #error No endianness defined
#endif

        Battery     = SWAP(0x00004000),
        Backlight   = SWAP(0x00008000),
        TrackLeft   = SWAP(0x04000000),
        TrackRight  = SWAP(0x40000000),
        TrackRec    = SWAP(0x00040000),
        TrackMute   = SWAP(0x00400000),
        TrackSolo   = SWAP(0x00000400),
        Undo        = SWAP(0x80000000),
        In          = SWAP(0x02000000),
        Out         = SWAP(0x20000000),
        Punch       = SWAP(0x00800000),
        Loop        = SWAP(0x00080000),
        Prev        = SWAP(0x00020000),
        Add         = SWAP(0x00200000),
        Next        = SWAP(0x00000200),
        Rewind      = SWAP(0x01000000),
        FastForward = SWAP(0x10000000),
        FootSwitch  = SWAP(0x00001000),
        Stop        = SWAP(0x00010000),
        Play        = SWAP(0x00100000),
        Record      = SWAP(0x00000100),
        Shift       = SWAP(0x08000000)
    #undef SWAP
    };

    enum Light
    {
        LightRecord = 0,
        LightTrackrec,
        LightTrackmute,
        LightTracksolo,
        LightAnysolo,
        LightLoop,
        LightPunch
    };

    void LightOn(Light l);
      
    void LightOff(Light l);
      
    enum Row
    {
        Top,
        Bottom,
    };
    
    void LCDWrite(const std::string& text,
                  Row row = Top,
                  uint8_t offset = 0);
      
    void write(uint64_t buf);
      
        
private:
    int m_descriptor;
      
    QSocketNotifier * m_socketReadNotifier;
    QSocketNotifier* m_socketWriteNotifier;

    bool device_online;

    uint8_t previousbuf[8];
    uint8_t currentbuf[8];
    volatile uint32_t& previous_buttons;
    volatile uint32_t& current_buttons;
    volatile uint8_t& datawheel;
    volatile uint8_t& status;

    RosegardenMainWindow* m_rgGUIApp;
    RosegardenDocument* m_rgDocument;
    Composition* m_composition;

    static const uint8_t LCDLength = 20;
    TranzportClient(const TranzportClient&);
      
    typedef uint64_t CommandType;
    std::queue<CommandType> commands;
      
    // CompositionObserver overrides
    void trackChanged(const Composition *c, Track* t) override;
    // tracksAdded() need not be overridden as adding a track will not change
    // anything the TranzPort would display.
    // tracksDeleted() should probably be overridden to clear the display on
    // the TranzPort when the selected track is deleted.

};
  
}

#endif // RG_TRANZPORTCLIENT_H
