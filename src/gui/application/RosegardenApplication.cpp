/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "RosegardenApplication.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "document/ConfigGroups.h"
#include "gui/application/RosegardenDCOP.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "RosegardenGUIApp.h"
#include <kcmdlineargs.h>
#include <kmainwindow.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kuniqueapplication.h>
#include <qcstring.h>
#include <qeventloop.h>
#include <qsessionmanager.h>
#include <qstring.h>


namespace Rosegarden
{

int RosegardenApplication::newInstance()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (RosegardenGUIApp::self() && args->count() &&
            RosegardenGUIApp::self()->getDocument() &&
            RosegardenGUIApp::self()->getDocument()->saveIfModified()) {
        // Check for modifications and save if necessary - if cancelled
        // then don't load the new file.
        //
        RosegardenGUIApp::self()->openFile(args->arg(0));
    }

    return KUniqueApplication::newInstance();
}

bool RosegardenApplication::isSequencerRegistered()
{
    if (noSequencerMode())
        return false;
    return dcopClient()->isApplicationRegistered(ROSEGARDEN_SEQUENCER_APP_NAME);
}

bool RosegardenApplication::sequencerSend(QCString dcopCall, QByteArray params)
{
    if (noSequencerMode())
        return false;

    return dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                              ROSEGARDEN_SEQUENCER_IFACE_NAME,
                              dcopCall, params);
}

bool RosegardenApplication::sequencerCall(QCString dcopCall, QCString& replyType, QByteArray& replyData,
        QByteArray params, bool useEventLoop)
{
    if (noSequencerMode())
        return false;
    return dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                              ROSEGARDEN_SEQUENCER_IFACE_NAME,
                              dcopCall, params, replyType, replyData, useEventLoop);
}

void RosegardenApplication::sfxLoadExited(KProcess *proc)
{
    if (!proc->normalExit()) {
        QString configGroup = config()->group();
        config()->setGroup(SequencerOptionsConfigGroup);
        QString soundFontPath = config()->readEntry("soundfontpath", "");
        config()->setGroup(configGroup);

        KMessageBox::error(mainWidget(),
                           i18n("Failed to load soundfont %1").arg(soundFontPath));
    } else {
        RG_DEBUG << "RosegardenApplication::sfxLoadExited() : sfxload exited normally\n";
    }

}

void RosegardenApplication::slotSetStatusMessage(QString msg)
{
    KMainWindow* mainWindow = dynamic_cast<KMainWindow*>(mainWidget());
    if (mainWindow) {
        if (msg.isEmpty())
            msg = KTmpStatusMsg::getDefaultMsg();
        mainWindow->statusBar()->changeItem(QString("  %1").arg(msg), KTmpStatusMsg::getDefaultId());
    }

}

void
RosegardenApplication::refreshGUI(int maxTime)
{
    eventLoop()->processEvents(QEventLoop::ExcludeUserInput |
                               QEventLoop::ExcludeSocketNotifiers,
                               maxTime);
}

void RosegardenApplication::saveState(QSessionManager& sm)
{
    emit aboutToSaveState();
    KUniqueApplication::saveState(sm);
}

QByteArray RosegardenApplication::Empty;

#include "rgapplication.moc"

}
