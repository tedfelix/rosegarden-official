/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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


using namespace Rosegarden;

// to be outside the Rosegarden namespace
static void initResources()
{
    // Initialize .qrc resources (necessary when using a static lib)
    Q_INIT_RESOURCE(data);
    Q_INIT_RESOURCE(locale);
}

RosegardenApplication::RosegardenApplication(int &argc, char **argv) :
    QApplication(argc, argv) {

    initResources();
}

void RosegardenApplication::saveState(QSessionManager& /* sm */)
{
    emit aboutToSaveState();
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
