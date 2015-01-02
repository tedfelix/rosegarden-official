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

#define RG_MODULE_STRING "[SetWaitCursor]"

#include "SetWaitCursor.h"

#include "gui/editors/segment/TrackEditor.h"
#include "gui/editors/segment/compositionview/CompositionView.h"
#include "misc/Debug.h"
#include "RosegardenMainWindow.h"
#include "RosegardenMainViewWidget.h"
#include <QCursor>
#include <QWidget>
#include <QApplication>

namespace Rosegarden
{

//### dtb: Using topLevelWidgets()[0] in place of mainWidget() is a big assumption on my part.
SetWaitCursor::SetWaitCursor()
    : m_guiApp(dynamic_cast<RosegardenMainWindow*>(qApp->topLevelWidgets()[0]))
{
    if (m_guiApp) {

        // play it safe, so we can use this class at anytime even very early in the app init
        if ((m_guiApp->getView() &&
                m_guiApp->getView()->getTrackEditor() &&
                m_guiApp->getView()->getTrackEditor()->getCompositionView() &&
                m_guiApp->getView()->getTrackEditor()->getCompositionView()->viewport())) {

            m_saveCompositionViewCursor = m_guiApp->getView()->getTrackEditor()->getCompositionView()->viewport()->cursor();

        }

        RG_DEBUG << "SetWaitCursor::SetWaitCursor() : setting waitCursor\n";
        m_saveCursor = m_guiApp->cursor();

        m_guiApp->setCursor(Qt::WaitCursor);
    }
}

SetWaitCursor::~SetWaitCursor()
{
    if (m_guiApp) {

        RG_DEBUG << "SetWaitCursor::SetWaitCursor() : restoring normal cursor\n";
        QWidget* viewport = 0;
        QCursor currentCompositionViewCursor;

        if ((m_guiApp->getView() &&
                m_guiApp->getView()->getTrackEditor() &&
                m_guiApp->getView()->getTrackEditor()->getCompositionView() &&
                m_guiApp->getView()->getTrackEditor()->getCompositionView()->viewport())) {
            viewport = m_guiApp->getView()->getTrackEditor()->getCompositionView()->viewport();
            currentCompositionViewCursor = viewport->cursor();
        }

        m_guiApp->setCursor(m_saveCursor);

        if (viewport) {
            if (currentCompositionViewCursor.shape() == Qt::WaitCursor) {
                viewport->setCursor(m_saveCompositionViewCursor);
            } else {
                viewport->setCursor(currentCompositionViewCursor); // because m_guiApp->setCursor() has replaced it
            }
        }

        // otherwise, it's been modified elsewhere, so leave it as is

    }

}

}
