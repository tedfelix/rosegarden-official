// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <kconfig.h>

#include "constants.h"
#include "rgapplication.h"
#include "rosegardengui.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"

int RosegardenApplication::newInstance()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (RosegardenGUIApp::self() && args->count() &&
        RosegardenGUIApp::self()->getDocument() &&
        RosegardenGUIApp::self()->getDocument()->saveIfModified())
    {
        // Check for modifications and save if necessary - if cancelled
        // then don't load the new file.
        //
        RosegardenGUIApp::self()->openFile(args->arg(0));
    }

    return KUniqueApplication::newInstance();
}

bool RosegardenApplication::isSequencerRegistered()
{
    if (noSequencerMode()) return false;
    return dcopClient()->isApplicationRegistered(ROSEGARDEN_SEQUENCER_APP_NAME);
}

bool RosegardenApplication::sequencerSend(QCString dcopCall, QByteArray params)
{
    if (noSequencerMode()) return false;

    return dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                              ROSEGARDEN_SEQUENCER_IFACE_NAME,
                              dcopCall, params);
}

bool RosegardenApplication::sequencerCall(QCString dcopCall, QCString& replyType, QByteArray& replyData,
                                          QByteArray params, bool useEventLoop)
{
    if (noSequencerMode()) return false;
    return dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                              ROSEGARDEN_SEQUENCER_IFACE_NAME,
                              dcopCall, params, replyType, replyData, useEventLoop);
}

void RosegardenApplication::sfxLoadExited(KProcess *proc)
{
    if (!proc->normalExit()) {
        QString configGroup = config()->group();
        config()->setGroup(Rosegarden::SequencerOptionsConfigGroup);
        QString soundFontPath = config()->readEntry("soundfontpath", "");
        config()->setGroup(configGroup);
        
        KMessageBox::error(mainWidget(),
                           QString(i18n("Failed to load soundfont %1")).arg(soundFontPath));
    } else {
        RG_DEBUG << "RosegardenApplication::sfxLoadExited() : sfxload exited normally\n";
    }
    
}

void
RosegardenApplication::refreshGUI(int maxTime)
{
    eventLoop()->processEvents(QEventLoop::ExcludeUserInput |
			       QEventLoop::ExcludeSocketNotifiers,
			       maxTime);
}

RosegardenApplication* RosegardenApplication::rgApp()
{
    return dynamic_cast<RosegardenApplication*>(kApplication());
}


QByteArray RosegardenApplication::Empty;

