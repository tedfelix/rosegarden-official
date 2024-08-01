/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ListEditView]"
#define RG_NO_DEBUG_PRINT

#include "ListEditView.h"

#include "misc/Debug.h"
#include "base/Segment.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "gui/general/EditViewTimeSigNotifier.h"

#include <QFrame>
#include <QStatusBar>
#include <QMainWindow>
#include <QGridLayout>


namespace Rosegarden
{


ListEditView::ListEditView(const std::vector<Segment *> &segments) :
    EditViewBase(segments)
{
    m_compositionRefreshStatusId = RosegardenDocument::currentDocument->
            getComposition().getNewRefreshStatusId();
    m_timeSigNotifier =
            new EditViewTimeSigNotifier(RosegardenDocument::currentDocument);

    setStatusBar(new QStatusBar(this));

    m_frame = new QFrame(this);
    m_frame->setObjectName("centralframe");
	m_frame->setMinimumSize(500, 300);
	m_frame->setMaximumSize(2200, 1400);

	m_gridLayout = new QGridLayout(m_frame);
	m_frame->setLayout(m_gridLayout);

	setCentralWidget(m_frame);

    initSegmentRefreshStatusIds();
}

ListEditView::~ListEditView()
{
    delete m_timeSigNotifier;
}

void
ListEditView::setupActions(const QString &rcFileName, bool haveClipboard)
{
    m_rcFileName = rcFileName;
    setupBaseActions(haveClipboard);
}

void
ListEditView::paintEvent(QPaintEvent* e)
{

    // ??? Comments in the header seem to indicate this is no longer
    //     necessary.  Try to get rid of this routine.  Use document
    //     modification handlers instead.


    // ??? This should not be done in paintEvent().  It should be handled
    //     in a document modified handler.  See how matrix does this and
    //     do the same (so long as it doesn't involve paintEvent()).
    if (isCompositionModified()) {
        for (unsigned int i = 0; i < m_segments.size(); ++i) {
            // If this Segment no longer has a Composition, close.
            if (!m_segments[i]->getComposition()) {
                close();
                return;
            }
        }
    }

    // ??? Again, do this in response to a document modification, not
    //     in paintEvent().  That makes no sense.

    // Scan all segments and check if they've been modified.

    int segmentsToUpdate = 0;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {
        Segment *segment = m_segments[i];

        unsigned int refreshStatusId = m_segmentsRefreshStatusIds[i];
        SegmentRefreshStatus &refreshStatus =
                segment->getRefreshStatus(refreshStatusId);

        if (refreshStatus.needsRefresh() && isCompositionModified()) {

            // if composition is also modified, relayout everything
            refreshList();
            segmentsToUpdate = 0;
            break;

        } else if (m_timeSigNotifier->hasTimeSigChanged()) {

            // not exactly optimal!
            refreshList();
            segmentsToUpdate = 0;
            m_timeSigNotifier->reset();
            break;

        } else if (refreshStatus.needsRefresh()) {
            ++segmentsToUpdate;
            refreshStatus.setNeedsRefresh(false);
        }
    }

    if (segmentsToUpdate > 0)
        refreshList();

    if (e)
        QMainWindow::paintEvent(e);

    // moved this to the end of the method so that things called
    // from this method can still test whether the composition had
    // been modified (it's sometimes useful to know whether e.g.
    // any time signatures have changed)
    setCompositionModified(false);

}

void ListEditView::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
}

void ListEditView::initSegmentRefreshStatusIds()
{
    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        unsigned int rid = m_segments[i]->getNewRefreshStatusId();
        m_segmentsRefreshStatusIds.push_back(rid);
    }
}

bool ListEditView::isCompositionModified()
{
    return RosegardenDocument::currentDocument->getComposition().getRefreshStatus
           (m_compositionRefreshStatusId).needsRefresh();
}

void ListEditView::setCompositionModified(bool c)
{
    RosegardenDocument::currentDocument->getComposition().getRefreshStatus
    (m_compositionRefreshStatusId).setNeedsRefresh(c);
}


}
