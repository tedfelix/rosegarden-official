// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

// include files for Qt
#include <qhbox.h>
#include <qvbox.h>
#include <qpushbutton.h>
#include <qfileinfo.h>
#include <qwmatrix.h>

// KDE includes
#include <kdebug.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kconfig.h>
#include <kapp.h>
#include <kprocess.h>
#include <kcommand.h>

// application specific includes
#include "MappedEvent.h"
#include "RulerScale.h"
#include "Instrument.h"
#include "Selection.h"
#include "AudioLevel.h"

#include "constants.h"
#include "rosestrings.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "trackeditor.h"
#include "compositionview.h"
#include "segmenttool.h"
#include "notationview.h"
#include "matrixview.h"
#include "trackbuttons.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "temporuler.h"
#include "chordnameruler.h"
#include "segmentparameterbox.h"
#include "instrumentparameterbox.h"
#include "trackparameterbox.h"
#include "rosegardenconfigurationpage.h"
#include "rosegardenconfiguredialog.h"
#include "eventview.h"
#include "dialogs.h"
#include "sequencemanager.h"
#include "sequencermapper.h"
#include "tempoview.h"

using Rosegarden::SimpleRulerScale;
using Rosegarden::Composition;
using Rosegarden::timeT;

// Use this to define the basic unit of the main QCanvas size.
//
// This apparently arbitrary figure is what we think is an
// appropriate width in pixels for a 4/4 bar.  Beware of making it
// too narrow, as shorter bars will be proportionally smaller --
// the visual difference between 2/4 and 4/4 is perhaps greater
// than it sounds.
//
static double barWidth44 = 100.0;

const QWidget *RosegardenGUIView::m_lastActiveMainWindow = 0;

// This is the maximum number of matrix, event view or percussion
// matrix editors to open in a single operation (not the maximum that
// can be open at a time -- there isn't one)
//
static int maxEditorsToOpen = 8;

RosegardenGUIView::RosegardenGUIView(bool showTrackLabels,
                                     SegmentParameterBox* segmentParameterBox,
                                     InstrumentParameterBox* instrumentParameterBox,
                                     TrackParameterBox* trackParameterBox,
                                     QWidget *parent,
                                     const char* /*name*/)
    : QVBox(parent),
      m_rulerScale(0),
      m_trackEditor(0),
      m_segmentParameterBox(segmentParameterBox),
      m_instrumentParameterBox(instrumentParameterBox),
      m_trackParameterBox(trackParameterBox)
{
    RosegardenGUIDoc* doc = getDocument();
    Composition *comp = &doc->getComposition();

    double unitsPerPixel =
        Rosegarden::TimeSignature(4, 4).getBarDuration() / barWidth44;
    m_rulerScale = new SimpleRulerScale(comp, 0, unitsPerPixel);

    // Construct the trackEditor first so we can then
    // query it for placement information
    //
    m_trackEditor  = new TrackEditor(doc, this,
                                     m_rulerScale, showTrackLabels, unitsPerPixel, this /*hbox*/);

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegment(Rosegarden::Segment*)),
            SLOT(slotEditSegment(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentNotation(Rosegarden::Segment*)),
            SLOT(slotEditSegmentNotation(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentMatrix(Rosegarden::Segment*)),
            SLOT(slotEditSegmentMatrix(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentAudio(Rosegarden::Segment*)),
            SLOT(slotEditSegmentAudio(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(audioSegmentAutoSplit(Rosegarden::Segment*)),
            SLOT(slotSegmentAutoSplit(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editSegmentEventList(Rosegarden::Segment*)),
            SLOT(slotEditSegmentEventList(Rosegarden::Segment*)));

    connect(m_trackEditor->getSegmentCanvas(),
            SIGNAL(editRepeat(Rosegarden::Segment*, Rosegarden::timeT)),
            SLOT(slotEditRepeat(Rosegarden::Segment*, Rosegarden::timeT)));

    connect(m_trackEditor->getTempoRuler(),
	    SIGNAL(doubleClicked(Rosegarden::timeT)),
	    SLOT(slotEditTempos(Rosegarden::timeT)));

    connect(m_trackEditor,
            SIGNAL(droppedDocument(QString)),
            parent,
            SLOT(slotOpenDroppedURL(QString)));

    connect(m_trackEditor,
            SIGNAL(droppedAudio(QString)),
            this,
            SLOT(slotDroppedAudio(QString)));
    
    connect(m_trackEditor,
            SIGNAL(droppedNewAudio(QString)),
            this,
            SLOT(slotDroppedNewAudio(QString)));

    connect(m_instrumentParameterBox,
            SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)),
            this,
            SLOT(slotChangeInstrumentLabel(Rosegarden::InstrumentId, QString)));

    connect(m_instrumentParameterBox,
            SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)),
            m_trackParameterBox,
            SLOT(slotInstrumentLabelChanged(Rosegarden::InstrumentId, QString)));
            
    connect(m_trackEditor->getTrackButtons(),
            SIGNAL(nameChanged()),
            m_trackParameterBox,
            SLOT(slotSelectedTrackNameChanged()));            

    connect(m_trackEditor->getTrackButtons(),
            SIGNAL(instrumentSelected(int)),
            m_trackParameterBox,
            SLOT(slotUpdateControls(int)));
            
    connect(m_trackParameterBox,
            SIGNAL(instrumentSelected(Rosegarden::TrackId, int)),
            m_trackEditor->getTrackButtons(),
            SLOT(slotTrackInstrumentSelection(Rosegarden::TrackId, int)));
            
    connect(this, SIGNAL(controllerDeviceEventReceived(Rosegarden::MappedEvent *, const void *)),
	    this, SLOT(slotControllerDeviceEventReceived(Rosegarden::MappedEvent *, const void *)));

    if (doc) {
/* signal no longer exists
	connect(doc, SIGNAL(recordingSegmentUpdated(Rosegarden::Segment *,
						    Rosegarden::timeT)),
		this, SLOT(slotUpdateRecordingSegment(Rosegarden::Segment *,
						      Rosegarden::timeT)));
*/

        QObject::connect
            (getCommandHistory(), SIGNAL(commandExecuted()),
             m_trackEditor->getSegmentCanvas(), SLOT(slotUpdateSegmentsDrawBuffer()));
    }
}


RosegardenGUIView::~RosegardenGUIView()
{
    RG_DEBUG << "~RosegardenGUIView()" << endl;
    delete m_rulerScale;
}

RosegardenGUIDoc*
RosegardenGUIView::getDocument() const
{
    return RosegardenGUIApp::self()->getDocument();
}

void RosegardenGUIView::print(Composition* p, bool previewOnly)
{
    SetWaitCursor waitCursor;

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    for (Composition::iterator i = p->begin(); i != p->end(); ++i) {
	if ((*i)->getType() != Rosegarden::Segment::Audio) {
	    segmentsToEdit.push_back(*i);
	}
    }

    if (segmentsToEdit.empty()) {
	KMessageBox::sorry(this, i18n("No non-audio segments in composition"));
	return;
    }

    NotationView *notationView = new NotationView(getDocument(),
                                                  segmentsToEdit,
                                                  this,
						  (NotationView *)0);

    if (!notationView->isOK()) {
	RG_DEBUG << "RosegardenGUIView::print : operation cancelled" << endl;
	delete notationView;
	return;
    }
    
    notationView->print(previewOnly);

    delete notationView;
}

void RosegardenGUIView::selectTool(const QString toolName)
{
    m_trackEditor->getSegmentCanvas()->slotSetTool(toolName);
}

bool
RosegardenGUIView::haveSelection()
{
    return m_trackEditor->getSegmentCanvas()->haveSelection();
}

Rosegarden::SegmentSelection
RosegardenGUIView::getSelection()
{
    return m_trackEditor->getSegmentCanvas()->getSelectedSegments();
}

void RosegardenGUIView::updateSelectionContents()
{
    m_trackEditor->getSegmentCanvas()->updateSelectionContents();
}

void
RosegardenGUIView::slotEditTempos(Rosegarden::timeT t)
{
    TempoView *tempoView = new TempoView(getDocument(), this, t);

    connect(tempoView, SIGNAL(windowActivated()),
	    this, SLOT(slotActiveMainWindowChanged()));

    connect(tempoView,
            SIGNAL(changeTempo(Rosegarden::timeT,
                               Rosegarden::tempoT,
                               Rosegarden::tempoT,
			       TempoDialog::TempoDialogAction)),
	    RosegardenGUIApp::self(),
            SLOT(slotChangeTempo(Rosegarden::timeT,
                                 Rosegarden::tempoT,
                                 Rosegarden::tempoT,
				 TempoDialog::TempoDialogAction)));

    connect(tempoView, SIGNAL(saveFile()),
	    RosegardenGUIApp::self(), SLOT(slotFileSave()));

    tempoView->show();
}

void
RosegardenGUIView::slotEditMetadata(QString name)
{
    const QWidget *ww = dynamic_cast<const QWidget *>(sender());
    QWidget *w = const_cast<QWidget *>(ww);

    Rosegarden::DocumentConfigureDialog *configDlg = 
        new Rosegarden::DocumentConfigureDialog(getDocument(), w ? w : this);

    configDlg->selectMetadata(name);

    configDlg->show();
}

void RosegardenGUIView::slotEditSegment(Rosegarden::Segment* segment)
{
    Rosegarden::Segment::SegmentType type = Rosegarden::Segment::Internal;

    if (segment) {
	type = segment->getType();
    } else {
	if (haveSelection()) {

	    bool haveType = false;

	    Rosegarden::SegmentSelection selection = getSelection(); 
	    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
		 i != selection.end(); ++i) {

		Rosegarden::Segment::SegmentType myType = (*i)->getType();
		if (haveType) {
		    if (myType != type) {
			KMessageBox::sorry(this, i18n("Selection must contain only audio or non-audio segments"));
			return;
		    }
		} else {
		    type = myType; 
		    haveType = true;
		    segment = *i;
		}
	    }
	} else return;
    }

    if (type == Rosegarden::Segment::Audio) {
	slotEditSegmentAudio(segment);
    } else {

        KConfig* config = kapp->config();
        config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
	Rosegarden::GeneralConfigurationPage::DoubleClickClient
            client =
	    (Rosegarden::GeneralConfigurationPage::DoubleClickClient)
            (config->readUnsignedNumEntry("doubleclickclient",
	    (unsigned int)Rosegarden::GeneralConfigurationPage::NotationView));

	if (client == Rosegarden::GeneralConfigurationPage::MatrixView) {
	    slotEditSegmentMatrix(segment);
	} else if (client == Rosegarden::GeneralConfigurationPage::EventView) {
	    slotEditSegmentEventList(segment);
	} else {
	    slotEditSegmentNotation(segment);
	}
    }
}


void RosegardenGUIView::slotEditRepeat(Rosegarden::Segment *segment,
				       Rosegarden::timeT time)
{
    SegmentSingleRepeatToCopyCommand *command =
	new SegmentSingleRepeatToCopyCommand(segment, time);
    slotAddCommandToHistory(command);
}


void RosegardenGUIView::slotEditSegmentNotation(Rosegarden::Segment* p)
{
    SetWaitCursor waitCursor;
    std::vector<Rosegarden::Segment *> segmentsToEdit;

    RG_DEBUG << "\n\n\n\nRosegardenGUIView::slotEditSegmentNotation: p is " << p << endl;

    // The logic here is: If we're calling for this operation to
    // happen on a particular segment, then open that segment and if
    // it's part of a selection open all other selected segments too.
    // If we're not calling for any particular segment, then open all
    // selected segments if there are any.

    if (haveSelection()) {

	Rosegarden::SegmentSelection selection = getSelection(); 

	if (!p || (selection.find(p) != selection.end())) {
	    for (Rosegarden::SegmentSelection::iterator i = selection.begin();
		 i != selection.end(); ++i) {
		if ((*i)->getType() != Rosegarden::Segment::Audio) {
		    segmentsToEdit.push_back(*i);
		}
	    }
	} else {
	    if (p->getType() != Rosegarden::Segment::Audio) {
		segmentsToEdit.push_back(p);
	    }
	}

    } else if (p) {
	if (p->getType() != Rosegarden::Segment::Audio) {
	    segmentsToEdit.push_back(p);
	}
    } else {
	return;
    }

    if (segmentsToEdit.empty()) {
	KMessageBox::sorry(this, i18n("No non-audio segments selected"));
	return;
    }

    slotEditSegmentsNotation(segmentsToEdit);
}

void RosegardenGUIView::slotEditSegmentsNotation(std::vector<Rosegarden::Segment *> segmentsToEdit)
{
    NotationView *view = createNotationView(segmentsToEdit);
    if (view) view->show();
}

NotationView *
RosegardenGUIView::createNotationView(std::vector<Rosegarden::Segment *> segmentsToEdit)
{
    NotationView *notationView =
	new NotationView(getDocument(), segmentsToEdit, this, true);

    if (!notationView->isOK()) {
	RG_DEBUG << "slotEditSegmentNotation : operation cancelled" << endl;
	delete notationView;
	return 0;
    }

    // For tempo changes (ugh -- it'd be nicer to make a tempo change
    // command that could interpret all this stuff from the dialog)
    //
    connect(notationView, SIGNAL(changeTempo(Rosegarden::timeT,
					     Rosegarden::tempoT,
					     Rosegarden::tempoT,
					     TempoDialog::TempoDialogAction)),
            RosegardenGUIApp::self(), SLOT(slotChangeTempo(Rosegarden::timeT,
							   Rosegarden::tempoT,
							   Rosegarden::tempoT,
                                                           TempoDialog::TempoDialogAction)));


    connect(notationView, SIGNAL(windowActivated()),
	    this, SLOT(slotActiveMainWindowChanged()));

    connect(notationView, SIGNAL(selectTrack(int)),
            this, SLOT(slotSelectTrackSegments(int)));

    connect(notationView, SIGNAL(play()),
            RosegardenGUIApp::self(), SLOT(slotPlay()));
    connect(notationView, SIGNAL(stop()),
            RosegardenGUIApp::self(), SLOT(slotStop()));
    connect(notationView, SIGNAL(fastForwardPlayback()),
            RosegardenGUIApp::self(), SLOT(slotFastforward()));
    connect(notationView, SIGNAL(rewindPlayback()),
            RosegardenGUIApp::self(), SLOT(slotRewind()));
    connect(notationView, SIGNAL(fastForwardPlaybackToEnd()),
            RosegardenGUIApp::self(), SLOT(slotFastForwardToEnd()));
    connect(notationView, SIGNAL(rewindPlaybackToBeginning()),
            RosegardenGUIApp::self(), SLOT(slotRewindToBeginning()));
    connect(notationView, SIGNAL(panic()),
            RosegardenGUIApp::self(), SLOT(slotPanic()));
            
    connect(notationView, SIGNAL(saveFile()),
            RosegardenGUIApp::self(), SLOT(slotFileSave()));
    connect(notationView, SIGNAL(jumpPlaybackTo(Rosegarden::timeT)),
	    getDocument(), SLOT(slotSetPointerPosition(Rosegarden::timeT)));
    connect(notationView, SIGNAL(openInNotation(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsNotation(std::vector<Rosegarden::Segment *>)));
    connect(notationView, SIGNAL(openInMatrix(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsMatrix(std::vector<Rosegarden::Segment *>)));
    connect(notationView, SIGNAL(openInPercussionMatrix(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsPercussionMatrix(std::vector<Rosegarden::Segment *>)));
    connect(notationView, SIGNAL(openInEventList(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsEventList(std::vector<Rosegarden::Segment *>)));
    connect(notationView, SIGNAL(editTimeSignature(Rosegarden::timeT)),
	    this, SLOT(slotEditTempos(Rosegarden::timeT)));
    connect(notationView, SIGNAL(editMetadata(QString)),
	    this, SLOT(slotEditMetadata(QString)));
    connect(notationView, SIGNAL(editTriggerSegment(int)),
	    this, SLOT(slotEditTriggerSegment(int)));
    connect(notationView, SIGNAL(staffLabelChanged(Rosegarden::TrackId, QString)),
	    this, SLOT(slotChangeTrackLabel(Rosegarden::TrackId, QString)));
    connect(notationView, SIGNAL(toggleSolo(bool)),
            RosegardenGUIApp::self(), SLOT(slotToggleSolo(bool)));

    Rosegarden::SequenceManager *sM = getDocument()->getSequenceManager();

    connect(sM, SIGNAL(insertableNoteOnReceived(int, int)),
	    notationView, SLOT(slotInsertableNoteOnReceived(int, int)));
    connect(sM, SIGNAL(insertableNoteOffReceived(int, int)),
	    notationView, SLOT(slotInsertableNoteOffReceived(int, int)));

    connect(notationView, SIGNAL(stepByStepTargetRequested(QObject *)),
	    this, SIGNAL(stepByStepTargetRequested(QObject *)));
    connect(this, SIGNAL(stepByStepTargetRequested(QObject *)),
	    notationView, SLOT(slotStepByStepTargetRequested(QObject *)));
    connect(RosegardenGUIApp::self(), SIGNAL(compositionStateUpdate()),
	    notationView, SLOT(slotCompositionStateUpdate()));
    connect(this, SIGNAL(compositionStateUpdate()),
	    notationView, SLOT(slotCompositionStateUpdate()));

    // Encourage the notation view window to open to the same
    // interval as the current segment view
    if (m_trackEditor->getSegmentCanvas()->horizontalScrollBar()->value() > 1) { // don't scroll unless we need to
        // first find the time at the center of the visible segment canvas
        int centerX = (int)(m_trackEditor->getSegmentCanvas()->contentsX() +
                            m_trackEditor->getSegmentCanvas()->visibleWidth() / 2);
        timeT centerSegmentView = m_trackEditor->getRulerScale()->getTimeForX(centerX);
        // then scroll the notation view to that time, "localized" for the current segment
        notationView->scrollToTime(centerSegmentView);
        notationView->updateView();
    }

    return notationView;
}

void RosegardenGUIView::slotEditSegmentMatrix(Rosegarden::Segment* p)
{
    SetWaitCursor waitCursor;

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    // unlike notation, if we're calling for this on a particular
    // segment we don't open all the other selected segments as well
    // (fine in notation because they're in a single window)

    if (p) {
	if (p->getType() != Rosegarden::Segment::Audio) {
	    segmentsToEdit.push_back(p);
	}
    } else {
	int count = 0;
	Rosegarden::SegmentSelection selection = getSelection();
	for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	     i != selection.end(); ++i) {
	    if ((*i)->getType() != Rosegarden::Segment::Audio) {
		slotEditSegmentMatrix(*i);
		if (++count == maxEditorsToOpen) break;
	    }
	}
	return;
    }

    if (segmentsToEdit.empty()) {
	KMessageBox::sorry(this, i18n("No non-audio segments selected"));
	return;
    }

    slotEditSegmentsMatrix(segmentsToEdit);
}

void RosegardenGUIView::slotEditSegmentPercussionMatrix(Rosegarden::Segment* p)
{
    SetWaitCursor waitCursor;

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    // unlike notation, if we're calling for this on a particular
    // segment we don't open all the other selected segments as well
    // (fine in notation because they're in a single window)

    if (p) {
	if (p->getType() != Rosegarden::Segment::Audio) {
	    segmentsToEdit.push_back(p);
	}
    } else {
	int count = 0;
	Rosegarden::SegmentSelection selection = getSelection();
	for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	     i != selection.end(); ++i) {
	    if ((*i)->getType() != Rosegarden::Segment::Audio) {
		slotEditSegmentPercussionMatrix(*i);
		if (++count == maxEditorsToOpen) break;
	    }
	}
	return;
    }

    if (segmentsToEdit.empty()) {
	KMessageBox::sorry(this, i18n("No non-audio segments selected"));
	return;
    }

    slotEditSegmentsPercussionMatrix(segmentsToEdit);
}

void RosegardenGUIView::slotEditSegmentsMatrix(std::vector<Rosegarden::Segment *> segmentsToEdit)
{
    int count = 0;
    for (std::vector<Rosegarden::Segment *>::iterator i = segmentsToEdit.begin();
	 i != segmentsToEdit.end(); ++i) {
	std::vector<Rosegarden::Segment *> tmpvec;
	tmpvec.push_back(*i);
	MatrixView *view = createMatrixView(tmpvec, false);
	if (view) {
	    view->show();
	    if (++count == maxEditorsToOpen) break;
	}
    }
}

void RosegardenGUIView::slotEditSegmentsPercussionMatrix(std::vector<Rosegarden::Segment *> segmentsToEdit)
{
    int count = 0;
    for (std::vector<Rosegarden::Segment *>::iterator i = segmentsToEdit.begin();
	 i != segmentsToEdit.end(); ++i) {
	std::vector<Rosegarden::Segment *> tmpvec;
	tmpvec.push_back(*i);
	MatrixView *view = createMatrixView(tmpvec, true);
	if (view) {
	    view->show();
	    if (++count == maxEditorsToOpen) break;
	}
    }
}

MatrixView *
RosegardenGUIView::createMatrixView(std::vector<Rosegarden::Segment *> segmentsToEdit, bool drumMode)
{
    MatrixView *matrixView = new MatrixView(getDocument(),
                                            segmentsToEdit,
                                            this,
					    drumMode);

    // For tempo changes (ugh -- it'd be nicer to make a tempo change
    // command that could interpret all this stuff from the dialog)
    //
    connect(matrixView, SIGNAL(changeTempo(Rosegarden::timeT,
					   Rosegarden::tempoT,
					   Rosegarden::tempoT,
					   TempoDialog::TempoDialogAction)),
		    RosegardenGUIApp::self(), SLOT(slotChangeTempo(Rosegarden::timeT,
								   Rosegarden::tempoT,
								   Rosegarden::tempoT,
								   TempoDialog::TempoDialogAction)));

    connect(matrixView, SIGNAL(windowActivated()),
	    this, SLOT(slotActiveMainWindowChanged()));

    connect(matrixView, SIGNAL(selectTrack(int)),
            this, SLOT(slotSelectTrackSegments(int)));

    connect(matrixView, SIGNAL(play()),
		    RosegardenGUIApp::self(), SLOT(slotPlay()));
    connect(matrixView, SIGNAL(stop()),
		    RosegardenGUIApp::self(), SLOT(slotStop()));
    connect(matrixView, SIGNAL(fastForwardPlayback()),
		    RosegardenGUIApp::self(), SLOT(slotFastforward()));
    connect(matrixView, SIGNAL(rewindPlayback()),
		    RosegardenGUIApp::self(), SLOT(slotRewind()));
    connect(matrixView, SIGNAL(fastForwardPlaybackToEnd()),
		    RosegardenGUIApp::self(), SLOT(slotFastForwardToEnd()));
    connect(matrixView, SIGNAL(rewindPlaybackToBeginning()),
		    RosegardenGUIApp::self(), SLOT(slotRewindToBeginning()));
    connect(matrixView, SIGNAL(panic()),
                    RosegardenGUIApp::self(), SLOT(slotPanic()));

    connect(matrixView, SIGNAL(saveFile()),
		    RosegardenGUIApp::self(), SLOT(slotFileSave()));
    connect(matrixView, SIGNAL(jumpPlaybackTo(Rosegarden::timeT)),
	    getDocument(), SLOT(slotSetPointerPosition(Rosegarden::timeT)));
    connect(matrixView, SIGNAL(openInNotation(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsNotation(std::vector<Rosegarden::Segment *>)));
    connect(matrixView, SIGNAL(openInMatrix(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsMatrix(std::vector<Rosegarden::Segment *>)));
    connect(matrixView, SIGNAL(openInEventList(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsEventList(std::vector<Rosegarden::Segment *>)));
    connect(matrixView, SIGNAL(editTriggerSegment(int)),
	    this, SLOT(slotEditTriggerSegment(int)));
    connect(matrixView, SIGNAL(toggleSolo(bool)),
            RosegardenGUIApp::self(), SLOT(slotToggleSolo(bool)));
    connect(matrixView, SIGNAL(editTimeSignature(Rosegarden::timeT)),
            this, SLOT(slotEditTempos(Rosegarden::timeT)));

    Rosegarden::SequenceManager *sM = getDocument()->getSequenceManager();

    connect(sM, SIGNAL(insertableNoteOnReceived(int, int)),
	    matrixView, SLOT(slotInsertableNoteOnReceived(int, int)));
    connect(sM, SIGNAL(insertableNoteOffReceived(int, int)),
	    matrixView, SLOT(slotInsertableNoteOffReceived(int, int)));

    connect(matrixView, SIGNAL(stepByStepTargetRequested(QObject *)),
	    this, SIGNAL(stepByStepTargetRequested(QObject *)));
    connect(this, SIGNAL(stepByStepTargetRequested(QObject *)),
	    matrixView, SLOT(slotStepByStepTargetRequested(QObject *)));
    connect(RosegardenGUIApp::self(), SIGNAL(compositionStateUpdate()),
	    matrixView, SLOT(slotCompositionStateUpdate()));
    connect(this, SIGNAL(compositionStateUpdate()),
	    matrixView, SLOT(slotCompositionStateUpdate()));
    connect(this,
	    SIGNAL(instrumentLevelsChanged(Rosegarden::InstrumentId,
					   const Rosegarden::LevelInfo &)),
	    matrixView,
	    SLOT(slotInstrumentLevelsChanged(Rosegarden::InstrumentId,
					     const Rosegarden::LevelInfo &)));

    // Encourage the matrix view window to open to the same
    // interval as the current segment view
    if (m_trackEditor->getSegmentCanvas()->horizontalScrollBar()->value() > 1) { // don't scroll unless we need to
        // first find the time at the center of the visible segment canvas
        int centerX = (int)(m_trackEditor->getSegmentCanvas()->contentsX());
        // Seems to work better for matrix view to scroll to left side
        // + m_trackEditor->getSegmentCanvas()->visibleWidth() / 2);
        timeT centerSegmentView = m_trackEditor->getRulerScale()->getTimeForX(centerX);
        // then scroll the notation view to that time, "localized" for the current segment
        matrixView->scrollToTime(centerSegmentView);
        matrixView->updateView();
    }

    return matrixView;
}

void RosegardenGUIView::slotEditSegmentEventList(Rosegarden::Segment *p)
{
    SetWaitCursor waitCursor;

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    // unlike notation, if we're calling for this on a particular
    // segment we don't open all the other selected segments as well
    // (fine in notation because they're in a single window)

    if (p) {
	if (p->getType() != Rosegarden::Segment::Audio) {
	    segmentsToEdit.push_back(p);
	}
    } else {
	int count = 0;
	Rosegarden::SegmentSelection selection = getSelection();
	for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	     i != selection.end(); ++i) {
	    if ((*i)->getType() != Rosegarden::Segment::Audio) {
		slotEditSegmentEventList(*i);
		if (++count == maxEditorsToOpen) break;
	    }
	}
	return;
    }

    if (segmentsToEdit.empty()) {
	KMessageBox::sorry(this, i18n("No non-audio segments selected"));
	return;
    }

    slotEditSegmentsEventList(segmentsToEdit);
}

void RosegardenGUIView::slotEditSegmentsEventList(std::vector<Rosegarden::Segment *> segmentsToEdit)
{
    int count = 0;
    for (std::vector<Rosegarden::Segment *>::iterator i = segmentsToEdit.begin();
	 i != segmentsToEdit.end(); ++i) {
	std::vector<Rosegarden::Segment *> tmpvec;
	tmpvec.push_back(*i);
	EventView *view = createEventView(tmpvec);
	if (view) {
	    view->show();
	    if (++count == maxEditorsToOpen) break;
	}
    }
}

EventView *RosegardenGUIView::createEventView(std::vector<Rosegarden::Segment *> segmentsToEdit)
{
    EventView *eventView = new EventView(getDocument(),
                                         segmentsToEdit,
                                         this);

    connect(eventView, SIGNAL(windowActivated()),
	    this, SLOT(slotActiveMainWindowChanged()));

    connect(eventView, SIGNAL(selectTrack(int)),
            this, SLOT(slotSelectTrackSegments(int)));

    connect(eventView, SIGNAL(saveFile()),
	    RosegardenGUIApp::self(), SLOT(slotFileSave()));

    connect(eventView, SIGNAL(openInNotation(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsNotation(std::vector<Rosegarden::Segment *>)));
    connect(eventView, SIGNAL(openInMatrix(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsMatrix(std::vector<Rosegarden::Segment *>)));
    connect(eventView, SIGNAL(openInPercussionMatrix(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsPercussionMatrix(std::vector<Rosegarden::Segment *>)));
    connect(eventView, SIGNAL(openInEventList(std::vector<Rosegarden::Segment *>)),
	    this, SLOT(slotEditSegmentsEventList(std::vector<Rosegarden::Segment *>)));
    connect(eventView, SIGNAL(editTriggerSegment(int)),
	    this, SLOT(slotEditTriggerSegment(int)));
    connect(this, SIGNAL(compositionStateUpdate()),
	    eventView, SLOT(slotCompositionStateUpdate()));
    connect(RosegardenGUIApp::self(), SIGNAL(compositionStateUpdate()),
	    eventView, SLOT(slotCompositionStateUpdate()));
    connect(eventView, SIGNAL(toggleSolo(bool)),
            RosegardenGUIApp::self(), SLOT(slotToggleSolo(bool)));

    // create keyboard accelerators on view
    //
    RosegardenGUIApp *par = dynamic_cast<RosegardenGUIApp*>(parent());

    if (par) {
        par->plugAccelerators(eventView, eventView->getAccelerators());
    }

    return eventView;
}

void RosegardenGUIView::slotEditTriggerSegment(int id)
{
    SetWaitCursor waitCursor;

    std::vector<Rosegarden::Segment *> segmentsToEdit;

    Rosegarden::Segment *s = getDocument()->getComposition().getTriggerSegment(id);

    if (s) {
	segmentsToEdit.push_back(s);
    } else {
	return;
    }

    slotEditSegmentsEventList(segmentsToEdit);
}

void RosegardenGUIView::slotSegmentAutoSplit(Rosegarden::Segment *segment)
{
    AudioSplitDialog aSD(this, segment, getDocument());

    if (aSD.exec() == QDialog::Accepted)
    {
        KCommand *command =
            new AudioSegmentAutoSplitCommand(getDocument(),
                    segment, aSD.getThreshold());
        slotAddCommandToHistory(command);
    }
}


void RosegardenGUIView::slotEditSegmentAudio(Rosegarden::Segment *segment)
{
    std::cout << "RosegardenGUIView::slotEditSegmentAudio() - "
              << "starting external audio editor" << std::endl;

    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);

    QString application = config->readEntry("externalaudioeditor", "");

    if (application.isEmpty())
    {
        std::cerr << "RosegardenGUIView::slotEditSegmentAudio() - "
                  << "external editor \"" << application.data()
                  << "\" not found" << std::endl;


        KMessageBox::sorry(this, 
                i18n("You've not yet defined an audio editor for Rosegarden to use.\nSee Settings -> Configure Rosegarden -> General -> External Editors."));

        return;
    }

    QFileInfo *appInfo = new QFileInfo(application);
    if (appInfo->exists() == false || appInfo->isExecutable() == false)
    {
        std::cerr << "RosegardenGUIView::slotEditSegmentAudio() - "
                  << "can't execute \"" << application << "\""
                  << std::endl;
        return;
    }

    Rosegarden::AudioFile *aF = getDocument()->getAudioFileManager().
                                    getAudioFile(segment->getAudioFileId());
    if (aF == 0)
    {
        std::cerr << "RosegardenGUIView::slotEditSegmentAudio() - "
                  << "can't find audio file" << std::endl;
        return;
    }


    // wait cursor
    QApplication::setOverrideCursor(QCursor(Qt::waitCursor));

    // Prepare the process
    //
    KProcess *process = new KProcess();
    (*process) << application;
    (*process) << QString(aF->getFilename().c_str());

    // Start it
    //
    if (!process->start())
    {
        std::cerr << "RosegardenGUIView::slotEditSegmentAudio() - "
                  << "can't start external editor" << std::endl;
    }

    // restore cursor
    QApplication::restoreOverrideCursor();

}


void RosegardenGUIView::setZoomSize(double size)
{
    m_rulerScale->setUnitsPerPixel(size);

    double duration44 = Rosegarden::TimeSignature(4,4).getBarDuration();

    double xScale = duration44/(size * barWidth44);
    RG_DEBUG << "RosegardenGUIView::setZoomSize - xScale =  " << xScale << endl;

    m_trackEditor->slotSetPointerPosition
	(getDocument()->getComposition().getPosition());

    m_trackEditor->getSegmentCanvas()->clearSegmentRectsCache(true);
    m_trackEditor->getSegmentCanvas()->slotUpdateSize();
    m_trackEditor->getSegmentCanvas()->slotUpdateSegmentsDrawBuffer();

    if (m_trackEditor->getTempoRuler()) {
	m_trackEditor->getTempoRuler()->repaint();
    }

    if (m_trackEditor->getChordNameRuler()) {
	m_trackEditor->getChordNameRuler()->repaint();
    }

    if (m_trackEditor->getTopBarButtons()) {
	m_trackEditor->getTopBarButtons()->repaint();
    }

    if (m_trackEditor->getBottomBarButtons()) {
	m_trackEditor->getBottomBarButtons()->repaint();
    }
}


// Select a track label and segments (when we load a file or
// move up a track).   Also change record track if the one
// we're moving from is MIDI.
//
//
void RosegardenGUIView::slotSelectTrackSegments(int trackId)
{
    // update the instrument parameter box
    Composition &comp = getDocument()->getComposition();
    Rosegarden::Track *track = comp.getTrackById(trackId);

    if (track == 0) return;

    // Show the selection on the track buttons.  Find the position.
    //
    m_trackEditor->getTrackButtons()->selectLabel(track->getPosition());
    m_trackEditor->slotScrollToTrack(track->getPosition());

    Rosegarden::SegmentSelection segments;

    for (Composition::iterator i =
              getDocument()->getComposition().begin();
         i != getDocument()->getComposition().end(); i++)
    {
        if (((int)(*i)->getTrack()) == trackId)
            segments.insert(*i);
    }

    // Store the selected Track in the Composition
    //
    comp.setSelectedTrack(trackId);

    m_trackParameterBox->slotSelectedTrackChanged();

    slotUpdateInstrumentParameterBox(comp.getTrackById(trackId)->
                                     getInstrument());


    slotSetSelectedSegments(segments);

    // inform
    emit segmentsSelected(segments);
    emit compositionStateUpdate();
}

void RosegardenGUIView::slotSetSelectedSegments(
        const Rosegarden::SegmentSelection &segments)
{
    // Send this signal to the GUI to activate the correct tool
    // on the toolbar so that we have a SegmentSelector object
    // to write the Segments into
    //
    if (segments.size() > 0) emit activateTool(SegmentSelector::ToolName);

    // Send the segment list even if it's empty as we
    // use that to clear any current selection
    //
    m_trackEditor->getSegmentCanvas()->slotSelectSegments(segments);

    // update the segment parameter box
    m_segmentParameterBox->useSegments(segments);

    emit stateChange("have_selection", true);
    if (!segments.hasNonAudioSegment())
        emit stateChange("audio_segment_selected", true);
}

void RosegardenGUIView::slotSelectAllSegments()
{
    Rosegarden::SegmentSelection segments;

    Rosegarden::InstrumentId instrument = 0;
    bool haveInstrument = false;
    bool multipleInstruments = false;

    Composition &comp = getDocument()->getComposition();

    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {

	Rosegarden::InstrumentId myInstrument = 
	    comp.getTrackById((*i)->getTrack())->getInstrument();

	if (haveInstrument) {
	    if (myInstrument != instrument) {
		multipleInstruments = true;
	    }
	} else {
	    instrument = myInstrument;
	    haveInstrument = true;
	}

	segments.insert(*i);
    }

    // Send this signal to the GUI to activate the correct tool
    // on the toolbar so that we have a SegmentSelector object
    // to write the Segments into
    //
    if (segments.size() > 0) emit activateTool(SegmentSelector::ToolName);

    // Send the segment list even if it's empty as we
    // use that to clear any current selection
    //
    m_trackEditor->getSegmentCanvas()->slotSelectSegments(segments);

    // update the segment parameter box
    m_segmentParameterBox->useSegments(segments);

    // update the instrument parameter box
    if (haveInstrument && !multipleInstruments) {
	slotUpdateInstrumentParameterBox(instrument);
    } else {
	m_instrumentParameterBox->useInstrument(0);
    }

    //!!! similarly, how to set no selected track?
    //comp.setSelectedTrack(trackId);

    emit stateChange("have_selection", true);

    if (!segments.hasNonAudioSegment())
        emit stateChange("audio_segment_selected", true);

    // inform
    //!!! inform what? is this signal actually used?
    emit segmentsSelected(segments);
}
    

void RosegardenGUIView::slotUpdateInstrumentParameterBox(int id)
{
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Instrument *instrument = studio.getInstrumentById(id);
    Rosegarden::Composition &comp = getDocument()->getComposition();

    Rosegarden::Track *track = comp.getTrackById(comp.getSelectedTrack());

    // Reset the instrument
    //
    m_instrumentParameterBox->useInstrument(instrument);

    // Then do this instrument/track fiddling
    //
/*
    if (track && instrument &&
            instrument->getType() == Rosegarden::Instrument::Audio)
    {
        // Set the mute status
        m_instrumentParameterBox->setMute(track->isMuted());

        // Set the record track
        m_instrumentParameterBox->setRecord(
                    track->getId() == comp.getRecordTrack());

        // Set solo
        m_instrumentParameterBox->setSolo(
                comp.isSolo() && (track->getId() == comp.getSelectedTrack()));
    }
*/
    emit checkTrackAssignments();
}

// Send a NoteOn or AudioLevel MappedEvent to the track meters
// by Instrument id
//
void RosegardenGUIView::showVisuals(const Rosegarden::MappedEvent *mE)
{
    double valueLeft = ((double)mE->getData1()) / 127.0;
    double valueRight = ((double)mE->getData2()) / 127.0;

    if (mE->getType() == Rosegarden::MappedEvent::AudioLevel)
    {

        // Send to the high sensitivity instrument parameter box 
	// (if any)
        //
        if (m_instrumentParameterBox->getSelectedInstrument() &&
	    mE->getInstrument() ==
	        m_instrumentParameterBox->getSelectedInstrument()->getId())
        {
	    float dBleft = Rosegarden::AudioLevel::fader_to_dB
		(mE->getData1(), 127, Rosegarden::AudioLevel::LongFader);
	    float dBright = Rosegarden::AudioLevel::fader_to_dB
		(mE->getData2(), 127, Rosegarden::AudioLevel::LongFader);

            m_instrumentParameterBox->setAudioMeter(dBleft, dBright,
						    Rosegarden::AudioLevel::DB_FLOOR,
						    Rosegarden::AudioLevel::DB_FLOOR);
        }

        // Don't always send all audio levels so we don't
        // get vu meter flickering on track meters
        //
        if (valueLeft < 0.05 && valueRight < 0.05) return;

    }
    else if (mE->getType() != Rosegarden::MappedEvent::MidiNote)
        return;

    m_trackEditor->getTrackButtons()->
        slotSetMetersByInstrument((valueLeft + valueRight) / 2, 
                                  mE->getInstrument());

}

void
RosegardenGUIView::updateMeters(SequencerMapper *mapper)
{
    const int unknownState = 0, oldState = 1, newState = 2;

    typedef std::map<Rosegarden::InstrumentId, int> StateMap;
    static StateMap states;
    static StateMap recStates;

    typedef std::map<Rosegarden::InstrumentId, Rosegarden::LevelInfo> LevelMap;
    static LevelMap levels;
    static LevelMap recLevels;

    for (StateMap::iterator i = states.begin(); i != states.end(); ++i) {
	i->second = unknownState;
    }
    for (StateMap::iterator i = recStates.begin(); i != recStates.end(); ++i) {
	i->second = unknownState;
    }

    for (Rosegarden::Composition::trackcontainer::iterator i =
	     getDocument()->getComposition().getTracks().begin();
	 i != getDocument()->getComposition().getTracks().end(); ++i) {

	Rosegarden::Track *track = i->second;
	if (!track) continue;

	Rosegarden::InstrumentId instrumentId = track->getInstrument();

	if (states[instrumentId] == unknownState) {
	    bool isNew = mapper->getInstrumentLevel(instrumentId, 
						    levels[instrumentId]);
	    states[instrumentId] = (isNew ? newState : oldState);
	}

	if (recStates[instrumentId] == unknownState) {
	    bool isNew = mapper->getInstrumentRecordLevel(instrumentId,
							  recLevels[instrumentId]);
	    recStates[instrumentId] = (isNew ? newState : oldState);
	}

	if (states[instrumentId] == oldState &&
	    recStates[instrumentId] == oldState) continue;

	Rosegarden::Instrument *instrument =
	    getDocument()->getStudio().getInstrumentById(instrumentId);
	if (!instrument) continue;

	Rosegarden::LevelInfo &info = levels[instrumentId];
	Rosegarden::LevelInfo &recInfo = recLevels[instrumentId];

	if (instrument->getType() == Rosegarden::Instrument::Audio ||
	    instrument->getType() == Rosegarden::Instrument::SoftSynth) {

	    float dBleft = Rosegarden::AudioLevel::DB_FLOOR;
	    float dBright = Rosegarden::AudioLevel::DB_FLOOR;
	    float recDBleft = Rosegarden::AudioLevel::DB_FLOOR;
	    float recDBright = Rosegarden::AudioLevel::DB_FLOOR;

	    bool toSet = false;

	    if (states[instrumentId] == newState &&
		(getDocument()->getSequenceManager()->getTransportStatus()
		 != STOPPED)) {

		if (info.level != 0 || info.levelRight != 0) {
		    dBleft = Rosegarden::AudioLevel::fader_to_dB
			(info.level, 127, Rosegarden::AudioLevel::LongFader);
		    dBright = Rosegarden::AudioLevel::fader_to_dB
			(info.levelRight, 127, Rosegarden::AudioLevel::LongFader);
		}
		toSet = true;
		m_trackEditor->getTrackButtons()->slotSetTrackMeter
		    ((info.level + info.levelRight) / 254.0, track->getPosition());
	    }
	    
	    if (recStates[instrumentId] == newState &&
		instrument->getType() == Rosegarden::Instrument::Audio &&
		(getDocument()->getSequenceManager()->getTransportStatus()
		 != PLAYING)) {

		if (recInfo.level != 0 || recInfo.levelRight != 0) {
		    recDBleft = Rosegarden::AudioLevel::fader_to_dB
			(recInfo.level, 127, Rosegarden::AudioLevel::LongFader);
		    recDBright = Rosegarden::AudioLevel::fader_to_dB
			(recInfo.levelRight, 127, Rosegarden::AudioLevel::LongFader);
		}
		toSet = true;
	    }

	    if (toSet &&
		m_instrumentParameterBox->getSelectedInstrument() &&
		instrument->getId() ==
		m_instrumentParameterBox->getSelectedInstrument()->getId()) {

		m_instrumentParameterBox->setAudioMeter(dBleft, dBright,
							recDBleft, recDBright);
	    }

	} else {

            if (info.level == 0) continue;

	    if (getDocument()->getSequenceManager()->getTransportStatus()
		 != STOPPED) {

		m_trackEditor->getTrackButtons()->slotSetMetersByInstrument
		    (info.level / 127.0, instrumentId);
	    }
	}
    }

    for (StateMap::iterator i = states.begin(); i != states.end(); ++i) {
	if (i->second == newState) {
	    emit instrumentLevelsChanged(i->first, levels[i->first]);
	}
    }
}    

void
RosegardenGUIView::updateMonitorMeters(SequencerMapper *mapper)
{
    Rosegarden::Instrument *instrument =
	m_instrumentParameterBox->getSelectedInstrument();
    if (!instrument ||
	(instrument->getType() != Rosegarden::Instrument::Audio)) return;

    Rosegarden::LevelInfo level;
    if (!mapper->getInstrumentRecordLevel(instrument->getId(), level)) return;

    float dBleft = Rosegarden::AudioLevel::fader_to_dB
	(level.level, 127, Rosegarden::AudioLevel::LongFader);
    float dBright = Rosegarden::AudioLevel::fader_to_dB
	(level.levelRight, 127, Rosegarden::AudioLevel::LongFader);

    m_instrumentParameterBox->setAudioMeter
	(Rosegarden::AudioLevel::DB_FLOOR, Rosegarden::AudioLevel::DB_FLOOR,
	 dBleft, dBright);
}    

void
RosegardenGUIView::slotSelectedSegments(const Rosegarden::SegmentSelection &segments)
{
    // update the segment parameter box
    m_segmentParameterBox->useSegments(segments);

    if (segments.size()) {
        emit stateChange("have_selection", true);
        if (!segments.hasNonAudioSegment())
            emit stateChange("audio_segment_selected", true);
    } else
        emit stateChange("have_selection", false);

    emit segmentsSelected(segments);
}

void RosegardenGUIView::slotShowRulers(bool v)
{
    if (v) {
        m_trackEditor->getTopBarButtons()->getLoopRuler()->show();
        m_trackEditor->getBottomBarButtons()->getLoopRuler()->show();
    } else {
        m_trackEditor->getTopBarButtons()->getLoopRuler()->hide();
        m_trackEditor->getBottomBarButtons()->getLoopRuler()->hide();
    }
}

void RosegardenGUIView::slotShowTempoRuler(bool v)
{
    if (v) {
        m_trackEditor->getTempoRuler()->show();
    } else {
        m_trackEditor->getTempoRuler()->hide();
    }
}

void RosegardenGUIView::slotShowChordNameRuler(bool v)
{
    if (v) {
	m_trackEditor->getChordNameRuler()->setStudio(&getDocument()->getStudio());
        m_trackEditor->getChordNameRuler()->show();
    } else {
        m_trackEditor->getChordNameRuler()->hide();
    }
}

void RosegardenGUIView::slotShowPreviews(bool v)
{
    m_trackEditor->getSegmentCanvas()->setShowPreviews(v);
    m_trackEditor->getSegmentCanvas()->slotUpdateSegmentsDrawBuffer();
}

void RosegardenGUIView::slotShowSegmentLabels(bool v)
{
    m_trackEditor->getSegmentCanvas()->setShowSegmentLabels(v);
    m_trackEditor->getSegmentCanvas()->slotUpdateSegmentsDrawBuffer();
}
	
void RosegardenGUIView::slotAddTracks(unsigned int nbTracks,
                                      Rosegarden::InstrumentId id)
{
    RG_DEBUG << "RosegardenGUIView::slotAddTracks(" << nbTracks << ")" << endl;
    m_trackEditor->slotAddTracks(nbTracks, id);
}

void RosegardenGUIView::slotDeleteTracks(
        std::vector<Rosegarden::TrackId> tracks)
{
    RG_DEBUG << "RosegardenGUIView::slotDeleteTracks - "
             << "deleting " << tracks.size() << " tracks"
             << endl;

    m_trackEditor->slotDeleteTracks(tracks);
}



MultiViewCommandHistory*
RosegardenGUIView::getCommandHistory()
{
    return getDocument()->getCommandHistory();
}

void
RosegardenGUIView::slotAddCommandToHistory(KCommand *command)
{           
    getCommandHistory()->addCommand(command);
}

void
RosegardenGUIView::slotChangeInstrumentLabel(Rosegarden::InstrumentId id,
                                             QString label)
{
    m_trackEditor->getTrackButtons()->changeInstrumentLabel(id, label);
}

void
RosegardenGUIView::slotChangeTrackLabel(Rosegarden::TrackId id,
					QString label)
{
    m_trackEditor->getTrackButtons()->changeTrackLabel(id, label);
}


void 
RosegardenGUIView::slotAddAudioSegment(Rosegarden::AudioFileId audioId,
                                       Rosegarden::TrackId trackId,
                                       Rosegarden::timeT position,
                                       const Rosegarden::RealTime &startTime,
                                       const Rosegarden::RealTime &endTime)
{
    AudioSegmentInsertCommand *command = 
                      new AudioSegmentInsertCommand(getDocument(),
                                                    trackId,
                                                    position,
                                                    audioId,
                                                    startTime,
                                                    endTime);
    slotAddCommandToHistory(command);
}


void
RosegardenGUIView::slotAddAudioSegmentCurrentPosition(Rosegarden::AudioFileId audioFileId,
                                                      const Rosegarden::RealTime &startTime,
                                                      const Rosegarden::RealTime &endTime)
{
    Rosegarden::Composition &comp = getDocument()->getComposition();

    AudioSegmentInsertCommand *command = 
        new AudioSegmentInsertCommand(getDocument(),
                                      comp.getSelectedTrack(),
                                      comp.getPosition(),
                                      audioFileId,
                                      startTime,
                                      endTime);
    slotAddCommandToHistory(command);
}


void
RosegardenGUIView::slotAddAudioSegmentDefaultPosition(Rosegarden::AudioFileId audioFileId,
                                                      const Rosegarden::RealTime &startTime,
                                                      const Rosegarden::RealTime &endTime)
{
    // Add at current track if it's an audio track, otherwise at first
    // empty audio track if there is one, otherwise at first audio track.
    // This behaviour should be of no interest to proficient users (who
    // should have selected the right track already, or be using drag-
    // and-drop) but it should save beginners from inserting an audio
    // segment and being quite unable to work out why it won't play

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();

    Rosegarden::TrackId currentTrackId = comp.getSelectedTrack();
    Rosegarden::Track *track = comp.getTrackById(currentTrackId);

    if (track) {
	Rosegarden::InstrumentId ii = track->getInstrument();
	Rosegarden::Instrument *instrument = studio.getInstrumentById(ii);

	if (instrument &&
	    instrument->getType() == Rosegarden::Instrument::Audio) {
	    slotAddAudioSegment(audioFileId, currentTrackId,
				comp.getPosition(), startTime, endTime);
	    return;
	}
    }

    // current track is not an audio track, find a more suitable one

    Rosegarden::TrackId bestSoFar = currentTrackId;

    for (Rosegarden::Composition::trackcontainer::const_iterator
	     ti = comp.getTracks().begin();
	 ti != comp.getTracks().end(); ++ti) {

	Rosegarden::InstrumentId ii = ti->second->getInstrument();
	Rosegarden::Instrument *instrument = studio.getInstrumentById(ii);

	if (instrument &&
	    instrument->getType() == Rosegarden::Instrument::Audio) {

	    if (bestSoFar == currentTrackId) bestSoFar = ti->first;
	    bool haveSegment = false;

	    for (Rosegarden::Composition::segmentcontainer::const_iterator si =
		     comp.getSegments().begin();
		 si != comp.getSegments().end(); ++si) {
		if ((*si)->getTrack() == ti->first) {
		    // there's a segment on this track
		    haveSegment = true;
		    break;
		}
	    }

	    if (!haveSegment) { // perfect
		slotAddAudioSegment(audioFileId, ti->first,
				    comp.getPosition(), startTime, endTime);
		return;
	    }
	}
    }

    slotAddAudioSegment(audioFileId, bestSoFar,
			comp.getPosition(), startTime, endTime);
    return;
}


void
RosegardenGUIView::slotDroppedNewAudio(QString audioDesc)
{
    QTextIStream s(&audioDesc);

    QString audioFile;
    int trackId;
    Rosegarden::timeT time;
    audioFile = s.readLine();
    s >> trackId;
    s >> time;

    RG_DEBUG << "RosegardenGUIView::slotDroppedNewAudio: audioFile " << audioFile << ", trackId " << trackId << ", time " << time << endl;

    RosegardenGUIApp *app = RosegardenGUIApp::self();
    Rosegarden::AudioFileManager &aFM = getDocument()->getAudioFileManager();
    
    Rosegarden::AudioFileId audioFileId = 0;

    if (app->getAudioManagerDialog()) {

        // Add audio file through manager dialog
        //
        if (app->getAudioManagerDialog()->addAudioFile(audioFile) == false)
            return;

        // get the last added audio file id and insert the segment
        //
        audioFileId = aFM.getLastAudioFile()->getId();

    } else {

        try {
            audioFileId = aFM.addFile(qstrtostr(audioFile));
        } catch(std::string e) {
            QString errorString = i18n("Can't add dropped file. ") + strtoqstr(e);
            KMessageBox::sorry(this, errorString);
            return;
        } catch(QString e) {
            QString errorString = i18n("Can't add dropped file. ") + e;
            KMessageBox::sorry(this, errorString);
            return;
        }

        // add the file at the sequencer
        emit addAudioFile(audioFileId);
    }

    // Now fetch file details
    //
    Rosegarden::AudioFile *aF = aFM.getAudioFile(audioFileId);

    if (aF) {
        slotAddAudioSegment(audioFileId, trackId, time, 
                            Rosegarden::RealTime(0, 0), aF->getLength());

        RG_DEBUG << "RosegardenGUIView::slotDroppedNewAudio("
                 << "filename = " << audioFile 
                 << ", trackid = " << trackId
                 << ", time = " << time << endl;

    }
}

void
RosegardenGUIView::slotDroppedAudio(QString audioDesc)
{
    QTextIStream s(&audioDesc);

    Rosegarden::AudioFileId audioFileId;
    Rosegarden::TrackId trackId;
    Rosegarden::timeT position;
    Rosegarden::RealTime startTime, endTime;

    // read the audio info
    s >> audioFileId;
    s >> trackId;
    s >> position;
    s >> startTime.sec;
    s >> startTime.nsec;
    s >> endTime.sec;
    s >> endTime.nsec;

    RG_DEBUG << "RosegardenGUIView::slotDroppedAudio("
                         //<< audioDesc
                         << ") : audioFileId = " << audioFileId
                         << " - trackId = " << trackId
                         << " - position = " << position
                         << " - startTime.sec = " << startTime.sec
                         << " - startTime.nsec = " << startTime.nsec
                         << " - endTime.sec = " << endTime.sec
                         << " - endTime.nsec = " << endTime.nsec
                         << endl;

    slotAddAudioSegment(audioFileId, trackId, position, startTime, endTime);
}

void
RosegardenGUIView::slotSetMuteButton(Rosegarden::TrackId track, bool value)
{
    RG_DEBUG << "RosegardenGUIView::slotSetMuteButton - track id = " << track
             << ", value = " << value << endl;

    m_trackEditor->getTrackButtons()->setMuteButton(track, value);
    Rosegarden::Track *trackObj = getDocument()->
        getComposition().getTrackById(track);
/*
    // to fix 739544
    if (m_instrumentParameterBox->getSelectedInstrument() &&
        m_instrumentParameterBox->getSelectedInstrument()->getId() ==
        trackObj->getInstrument())
    {
        m_instrumentParameterBox->setMute(value);
    }
*/
    // set the value in the composition
    trackObj->setMuted(value);

    getDocument()->slotDocumentModified(); // set the modification flag

}

void
RosegardenGUIView::slotSetMute(Rosegarden::InstrumentId id, bool value)
{
    RG_DEBUG << "RosegardenGUIView::slotSetMute - "
             << "id = " << id
             << ",value = " << value << endl;

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Composition::trackcontainer &tracks = comp.getTracks();
    Rosegarden::Composition::trackiterator it;

    for (it = tracks.begin(); it != tracks.end(); ++it)
    {
        if ((*it).second->getInstrument() == id)
            slotSetMuteButton((*it).second->getId(), value);
    }

}

void
RosegardenGUIView::slotSetRecord(Rosegarden::InstrumentId id, bool value)
{
    RG_DEBUG << "RosegardenGUIView::slotSetRecord - "
             << "id = " << id
             << ",value = " << value << endl;
/*
    // IPB
    //
    m_instrumentParameterBox->setRecord(value);
*/
    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Composition::trackcontainer &tracks = comp.getTracks();
    Rosegarden::Composition::trackiterator it;
#ifdef NOT_DEFINED
    for (it = tracks.begin(); it != tracks.end(); ++it)
    {
        if (comp.getSelectedTrack() == (*it).second->getId())
        {
//!!! MTR            m_trackEditor->getTrackButtons()->
//                setRecordTrack((*it).second->getPosition());
//!!! MTR is this needed? I think probably not
            slotUpdateInstrumentParameterBox((*it).second->getInstrument());
        }
    }
#endif
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Instrument *instr = studio.getInstrumentById(id);
}

void
RosegardenGUIView::slotSetSolo(Rosegarden::InstrumentId id, bool value)
{
    RG_DEBUG << "RosegardenGUIView::slotSetSolo - "
             << "id = " << id
             << ",value = " << value << endl;

    emit toggleSolo(value);
}

void
RosegardenGUIView::slotUpdateRecordingSegment(Rosegarden::Segment *segment,
					      Rosegarden::timeT )
{
    // We're only interested in this on the first call per recording segment,
    // when we possibly create a view for it
    static Rosegarden::Segment *lastRecordingSegment = 0;

    if (segment == lastRecordingSegment) return;
    lastRecordingSegment = segment;

    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);

    int tracking = config->readUnsignedNumEntry("recordtracking", 0);
    if (tracking != 1) return;

    RG_DEBUG << "RosegardenGUIView::slotUpdateRecordingSegment: segment is " << segment << ", lastRecordingSegment is " << lastRecordingSegment << ", opening a new view" << endl;

    std::vector<Rosegarden::Segment *> segments;
    segments.push_back(segment);

    NotationView *view = createNotationView(segments);
    if (!view) return;

/* signal no longer exists
    QObject::connect
	(getDocument(), SIGNAL(recordingSegmentUpdated(Rosegarden::Segment *, Rosegarden::timeT)),
	 view, SLOT(slotUpdateRecordingSegment(Rosegarden::Segment *, Rosegarden::timeT)));
*/

    view->show();
}

void 
RosegardenGUIView::slotSynchroniseWithComposition()
{
    // Track buttons
    //
    m_trackEditor->getTrackButtons()->slotSynchroniseWithComposition();

    // Update all IPBs
    //
    Composition &comp = getDocument()->getComposition();
    Rosegarden::Track *track = comp.getTrackById(comp.getSelectedTrack());
    slotUpdateInstrumentParameterBox(track->getInstrument());

    m_instrumentParameterBox->slotUpdateAllBoxes();
}

void
RosegardenGUIView::windowActivationChange(bool)
{
    if (isActiveWindow()) {
	slotActiveMainWindowChanged(this);
    }
}

void
RosegardenGUIView::slotActiveMainWindowChanged(const QWidget *w)
{
    m_lastActiveMainWindow = w;
}

void
RosegardenGUIView::slotActiveMainWindowChanged()
{
    const QWidget *w = dynamic_cast<const QWidget *>(sender());
    if (w) slotActiveMainWindowChanged(w);
}

void
RosegardenGUIView::slotControllerDeviceEventReceived(Rosegarden::MappedEvent *e)
{
    RG_DEBUG << "Controller device event received - send to " << (void *)m_lastActiveMainWindow << " (I am " << this << ")" << endl;

    //!!! So, what _should_ we do with these?

    // -- external controller that sends e.g. volume control for each
    // of a number of channels -> if mixer present, use control to adjust
    // tracks on mixer

    // -- external controller that sends e.g. separate controllers on
    // the same channel for adjusting various parameters -> if IPB
    // visible, adjust it.  Should we use the channel to select the
    // track? maybe as an option

    // do we actually need the last active main window for either of
    // these? -- yes, to determine whether to send to mixer or to IPB
    // in the first place.  Send to audio mixer if active, midi mixer
    // if active, plugin dialog if active, otherwise keep it for
    // ourselves for the IPB.  But, we'll do that by having the edit
    // views pass it back to us.

    // -- then we need to send back out to device. 

    //!!! special cases: controller 81 received by any window ->
    // select window 0->main, 1->audio mix, 2->midi mix

    //!!! controller 82 received by main window -> select track

    //!!! these obviously should be configurable

    if (e->getType() == Rosegarden::MappedEvent::MidiController) {

	if (e->getData1() == 81) {

	    // select window
	    int window = e->getData2();

	    if (window < 10) { // me

		show();
		raise();
		setActiveWindow();

	    } else if (window < 20) {

		RosegardenGUIApp::self()->slotOpenAudioMixer();

	    } else if (window < 30) {

		RosegardenGUIApp::self()->slotOpenMidiMixer();
	    }
	}
    }

    emit controllerDeviceEventReceived(e, m_lastActiveMainWindow);
}

void
RosegardenGUIView::slotControllerDeviceEventReceived(Rosegarden::MappedEvent *e, const void *preferredCustomer)
{
    if (preferredCustomer != this) return;
    RG_DEBUG << "RosegardenGUIView::slotControllerDeviceEventReceived: this one's for me" << endl;
    raise();

    RG_DEBUG << "Event is type: " << int(e->getType()) << ", channel " << int(e->getRecordedChannel()) << ", data1 " << int(e->getData1()) << ", data2 " << int(e->getData2()) << endl;

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();

    Rosegarden::TrackId currentTrackId = comp.getSelectedTrack();
    Rosegarden::Track *track = comp.getTrackById(currentTrackId);
    
    // If the event is a control change on channel n, then (if
    // follow-channel is on) switch to the nth track of the same type
    // as the current track -- or the first track of the given
    // channel?, and set the control appropriately.  Any controls in
    // IPB are supported for a MIDI device plus program and bank; only
    // volume and pan are supported for audio/synth devices.
    //!!! complete this

    if (e->getType() != Rosegarden::MappedEvent::MidiController) {

	if (e->getType() == Rosegarden::MappedEvent::MidiProgramChange) {
	    int program = e->getData1();
	    if (!track) return;
	    Rosegarden::InstrumentId ii = track->getInstrument();
	    Rosegarden::Instrument *instrument = studio.getInstrumentById(ii);
	    if (!instrument) return;
	    instrument->setProgramChange(program);
	    emit instrumentParametersChanged(ii);
	}
	return;
    }

    unsigned int channel = e->getRecordedChannel();
    Rosegarden::MidiByte controller = e->getData1();
    Rosegarden::MidiByte value = e->getData2();

    if (controller == 82) { //!!! magic select-track controller
	int tracks = comp.getNbTracks();
	Rosegarden::Track *track = comp.getTrackByPosition(value * tracks / 127);
	if (track) {
	    slotSelectTrackSegments(track->getId());
	}
	return;
    }

    if (!track) return;

    Rosegarden::InstrumentId ii = track->getInstrument();
    Rosegarden::Instrument *instrument = studio.getInstrumentById(ii);

    if (!instrument) return;

    switch (instrument->getType()) {

    case Rosegarden::Instrument::Midi:
    {
	Rosegarden::MidiDevice *md = dynamic_cast<Rosegarden::MidiDevice *>
	    (instrument->getDevice());
	if (!md) {
	    std::cerr << "WARNING: MIDI instrument has no MIDI device in slotControllerDeviceEventReceived" << std::endl;
	    return;
	}

	//!!! we need a central clearing house for these changes,
	// for a proper mvc structure.  reqd for automation post-1.2.
	// in the mean time this duplicates much of
	// MIDIInstrumentParameterPanel::slotControllerChanged etc
	
	switch (controller) {

	case Rosegarden::MIDI_CONTROLLER_VOLUME:
	    RG_DEBUG << "Setting volume for instrument " << instrument->getId() << " to " << value << endl;
	    instrument->setVolume(value);
	    break;

	case Rosegarden::MIDI_CONTROLLER_PAN:
	    RG_DEBUG << "Setting pan for instrument " << instrument->getId() << " to " << value << endl;
	    instrument->setPan(value);
	    break;

	default:
	{
	    Rosegarden::ControlList cl = md->getIPBControlParameters();
	    for (Rosegarden::ControlList::const_iterator i = cl.begin();
		 i != cl.end(); ++i) {
		if ((*i).getControllerValue() == controller) {
		    RG_DEBUG << "Setting controller " << controller << " for instrument " << instrument->getId() << " to " << value << endl;
		    instrument->setControllerValue(controller, value);
		    break;
		}
	    }
	    break;
	}
	}
		     
	break;
    }

    case Rosegarden::Instrument::SoftSynth:
    case Rosegarden::Instrument::Audio: 
	
	switch (controller) {

	case Rosegarden::MIDI_CONTROLLER_VOLUME:
	    RG_DEBUG << "Setting volume for instrument " << instrument->getId() << " to " << value << endl;
	    instrument->setLevel(Rosegarden::AudioLevel::fader_to_dB
				 (value, 127, Rosegarden::AudioLevel::ShortFader));
	    break;

	case Rosegarden::MIDI_CONTROLLER_PAN:
	    RG_DEBUG << "Setting pan for instrument " << instrument->getId() << " to " << value << endl;
	    instrument->setPan(Rosegarden::MidiByte((value / 64.0) * 100.0 + 0.01));
	    break;
	   
	default:
	    break;
	}

	break;
    }

    emit instrumentParametersChanged(instrument->getId());

    //!!! send out updates via MIDI
}

void
RosegardenGUIView::initChordNameRuler()
{
    getTrackEditor()->getChordNameRuler()->setReady();
}


#include "rosegardenguiview.moc"

