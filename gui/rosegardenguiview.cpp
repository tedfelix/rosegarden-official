// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
#include <qabstractlayout.h> 
#include <qlayout.h>
#include <qfileinfo.h>

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
#include "rosestrings.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "trackeditor.h"
#include "segmentcanvas.h"
#include "notationview.h"
#include "matrixview.h"
#include "trackbuttons.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "temporuler.h"
#include "RulerScale.h"
#include "Instrument.h"
#include "Selection.h"
#include "segmentparameterbox.h"
#include "instrumentparameterbox.h"
#include "rosegardenconfigurationpage.h"
#include "eventview.h"
#include "dialogs.h"
#include "sequencemanager.h"

using Rosegarden::SimpleRulerScale;
using Rosegarden::Composition;
using Rosegarden::timeT;


RosegardenGUIView::RosegardenGUIView(bool showTrackLabels,
                                     QWidget *parent,
                                     const char* /*name*/)
    : QVBox(parent),
      m_rulerScale(0),
      m_trackEditor(0)
{
    RosegardenGUIDoc* doc = getDocument();

    // This apparently arbitrary figure is what we think is an
    // appropriate width in pixels for a 4/4 bar.  Beware of making it
    // too narrow, as shorter bars will be proportionally smaller --
    // the visual difference between 2/4 and 4/4 is perhaps greater
    // than it sounds.

    double barWidth44 = 100.0;  // so random, so rare
    double unitsPerPixel =
        Rosegarden::TimeSignature(4, 4).getBarDuration() / barWidth44;

    Composition *comp = &doc->getComposition();
    m_rulerScale = new SimpleRulerScale(comp, 0, unitsPerPixel);

    QHBox *hbox = new QHBox(this);
    QFrame *vbox = new QFrame(hbox);
    QVBoxLayout* vboxLayout = new QVBoxLayout(vbox, 5);

    // Segment and Instrument Parameter Boxes [rwb]
    //
    m_segmentParameterBox = new SegmentParameterBox(this, vbox);
    vboxLayout->addWidget(m_segmentParameterBox);

    m_instrumentParameterBox = new InstrumentParameterBox(getDocument(), vbox);
    vboxLayout->addWidget(m_instrumentParameterBox);
    vboxLayout->addStretch();

    // Construct the trackEditor first so we can then
    // query it for placement information
    //
    m_trackEditor  = new TrackEditor(doc, this,
                                     m_rulerScale, showTrackLabels, hbox);

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

    connect(m_trackEditor,
            SIGNAL(droppedURI(QString)),
            parent,
            SLOT(slotOpenDroppedURL(QString)));


    connect(m_trackEditor,
            SIGNAL(droppedAudio(QString)),
            this,
            SLOT(slotDroppedAudio(QString)));
    

    connect(m_instrumentParameterBox,
            SIGNAL(changeInstrumentLabel(Rosegarden::InstrumentId, QString)),
            this,
            SLOT(slotChangeInstrumentLabel(Rosegarden::InstrumentId, QString)));

    if (doc)
        m_trackEditor->setupSegments();
}


RosegardenGUIView::~RosegardenGUIView()
{
    RG_DEBUG << "~RosegardenGUIView()\n";
    delete m_rulerScale;
}

RosegardenGUIDoc*
RosegardenGUIView::getDocument() const
{
    QWidget *t = parentWidget();
    
    if (!t) {
        RG_DEBUG << "CRITICAL ERROR : RosegardenGUIView::getDocument() : widget parent is 0\n";
        return 0;
    }

    RosegardenGUIApp *theApp = dynamic_cast<RosegardenGUIApp*>(t);
    
    if (!theApp) {
        RG_DEBUG << "CRITICAL ERROR : RosegardenGUIView::getDocument() : widget parent is of the wrong type\n";
        return 0;
    }
    
    return theApp->getDocument();
}

void RosegardenGUIView::print(KPrinter *printer, Composition* p)
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
                                                  printer);
    // For debug
    notationView->show();
    kapp->processEvents();
    // For debug - end

    notationView->print(printer);

    delete notationView;
}

void RosegardenGUIView::selectTool(SegmentCanvas::ToolType tool)
{
    m_trackEditor->getSegmentCanvas()->slotSetTool(tool);
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
		}
	    }
	} else return;
    }

    if (type == Rosegarden::Segment::Audio) {
	slotEditSegmentAudio(segment);
    } else {

	Rosegarden::GeneralConfigurationPage::DoubleClickClient client =
	    (Rosegarden::GeneralConfigurationPage::DoubleClickClient)(kapp->config()->readUnsignedNumEntry
	    ("doubleclickclient",
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


void RosegardenGUIView::slotEditSegmentNotation(Rosegarden::Segment* p)
{
    SetWaitCursor waitCursor;
    std::vector<Rosegarden::Segment *> segmentsToEdit;

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

    // Tell the sequencer to take a big suck of events
    //
    Rosegarden::SequenceManager *sM = getDocument()->getSequenceManager();
    sM->setSequencerSliceSize(Rosegarden::RealTime(5, 0));

    NotationView *notationView =
	new NotationView(getDocument(), segmentsToEdit, this);

    if (!notationView->isOK()) {
        sM->setSequencerSliceSize(Rosegarden::RealTime(0, 0));
	delete notationView;
	return;
    }

    // create keyboard accelerators on view
    //
    RosegardenGUIApp *par = dynamic_cast<RosegardenGUIApp*>(parent());

    // For tempo changes (ugh -- it'd be nicer to make a tempo change
    // command that could interpret all this stuff from the dialog)
    //
    connect(notationView, SIGNAL(changeTempo(Rosegarden::timeT, double,
					     TempoDialog::TempoDialogAction)),
	    par, SLOT(slotChangeTempo(Rosegarden::timeT, double,
				      TempoDialog::TempoDialogAction)));

    connect(notationView, SIGNAL(play()),
	    par, SLOT(slotPlay()));
    connect(notationView, SIGNAL(stop()),
	    par, SLOT(slotStop()));
    connect(notationView, SIGNAL(fastForwardPlayback()),
	    par, SLOT(slotFastforward()));
    connect(notationView, SIGNAL(rewindPlayback()),
	    par, SLOT(slotRewind()));
    connect(notationView, SIGNAL(fastForwardPlaybackToEnd()),
	    par, SLOT(slotFastForwardToEnd()));
    connect(notationView, SIGNAL(rewindPlaybackToBeginning()),
	    par, SLOT(slotRewindToBeginning()));
    connect(notationView, SIGNAL(jumpPlaybackTo(Rosegarden::timeT)),
	    getDocument(), SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    // Encourage the notation view window to open to the same
    // interval as the current segment view
    if (m_trackEditor->getHorizontalScrollBar()->value() > 1) { // don't scroll unless we need to
        // first find the time at the center of the visible segment canvas
        int centerX = (int)(m_trackEditor->getSegmentCanvas()->contentsX() +
                            m_trackEditor->getSegmentCanvas()->visibleWidth() / 2);
        timeT centerSegmentView = m_trackEditor->getRulerScale()->getTimeForX(centerX);
        // then scroll the notation view to that time, "localized" for the current segment
        notationView->scrollToTime(centerSegmentView);
        notationView->updateView();
    }
    notationView->show();

    // reset the slice size
    //
    sM->setSequencerSliceSize(Rosegarden::RealTime(0, 0));
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
	Rosegarden::SegmentSelection selection = getSelection();
	for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	     i != selection.end(); ++i) {
	    slotEditSegmentMatrix(*i);
	}
	return;
    }

    // Tell the sequencer to take a big suck of events
    //
    /*
    Rosegarden::SequenceManager *sM = getDocument()->getSequenceManager();
    sM->setSequencerSliceSize(Rosegarden::RealTime(5, 0));
    */

    MatrixView *matrixView = new MatrixView(getDocument(),
                                            segmentsToEdit,
                                            this);

    // create keyboard accelerators on view
    //
    RosegardenGUIApp *par = dynamic_cast<RosegardenGUIApp*>(parent());

    connect(matrixView, SIGNAL(play()),
	    par, SLOT(slotPlay()));
    connect(matrixView, SIGNAL(stop()),
	    par, SLOT(slotStop()));
    connect(matrixView, SIGNAL(fastForwardPlayback()),
	    par, SLOT(slotFastforward()));
    connect(matrixView, SIGNAL(rewindPlayback()),
	    par, SLOT(slotRewind()));
    connect(matrixView, SIGNAL(fastForwardPlaybackToEnd()),
	    par, SLOT(slotFastForwardToEnd()));
    connect(matrixView, SIGNAL(rewindPlaybackToBeginning()),
	    par, SLOT(slotRewindToBeginning()));
    connect(matrixView, SIGNAL(jumpPlaybackTo(Rosegarden::timeT)),
	    getDocument(), SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    // Encourage the matrix view window to open to the same
    // interval as the current segment view
    if (m_trackEditor->getHorizontalScrollBar()->value() > 1) { // don't scroll unless we need to
        // first find the time at the center of the visible segment canvas
        int centerX = (int)(m_trackEditor->getSegmentCanvas()->contentsX());
        // Seems to work better for matrix view to scroll to left side
        // + m_trackEditor->getSegmentCanvas()->visibleWidth() / 2);
        timeT centerSegmentView = m_trackEditor->getRulerScale()->getTimeForX(centerX);
        // then scroll the notation view to that time, "localized" for the current segment
        matrixView->scrollToTime(centerSegmentView);
        matrixView->updateView();
    }
    matrixView->show();

    // Revert slice size
    //
    //sM->setSequencerSliceSize(Rosegarden::RealTime(0, 0));
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
	Rosegarden::SegmentSelection selection = getSelection();
	for (Rosegarden::SegmentSelection::iterator i = selection.begin();
	     i != selection.end(); ++i) {
	    slotEditSegmentEventList(*i);
	}
	return;
    }

    // Tell the sequencer to take a big suck of events
    //
    /*
    Rosegarden::SequenceManager *sM = getDocument()->getSequenceManager();
    sM->setSequencerSliceSize(Rosegarden::RealTime(2, 0));
    */

    EventView *eventView = new EventView(getDocument(),
                                         segmentsToEdit,
                                         this);

    // create keyboard accelerators on view
    //
    RosegardenGUIApp *par = dynamic_cast<RosegardenGUIApp*>(parent());

    if (par)
        par->plugAccelerators(eventView, eventView->getAccelerators());

    eventView->show();

    // Revert slice size
    //
    //sM->setSequencerSliceSize(Rosegarden::RealTime(0, 0));
}

void RosegardenGUIView::slotSegmentAutoSplit(Rosegarden::Segment *segment)
{
    AudioSplitDialog *aSD = new AudioSplitDialog(this,
                                                 segment,
                                                 getDocument());

    if (aSD->exec() == QDialog::Accepted)
    {
        KCommand *command =
            new AudioSegmentAutoSplitCommand(getDocument(),
                    segment, aSD->getThreshold());
        slotAddCommandToHistory(command);
    }
}


void RosegardenGUIView::slotEditSegmentAudio(Rosegarden::Segment *segment)
{
    std::cout << "RosegardenGUIView::slotEditSegmentAudio() - "
              << "starting external audio editor" << endl;

    KConfig* config = kapp->config();
    config->setGroup("General Options");

    QString application = config->readEntry("externalaudioeditor", "");

    if (application.isEmpty())
    {
        std::cerr << "RosegardenGUIView::slotEditSegmentAudio() - "
                  << "external editor \"" << application.data()
                  << "\" not found" << std::endl;
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

    m_trackEditor->slotReadjustCanvasSize();
    m_trackEditor->slotSetPointerPosition
	(getDocument()->getComposition().getPosition());

    for (Composition::iterator i = 
              getDocument()->getComposition().begin();
         i != getDocument()->getComposition().end(); i++) {
	m_trackEditor->getSegmentCanvas()->updateSegmentItem(*i);
    }

    m_trackEditor->getSegmentCanvas()->slotUpdate();

    if (m_trackEditor->getTempoRuler()) {
	m_trackEditor->getTempoRuler()->repaint();
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
void RosegardenGUIView::selectTrack(int trackId)
{
    m_trackEditor->getTrackButtons()->slotLabelSelected(trackId);
    slotSelectTrackSegments(trackId);

    // Select track for recording if the current one is MIDI
    // and the one we're going to is MIDI.
    //
    Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Track *track = comp.getTrackByIndex(comp.getRecordTrack());

    if (track)
    {
        Rosegarden::Instrument *instr =
            studio.getInstrumentById(track->getInstrument());

        if (instr && instr->getType() == Rosegarden::Instrument::Midi)
        {
            track = comp.getTrackByIndex(trackId);
            instr = studio.getInstrumentById(track->getInstrument());

            if (instr->getType() == Rosegarden::Instrument::Midi)
            {
                comp.setRecordTrack(trackId);
                getTrackEditor()->getTrackButtons()->slotSetRecordTrack(trackId);
            }
        }
    }
}


void RosegardenGUIView::slotSelectTrackSegments(int trackId)
{
    Rosegarden::SegmentSelection segments;

    for (Composition::iterator i =
              getDocument()->getComposition().begin();
         i != getDocument()->getComposition().end(); i++)
    {
        if (((int)(*i)->getTrack()) == trackId)
            segments.insert(*i);
    }

    // update the instrument parameter box
    Composition &comp = getDocument()->getComposition();

    slotUpdateInstrumentParameterBox(comp.getTrackByIndex(trackId)->
                                     getInstrument());

    // Store the selected Track in the Composition
    //
    comp.setSelectedTrack(trackId);

    slotSetSelectedSegments(segments);

    // Select track for recording if the current one is MIDI
    // and the one we're going to is MIDI.
    //
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Track *track = comp.getTrackByIndex(comp.getRecordTrack());

    if (track)
    {
        Rosegarden::Instrument *instr =
            studio.getInstrumentById(track->getInstrument());

        if (instr && instr->getType() == Rosegarden::Instrument::Midi)
        {
            track = comp.getTrackByIndex(trackId);
            instr = studio.getInstrumentById(track->getInstrument());

            if (instr && instr->getType() == Rosegarden::Instrument::Midi)
            {
                comp.setRecordTrack(trackId);
                getTrackEditor()->getTrackButtons()->slotSetRecordTrack(trackId);
            }
        }
    }
    
    // inform
    emit segmentsSelected(segments);
}

void RosegardenGUIView::slotSetSelectedSegments(
        const Rosegarden::SegmentSelection &segments)
{
    // Send this signal to the GUI to activate the correct tool
    // on the toolbar so that we have a SegmentSelector object
    // to write the Segments into
    //
    if (segments.size() > 0) emit activateTool(SegmentCanvas::Selector);

    // Send the segment list even if it's empty as we
    // use that to clear any current selection
    //
    m_trackEditor->getSegmentCanvas()->slotSelectSegments(segments);

    // update the segment parameter box
    m_segmentParameterBox->useSegments(segments);

#ifdef RGKDE3
    emit stateChange("segment_selected", false);
#endif
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
	    comp.getTrackByIndex((*i)->getTrack())->getInstrument();

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
    if (segments.size() > 0) emit activateTool(SegmentCanvas::Selector);

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

#ifdef RGKDE3
    emit stateChange("segment_selected", false);
#endif

    // inform
    emit segmentsSelected(segments);
}
    

void RosegardenGUIView::slotUpdateInstrumentParameterBox(int id)
{
    Rosegarden::Studio &studio = getDocument()->getStudio();
//     Composition &comp = getDocument()->getComposition();

    Rosegarden::Instrument *instrument = studio.getInstrumentById(id);

    m_instrumentParameterBox->useInstrument(instrument);
}


// Show a segment as it records
//
void RosegardenGUIView::showRecordingSegmentItem(Rosegarden::Segment* segment)
{
    m_trackEditor->slotUpdateRecordingSegmentItem(segment);
}

void RosegardenGUIView::deleteRecordingSegmentItem()
{
    m_trackEditor->slotDeleteRecordingSegmentItem();
}


// Send a NoteOn or AudioLevel MappedEvent to the track meters
// by Instrument id
//
void RosegardenGUIView::showVisuals(const Rosegarden::MappedEvent *mE)
{
    double value = ((double)mE->getVelocity()) / 127.0;

    if (mE->getType() == Rosegarden::MappedEvent::AudioLevel)
    {
        // Don't always send all audio levels so we don't
        // get vu meter flickering
        //
        if (value < 0.05) return;

    }
    else if (mE->getType() != Rosegarden::MappedEvent::MidiNote)
        return;

    m_trackEditor->getTrackButtons()->
        slotSetMetersByInstrument(value, mE->getInstrument());
}


void
RosegardenGUIView::setControl(const bool &value)
{
    m_trackEditor->slotSetSelectCopy(value);
}

void
RosegardenGUIView::setShift(const bool &value)
{
    m_trackEditor->slotSetSelectAdd(value);
    m_trackEditor->getTopBarButtons()->getLoopRuler()->
	slotSetLoopingMode(value);
    m_trackEditor->getBottomBarButtons()->getLoopRuler()->
	slotSetLoopingMode(value);
    m_trackEditor->slotSetFineGrain(value);
} 

void
RosegardenGUIView::slotSelectedSegments(const Rosegarden::SegmentSelection &segments)
{
    // update the segment parameter box
    m_segmentParameterBox->useSegments(segments);
    emit segmentsSelected(segments);
}

void RosegardenGUIView::slotShowSegmentParameters(bool v)
{
    if (v)
        m_segmentParameterBox->show();
    else
        m_segmentParameterBox->hide();
}

void RosegardenGUIView::slotShowInstrumentParameters(bool v)
{
    if (v)
        m_instrumentParameterBox->show();
    else
        m_instrumentParameterBox->hide();
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

void RosegardenGUIView::slotShowPreviews(bool v)
{
    m_trackEditor->getSegmentCanvas()->setShowPreviews(v);
    m_trackEditor->getSegmentCanvas()->repaint();
}

void RosegardenGUIView::slotAddTracks(unsigned int nbTracks,
                                      Rosegarden::InstrumentId id)
{
    RG_DEBUG << "RosegardenGUIView::slotAddTracks(" << nbTracks << ")\n";
    m_trackEditor->slotAddTracks(nbTracks, id);
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
RosegardenGUIView::slotAddAudioSegmentAndTrack(
                                       Rosegarden::AudioFileId audioFileId,
                                       Rosegarden::InstrumentId instrumentId,
                                       const Rosegarden::RealTime &startTime,
                                       const Rosegarden::RealTime &endTime)
{
    KMacroCommand *macro = new KMacroCommand("Insert Audio Segment");

    Rosegarden::Composition &comp = getDocument()->getComposition();
    macro->addCommand(new AddTracksCommand(&comp,
                                           1,
                                           instrumentId));

    // Note, the new track _will be_ comp.getNbTracks
    //
    macro->addCommand(new AudioSegmentInsertCommand(getDocument(),
                                                    comp.getNbTracks(),
                                                    0,
                                                    audioFileId,
                                                    startTime,
                                                    endTime));
    slotAddCommandToHistory(macro);
}

void
RosegardenGUIView::slotDroppedAudio(QString audioDesc)
{
    QTextIStream s(&audioDesc);

    Rosegarden::AudioFileId audioFileId;
    Rosegarden::InstrumentId instrumentId;
    Rosegarden::RealTime startTime, endTime;

    // read the audio info
    s >> audioFileId;
    s >> instrumentId;
    s >> startTime.sec;
    s >> startTime.usec;
    s >> endTime.sec;
    s >> endTime.usec;

    RG_DEBUG << "RosegardenGUIView::slotDroppedAudio("
                         << audioDesc
                         << ") : audioFileId = " << audioFileId
                         << " - instrumentId = " << instrumentId
                         << " - startTime.sec = " << startTime.sec
                         << " - startTime.usec = " << startTime.usec
                         << " - endTime.sec = " << endTime.sec
                         << " - endTime.usec = " << endTime.usec
                         << endl;

    if (instrumentId != 0)
        slotAddAudioSegmentAndTrack(audioFileId, instrumentId,
                                    startTime, endTime);
    else
        RG_DEBUG << "instrument id == 0\n";
}
