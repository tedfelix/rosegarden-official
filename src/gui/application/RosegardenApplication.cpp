/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RosegardenApplication.h"

#include "misc/Debug.h"
#include "document/ConfigGroups.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "RosegardenGUIApp.h"
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kmessagebox.h>
#include <QProcess>
#include <kuniqueapplication.h>
#include <QByteArray>
#include <QEventLoop>
#include <QSessionManager>
#include <QString>
#include <dcopclient.h>
#include <kconfig.h>
#include <kstatusbar.h>


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

void RosegardenApplication::sfxLoadExited(QProcess *proc)
{
    if (!proc->normalExit()) {
        QString configGroup = config()->group();
        QSettings config();
        config().beginGroup( SequencerOptionsConfigGroup );
        // 
        // FIX-manually-(GW), add:
        // config().endGroup();		// corresponding to: config().beginGroup( SequencerOptionsConfigGroup );
        //  

        QString soundFontPath = config().value("soundfontpath", "") ;
        QSettings config();
        config().beginGroup( configGroup );
        // 
        // FIX-manually-(GW), add:
        // config().endGroup();		// corresponding to: config().beginGroup( configGroup );
        //  


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

RosegardenApplication* RosegardenApplication::rgApp()
{
    return dynamic_cast<RosegardenApplication*>(kApplication());
}

QByteArray RosegardenApplication::Empty;

}

#include "RosegardenApplication.moc"
