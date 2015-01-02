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

#define RG_MODULE_STRING "[RosegardenApplication]"

#include "RosegardenApplication.h"

#include "misc/Debug.h"
#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "RosegardenMainWindow.h"

#include <QMainWindow>
#include <QStatusBar>
#include <QMessageBox>
#include <QProcess>
#include <QByteArray>
#include <QEventLoop>
#include <QSessionManager>
#include <QString>
#include <QSettings>


namespace Rosegarden
{
/*&&&
int RosegardenApplication::newInstance()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (RosegardenMainWindow::self() && args->count() &&
        RosegardenMainWindow::self()->getDocument() &&
        RosegardenMainWindow::self()->getDocument()->saveIfModified()) {
        // Check for modifications and save if necessary - if cancelled
        // then don't load the new file.
        //
        RosegardenMainWindow::self()->openFile(args->arg(0));
    }

    return KUniqueApplication::newInstance();
}
*/
void RosegardenApplication::sfxLoadExited(QProcess *proc)
{
    if (proc->exitStatus() != QProcess::NormalExit ) {
        QSettings settings;
        settings.beginGroup( SequencerOptionsConfigGroup );

        QString soundFontPath = settings.value("soundfontpath", "").toString() ;
        settings.endGroup();                // corresponding to: settings().beginGroup( SequencerOptionsConfigGroup );
        //### dtb: Using topLevelWidgets()[0] in place of mainWidget() is a big assumption on my part.
        QMessageBox::critical( topLevelWidgets()[0], "",
                               tr("Failed to load soundfont %1").arg(soundFontPath ));
    } else {
        RG_DEBUG << "RosegardenApplication::sfxLoadExited() : sfxload exited normally\n";
    }
}

void RosegardenApplication::slotSetStatusMessage(QString msg)
{
    //### dtb: Using topLevelWidgets()[0] in place of mainWidget() is a big assumption on my part.
    QMainWindow* window = dynamic_cast<QMainWindow*>(topLevelWidgets()[0]);
    if (window) {
        if (msg.isEmpty())
            msg = TmpStatusMsg::getDefaultMsg();
//@@@        mainWindow->statusBar()->changeItem(QString("  %1").arg(msg), TmpStatusMsg::getDefaultId());
        window->statusBar()->showMessage(QString("  %1").arg(msg)); 
    }

}

void
RosegardenApplication::refreshGUI(int /* maxTime */)
{
//    eventLoop()->processEvents(QEventLoop::ExcludeUserInput |                 //&&& eventLoop()->processEvents()
//                               QEventLoop::ExcludeSocketNotifiers,
   //                            maxTime);
}

void RosegardenApplication::saveState(QSessionManager& /* sm */)
{
    emit aboutToSaveState();
//&&&    KUniqueApplication::saveState(sm);
}

RosegardenApplication* RosegardenApplication::ApplicationObject()
{
    return dynamic_cast<RosegardenApplication*>(qApp);
}

bool
RosegardenApplication::notify(QObject * receiver, QEvent * event) 
{
    try { return QApplication::notify(receiver, event); } 
    catch(std::exception& e) {
        RG_DEBUG << "Exception thrown:" << e.what();
        return false;
    }
}

QByteArray RosegardenApplication::Empty;

}

#include "RosegardenApplication.moc"
