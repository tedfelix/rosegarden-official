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

#define RG_MODULE_STRING "[MixerWindow]"

#include "MixerWindow.h"

#include "misc/Debug.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"

#include <QMainWindow>
#include <QShortcut>
#include <QWidget>


namespace Rosegarden
{

MixerWindow::MixerWindow(QWidget *parent,
                         RosegardenDocument *document) :
    QMainWindow(parent),
    m_document(document),
    m_studio(&document->getStudio()),
    m_currentId(0)
{
    m_shortcuts = new QShortcut(this);
    setAttribute(Qt::WA_DeleteOnClose);
}

void
MixerWindow::closeEvent(QCloseEvent *e)
{
    RG_DEBUG << "MixerWindow::closeEvent()\n";
    emit closing();
    QMainWindow::closeEvent(e);
}

void
MixerWindow::windowActivationChange(bool)
{
    if (isActiveWindow()) {
        emit windowActivated();
        sendControllerRefresh();
    }
}

void
MixerWindow::slotClose()
{
    RG_DEBUG << "MixerWindow::slotClose()\n";
    close();
}

}
