/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ListEditView]"

#include "ListEditView.h"

#include "misc/Debug.h"
#include "base/Clipboard.h"
#include "base/Event.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "gui/dialogs/ConfigureDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "gui/general/EditViewTimeSigNotifier.h"
#include "misc/Strings.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "document/Command.h"

#include <QSettings>
#include <QAction>
#include <QShortcut>
#include <QDialog>
#include <QFrame>
#include <QIcon>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <QStatusBar>
#include <QMainWindow>
#include <QCloseEvent>
#include <QLayout>
#include <QApplication>
#include <QTreeWidget>
#include <QTreeWidgetItem>


namespace Rosegarden
{

bool ListEditView::m_inPaintEvent = false;
const unsigned int ListEditView::NbLayoutRows = 6;


ListEditView::ListEditView(RosegardenDocument *doc,
                           std::vector<Segment *> segments,
                           unsigned int cols,
                           QWidget *parent) :
    EditViewBase(doc, segments, parent),
    m_viewNumber( -1),
    m_viewLocalPropertyPrefix(makeViewLocalPropertyPrefix()),
    m_centralFrame(0),
    m_grid(0),
    m_mainCol(cols - 1),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId()),
    m_needUpdate(false),
    m_pendingPaintEvent(0),
    m_havePendingPaintEvent(false),
    m_inCtor(true),
    m_timeSigNotifier(new EditViewTimeSigNotifier(doc))
{
    setStatusBar( new QStatusBar(this) );

    m_centralFrame = new QFrame(this);
    m_centralFrame->setObjectName("centralframe");
	m_centralFrame->setMinimumSize( 500, 300 );
	m_centralFrame->setMaximumSize( 2200, 1400 );
	
	// 
	m_grid = new QGridLayout(m_centralFrame);
	m_centralFrame->setLayout( m_grid );
	
	// Note: We add Widget bottom-right, so the grid gets the propper col,row count
	// NbLayoutRows, cols
	//m_grid->addWidget( new QWidget(this), NbLayoutRows, cols);
	
	
	//this->setLayout( new QVBoxLayout(this) );
	//this->layout()->addWidget( m_centralFrame );
	setCentralWidget( m_centralFrame );
	
    initSegmentRefreshStatusIds();
}

ListEditView::~ListEditView()
{
    delete m_timeSigNotifier;

//&&& Detact CommandHistory
//    CommandHistory::getInstance()->detachView(actionCollection());
    m_viewNumberPool.erase(m_viewNumber);
}

std::set<int> ListEditView::m_viewNumberPool;

std::string
ListEditView::makeViewLocalPropertyPrefix()
{
    static char buffer[100];
    int i = 0;
    while (m_viewNumberPool.find(i) != m_viewNumberPool.end())
        ++i;
    m_viewNumber = i;
    m_viewNumberPool.insert(i);
    sprintf(buffer, "View%d::", i);
    return buffer;
}

void
ListEditView::setupActions(QString rcFileName, bool haveClipboard)
{
    setRCFileName(rcFileName);
    setupBaseActions(haveClipboard);
}

void
ListEditView::paintEvent(QPaintEvent* e)
{
//    QMainWindow::paintEvent(e); return;//&&& //!!! for experimental purposes

    // It is possible for this function to be called re-entrantly,
    // because a re-layout procedure may deliberately ask the event
    // loop to process some more events so as to keep the GUI looking
    // responsive.  If that happens, we remember the events that came
    // in in the middle of one paintEvent call and process their union
    // again at the end of the call.

    // Comment from David Faure: do NOT, I repeat, do NOT, relayout from within paintEvent.
    // Do it before, i.e. all of this code here does not belong to a paintEvent.

    /*
        if (m_inPaintEvent) {
    	NOTATION_DEBUG << "ListEditView::paintEvent: in paint event already";
    	if (e) {
    	    if (m_havePendingPaintEvent) {
    		if (m_pendingPaintEvent) {
    		    QRect r = m_pendingPaintEvent->rect().unite(e->rect());
    		    *m_pendingPaintEvent = QPaintEvent(r);
    		} else {
    		    m_pendingPaintEvent = new QPaintEvent(*e);
    		}
    	    } else {
    		m_pendingPaintEvent = new QPaintEvent(*e);
    	    }
    	}
    	m_havePendingPaintEvent = true;
    	return;
        }
    */ 
    //!!!    m_inPaintEvent = true;

    RG_DEBUG << "ListEditView::paintEvent";

    if (isCompositionModified()) {

        // Check if one of the segments we display has been removed
        // from the composition.
        //
        // For the moment we'll have to close the view if any of the
        // segments we handle has been deleted.

        for (unsigned int i = 0; i < m_segments.size(); ++i) {

            if (!m_segments[i]->getComposition()) {
                // oops, I think we've been deleted
                close();
                return ;
            }
        }
    }


    m_needUpdate = false;

    // Scan all segments and check if they've been modified.
    //
    // If we have more than one segment modified, we need to update
    // them all at once with the same time range, otherwise we can run
    // into problems when the layout of one depends on the others.  So
    // we use updateStart/End to calculate a bounding range for all
    // modifications.

    timeT updateStart = 0, updateEnd = 0;
    int segmentsToUpdate = 0;
    Segment *singleSegment = 0;

    for (unsigned int i = 0; i < m_segments.size(); ++i) {

        Segment* segment = m_segments[i];
        unsigned int refreshStatusId = m_segmentsRefreshStatusIds[i];
        SegmentRefreshStatus &refreshStatus =
            segment->getRefreshStatus(refreshStatusId);

        if (refreshStatus.needsRefresh() && isCompositionModified()) {

            // if composition is also modified, relayout everything
            refreshSegment(0);
            segmentsToUpdate = 0;
            break;

        } else if (m_timeSigNotifier->hasTimeSigChanged()) {

            // not exactly optimal!
            refreshSegment(0);
            segmentsToUpdate = 0;
            m_timeSigNotifier->reset();
            break;

        } else if (refreshStatus.needsRefresh()) {

            timeT startTime = refreshStatus.from(),
                              endTime = refreshStatus.to();

            if (segmentsToUpdate == 0 || startTime < updateStart) {
                updateStart = startTime;
            }
            if (segmentsToUpdate == 0 || endTime > updateEnd) {
                updateEnd = endTime;
            }
            singleSegment = segment;
            ++segmentsToUpdate;

            refreshStatus.setNeedsRefresh(false);
            m_needUpdate = true;
        }
    }

    if (segmentsToUpdate > 1) {
        refreshSegment(0, updateStart, updateEnd);
    } else if (segmentsToUpdate > 0) {
        refreshSegment(singleSegment, updateStart, updateEnd);
    }

    if (e) QMainWindow::paintEvent(e);

    // moved this to the end of the method so that things called
    // from this method can still test whether the composition had
    // been modified (it's sometimes useful to know whether e.g.
    // any time signatures have changed)
    setCompositionModified(false);

    //!!!    m_inPaintEvent = false;
    /*
        if (m_havePendingPaintEvent) {
    	e = m_pendingPaintEvent;
    	m_havePendingPaintEvent = false;
    	m_pendingPaintEvent = 0;
    	paintEvent(e);
    	delete e;
        }
    */
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
    return getDocument()->getComposition().getRefreshStatus
           (m_compositionRefreshStatusId).needsRefresh();
}

void ListEditView::setCompositionModified(bool c)
{
    getDocument()->getComposition().getRefreshStatus
    (m_compositionRefreshStatusId).setNeedsRefresh(c);
}

void ListEditView::toggleWidget(QWidget* widget,
                                const QString& toggleActionName)
{
    QAction *toggleAction = findAction(toggleActionName);

    if (!toggleAction) {
        RG_DEBUG << "!!! Unknown toggle action : " << toggleActionName;
        return ;
    }

    widget->setVisible(toggleAction->isChecked());
}

}
