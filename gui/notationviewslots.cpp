
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

#include <qlabel.h>
#include <qinputdialog.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>
#include <kapp.h>
#include <kconfig.h>

#include "notationview.h"

#include "NotationTypes.h"
#include "Selection.h"
#include "Segment.h"
#include "Clipboard.h"
#include "CompositionTimeSliceAdapter.h"
#include "AnalysisTypes.h"

#include "rosegardenguidoc.h"
#include "notationtool.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "segmentcommands.h"
#include "dialogs.h"
#include "chordnameruler.h"
#include "temporuler.h"
#include "notationhlayout.h"

#include "ktmpstatusmsg.h"

using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::EventSelection;


void
NotationView::slotDocumentDestroyed()
{
    NOTATION_DEBUG << "NotationView::slotDocumentDestroyed()\n";
    m_documentDestroyed = true;
    m_inhibitRefresh = true;
}

void
NotationView::slotUpdateInsertModeStatus()
{
    QString message;
    if (isInChordMode()) {
	if (isInTripletMode()) {
	    message = i18n(" Triplet Chord");
	} else {
	    message = i18n(" Chord");
	}
    } else {
	if (isInTripletMode()) {
	    message = i18n(" Triplet");
	} else {
	    message = "";
	}
    }
    m_insertModeLabel->setText(message);
}

void
NotationView::slotChangeSpacingFromIndex(int n)
{
    std::vector<int> spacings = m_hlayout->getAvailableSpacings();
    if (n >= (int)spacings.size()) n = spacings.size() - 1;
    slotChangeSpacing(spacings[n]);
}

void
NotationView::slotChangeSpacingFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(8) == "spacing_") {
	int spacing = name.right(name.length() - 8).toInt();

	if (spacing > 0) slotChangeSpacing(spacing);

    } else {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown spacing action %1").arg(name)));
    }
}

void
NotationView::slotChangeSpacing(int spacing)
{
    if (m_hlayout->getSpacing() == spacing) return;

    m_hlayout->setSpacing(spacing);
    
    m_spacingSlider->setSize(spacing);

    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action(QString("spacing_%1").arg(spacing)));
    if (action) action->setChecked(true);
    else {
	NOTATION_DEBUG
	    << "WARNING: Expected action \"spacing_" << spacing
	    << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
	    << endl;
    }

    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->positionAllElements();
    }

    updateView();
}

void NotationView::slotChangeLegatoFromIndex(int n)
{
    if (n >= (int)m_legatoDurations.size())
        n = m_legatoDurations.size() - 1;
    slotChangeLegato(m_legatoDurations[n]);
}

void
NotationView::slotChangeLegatoFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(10) == "smoothing_") {
	int smoothing = name.right(name.length() - 10).toInt();
	if (smoothing > 0) slotChangeLegato(smoothing);
    } else {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown smoothing action %1").arg(name)));
    }
}

void NotationView::slotChangeLegato(timeT duration)
{
    if (m_legatoQuantizer->getUnit() == duration) return;

    Rosegarden::Quantizer q(Rosegarden::Quantizer::RawEventData,
			    getViewLocalPropertyPrefix() + "Q",
			    Rosegarden::Quantizer::LegatoQuantize,
			    duration);
    *m_legatoQuantizer = q;
    
    m_smoothingSlider->setSize(duration);

    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action(QString("smoothing_%1").arg(duration)));
    if (action) action->setChecked(true);
    else {
	NOTATION_DEBUG
	    << "WARNING: Expected action \"smoothing_" << duration
	    << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
	    << endl;
    }
    
    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->renderAllElements();
        m_staffs[i]->positionAllElements();
    }

    updateView();
}


void
NotationView::slotChangeFontFromAction()
{
    const QObject *s = sender();
    QString name = s->name();
    if (name.left(10) == "note_font_") {
	name = name.right(name.length() - 10);
	slotChangeFont(name);
    } else {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown font action %1").arg(name)));
    }
}

void
NotationView::slotChangeFontSizeFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(15) == "note_font_size_") {
	name = name.right(name.length() - 15);
	bool ok = false;
	int size = name.toInt(&ok);
	if (ok) slotChangeFont(m_fontName, size);
	else {
	    KMessageBox::sorry
		(this, QString(i18n("Unknown font size %1").arg(name)));
	}	    
    } else {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown font size action %1").arg(name)));
    }
}


void
NotationView::slotChangeFont(const QString &newName)
{
    NOTATION_DEBUG << "changeFont: " << newName << endl;
    slotChangeFont(std::string(newName.utf8()));
}


void
NotationView::slotChangeFont(std::string newName)
{
    slotChangeFont(newName, NotePixmapFactory::getDefaultSize(newName));
}


void
NotationView::slotChangeFontSize(int newSize)
{
    slotChangeFont(m_fontName, newSize);
}


void
NotationView::slotChangeFontSizeFromIndex(int n)
{
    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    if (n >= (int)sizes.size()) n = sizes.size()-1;
    slotChangeFont(m_fontName, sizes[n]);
}


void
NotationView::slotChangeFont(std::string newName, int newSize)
{
    if (newName == m_fontName && newSize == m_fontSize) return;

    NotePixmapFactory* npf = 0;
    
    try {
        npf = new NotePixmapFactory(newName, newSize);
    } catch (...) {
        return;
    }

//!!! not used?    bool changedFont = (newName != m_fontName);

    std::string oldName = m_fontName;
    m_fontName = newName;
    m_fontSize = newSize;
    setNotePixmapFactory(npf);

    // update the various GUI elements

    std::set<std::string> fs(NotePixmapFactory::getAvailableFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (unsigned int i = 0; i < f.size(); ++i) {
	bool thisOne = (f[i] == m_fontName);
	if (thisOne) m_fontCombo->setCurrentItem(i);
	KToggleAction *action = dynamic_cast<KToggleAction *>
	    (actionCollection()->action("note_font_" + strtoqstr(f[i])));
	if (action) action->setChecked(thisOne);
	else {
	    NOTATION_DEBUG
		<< "WARNING: Expected action \"note_font_" << f[i]
		<< "\" to be a KToggleAction, but it isn't (or doesn't exist)"
		<< endl;
	}
    }

    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    m_fontSizeSlider->reinitialise(sizes, m_fontSize);
    setupFontSizeMenu(oldName);

    m_hlayout->setNotePixmapFactory(m_notePixmapFactory);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->changeFont(m_fontName, m_fontSize);
    }

    positionStaffs();

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->renderAllElements();
            m_staffs[i]->positionAllElements();
        }
    }

    updateView();
}

//
// Cut, Copy, Paste
//
void NotationView::slotEditCut()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
				       m_document->getClipboard()));
}

void NotationView::slotEditDelete()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Deleting selection..."), this);

    addCommandToHistory(new EraseCommand(*m_currentEventSelection));
}

void NotationView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), this);

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
					m_document->getClipboard()));
}

void NotationView::slotEditCutAndClose()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutAndCloseCommand(*m_currentEventSelection,
					       m_document->getClipboard()));
}

#define RESTRICTED_PASTE_FAILED_DESCRIPTION \
		      "The Restricted paste type requires enough empty\n" \
		      "space (containing only rests) at the paste position\n" \
		      "to hold all of the events to be pasted.\n" \
		      "Not enough space was found.\n" \
		      "If you want to paste anyway, consider using one of\n" \
		      "the other paste types from the \"Paste...\" option\n" \
		      "on the Edit menu.  You can also change the default\n" \
		      "paste type to something other than Restricted if\n" \
		      "you wish."

void NotationView::slotEditPaste()
{
    Rosegarden::Clipboard *clipboard = m_document->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }
    if (!clipboard->isSingleSegment()) {
        slotStatusHelpMsg(i18n("Can't paste multiple Segments into one"));
        return;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    
    // Paste at cursor position
    //
    timeT insertionTime = getInsertionTime();
    timeT endTime = insertionTime +
	(clipboard->getSingleSegment()->getEndTime() - 
	 clipboard->getSingleSegment()->getStartTime());

    KConfig *config = kapp->config();
    config->setGroup("Notation Options");
    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
	config->readUnsignedNumEntry("pastetype",
				     PasteEventsCommand::Restricted);

    PasteEventsCommand *command = new PasteEventsCommand
	(segment, clipboard, insertionTime, defaultType);

    if (!command->isPossible()) {
	KMessageBox::detailedError
	    (this,
	     i18n("Couldn't paste at this point."),
	     i18n(RESTRICTED_PASTE_FAILED_DESCRIPTION));
    } else {
	addCommandToHistory(command);
	//!!! well, we really just want to select the events
	// we just pasted
	setCurrentSelection(new EventSelection
			    (segment, insertionTime, endTime));
    }
}

void NotationView::slotEditGeneralPaste()
{
    Rosegarden::Clipboard *clipboard = m_document->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();

    KConfig *config = kapp->config();
    config->setGroup("Notation Options");
    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
	config->readUnsignedNumEntry("pastetype",
				     PasteEventsCommand::Restricted);
    
    PasteNotationDialog *dialog =
	new PasteNotationDialog(this, defaultType);

    if (dialog->exec() == QDialog::Accepted) {

	PasteEventsCommand::PasteType type = dialog->getPasteType();
	if (dialog->setAsDefault()) {
	    config->writeEntry("pastetype", type);
	}

	timeT insertionTime = getInsertionTime();
	timeT endTime = insertionTime +
	    (clipboard->getSingleSegment()->getEndTime() - 
	     clipboard->getSingleSegment()->getStartTime());

	PasteEventsCommand *command = new PasteEventsCommand
	    (segment, clipboard, insertionTime, type);

	if (!command->isPossible()) {
	    KMessageBox::detailedError
		(this,
		 i18n("Couldn't paste at this point."),
		 i18n(RESTRICTED_PASTE_FAILED_DESCRIPTION));
	} else {
	    addCommandToHistory(command);
	    setCurrentSelection(new EventSelection
				(segment, insertionTime, endTime));
	}
    }
}

void NotationView::slotExtendSelectionBackward()
{
    slotExtendSelectionBackward(false);
}

void NotationView::slotExtendSelectionBackwardBar()
{
    slotExtendSelectionBackward(true);
}

void NotationView::slotExtendSelectionBackward(bool bar)
{
    // If there is no current selection, or the selection is entirely
    // to the right of the cursor, move the cursor left and add to the
    // selection

    timeT oldTime = getInsertionTime();
    if (bar) slotJumpBackward();
    else slotStepBackward();
    timeT newTime = getInsertionTime();

    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    EventSelection *es = new EventSelection(segment);
    if (m_currentEventSelection) es->addFromSelection(m_currentEventSelection);

    if (!m_currentEventSelection ||
	&m_currentEventSelection->getSegment() != &segment ||
	m_currentEventSelection->getSegmentEvents().size() == 0 ||
	m_currentEventSelection->getStartTime() >= oldTime) {

	Segment::iterator extendFrom = segment.findTime(oldTime);

	while (extendFrom != segment.begin() &&
	       (*--extendFrom)->getAbsoluteTime() >= newTime) {
	    es->addEvent(*extendFrom);
	}

    } else { // remove an event

	EventSelection::eventcontainer::iterator i =
	    es->getSegmentEvents().end();

	std::vector<Event *> toErase;

	while (i != es->getSegmentEvents().begin() &&
	       (*--i)->getAbsoluteTime() >= newTime) {
	    toErase.push_back(*i);
	}

	for (unsigned int j = 0; j < toErase.size(); ++j) {
	    es->removeEvent(toErase[j]);
	}
    }
    
    setCurrentSelection(es);
}

void NotationView::slotExtendSelectionForward()
{
    slotExtendSelectionForward(false);
}

void NotationView::slotExtendSelectionForwardBar()
{
    slotExtendSelectionForward(true);
}

void NotationView::slotExtendSelectionForward(bool bar)
{
    // If there is no current selection, or the selection is entirely
    // to the left of the cursor, move the cursor right and add to the
    // selection

    timeT oldTime = getInsertionTime();
    if (bar) slotJumpForward();
    else slotStepForward();
    timeT newTime = getInsertionTime();

    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    EventSelection *es = new EventSelection(segment);
    if (m_currentEventSelection) es->addFromSelection(m_currentEventSelection);

    if (!m_currentEventSelection ||
	&m_currentEventSelection->getSegment() != &segment ||
	m_currentEventSelection->getSegmentEvents().size() == 0 ||
	m_currentEventSelection->getEndTime() <= oldTime) {

	Segment::iterator extendFrom = segment.findTime(oldTime);

	while (extendFrom != segment.end() &&
	       (*extendFrom)->getAbsoluteTime() < newTime) {
	    es->addEvent(*extendFrom);
	    ++extendFrom;
	}

    } else { // remove an event

	EventSelection::eventcontainer::iterator i =
	    es->getSegmentEvents().begin();

	std::vector<Event *> toErase;

	while (i != es->getSegmentEvents().end() &&
	       (*i)->getAbsoluteTime() < newTime) {
	    toErase.push_back(*i);
	    ++i;
	}

	for (unsigned int j = 0; j < toErase.size(); ++j) {
	    es->removeEvent(toErase[j]);
	}
    }
    
    setCurrentSelection(es);
}


void NotationView::slotPreviewSelection()
{
    if (!m_currentEventSelection) return;

    m_document->slotSetLoop(m_currentEventSelection->getStartTime(),
			    m_currentEventSelection->getEndTime());
}


void NotationView::slotClearLoop()
{
    m_document->slotSetLoop(0, 0);
}


void NotationView::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    NotationSelector *selector = dynamic_cast<NotationSelector *>(m_tool);
    
    if (!selector) {
	slotSelectSelected();
    } else {
	setCurrentSelection(0);
    }
}

void NotationView::slotEditSelectFromStart()
{
    timeT t = getInsertionTime();
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
					   segment.getStartTime(),
					   t));
}

void NotationView::slotEditSelectToEnd()
{
    timeT t = getInsertionTime();
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
					   t,
					   segment.getEndTime()));
}

void NotationView::slotEditSelectWholeStaff()
{
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
					   segment.getStartTime(),
					   segment.getEndTime()));
}

void NotationView::slotToggleNotesToolBar()
{
    toggleNamedToolBar("notesToolBar");
}

void NotationView::slotToggleRestsToolBar()
{
    toggleNamedToolBar("restsToolBar");
}

void NotationView::slotToggleAccidentalsToolBar()
{
    toggleNamedToolBar("accidentalsToolBar");
}

void NotationView::slotToggleClefsToolBar()
{
    toggleNamedToolBar("clefsToolBar");
}

void NotationView::slotToggleFontToolBar()
{
    toggleNamedToolBar("fontToolBar");
}

void NotationView::slotToggleTransportToolBar()
{
    toggleNamedToolBar("transportToolBar");
}

void NotationView::toggleNamedToolBar(const QString& toolBarName, bool* force)
{
    KToolBar *namedToolBar = toolBar(toolBarName);

    if (!namedToolBar) {
        NOTATION_DEBUG << "NotationView::toggleNamedToolBar() : toolBar "
                             << toolBarName << " not found" << endl;
        return;
    }

    if (!force) {
    
        if (namedToolBar->isVisible())
            namedToolBar->hide();
        else
            namedToolBar->show();
    } else {

        if (*force)
            namedToolBar->show();
        else
            namedToolBar->hide();
    }

}

//
// Group stuff
//

void NotationView::slotGroupBeam()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Beaming group..."), this);

    addCommandToHistory(new GroupMenuBeamCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupAutoBeam()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Auto-beaming selection..."), this);

    addCommandToHistory(new GroupMenuAutoBeamCommand
                        (*m_currentEventSelection, m_legatoQuantizer));
}

void NotationView::slotGroupBreak()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Breaking groups..."), this);

    addCommandToHistory(new GroupMenuBreakCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupSimpleTuplet()
{
    slotGroupTuplet(true);
}

void NotationView::slotGroupGeneralTuplet()
{
    slotGroupTuplet(false);
}

void NotationView::slotGroupTuplet(bool simple)
{
    timeT t = 0;
    timeT unit = 0;
    int tupled = 2;
    int untupled = 3;
    Segment *segment = 0;

/*!!! nah -- selection rectangle is never visible now
    NotationSelector *selector = dynamic_cast<NotationSelector *>
	(m_toolBox->getTool(NotationSelector::ToolName));
*/
    if (m_currentEventSelection /*!!! &&
  selector && selector->isRectangleVisible() */ ) {

	t = m_currentEventSelection->getStartTime();

	timeT duration = m_currentEventSelection->getTotalDuration();
	Rosegarden::Note::Type unitType =
	    Rosegarden::Note::getNearestNote(duration / 3, 0).getNoteType();
	unit = Rosegarden::Note(unitType).getDuration();

	if (!simple) {
	    TupletDialog *dialog = new TupletDialog(this, unitType, duration);
	    if (dialog->exec() != QDialog::Accepted) return;
	    unit = Rosegarden::Note(dialog->getUnitType()).getDuration();
	    tupled = dialog->getTupledCount();
	    untupled = dialog->getUntupledCount();
	}

	segment = &m_currentEventSelection->getSegment();

    } else {

	t = getInsertionTime();

	NoteInserter *currentInserter = dynamic_cast<NoteInserter *>
	    (m_toolBox->getTool(NoteInserter::ToolName));

	Rosegarden::Note::Type unitType;

	if (currentInserter) {
	    unitType = currentInserter->getCurrentNote().getNoteType();
	} else {
	    unitType = Rosegarden::Note::Quaver;
	}

	unit = Rosegarden::Note(unitType).getDuration();

	if (!simple) {
	    TupletDialog *dialog = new TupletDialog(this, unitType);
	    if (dialog->exec() != QDialog::Accepted) return;
	    unit = Rosegarden::Note(dialog->getUnitType()).getDuration();
	    tupled = dialog->getTupledCount();
	    untupled = dialog->getUntupledCount();
	}

	segment = &m_staffs[m_currentStaff]->getSegment();
    }

    addCommandToHistory(new GroupMenuTupletCommand
			(*segment, t, unit, untupled, tupled));
}

void NotationView::slotGroupGrace()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making grace notes..."), this);

    addCommandToHistory(new GroupMenuGraceCommand(*m_currentEventSelection));
}

void NotationView::slotGroupUnGrace()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making non-grace notes..."), this);

    addCommandToHistory(new GroupMenuUnGraceCommand(*m_currentEventSelection));
}


//
// indications stuff
//

void NotationView::slotGroupSlur()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Adding slur..."), this);

    GroupMenuAddIndicationCommand *command =
        new GroupMenuAddIndicationCommand(Rosegarden::Indication::Slur,
                                          *m_currentEventSelection);
    
    addCommandToHistory(command);

    setSingleSelectedEvent(m_currentEventSelection->getSegment(),
                           command->getLastInsertedEvent());
} 
  
void NotationView::slotGroupCrescendo()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Adding crescendo..."), this);

    GroupMenuAddIndicationCommand *command =
        new GroupMenuAddIndicationCommand(Rosegarden::Indication::Crescendo,
                                          *m_currentEventSelection);
    
    addCommandToHistory(command);

    setSingleSelectedEvent(m_currentEventSelection->getSegment(),
                           command->getLastInsertedEvent());
} 
  
void NotationView::slotGroupDecrescendo()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Adding decrescendo..."), this);

    GroupMenuAddIndicationCommand *command =
        new GroupMenuAddIndicationCommand(Rosegarden::Indication::Decrescendo,
                                          *m_currentEventSelection);
    
    addCommandToHistory(command);

    setSingleSelectedEvent(m_currentEventSelection->getSegment(),
                           command->getLastInsertedEvent());
} 
  
 
// 
// transforms stuff
//
 
void NotationView::slotTransformsNormalizeRests()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Normalizing rests..."), this);

    addCommandToHistory(new TransformsMenuNormalizeRestsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseRests()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing rests..."), this);

    addCommandToHistory(new TransformsMenuCollapseRestsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing notes..."), this);

    addCommandToHistory(new TransformsMenuCollapseNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsTieNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Tying notes..."), this);

    addCommandToHistory(new TransformsMenuTieNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsUntieNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Untying notes..."), this);

    addCommandToHistory(new TransformsMenuUntieNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsMakeNotesViable()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making notes viable..."), this);

    addCommandToHistory(new TransformsMenuMakeNotesViableCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsDeCounterpoint()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Removing counterpoint..."), this);

    addCommandToHistory(new TransformsMenuDeCounterpointCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsStemsUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pointing stems up..."), this);

    addCommandToHistory(new TransformsMenuChangeStemsCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotTransformsStemsDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pointing stems down..."), this);

    addCommandToHistory(new TransformsMenuChangeStemsCommand
                        (false, *m_currentEventSelection));

}

void NotationView::slotTransformsRestoreStems()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Restoring computed stem directions..."), this);

    addCommandToHistory(new TransformsMenuRestoreStemsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotSetStyleFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (!m_currentEventSelection) return;

    if (name.left(6) == "style_") {
	name = name.right(name.length() - 6);

	KTmpStatusMsg msg(i18n("Changing to %1 style...").arg(name),
			  this);

	addCommandToHistory(new TransformsMenuChangeStyleCommand
			    (NoteStyleName(name),
			     *m_currentEventSelection));
    } else {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown style action %1").arg(name)));
    }
}

void NotationView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    Segment &segment = m_staffs[m_currentStaff]->getSegment();

    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
	KMessageBox::sorry(this, i18n("No note duration selected"));
	return;
    }

    if (name.left(7) == "insert_") {
	name = name.right(name.length()-7);
	
	KTmpStatusMsg msg(i18n("Inserting note"), this);
	
	int modify = 0;
	int octave = 0;

	if (name.right(5) == "_high") {
	    
	    octave = 1;
	    name = name.left(name.length()-5);
	
	} else if (name.right(4) == "_low") {

	    octave = -1;
	    name = name.left(name.length()-4);
	}

	if (name.right(6) == "_sharp") {

	    modify = 1;
	    name = name.left(name.length()-6);

	} else if (name.right(5) == "_flat") {

	    modify = -1;
	    name = name.left(name.length()-5);
	}

	int scalePitch = name.toInt();
	
	if (scalePitch < 0 || scalePitch > 7) {
	    NOTATION_DEBUG << "NotationView::slotInsertNoteFromAction: pitch "
			   << scalePitch << " out of range, using 0" <<endl;
	    scalePitch = 0;
	}

	Event *clefEvt = 0, *keyEvt = 0;
	timeT time = getInsertionTime(clefEvt, keyEvt);

	Rosegarden::Key key;
	if (keyEvt) key = Rosegarden::Key(*keyEvt);

	Rosegarden::Clef clef;
	if (clefEvt) clef = Rosegarden::Clef(*clefEvt);

	int pitch =
	    key.getTonicPitch() + 60 + 12*(octave + clef.getOctave()) + modify;

	static int scale[] = { 0, 2, 4, 5, 7, 9, 11 };
	pitch += scale[scalePitch];
	
	NOTATION_DEBUG << "Inserting note at pitch " << pitch << " with modifier " << modify << endl;

	noteInserter->insertNote(segment, time, pitch,
				 Rosegarden::Accidentals::NoAccidental);

    } else {
	
	KMessageBox::sorry
	    (this, QString(i18n("Unknown note insert action %1").arg(name)));
    }
}

void NotationView::slotInsertRest()
{
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    timeT time = getInsertionTime();

    RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);
    if (!restInserter) {
	KMessageBox::sorry(this, i18n("No rest duration selected"));
	return;
    }

    restInserter->insertNote(segment, time,
			     0, Rosegarden::Accidentals::NoAccidental);
}

void NotationView::slotSwitchFromRestToNote()
{
    RestInserter *restInserter = dynamic_cast<RestInserter *>(m_tool);
    if (!restInserter) {
	KMessageBox::sorry(this, i18n("No rest duration selected"));
	return;
    }

    Rosegarden::Note note(restInserter->getCurrentNote());
    
    NoteInserter *noteInserter = dynamic_cast<NoteInserter*>
	(m_toolBox->getTool(NoteInserter::ToolName));

    noteInserter->slotSetNote(note.getNoteType());
    noteInserter->slotSetDots(note.getDots());

    setTool(noteInserter);
}

void NotationView::slotSwitchFromNoteToRest()
{
    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
	KMessageBox::sorry(this, i18n("No note duration selected"));
	return;
    }

    Rosegarden::Note note(noteInserter->getCurrentNote());
    
    RestInserter *restInserter = dynamic_cast<RestInserter*>
	(m_toolBox->getTool(RestInserter::ToolName));

    restInserter->slotSetNote(note.getNoteType());
    restInserter->slotSetDots(note.getDots());

    setTool(restInserter);
}

void NotationView::slotTranspose()
{
    if (!m_currentEventSelection) return;

    bool ok = false;
    int semitones = QInputDialog::getInteger
	(i18n("Transpose"),
	 i18n("Enter the number of semitones to transpose up by:"),
	 0, -127, 127, 1, &ok, this);
    if (!ok || semitones == 0) return;

    KTmpStatusMsg msg(i18n("Transposing..."), this);
    addCommandToHistory(new TransposeCommand
                        (semitones, *m_currentEventSelection));
}

void NotationView::slotTransposeUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one semitone..."), this);

    addCommandToHistory(new TransposeCommand(1, *m_currentEventSelection));
}

void NotationView::slotTransposeUpOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one octave..."), this);

    addCommandToHistory(new TransposeCommand(12, *m_currentEventSelection));
}

void NotationView::slotTransposeDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one semitone..."), this);

    addCommandToHistory(new TransposeCommand(-1, *m_currentEventSelection));
}

void NotationView::slotTransposeDownOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one octave..."), this);

    addCommandToHistory(new TransposeCommand(-12, *m_currentEventSelection));
}

void NotationView::slotTransformsQuantize()
{
    if (!m_currentEventSelection) return;

    QuantizeDialog *dialog = new QuantizeDialog
	(this,
         Rosegarden::Quantizer::GlobalSource,
	 Rosegarden::Quantizer::RawEventData);

    if (dialog->exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Quantizing..."), this);
	addCommandToHistory(new EventQuantizeCommand
			    (*m_currentEventSelection,
			     dialog->getQuantizer()));
    }
}

void NotationView::slotTransformsFixSmoothing()
{
    //!!! actually, this is a pretty useless function

    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Fixing smoothed values..."), this);
    addCommandToHistory(new TransformsMenuFixSmoothingCommand
			(*m_currentEventSelection, m_legatoQuantizer));
}

void NotationView::slotAddSlashes()
{
    const QObject *s = sender();
    if (!m_currentEventSelection) return;

    QString name = s->name();
    int slashes = name.right(1).toInt();

    addCommandToHistory(new NotesMenuAddSlashesCommand
			(slashes, *m_currentEventSelection));
}


void NotationView::slotMarksAddTextMark()
{
    if (m_currentEventSelection) {
	SimpleTextDialog *dialog = new SimpleTextDialog(this, 20);
	if (dialog->exec() == QDialog::Accepted) {
	    addCommandToHistory(new MarksMenuAddTextMarkCommand
				(dialog->getText(), *m_currentEventSelection));
	}
	delete dialog;
    }
}

void NotationView::slotMarksRemoveMarks()
{
    if (m_currentEventSelection)
        addCommandToHistory(new MarksMenuRemoveMarksCommand
                            (*m_currentEventSelection));
}

void NotationView::slotEditAddClef()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Event *clefEvt = 0, *keyEvt = 0;
    Segment &segment = staff->getSegment();
    timeT insertionTime = getInsertionTime(clefEvt, keyEvt);

    Rosegarden::Clef clef;
    if (clefEvt) clef = Rosegarden::Clef(*clefEvt);
    
    ClefDialog *dialog = new ClefDialog(this, m_notePixmapFactory, clef);
    
    if (dialog->exec() == QDialog::Accepted) {
	
	ClefDialog::ConversionType conversion = dialog->getConversionType();
	
	bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
	bool shouldTranspose = (conversion == ClefDialog::Transpose);
	
	addCommandToHistory
	    (new ClefInsertionCommand
	     (segment, insertionTime, dialog->getClef(),
	      shouldChangeOctave, shouldTranspose));
    }
    
    delete dialog;
}			

void NotationView::slotEditAddTempo()
{
    Event *clefEvt = 0, *keyEvt = 0;
    timeT insertionTime = getInsertionTime(clefEvt, keyEvt);

    TempoDialog *tempoDlg = new TempoDialog(this, m_document);

    connect(tempoDlg,
	    SIGNAL(changeTempo(Rosegarden::timeT,
			       double, TempoDialog::TempoDialogAction)),
	    this,
	    SIGNAL(changeTempo(Rosegarden::timeT,
			       double, TempoDialog::TempoDialogAction)));
	
    tempoDlg->setTempoPosition(insertionTime);
    tempoDlg->show();
}

void NotationView::slotEditAddTimeSignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Event *clefEvt = 0, *keyEvt = 0;
    Segment &segment = staff->getSegment();
    Rosegarden::Composition *composition = segment.getComposition();
    timeT insertionTime = getInsertionTime(clefEvt, keyEvt);

    int barNo = composition->getBarNumber(insertionTime);
    bool atStartOfBar = (insertionTime == composition->getBarStart(barNo));
//    TimeSignature timeSig = composition->getTimeSignatureAt(insertionTime);


    //!!! experimental:
    Rosegarden::CompositionTimeSliceAdapter adapter
	(&m_document->getComposition(), insertionTime,
	 m_document->getComposition().getDuration());
    Rosegarden::AnalysisHelper helper;
    Rosegarden::TimeSignature timeSig = helper.guessTimeSignature(adapter);


    TimeSignatureDialog *dialog = new TimeSignatureDialog
	(this, timeSig, barNo, atStartOfBar,
	 i18n("Estimated time signature shown"));
    
    if (dialog->exec() == QDialog::Accepted) {

	TimeSignatureDialog::Location location = dialog->getLocation();
	if (location == TimeSignatureDialog::StartOfBar) {
	    insertionTime = composition->getBarStartForTime(insertionTime);
	}
	
	if (dialog->shouldNormalizeRests()) {
	    
	    addCommandToHistory(new AddTimeSignatureAndNormalizeCommand
				(composition, insertionTime,
				 dialog->getTimeSignature()));
	    
	} else {
	    
	    addCommandToHistory(new AddTimeSignatureCommand
				(composition, insertionTime,
				 dialog->getTimeSignature()));
	}
    }
    
    delete dialog;
}			

void NotationView::slotEditAddKeySignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Event *clefEvt = 0, *keyEvt = 0;
    Segment &segment = staff->getSegment();
    timeT insertionTime = getInsertionTime(clefEvt, keyEvt);
    
/*!!!
	Rosegarden::Key key;
	if (keyEvt) key = Rosegarden::Key(*keyEvt);
*/

    //!!! experimental:
    Rosegarden::CompositionTimeSliceAdapter adapter
	(&m_document->getComposition(), insertionTime,
	 m_document->getComposition().getDuration());
    Rosegarden::AnalysisHelper helper;
    Rosegarden::Key key = helper.guessKey(adapter);

    Rosegarden::Clef clef;
    if (clefEvt) clef = Rosegarden::Clef(*clefEvt);

    KeySignatureDialog *dialog =
	new KeySignatureDialog
	(this, m_notePixmapFactory, clef, key, true, true,
	 i18n("Estimated key signature shown"));
    
    if (dialog->exec() == QDialog::Accepted &&
	dialog->isValid()) {
	
	KeySignatureDialog::ConversionType conversion =
	    dialog->getConversionType();
	
	bool applyToAll = dialog->shouldApplyToAll();
	
	if (applyToAll) {
	    addCommandToHistory
		(new MultiKeyInsertionCommand
		 (m_document->getComposition(),
		  insertionTime, dialog->getKey(),
		  conversion == KeySignatureDialog::Convert,
		  conversion == KeySignatureDialog::Transpose));
	} else {
	    addCommandToHistory
		(new KeyInsertionCommand
		 (segment,
		  insertionTime, dialog->getKey(),
		  conversion == KeySignatureDialog::Convert,
		  conversion == KeySignatureDialog::Transpose));
	}
    }

    delete dialog;
}			


void NotationView::slotDebugDump()
{
    if (m_currentEventSelection) {
	EventSelection::eventcontainer &ec =
	    m_currentEventSelection->getSegmentEvents();
	int n = 0;
	for (EventSelection::eventcontainer::iterator i =
		 ec.begin();
	     i != ec.end(); ++i) {
	    cerr << "\n" << n++ << " [" << (*i) << "]" << endl;
	    (*i)->dump(cerr);
	}
    }
}


void
NotationView::slotSetPointerPosition(timeT time, bool scroll)
{
    Rosegarden::Composition &comp = m_document->getComposition();
    int barNo = comp.getBarNumber(time);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	if (barNo < m_hlayout->getFirstVisibleBarOnStaff(*m_staffs[i]) ||
	    barNo > m_hlayout-> getLastVisibleBarOnStaff(*m_staffs[i])) {
	    m_staffs[i]->hidePointer();
	} else {
	    m_staffs[i]->setPointerPosition(*m_hlayout, time);
	}
    }

    if (scroll) getCanvasView()->slotScrollHoriz(int(m_hlayout->getXForTime(time)));
    updateView();
}

void
NotationView::slotSetCurrentStaff(int y)
{
    unsigned int staffNo;
    for (staffNo = 0; staffNo < m_staffs.size(); ++staffNo) {
	if (m_staffs[staffNo]->containsCanvasY(y)) break;
    }
    
    if (staffNo < m_staffs.size()) {
	if (m_currentStaff != signed(staffNo)) {
	    m_staffs[m_currentStaff]->setCurrent(false);
	    m_currentStaff = staffNo;
	    m_staffs[m_currentStaff]->setCurrent(true, y);
	}
	m_chordNameRuler->setCurrentSegment
	    (&m_staffs[m_currentStaff]->getSegment());
    }
    
    updateView();
}

void
NotationView::slotCurrentStaffUp()
{
    if (m_staffs.size() < 2) return;
    m_staffs[m_currentStaff]->setCurrent(false);
    if (m_currentStaff-- <= 0) m_currentStaff = m_staffs.size()-1;
    m_staffs[m_currentStaff]->setCurrent(true);
    slotSetInsertCursorPosition(getInsertionTime());
}

void
NotationView::slotCurrentStaffDown()
{
    if (m_staffs.size() < 2) return;
    m_staffs[m_currentStaff]->setCurrent(false);
    if (++m_currentStaff >= (int)m_staffs.size()) m_currentStaff = 0;
    m_staffs[m_currentStaff]->setCurrent(true);
    slotSetInsertCursorPosition(getInsertionTime());
}

void
NotationView::slotSetInsertCursorPosition(double x, int y, bool scroll,
					  bool updateNow)
{
    slotSetCurrentStaff(y);

    NotationStaff *staff = m_staffs[m_currentStaff];
    Event *clefEvt, *keyEvt;
    NotationElementList::iterator i =
	staff->getElementUnderCanvasCoords(x, y, clefEvt, keyEvt);

    if (i == staff->getViewElementList()->end()) {
	slotSetInsertCursorPosition(staff->getSegment().getEndTime(), scroll,
				    updateNow);
    } else {
	slotSetInsertCursorPosition((*i)->getAbsoluteTime(), scroll,
				    updateNow);
    }
}    

void
NotationView::slotSetInsertCursorPosition(timeT t, bool scroll, bool updateNow)
{
    m_insertionTime = t;
    if (scroll) {
	m_deferredCursorMove = CursorMoveAndMakeVisible;
    } else {
	m_deferredCursorMove = CursorMoveOnly;
    }
    if (updateNow) doDeferredCursorMove();
}

void
NotationView::slotSetInsertCursorAndRecentre(timeT t, double cx, int,
					     bool updateNow)
{
    m_insertionTime = t;

    // We only do the scroll bit if cx is in the right two-thirds of
    // the window
    
    if (cx < (getCanvasView()->contentsX() +
	      getCanvasView()->visibleWidth() / 3)) {

	m_deferredCursorMove = CursorMoveOnly;
    } else {
	m_deferredCursorMove = CursorMoveAndScrollToPosition;
	m_deferredCursorScrollToX = cx;
    }

    if (updateNow) doDeferredCursorMove();
}

void
NotationView::doDeferredCursorMove()
{
    if (m_deferredCursorMove == NoCursorMoveNeeded) {
	return;
    }

    timeT t = m_insertionTime;

    if (m_staffs.size() == 0) return;
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();

    if (t < segment.getStartTime()) {
	t = segment.getStartTime();
    }
    if (t > segment.getEndTime()) {
	t = segment.getEndTime();
    }

    NotationElementList::iterator i = 
	staff->getViewElementList()->findNearestTime(t);

    if (i == staff->getViewElementList()->end()) {
	//!!! ???
	if (m_insertionTime >= staff->getSegment().getStartTime()) {
	    i = staff->getViewElementList()->begin();
	}
	m_insertionTime = staff->getSegment().getStartTime();
    } else {
	m_insertionTime = (*i)->getAbsoluteTime();
    }

    if (i == staff->getViewElementList()->end() ||
	t == segment.getEndTime() ||
	t == segment.getBarStartForTime(t)) {

	staff->setInsertCursorPosition(*m_hlayout, t);

    } else {

	// prefer a note or rest, if there is one, to a non-spacing event
	if (!(*i)->isNote() && !(*i)->isRest()) {
	    NotationElementList::iterator j = i;
	    while (j != staff->getViewElementList()->end()) {
		if ((*j)->getAbsoluteTime() != (*i)->getAbsoluteTime()) break;
		if ((*j)->isNote() || (*j)->isRest()) {
		    i = j;
		    break;
		}
		++j;
	    }
	}

	staff->setInsertCursorPosition
	    ((*i)->getCanvasX() - 2, int((*i)->getCanvasY()));

	if (m_deferredCursorMove == CursorMoveAndMakeVisible) {
	    getCanvasView()->slotScrollHoriz(int((*i)->getCanvasX()) - 4);
	}
    }

    if (m_deferredCursorMove == CursorMoveAndScrollToPosition) {

	// get current canvas x of insert cursor, which might not be
	// what we just set
    
	double ccx;

	NotationElementList::iterator i = 
	    staff->getViewElementList()->findTime(t);

	if (i == staff->getViewElementList()->end()) {
	    if (i == staff->getViewElementList()->begin()) return;
	    double lx, lwidth;
	    --i;
	    ccx = (*i)->getCanvasX();
	    (*i)->getLayoutAirspace(lx, lwidth);
	    ccx += lwidth;
	} else {
	    ccx = (*i)->getCanvasX();
	}
	
	QScrollBar* hbar = m_horizontalScrollBar;
	hbar->setValue(int(hbar->value() - (m_deferredCursorScrollToX - ccx)));
    }

    m_deferredCursorMove = NoCursorMoveNeeded;
    updateView();
}


void
NotationView::slotStepBackward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    timeT time = getInsertionTime();
    Segment::iterator i = segment.findTime(time);

    while (i != segment.begin() &&
	   (i == segment.end() || (*i)->getAbsoluteTime() == time)) --i;

    if (i != segment.end()) slotSetInsertCursorPosition((*i)->getAbsoluteTime());
}

void
NotationView::slotStepForward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    timeT time = getInsertionTime();
    Segment::iterator i = segment.findTime(time);

    while (segment.isBeforeEndMarker(i) &&
	   (*i)->getAbsoluteTime() == time) ++i;

    if (!segment.isBeforeEndMarker(i)) {
	slotSetInsertCursorPosition(segment.getEndMarkerTime());
    } else {
	slotSetInsertCursorPosition((*i)->getAbsoluteTime());
    }
}

void
NotationView::slotJumpBackward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    timeT time = segment.getBarStartForTime(getInsertionTime() - 1);
    slotSetInsertCursorPosition(time);
}

void
NotationView::slotJumpForward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    timeT time = segment.getBarEndForTime(getInsertionTime());
    slotSetInsertCursorPosition(time);
}

void
NotationView::slotJumpToStart()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    timeT time = segment.getStartTime();
    slotSetInsertCursorPosition(time);
}    

void
NotationView::slotJumpToEnd()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    timeT time = segment.getEndMarkerTime();
    slotSetInsertCursorPosition(time);
}    

void
NotationView::slotJumpCursorToPlayback()
{
    slotSetInsertCursorPosition(m_document->getComposition().getPosition());
}

void
NotationView::slotJumpPlaybackToCursor()
{
    emit jumpPlaybackTo(getInsertionTime());
}

//////////////////////////////////////////////////////////////////////
/*!!!
void NotationView::slotToggleTriplet()
{
    NOTATION_DEBUG << "NotationView::slotToggleTriplet()\n";
    
    m_tupletMode = !m_tupletMode;
    emit changeTupletMode(m_tupletMode);
}
*/

//----------------------------------------
// Accidentals
//----------------------------------------
void NotationView::slotNoAccidental()
{
    m_currentAccidental = Rosegarden::Accidentals::NoAccidental;
    emit changeAccidental(Rosegarden::Accidentals::NoAccidental);
}

void NotationView::slotSharp()
{
    m_currentAccidental = Rosegarden::Accidentals::Sharp;
    emit changeAccidental(Rosegarden::Accidentals::Sharp);
}

void NotationView::slotFlat()
{
    m_currentAccidental = Rosegarden::Accidentals::Flat;
    emit changeAccidental(Rosegarden::Accidentals::Flat);
}

void NotationView::slotNatural()
{
    m_currentAccidental = Rosegarden::Accidentals::Natural;
    emit changeAccidental(Rosegarden::Accidentals::Natural);
}

void NotationView::slotDoubleSharp()
{
    m_currentAccidental = Rosegarden::Accidentals::DoubleSharp;
    emit changeAccidental(Rosegarden::Accidentals::DoubleSharp);
}

void NotationView::slotDoubleFlat()
{
    m_currentAccidental = Rosegarden::Accidentals::DoubleFlat;
    emit changeAccidental(Rosegarden::Accidentals::DoubleFlat);
}


//----------------------------------------
// Clefs
//----------------------------------------
void NotationView::slotTrebleClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-treble"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Treble);

#ifdef RGKDE3
    stateChanged("note_insert_tool_current", KXMLGUIClient::StateReverse);
#endif
}

void NotationView::slotTenorClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Tenor);

#ifdef RGKDE3
    stateChanged("note_insert_tool_current", KXMLGUIClient::StateReverse);
#endif
}

void NotationView::slotAltoClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Alto);

#ifdef RGKDE3
    stateChanged("note_insert_tool_current", KXMLGUIClient::StateReverse);
#endif
}

void NotationView::slotBassClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Bass);

#ifdef RGKDE3
    stateChanged("note_insert_tool_current", KXMLGUIClient::StateReverse);
#endif
}



void NotationView::slotText()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("text"));
    setTool(m_toolBox->getTool(TextInserter::ToolName));

#ifdef RGKDE3
    stateChanged("note_insert_tool_current", KXMLGUIClient::StateReverse);
#endif
}


//----------------------------------------
// Editing Tools
//----------------------------------------

void NotationView::slotEraseSelected()
{
    NOTATION_DEBUG << "NotationView::slotEraseSelected()\n";
    setTool(m_toolBox->getTool(NotationEraser::ToolName));

#ifdef RGKDE3
    stateChanged("note_insert_tool_current", KXMLGUIClient::StateReverse);
#endif
}

void NotationView::slotSelectSelected()
{
    NOTATION_DEBUG << "NotationView::slotSelectSelected()\n";
    setTool(m_toolBox->getTool(NotationSelector::ToolName));

#ifdef RGKDE3
    stateChanged("note_insert_tool_current", KXMLGUIClient::StateReverse);
#endif
}




void NotationView::slotLinearMode()
{
    setPageMode(false);
}

void NotationView::slotPageMode()
{
    setPageMode(true);
}
/*!!!
void NotationView::slotInsertChordMode()
{
    m_insertChordMode = true;
}

void NotationView::slotInsertMelodyMode()
{
    m_insertChordMode = false;
}
*/
void NotationView::slotLabelChords()
{
    if (m_hlayout->isPageMode()) return;
    m_chordNamesVisible = !m_chordNamesVisible;

    if (!m_chordNamesVisible) {
	m_chordNameRuler->hide();
    } else {
	m_chordNameRuler->show();
    }
}

void NotationView::slotShowTempos()
{
    if (m_hlayout->isPageMode()) return;
    m_temposVisible = !m_temposVisible;

    if (!m_temposVisible) {
	m_tempoRuler->hide();
    } else {
	m_tempoRuler->show();
    }
}

void NotationView::slotShowAnnotations()
{
    m_annotationsVisible = !m_annotationsVisible;
    refreshSegment(0, 0, 0);
}

void NotationView::slotEditLyrics()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();

    LyricEditDialog *dialog = new LyricEditDialog(this, &segment);

    if (dialog->exec() == QDialog::Accepted) {
	
	SetLyricsCommand *command = new SetLyricsCommand
	    (&segment, dialog->getLyricData());

	addCommandToHistory(command);
    }
}

//----------------------------------------------------------------------

void NotationView::slotItemPressed(int height, int staffNo,
				   QMouseEvent* e,
				   NotationElement* el)
{
    NOTATION_DEBUG << "NotationView::slotItemPressed(height = "
                         << height << ", staffNo = " << staffNo
                         << ")\n";

    ButtonState btnState = e->state();

    if (btnState & ControlButton) { // on ctrl-click, set cursor position

	slotSetInsertCursorPosition(e->x(), (int)e->y());

    } else {

        setActiveItem(0);

        timeT unknownTime = 0;

        // This won't work because a double click event is always
        // preceded by a single click event
        if (e->type() == QEvent::MouseButtonDblClick)
            m_tool->handleMouseDblClick(unknownTime, height,
                                        staffNo, e, el);
        else
            m_tool->handleMousePress(unknownTime, height,
                                     staffNo, e, el);
    }
    
}

void NotationView::slotMouseMoved(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseMove(e);
        updateView();
    } else {
        int follow = m_tool->handleMouseMove(0, 0, // unknown time and height
                                             e);
        if (follow & EditTool::FollowHorizontal)
	    getCanvasView()->slotScrollHorizSmallSteps(e->pos().x());

        if (follow & EditTool::FollowVertical)
	    getCanvasView()->slotScrollVertSmallSteps(e->pos().y());
    }
}

void NotationView::slotMouseReleased(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        updateView();
    }
    else
        m_tool->handleMouseRelease(0, 0, // unknown time and height
                                   e);
}

void
NotationView::slotHoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(QString(" ") + noteName);
}

void
NotationView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    timeT t = time;
    Rosegarden::RealTime rt =
	m_document->getComposition().getElapsedRealTime(t);
    long ms = rt.usec / 1000;

    QString message;
    message.sprintf(" Time: %ld (%ld.%03lds)", t, rt.sec, ms);

    m_hoveredOverAbsoluteTime->setText(message);
}
