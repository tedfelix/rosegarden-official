
// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include "notationview.h"

#include "NotationTypes.h"
#include "Selection.h"
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

#include <qlabel.h>
#include <qinputdialog.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>

#include "ktmpstatusmsg.h"

using Rosegarden::timeT;


void
NotationView::slotDocumentDestroyed()
{
    NOTATION_DEBUG << "NotationView::slotDocumentDestroyed()\n";
    m_documentDestroyed = true;
    m_inhibitRefresh = true;
}

void
NotationView::slotChangeSpacing(int n)
{
    std::vector<int> spacings = m_hlayout.getAvailableSpacings();
    if (n >= (int)spacings.size()) n = spacings.size() - 1;
    m_hlayout.setSpacing(spacings[n]);

    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->positionAllElements();
    }

    updateView();
}

void NotationView::slotChangeLegato(int n)
{
    if (n >= (int)m_legatoDurations.size())
        n = m_legatoDurations.size() - 1;

    Rosegarden::Quantizer q(Rosegarden::Quantizer::RawEventData,
			    getViewLocalPropertyPrefix() + "Q",
			    Rosegarden::Quantizer::LegatoQuantize,
			    m_legatoDurations[n]);
    *m_legatoQuantizer = q;
    
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

    bool changedFont = (newName != m_fontName);

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

    m_hlayout.setNotePixmapFactory(m_notePixmapFactory);

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
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), statusBar());

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
				       m_document->getClipboard()));
}

void NotationView::slotEditDelete()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Deleting selection..."), statusBar());

    addCommandToHistory(new EraseCommand(*m_currentEventSelection));
}

void NotationView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
					m_document->getClipboard()));
}

void NotationView::slotEditCutAndClose()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), statusBar());

    addCommandToHistory(new CutAndCloseCommand(*m_currentEventSelection,
					       m_document->getClipboard()));
}

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
    Rosegarden::Segment &segment = staff->getSegment();
    
    // Paste at cursor position
    //
    timeT insertionTime = getInsertionTime();
    timeT endTime = insertionTime +
	(clipboard->getSingleSegment()->getEndTime() - 
	 clipboard->getSingleSegment()->getStartTime());

    PasteEventsCommand *command = new PasteEventsCommand
	(segment, clipboard, insertionTime);

    if (!command->isPossible()) {
	slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    } else {
	addCommandToHistory(command);
	setCurrentSelection(new Rosegarden::EventSelection
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
    Rosegarden::Segment &segment = staff->getSegment();
    
    PasteNotationDialog *dialog = new PasteNotationDialog
	(this, PasteEventsCommand::getDefaultPasteType());

    if (dialog->exec() == QDialog::Accepted) {

	PasteEventsCommand::PasteType type = dialog->getPasteType();
	if (dialog->setAsDefault()) {
	    PasteEventsCommand::setDefaultPasteType(type);
	}

	timeT insertionTime = getInsertionTime();
	timeT endTime = insertionTime +
	    (clipboard->getSingleSegment()->getEndTime() - 
	     clipboard->getSingleSegment()->getStartTime());

	PasteEventsCommand *command = new PasteEventsCommand
	    (segment, clipboard, insertionTime, type);

	if (!command->isPossible()) {
	    slotStatusHelpMsg(i18n("Couldn't paste at this point"));
	} else {
	    addCommandToHistory(command);
	    setCurrentSelection(new Rosegarden::EventSelection
				(segment, insertionTime, endTime));
	}
    }
}

void NotationView::slotEditSelectFromStart()
{
    timeT t = getInsertionTime();
    Rosegarden::Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new Rosegarden::EventSelection(segment,
						       segment.getStartTime(),
						       t));
}

void NotationView::slotEditSelectToEnd()
{
    timeT t = getInsertionTime();
    Rosegarden::Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new Rosegarden::EventSelection(segment,
						       t,
						       segment.getEndTime()));
}

void NotationView::slotEditSelectWholeStaff()
{
    Rosegarden::Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new Rosegarden::EventSelection(segment,
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
    KTmpStatusMsg msg(i18n("Beaming group..."), statusBar());

    addCommandToHistory(new GroupMenuBeamCommand
                        (*m_currentEventSelection));
}

void NotationView::slotGroupAutoBeam()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Auto-beaming selection..."), statusBar());

    addCommandToHistory(new GroupMenuAutoBeamCommand
                        (*m_currentEventSelection, m_legatoQuantizer));
}

void NotationView::slotGroupBreak()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Breaking groups..."), statusBar());

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
    Rosegarden::Segment *segment = 0;

    NotationSelector *selector = dynamic_cast<NotationSelector *>
	(m_toolBox->getTool(NotationSelector::ToolName));

    if (m_currentEventSelection &&
	selector && selector->isRectangleVisible()) {

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
    KTmpStatusMsg msg(i18n("Making grace notes..."), statusBar());

    addCommandToHistory(new GroupMenuGraceCommand(*m_currentEventSelection));
}

void NotationView::slotGroupUnGrace()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making non-grace notes..."), statusBar());

    addCommandToHistory(new GroupMenuUnGraceCommand(*m_currentEventSelection));
}


//
// indications stuff
//

void NotationView::slotGroupSlur()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Adding slur..."), statusBar());

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
    KTmpStatusMsg msg(i18n("Adding crescendo..."), statusBar());

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
    KTmpStatusMsg msg(i18n("Adding decrescendo..."), statusBar());

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
    KTmpStatusMsg msg(i18n("Normalizing rests..."), statusBar());

    addCommandToHistory(new TransformsMenuNormalizeRestsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseRests()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing rests..."), statusBar());

    addCommandToHistory(new TransformsMenuCollapseRestsCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing notes..."), statusBar());

    addCommandToHistory(new TransformsMenuCollapseNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsTieNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Tying notes..."), statusBar());

    addCommandToHistory(new TransformsMenuTieNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsUntieNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Untying notes..."), statusBar());

    addCommandToHistory(new TransformsMenuUntieNotesCommand
                        (*m_currentEventSelection));
}

void NotationView::slotTransformsStemsUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pointing stems up..."), statusBar());

    addCommandToHistory(new TransformsMenuChangeStemsCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotTransformsStemsDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pointing stems down..."), statusBar());

    addCommandToHistory(new TransformsMenuChangeStemsCommand
                        (false, *m_currentEventSelection));

}

void NotationView::slotTransformsRestoreStems()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Restoring computed stem directions..."), statusBar());

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
			  statusBar());

	addCommandToHistory(new TransformsMenuChangeStyleCommand
			    (NoteStyleName(name),
			     *m_currentEventSelection));
    } else {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown style action %1").arg(name)));
    }
}

void NotationView::slotTransformsTranspose()
{
    if (!m_currentEventSelection) return;

    bool ok = false;
    int semitones = QInputDialog::getInteger
	(i18n("Transpose"),
	 i18n("Enter the number of semitones to transpose up by:"),
	 0, -127, 127, 1, &ok, this);
    if (!ok || semitones == 0) return;

    KTmpStatusMsg msg(i18n("Transposing..."), statusBar());
    addCommandToHistory(new TransformsMenuTransposeCommand
                        (semitones, *m_currentEventSelection));
}

void NotationView::slotTransformsTransposeUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one semitone..."), statusBar());

    addCommandToHistory(new TransformsMenuTransposeCommand
                        (1, *m_currentEventSelection));
}

void NotationView::slotTransformsTransposeUpOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one octave..."), statusBar());

    addCommandToHistory(new TransformsMenuTransposeCommand
                        (12, *m_currentEventSelection));
}

void NotationView::slotTransformsTransposeDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one semitone..."), statusBar());

    addCommandToHistory(new TransformsMenuTransposeCommand
                        (-1, *m_currentEventSelection));
}

void NotationView::slotTransformsTransposeDownOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one octave..."), statusBar());

    addCommandToHistory(new TransformsMenuTransposeCommand
                        (-12, *m_currentEventSelection));
}

void NotationView::slotTransformsQuantize()
{
    if (!m_currentEventSelection) return;

    QuantizeDialog *dialog = new QuantizeDialog
	(this,
	 Rosegarden::Quantizer::RawEventData,
	 Rosegarden::Quantizer::RawEventData);

    if (dialog->exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Quantizing..."), statusBar());
	addCommandToHistory(new EventQuantizeCommand
			    (*m_currentEventSelection,
			     dialog->getQuantizer()));
    }
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
    Rosegarden::Event *clefEvt = 0, *keyEvt = 0;
    Rosegarden::Segment &segment = staff->getSegment();
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
    Rosegarden::Event *clefEvt = 0, *keyEvt = 0;
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
    Rosegarden::Event *clefEvt = 0, *keyEvt = 0;
    Rosegarden::Segment &segment = staff->getSegment();
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

	//!!!setCurrentSelection(0);
    }
    
    delete dialog;
}			

void NotationView::slotEditAddKeySignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Rosegarden::Event *clefEvt = 0, *keyEvt = 0;
    Rosegarden::Segment &segment = staff->getSegment();
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

	//!!!setCurrentSelection(0);
    }

    delete dialog;
}			


void NotationView::slotDebugDump()
{
    if (m_currentEventSelection) {
	Rosegarden::EventSelection::eventcontainer &ec =
	    m_currentEventSelection->getSegmentEvents();
	int n = 0;
	for (Rosegarden::EventSelection::eventcontainer::iterator i =
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
	if (barNo < m_hlayout.getFirstVisibleBarOnStaff(*m_staffs[i]) ||
	    barNo > m_hlayout. getLastVisibleBarOnStaff(*m_staffs[i])) {
	    m_staffs[i]->hidePointer();
	} else {
	    m_staffs[i]->setPointerPosition(m_hlayout, time);
	}
    }

    if (scroll) slotScrollHoriz(m_hlayout.getXForTime(time));
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
    slotSetInsertCursorPosition(m_insertionTime);
}

void
NotationView::slotCurrentStaffDown()
{
    if (m_staffs.size() < 2) return;
    m_staffs[m_currentStaff]->setCurrent(false);
    if (++m_currentStaff >= (int)m_staffs.size()) m_currentStaff = 0;
    m_staffs[m_currentStaff]->setCurrent(true);
    slotSetInsertCursorPosition(m_insertionTime);
}

void
NotationView::slotSetInsertCursorPosition(double x, int y, bool scroll,
					  bool updateNow)
{
    slotSetCurrentStaff(y);

    NotationStaff *staff = m_staffs[m_currentStaff];
    Rosegarden::Event *clefEvt, *keyEvt;
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
    // We only do this if cx is in the right two-thirds of
    // the window
    
    if (cx < (getCanvasView()->contentsX() +
	      getCanvasView()->visibleWidth() / 3)) return;

    m_insertionTime = t;
    m_deferredCursorMove = CursorMoveAndScrollToPosition;
    m_deferredCursorScrollToX = cx;
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
    Rosegarden::Segment &segment = staff->getSegment();

    if (t < segment.getStartTime()) {
	t = segment.getStartTime();
    }
    if (t > segment.getEndTime()) {
	t = segment.getEndTime();
    }

    NotationElementList::iterator i = 
	staff->getViewElementList()->findNearestTime(t);

    if (i == staff->getViewElementList()->end()) {
	m_insertionTime = staff->getSegment().getStartTime();
    } else {
	m_insertionTime = (*i)->getAbsoluteTime();
    }

    if (i == staff->getViewElementList()->end() ||
	t == segment.getEndTime() ||
	t == segment.getBarStartForTime(t)) {

	staff->setInsertCursorPosition(m_hlayout, t);

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
	    slotScrollHoriz(int((*i)->getCanvasX()) - 4);
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
	hbar->setValue(hbar->value() - (m_deferredCursorScrollToX - ccx));
    }

    m_deferredCursorMove = NoCursorMoveNeeded;
    updateView();
}


void
NotationView::slotStepBackward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Rosegarden::Segment &segment = staff->getSegment();
    timeT time = m_insertionTime;
    Rosegarden::Segment::iterator i = segment.findTime(time);

    while (i != segment.begin() &&
	   (i == segment.end() || (*i)->getAbsoluteTime() == time)) --i;
    if (i != segment.end()) slotSetInsertCursorPosition((*i)->getAbsoluteTime());
}

void
NotationView::slotStepForward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Rosegarden::Segment &segment = staff->getSegment();
    timeT time = m_insertionTime;
    Rosegarden::Segment::iterator i = segment.findTime(time);

    while (i != segment.end() && (*i)->getAbsoluteTime() == time) ++i;
    if (i == segment.end()) slotSetInsertCursorPosition(segment.getEndTime());
    else slotSetInsertCursorPosition((*i)->getAbsoluteTime());
}

void
NotationView::slotJumpBackward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Rosegarden::Segment &segment = staff->getSegment();
    timeT time = segment.getBarStartForTime(m_insertionTime - 1);
    slotSetInsertCursorPosition(time);
}

void
NotationView::slotJumpForward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Rosegarden::Segment &segment = staff->getSegment();
    timeT time = segment.getBarEndForTime(m_insertionTime);
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

void NotationView::slotToggleTriplet()
{
    NOTATION_DEBUG << "NotationView::slotToggleTriplet()\n";
    
    m_tupletMode = !m_tupletMode;
    emit changeTupletMode(m_tupletMode);
}

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
}

void NotationView::slotTenorClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Tenor);
}

void NotationView::slotAltoClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Alto);
}

void NotationView::slotBassClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Bass);
}



void NotationView::slotText()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("text"));
    setTool(m_toolBox->getTool(TextInserter::ToolName));
}


//----------------------------------------
// Edition Tools
//----------------------------------------

void NotationView::slotEraseSelected()
{
    NOTATION_DEBUG << "NotationView::slotEraseSelected()\n";
    setTool(m_toolBox->getTool(NotationEraser::ToolName));
}

void NotationView::slotSelectSelected()
{
    NOTATION_DEBUG << "NotationView::slotSelectSelected()\n";
    setTool(m_toolBox->getTool(NotationSelector::ToolName));
}




void NotationView::slotLinearMode()
{
    setPageMode(false);
}

void NotationView::slotPageMode()
{
    setPageMode(true);
}

void NotationView::slotLabelChords()
{
    if (m_hlayout.isPageMode()) return;
    m_chordNamesVisible = !m_chordNamesVisible;

    if (!m_chordNamesVisible) {
	m_chordNameRuler->hide();
    } else {
	m_chordNameRuler->show();
    }
}

void NotationView::slotShowTempos()
{
    if (m_hlayout.isPageMode()) return;
    m_temposVisible = !m_temposVisible;

    if (!m_temposVisible) {
	m_tempoRuler->hide();
    } else {
	m_tempoRuler->show();
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

    if (btnState & ShiftButton) { // on shift-click, set cursor position

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
        if (m_tool->handleMouseMove(0, 0, // unknown time and height
				    e)) {
	    slotScrollHorizSmallSteps(e->pos().x());
	}
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
