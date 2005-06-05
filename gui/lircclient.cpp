// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2005
        Toni Arnold         <toni__arnold@bluewin.ch>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "config.h"

#ifdef HAVE_LIRC

#include "lircclient.h"

#include <stdlib.h>
#include <fcntl.h>

#include <qsocketnotifier.h>

#include "Exception.h"

#include "rosedebug.h"

#include <lirc/lirc_client.h>


// constructor/destructor initializes the LircClient library
LircClient::LircClient(void)
    : QObject()
{   
    int socketFlags;

    // socket setup with nonblock
    m_socket = lirc_init("rosegarden", 1);
    if (m_socket == -1) {
	throw Rosegarden::Exception("Failed to connect to LIRC");
    }
    
    fcntl(m_socket, F_GETOWN, getpid());
    socketFlags = fcntl(m_socket, F_GETFL, 0);
    if(socketFlags != -1)
    {
        fcntl(socketFlags, F_SETFL, socketFlags|O_NONBLOCK);
    }    
    
    if(lirc_readconfig(NULL, &m_config, NULL)==-1) exit(EXIT_FAILURE);
    
    m_socketNotifier = new QSocketNotifier(m_socket, QSocketNotifier::Read, 0);
    connect(m_socketNotifier, SIGNAL(activated(int)), this, SLOT(readButton()) );
        
    RG_DEBUG << "LircClient::LircClient: connected to socket: " << m_socket << endl;
}

LircClient::~LircClient()
{    
    lirc_freeconfig(m_config);
    delete m_socketNotifier;
    lirc_deinit();
    
    RG_DEBUG << "LircClient::~LircClient: cleaned up" << endl;
}


// The slot which is called when a button on the IR remote is pressed
void LircClient::readButton()
{
    char *code;
    int ret;
            
    RG_DEBUG << "LircClient::readButton" << endl;
    
    if (lirc_nextcode(&code) == 0  &&  code != NULL)
    {   // no error && a string is available
    
        while ( (ret = lirc_code2char(m_config, code, &m_command)) == 0  &&  m_command != NULL )
        {   // handle any command attached to that button
        
            RG_DEBUG << "LircClient::readButton: emitting command: " << m_command << endl;
            
            emit buttonPressed(m_command);
        }
        free(code);
    }
}

#include "lircclient.moc"

#endif // HAVE_LIRC
