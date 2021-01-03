/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    This file is Copyright 2009
        Immanuel Litzroth         <immanuel203@gmail.com>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[TranzportClient]"

#include "base/Exception.h"
#include "commands/edit/AddMarkerCommand.h"
#include "TranzportClient.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/editors/segment/TrackButtons.h"
#include "RosegardenMainWindow.h"
#include "misc/Debug.h"
#include "misc/Strings.h"

#include <QSocketNotifier>

#include <errno.h>
#include <sstream>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <limits>

namespace Rosegarden
{

TranzportClient::TranzportClient(RosegardenMainWindow* rgGUIApp) :
    QObject(),
    device_online(true),
    previous_buttons(*reinterpret_cast<uint32_t*>(previousbuf+2)),
    current_buttons(*reinterpret_cast<uint32_t*>(currentbuf+2)),
    datawheel(currentbuf[6]),
    status(currentbuf[1]),
    m_rgGUIApp(rgGUIApp),
    m_rgDocument(rgGUIApp->getDocument()),
    m_composition(&m_rgDocument->getComposition())
{
    m_descriptor = open("/dev/tranzport0",O_RDWR);
    
    if (m_descriptor < 0) {
        throw Exception(qstrtostr(QObject::tr("Failed to open tranzport device /dev/tranzport0")));
    }

    bzero(currentbuf,8);
    bzero(previousbuf,8);

    fcntl(m_descriptor,F_SETOWN, getpid());
    int socketFlags = fcntl(m_descriptor, F_GETFL, 0);
    if (socketFlags != -1) {
        fcntl(m_descriptor, F_SETFL, socketFlags | O_NONBLOCK);
    }

    m_socketReadNotifier = new QSocketNotifier(m_descriptor, QSocketNotifier::Read, nullptr);
    m_socketWriteNotifier = new QSocketNotifier(m_descriptor, QSocketNotifier::Write,nullptr);

    connect(m_socketReadNotifier, &QSocketNotifier::activated, this, &TranzportClient::readData);
    connect(m_socketWriteNotifier, &QSocketNotifier::activated, this, &TranzportClient::writeCommandQueue);
        
    connect(this, &TranzportClient::play,
            m_rgGUIApp, &RosegardenMainWindow::slotPlay );
    connect(this, &TranzportClient::stop,
            m_rgGUIApp, &RosegardenMainWindow::slotStop );
    connect(this, &TranzportClient::record,
            m_rgGUIApp, &RosegardenMainWindow::slotRecord );
    connect(this, &TranzportClient::rewind,
            m_rgGUIApp, &RosegardenMainWindow::slotRewind );
    connect(this, &TranzportClient::rewindToBeginning,
            m_rgGUIApp, &RosegardenMainWindow::slotRewindToBeginning );
    connect(this, &TranzportClient::fastForward,
            m_rgGUIApp, &RosegardenMainWindow::slotFastforward );
    connect(this, &TranzportClient::fastForwardToEnd,
            m_rgGUIApp, &RosegardenMainWindow::slotFastForwardToEnd );
    connect(this, &TranzportClient::toggleRecord,
            m_rgGUIApp, &RosegardenMainWindow::slotToggleRecord );
    connect(this, &TranzportClient::trackDown,
            m_rgGUIApp, &RosegardenMainWindow::slotSelectNextTrack);
    connect(this, &TranzportClient::trackUp,
            m_rgGUIApp, &RosegardenMainWindow::slotSelectPreviousTrack);
    connect(this, SIGNAL(trackMute()),
            m_rgGUIApp, SLOT(slotToggleMute()) );
    connect(this, &TranzportClient::trackRecord,
            m_rgGUIApp, &RosegardenMainWindow::slotToggleRecordCurrentTrack );
    connect(this, &TranzportClient::solo,
            m_rgGUIApp, &RosegardenMainWindow::slotToggleSolo);

    connect(m_rgGUIApp, &RosegardenMainWindow::documentLoaded,
            this, &TranzportClient::slotDocumentLoaded);

    connect(m_rgDocument, &RosegardenDocument::pointerPositionChanged,
            this, &TranzportClient::pointerPositionChanged);

    connect(m_rgDocument, &RosegardenDocument::loopChanged,
            this, &TranzportClient::loopChanged);

    connect(this, &TranzportClient::undo,
            CommandHistory::getInstance(),&CommandHistory::undo);

    connect(this, &TranzportClient::redo,
            CommandHistory::getInstance(), &CommandHistory::redo);

    connect(this, &TranzportClient::setPosition,
            m_rgDocument, &RosegardenDocument::slotSetPointerPosition);

    m_composition->addObserver(this);
    m_socketWriteNotifier->setEnabled(false);
    stateUpdate();

    RG_DEBUG << "TranzportClient::TranzportClient: connected to tranzport device: " << m_descriptor;
}

void
TranzportClient::pointerPositionChanged(timeT time)
{
    RG_DEBUG << "TranzportClient, pointerPositionChanged";

    if (device_online) {
        static int prevbeat = 0;
        int bar, beat, fraction, remainder;

        m_composition->getMusicalTimeForAbsoluteTime(time,bar,beat,fraction,remainder);
        if (prevbeat != beat) {
            std::stringstream ss;
            ss << bar+1 << ":" << beat;
            LCDWrite(ss.str(), Bottom, 10);
            prevbeat = beat;
        }
    }
}

void
TranzportClient::slotDocumentLoaded(RosegardenDocument *doc)
{
    RG_DEBUG << "TranzportClient::DocumentChanged ";

    m_rgDocument = doc;
    m_composition = &m_rgDocument->getComposition();
    m_composition->addObserver(this);
    connect(m_rgDocument, &RosegardenDocument::pointerPositionChanged,
            this, &TranzportClient::pointerPositionChanged);
    connect(m_rgDocument, &RosegardenDocument::loopChanged,
            this, &TranzportClient::loopChanged);
    connect(this, &TranzportClient::setPosition,
            m_rgDocument, &RosegardenDocument::slotSetPointerPosition);
                
    while (not commands.empty()) {
        commands.pop();
    }
    stateUpdate();
}

#if 0
// soloChanged() has been removed.  Solo has moved from Composition to
// Track.  The new CompositionObserver::selectedTrackChanged() might be
// of interest in bringing this back to life.

/**
 * Called when solo status changes (solo on/off, and selected track)
 */
void
TranzportClient::soloChanged(const Composition * c,
                             bool  solo,
                             TrackId  selectedTrack )
{
    RG_DEBUG << "TranzportClient, CompostionObserver::soloChanged";

    if (device_online) {
        if (solo) {
            LightOn(LightAnysolo);
        } else {
            LightOff(LightAnysolo);
        }

        Track* track = c->getTrackById(selectedTrack);

        if (track->isArmed()) {
            LightOn(LightTrackrec);
        } else {
            LightOff(LightTrackrec);
        }

        if (track->isMuted()) {
            LightOn(LightTrackmute);
        } else {
            LightOff(LightTrackmute);
        }

        LCDWrite(track->getLabel(), Bottom);
    }
}
#endif

/**
 * Called when a track is changed (instrument id, muted status...)
 */
void
TranzportClient::trackChanged(const Composition *c,
                              Track* track)
{
    RG_DEBUG << "TranzportClient, CompostionObserver::trackChanged";
    
    if (device_online) {
        const Track* track2 = c->getTrackById(c->getSelectedTrack());

        // If the changed track is the selected track
        if (track == track2) {
            RG_DEBUG << "TranzportClient, CompostionObserver::trackChanged updateing";
            
            if (track->isArmed()) {
                LightOn(LightTrackrec);
            } else {
                LightOff(LightTrackrec);
            }

            if (track->isMuted()) {
                LightOn(LightTrackmute);
            } else {
                LightOff(LightTrackmute);
            }

            LCDWrite(track->getLabel(), Bottom);
        }
    }
}
    
void
TranzportClient::loopChanged(timeT t1,
                             timeT t2)
{
    RG_DEBUG << "TranzportClient: loopChanged" << t1 << ", " << t2;

    if (device_online) {
        if (t1 == 0  and  t2 == 0) {
            LightOff(LightLoop);
        } else {
            LightOn(LightLoop);
        }
    }
}

void
TranzportClient::stateUpdate()
{
    if (device_online) {
        LCDWrite("Rosegarden");

        // No longer supported.  Solo has moved to Track.
        //if (m_composition->isSolo()) {
        //    LightOn(LightAnysolo);
        //} else {
            LightOff(LightAnysolo);
        //}

        if (m_composition->isLooping()) {
            LightOn(LightLoop);
        } else {
            LightOff(LightLoop);
        }

        TrackId trackID = m_composition->getSelectedTrack();
        Track* track = m_composition->getTrackById(trackID);

        if (track->isArmed()) {
            LightOn(LightTrackrec);
        } else {
            LightOff(LightTrackrec);
        }

        if (track->isMuted()) {
            LightOn(LightTrackmute);
        } else {
            LightOff(LightTrackmute);
        }

        LCDWrite(track->getLabel().substr(0,9), Bottom);

        int bar, beat, fraction, remainder;
        m_composition->getMusicalTimeForAbsoluteTime(
                m_composition->getPosition(), bar, beat, fraction, remainder);
        std::stringstream ss;
        ss << bar+1 << ":" << beat;
        LCDWrite(ss.str(), Bottom, 10);
    }
}
    
TranzportClient::~TranzportClient()
{
    delete m_socketReadNotifier;
    delete m_socketWriteNotifier;

    close(m_descriptor);

    RG_DEBUG << "TranzportClient::~TranzportClient: cleaned up ";
}
  
    
void TranzportClient::writeCommandQueue()
{
    RG_DEBUG << "TranzportClient: writeCommandQueue";

    if (commands.empty()) {
        m_socketWriteNotifier->setEnabled(false);
        return;
    }

    uint64_t cmd = commands.front();
    int res = ::write(m_descriptor, (void*)&cmd, 8);
    m_socketWriteNotifier->setEnabled(false);

    if (res < 0) {
        RG_DEBUG << "TranzportClient::writeCommandQueue: could not write to device, error" << strerror(errno);

        m_socketWriteNotifier->setEnabled(true);
        return;
    } else if (res != 8) {
        RG_DEBUG << "TranzportClient::writeCommandQueue: could not write full data to device";
        
        commands.pop();
        m_socketWriteNotifier->setEnabled(true);
    }

    commands.pop();

    if (not commands.empty()) {
        m_socketWriteNotifier->setEnabled(true);
    }
}
    
void
TranzportClient::write(uint64_t buf)
{
    commands.push(buf);
    if (not m_socketWriteNotifier->isEnabled()) {
        RG_DEBUG << "TranzportClient::write Setting the socket write notifier to enabled";
        m_socketWriteNotifier->setEnabled(true);
    }
}
    
void
TranzportClient::LightOn(Light light)
{
    uint8_t cmd[8];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = light;
    cmd[3] = 0x01;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;

    write(*(uint64_t*) cmd);
}

void
TranzportClient::LightOff(Light light)
{
    uint8_t cmd[8];

    cmd[0] = 0x00;
    cmd[1] = 0x00;
    cmd[2] = light;
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;
    write(*(uint64_t*)cmd);
}

void
TranzportClient::LCDWrite(const std::string& text,
                          Row row,
                          uint8_t offset)
{
    if (offset >= LCDLength) {
        return;
    }

    std::string str(LCDLength, ' ' );
    str.insert(offset, text.c_str(),
            std::min(text.size(), static_cast<size_t>(LCDLength - offset)));

    uint8_t cmd[8];
    uint8_t cell = row == Top ? 0 : 5;
    
    for (int i = 0; i < LCDLength;) {
        cmd[0] = 0x00;
        cmd[1] = 0x01;
        cmd[2] = cell++;
        cmd[3] = str[i++];
        cmd[4] = str[i++];
        cmd[5] = str[i++];
        cmd[6] = str[i++];
        cmd[7] = 0x00;
        write(*(uint64_t*)cmd);
    }
}
    
void
TranzportClient::readData()
{
    memcpy(previousbuf, currentbuf, 8);
    ssize_t val;
    static timeT loop_start_time=0;
    static timeT loop_end_time=0;

    while ((val=read(m_descriptor,currentbuf,8)) == 8) {
        uint32_t new_buttons = current_buttons ^ previous_buttons;
        if (status == 0x1) {
            RG_DEBUG << "TranzportClient: device just came online";

            while (not commands.empty()) {
                commands.pop();
            }
            device_online = true;

            m_rgDocument = m_rgGUIApp->getDocument();
            m_composition = &m_rgGUIApp->getDocument()->getComposition();
            stateUpdate();
        }

        if (status == 0xff) {
            RG_DEBUG << "TranzportClient: device just went offline";

            device_online = false;
            return;
        }

        // Solo has moved to Track.
#if 0
        if (new_buttons & TrackSolo  and
            current_buttons & TrackSolo) {
            if (current_buttons & Shift) {
                bool soloflag = m_composition->isSolo();
                emit solo(not soloflag);
            }
        }
#endif

        if (new_buttons & Add  and
            current_buttons & Add) {
            if (current_buttons & Shift) {
            } else {
                AddMarkerCommand* cmd = new AddMarkerCommand(m_composition,
                                                             m_composition->getPosition(),
                                                             "tranzport",
                                                             "");
                CommandHistory::getInstance()->addCommand(cmd);
            }
        }

        if (new_buttons & Prev  and
            current_buttons & Prev) {
            RG_DEBUG << "TranzportClient:: received marker previous";

            if (current_buttons & Shift) {
            } else {
                timeT currentTime = m_composition->getPosition();
                Composition::markercontainer& mc = m_composition->getMarkers();
                timeT closestPrevious = -1;

                for (Composition::markerconstiterator it = mc.begin();
                     it != mc.end();
                     ++it) {
                    timeT markerTime = (*it)->getTime();
                    if (markerTime < currentTime  and
                        markerTime > closestPrevious) {
                        closestPrevious = markerTime;
                    }
                }

                if (closestPrevious >= 0) {
                    RG_DEBUG << "Tranzport:: setting position: " << closestPrevious;

                    emit setPosition(closestPrevious);
                }
            }
        }

        if (new_buttons & Next  and
            current_buttons & Next)
        {
            RG_DEBUG << "TranzportClient:: received marker next";

            if (current_buttons & Shift) {
            } else {
                timeT currentTime = m_composition->getPosition();
                Composition::markercontainer& mc = m_composition->getMarkers();
                timeT closestNext = std::numeric_limits<long>::max();
                
                for (Composition::markerconstiterator it = mc.begin();
                     it != mc.end();
                     ++it) {
                    timeT markerTime = (*it)->getTime();
                    if (markerTime > currentTime and
                        markerTime < closestNext) {
                        closestNext = markerTime;
                    }
                }

                if (closestNext < std::numeric_limits<long>::max()) {
                    RG_DEBUG << "Tranzport:: setting position: " << closestNext;

                    emit setPosition(closestNext);
                }
            }
        }

        if (new_buttons & Undo  and
            current_buttons & Undo) {
            if (current_buttons & Shift) {
                emit redo();
            } else {
                emit undo();
            }
        }

        if (new_buttons & Play  and
            current_buttons & Play) {
            if (current_buttons & Shift) {
            } else {
                emit play();
            }
        }

        if (new_buttons & Stop  and
            current_buttons & Stop) {
            if (current_buttons & Shift) {
            } else {
                emit stop();
            }
        }

        if (new_buttons & Record  and
            current_buttons & Record) {
            if (current_buttons & Shift) {
            } else {
                emit record();
            }
        }

        if (new_buttons & Loop  and
            current_buttons & Loop) {
            if (current_buttons & Shift) {
            } else {
                loop_start_time = m_composition->getPosition();
                loop_end_time = loop_start_time;
            }
        }

        if (new_buttons & Loop  and
            (not (current_buttons & Loop))) {
            if (current_buttons & Shift) {
            } else {
                if (loop_start_time == loop_end_time) {
                    m_rgDocument->setLoop(0,0);
                }

                loop_start_time = 0;
                loop_end_time = 0;
            }
        }

        if (new_buttons& Rewind  and
            current_buttons & Rewind) {
            if (current_buttons&Shift) {
                emit rewindToBeginning();
            } else {
                emit rewind();
            }
        }

        if (new_buttons & FastForward  and
            current_buttons & FastForward) {
            if (current_buttons & Shift) {
                emit fastForwardToEnd();
            } else {
                emit fastForward();
            }
        }

        if (new_buttons & TrackRec  and
            current_buttons & TrackRec) {
            if (current_buttons & Shift) {
            } else {
                emit trackRecord();
            }
        }

        if (new_buttons & TrackRight  and
            current_buttons & TrackRight) {
            if (current_buttons & Shift) {
            } else {
                emit trackDown();
            }
        }

        if (new_buttons & TrackLeft  and
            current_buttons & TrackLeft) {
            if (current_buttons& Shift) {
            } else {
                emit trackUp();
            }
        }

        if (new_buttons & TrackMute  and
            current_buttons & TrackMute) {
            if (current_buttons & Shift) {
            } else {
                emit trackMute();
            }
        }

        if (datawheel) {
            if (datawheel < 0x7F) {
                if (current_buttons & Loop) {
                    loop_end_time += datawheel *
                        m_composition->getDurationForMusicalTime(loop_end_time, 0,1,0,0);
                    m_rgDocument->setLoop(loop_start_time, loop_end_time);
                } else if(current_buttons & Shift) {
                    timeT here = m_composition->getPosition();
                    here += datawheel * m_composition->getDurationForMusicalTime(here,0,0,1,0);
                    if (here <= m_composition->getEndMarker()) {
                        emit setPosition(here);
                    }
                } else {
                    timeT here = m_composition->getPosition();
                    here += datawheel * m_composition->getDurationForMusicalTime(here,0,1,0,0);
                    if (here <= m_composition->getEndMarker()) {
                        emit setPosition(here);
                    }
                }
            } else {
#define DATAWHEEL_VALUE (1 + (0xFF - (datawheel)))
                if (current_buttons & Loop) {
                    loop_end_time -= (1 + (0xFF - datawheel)) *
                        m_rgGUIApp->getDocument()->getComposition().getDurationForMusicalTime(loop_end_time, 0,1,0,0);
                    m_rgDocument->setLoop(loop_start_time, loop_end_time);
                }

                if (current_buttons & Shift) {
                    timeT here = m_composition->getPosition();
                    here -= DATAWHEEL_VALUE *  m_composition->getDurationForMusicalTime(here,0,0,1,0);
                    if (here >= m_composition->getStartMarker()) {
                        emit setPosition(here);
                    }
                } else {
                    timeT here = m_composition->getPosition();
                    here -= DATAWHEEL_VALUE *  m_composition->getDurationForMusicalTime(here,0,1,0,0);
                    if (here >= m_composition->getStartMarker()) {
                        emit setPosition(here);
                    }
                }
#undef DATAWHEEL_VALUE
            }
        }

        memcpy(previousbuf, currentbuf, 8);
    }

    if (val == -1) {
        if (errno == EAGAIN) {
            return;
        } else {
            RG_DEBUG << "TranzportClient::readData: error " << strerror(errno);
        }
    } else {
        RG_DEBUG << "TranzportClient::readData: partial read of length " << val;
        RG_DEBUG << "TranzportClient::readData: this should not happen " << val;
    }
}

}

