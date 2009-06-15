/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
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

#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "base/Exception.h"
#include <QSocketNotifier>
#include "misc/Debug.h"
#include <errno.h>

#include "TranzportClient.h"
#include "document/RosegardenDocument.h"
#include "gui/editors/segment/TrackButtons.h"
#include "RosegardenMainWindow.h"


namespace Rosegarden
{
    TranzportClient::TranzportClient(RosegardenMainWindow* rgGUIApp)
    :QObject(),
     previous_buttons(*reinterpret_cast<uint32_t*>(previousbuf+2)),
     current_buttons(*reinterpret_cast<uint32_t*>(currentbuf+2)),
     datawheel(currentbuf[6]),
     status(currentbuf[1]),
     m_rgGUIApp(rgGUIApp)
    {
        m_descriptor = open("/dev/tranzport0",O_RDONLY);
        if(m_descriptor < 0)
        {
            throw Exception("Failed to open tranzport device /dev/tranzport0");      
        }
                
        m_writedescriptor = open("/dev/tranzport0", O_WRONLY);
        
        bzero(currentbuf,8);
        bzero(previousbuf,8);
    
        fcntl(m_descriptor,F_SETOWN, getpid());
         int socketFlags = fcntl(m_descriptor, F_GETFL, 0);
         if(socketFlags != -1)
         {
             fcntl(m_descriptor, F_SETFL, socketFlags | O_NONBLOCK);
         }


        m_socketNotifier = new QSocketNotifier(m_descriptor, QSocketNotifier::Read, 0);
        LCDWrite("Rosegarden");     

        connect(m_socketNotifier, SIGNAL(activated(int)), this, SLOT(readData()));
    
        connect(this, SIGNAL(play()),
                m_rgGUIApp, SLOT(slotPlay()) );
        connect(this, SIGNAL(stop()),
                m_rgGUIApp, SLOT(slotStop()) );
        connect(this, SIGNAL(record()),
                m_rgGUIApp, SLOT(slotRecord()) );
        connect(this, SIGNAL(rewind()),
                m_rgGUIApp, SLOT(slotRewind()) );
        connect(this, SIGNAL(rewindToBeginning()),
                m_rgGUIApp, SLOT(slotRewindToBeginning()) );
        connect(this, SIGNAL(fastForward()),
                m_rgGUIApp, SLOT(slotFastforward()) );
        connect(this, SIGNAL(fastForwardToEnd()),
                m_rgGUIApp, SLOT(slotFastForwardToEnd()) );
        connect(this, SIGNAL(toggleRecord()),
                m_rgGUIApp, SLOT(slotToggleRecord()) );
        connect(this, SIGNAL(trackDown()),
                m_rgGUIApp, SLOT(slotTrackDown()) );
        connect(this, SIGNAL(trackUp()),
                m_rgGUIApp, SLOT(slotTrackUp()) );
        connect(this, SIGNAL(trackMute()),
                m_rgGUIApp, SLOT(slotToggleMutedCurrentTrack()) );
        connect(this, SIGNAL(trackRecord()),
                m_rgGUIApp, SLOT(slotToggleRecordCurrentTrack()) );
        
        

        RG_DEBUG << "TranzportClient::TranzportClient: connected to tranzport device: " << m_descriptor << endl; 
    }
  
    TranzportClient::~TranzportClient()
    {
        delete m_socketNotifier;
        close(m_descriptor);
        close(m_writedescriptor);        
        RG_DEBUG << "TranzportClient::~TranzportClient: cleaned up " << endl;    
    }
  
    void
    TranzportClient::write(const uint8_t* buf)
    {
        int res = ::write(m_writedescriptor,buf,8);
        if(res < 0)
        {
            RG_DEBUG << "TranzportClient::Write: could not write to device, error" << strerror(errno) << endl;
        }
        else if(res != 8)
        {
            RG_DEBUG << "TranzportClient::Write: could not write full data to device" << endl;
        }
    }
    
    
    void
    TranzportClient::LCDWrite(const std::string& text,
                        Row row,
                        uint8_t offset)
    {
        if(offset >= LCDLength)
        {
            return;
        }
        
        std::string str(LCDLength, ' ' );
        str.insert(offset,text.c_str(),std::min(text.size(),static_cast<size_t>(LCDLength - offset)));
        
        uint8_t cmd[8];
        uint8_t cell = row == Top ? 0 : 5;
        for(int i = 0; i < LCDLength;)
        {
            cmd[0] = 0x00;
            cmd[1] = 0x01;
            cmd[2] = cell++;
            cmd[3] = str[i++];
            cmd[4] = str[i++];
            cmd[5] = str[i++];
            cmd[6] = str[i++];
            cmd[7] = 0x00;
            write(cmd);
        }
    }
    
    void
    TranzportClient::readData()  
    {
        memcpy(previousbuf, currentbuf, 8);
        ssize_t val;        
        while((val=read(m_descriptor,currentbuf,8)) == 8)
        {
            uint32_t new_buttons = current_buttons ^ previous_buttons;            
            if(status == 0x1)
            {
                LCDWrite("Rosegarden");                
            }
            
            if(new_buttons & Play and
                current_buttons & Play)
            {                
                if(current_buttons & Shift)
                {
                }
                else
                {                    
                    emit play();
                }                
            }
            if(new_buttons & Stop & 
               current_buttons & Stop)
            {
                if(current_buttons & Shift)
                {
                }
                else
                {
                    emit stop();
                }
            }
            if(new_buttons & Record and
               current_buttons & Record)
            {
                if(current_buttons & Shift)
                {
                }
                else
                {
                    emit record();
                }
            }
            if(new_buttons& Rewind and 
                current_buttons & Rewind)            
            {
                if(current_buttons&Shift)
                {
                    emit rewindToBeginning();
                }
                else
                {
                    emit rewind();
                }                
            }
            if(new_buttons & FastForward and
                current_buttons & FastForward)
            {
                if(current_buttons & Shift)
                {
                    emit fastForwardToEnd();                    
                }
                else
                {
                    emit fastForward();
                }                
            }
            if(new_buttons & TrackRec and
                current_buttons & TrackRec)
            {
                if(current_buttons & Shift)
                {
                }
                else
                {
                    emit  toggleRecord();
                }                
            }
            if(new_buttons & TrackRight and
                current_buttons & TrackRight)
            {
                if(current_buttons & Shift)
                {
                }
                else
                {
                    emit trackDown();                    
                }
            }
            if(new_buttons & TrackLeft and
                current_buttons & TrackLeft)
            {
                if(current_buttons& Shift)
                {
                }
                else
                {
                    emit trackUp();
                }
            }
            if(new_buttons & TrackMute and
               current_buttons & TrackMute)
            {
                if(current_buttons & Shift)
                {
                }
                else
                {
                    emit trackMute();
                }
            }
            
            if(datawheel)
            {                
                if(datawheel < 0x7F)
                {
                    if(current_buttons & Shift)
                    {          
                    }
                    else
                    {
                    }
                
                }
                
                else
                {
                    if(current_buttons & Shift)
                    {
                    }
                    else
                    {          
                    }        
                }
            }
            
            memcpy(previousbuf, currentbuf, 8);
        }
        
        
        if(val == -1)
        {
            if(errno == EAGAIN)
            {
                return;
            }
            else
            {
                RG_DEBUG << "TranzportClient::readData: error " << strerror(errno) << endl;
            }
            
        }
        else
        {
            RG_DEBUG << "TranzportClient::readData: partial read of length " << val << endl;
            RG_DEBUG << "TranzportClient::readData: this should not happen " << val << endl;
        }
    }
  
}
#include "TranzportClient.moc"
// Local Variables: **
// compile-command:"cd ../../../;make -j 6" **
// End: **
