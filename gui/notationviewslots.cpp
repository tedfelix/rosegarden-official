
// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include <qtimer.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>
#include <kapp.h>
#include <kconfig.h>
#include <klineeditdlg.h>
#include <kprinter.h>

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
#include "rawnoteruler.h"
#include "notationhlayout.h"
#include "notefont.h"
#include "eventfilter.h"

#include "ktmpstatusmsg.h"

using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::Event;
using Rosegarden::EventSelection;


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
NotationView::slotUpdateAnnotationsStatus()
{
    if (!areAnnotationsVisible()) {
        for (int i = 0; i < getStaffCount(); ++i) {
            Segment &s = getStaff(i)->getSegment();
            for (Segment::iterator j = s.begin(); j != s.end(); ++j) {
                if ((*j)->isa(Rosegarden::Text::EventType) &&
                    ((*j)->get<Rosegarden::String>
                     (Rosegarden::Text::TextTypePropertyName)
                     == Rosegarden::Text::Annotation)) {
                    m_annotationsLabel->setText(i18n("Hidden annotations"));
                    return;
                }
            }
        }
    }
    m_annotationsLabel->setText("");
    getToggleAction("show_annotations")->setChecked(areAnnotationsVisible());
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
	std::cerr
            << "WARNING: Expected action \"spacing_" << spacing
            << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
            << std::endl;
    }

    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	m_staffs[i]->markChanged();
//!!!        m_staffs[i]->positionAllElements();
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
    int newSize = m_fontSize;

    if (!NoteFontFactory::isAvailableInSize(newName, newSize)) {

	int defaultSize = NoteFontFactory::getDefaultSize(newName);
	newSize = m_config->readUnsignedNumEntry
	    ((getStaffCount() > 1 ?
	      "multistaffnotesize" : "singlestaffnotesize"), defaultSize);

	if (!NoteFontFactory::isAvailableInSize(newName, newSize)) {
	    newSize = defaultSize;
	}
    }

    slotChangeFont(newName, newSize);
}


void
NotationView::slotChangeFontSize(int newSize)
{
    slotChangeFont(m_fontName, newSize);
}


void
NotationView::slotChangeFontSizeFromIndex(int n)
{
    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
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

    bool changedFont = (newName != m_fontName || newSize != m_fontSize);

    std::string oldName = m_fontName;
    m_fontName = newName;
    m_fontSize = newSize;
    setNotePixmapFactory(npf);

    // update the various GUI elements

    std::set<std::string> fs(NoteFontFactory::getFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (unsigned int i = 0; i < f.size(); ++i) {
        bool thisOne = (f[i] == m_fontName);
        if (thisOne) m_fontCombo->setCurrentItem(i);
        KToggleAction *action = dynamic_cast<KToggleAction *>
            (actionCollection()->action("note_font_" + strtoqstr(f[i])));
        NOTATION_DEBUG << "inspecting " << f[i] << (action ? ", have action" : ", no action") << endl;
        if (action) action->setChecked(thisOne);
        else {
	    std::cerr
                << "WARNING: Expected action \"note_font_" << f[i]
                << "\" to be a KToggleAction, but it isn't (or doesn't exist)"
                << std::endl;
        }
    }

    NOTATION_DEBUG << "about to reinitialise sizes" << endl;

    std::vector<int> sizes = NoteFontFactory::getScreenSizes(m_fontName);
    m_fontSizeSlider->reinitialise(sizes, m_fontSize);
    setupFontSizeMenu(oldName);

    m_hlayout->setNotePixmapFactory(m_notePixmapFactory);

    if (!changedFont) return; // might have been called to initialise menus etc

    NOTATION_DEBUG << "about to change font" << endl;

    if (m_pageMode == LinedStaff::MultiPageMode) {

	int pageWidth = getPageWidth();
	int topMargin = 0, leftMargin = 0;
	getPageMargins(leftMargin, topMargin);

	m_hlayout->setPageWidth(pageWidth - leftMargin * 2);
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->changeFont(m_fontName, m_fontSize);
    }

    NOTATION_DEBUG << "about to position staffs" << endl;

    positionStaffs();

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->markChanged();
//!!!            m_staffs[i]->renderAllElements();
//!!!            m_staffs[i]->positionAllElements();
        }
    }

    positionPages();
    updateView();
}


void
NotationView::slotFilePrint()
{
    KTmpStatusMsg msg(i18n("Printing..."), this);

    SetWaitCursor waitCursor;
    NotationView printingView(getDocument(), m_segments,
			      (QWidget *)parent(), this);

    if (!printingView.isOK()) {
	NOTATION_DEBUG << "Print : operation cancelled\n";
	return;
    }

    printingView.print();
}

void
NotationView::slotFilePrintPreview()
{
    KTmpStatusMsg msg(i18n("Previewing..."), this);

    SetWaitCursor waitCursor;
    NotationView printingView(getDocument(), m_segments,
			      (QWidget *)parent(), this);

    if (!printingView.isOK()) {
	NOTATION_DEBUG << "Print preview : operation cancelled\n";
	return;
    }

    printingView.print(true);
}


//
// Cut, Copy, Paste
//
void NotationView::slotEditCut()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
                                       getDocument()->getClipboard()));
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
                                        getDocument()->getClipboard()));
}

void NotationView::slotEditCutAndClose()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutAndCloseCommand(*m_currentEventSelection,
                                               getDocument()->getClipboard()));
}

static const QString RESTRICTED_PASTE_FAILED_DESCRIPTION = i18n(
                      "The Restricted paste type requires enough empty\n" \
                      "space (containing only rests) at the paste position\n" \
                      "to hold all of the events to be pasted.\n" \
                      "Not enough space was found.\n" \
                      "If you want to paste anyway, consider using one of\n" \
                      "the other paste types from the \"Paste...\" option\n" \
                      "on the Edit menu.  You can also change the default\n" \
                      "paste type to something other than Restricted if\n" \
                      "you wish."
    );

void NotationView::slotEditPaste()
{
    Rosegarden::Clipboard *clipboard = getDocument()->getClipboard();

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
    config->setGroup(NotationView::ConfigGroup);
    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        config->readUnsignedNumEntry("pastetype",
                                     PasteEventsCommand::Restricted);

    PasteEventsCommand *command = new PasteEventsCommand
        (segment, clipboard, insertionTime, defaultType);

    if (!command->isPossible()) {
        KMessageBox::detailedError
            (this,
             i18n("Couldn't paste at this point."), RESTRICTED_PASTE_FAILED_DESCRIPTION);
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
    Rosegarden::Clipboard *clipboard = getDocument()->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();

    KConfig *config = kapp->config();
    config->setGroup(NotationView::ConfigGroup);
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

void NotationView::slotPreviewSelection()
{
    if (!m_currentEventSelection) return;

    getDocument()->slotSetLoop(m_currentEventSelection->getStartTime(),
                            m_currentEventSelection->getEndTime());
}


void NotationView::slotClearLoop()
{
    getDocument()->slotSetLoop(0, 0);
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

//!!! these should be in matrix too

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
                                           segment.getEndMarkerTime()));
}

void NotationView::slotEditSelectWholeStaff()
{
    Segment &segment = m_staffs[m_currentStaff]->getSegment();
    setCurrentSelection(new EventSelection(segment,
                                           segment.getStartTime(),
                                           segment.getEndMarkerTime()));
}

void NotationView::slotFilterSelection()
{
    NOTATION_DEBUG << "NotationView::slotFilterSelection" << endl;

    Segment *segment = getCurrentSegment();
    EventSelection *existingSelection = m_currentEventSelection;
    if (!segment || !existingSelection) return;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        NOTATION_DEBUG << "slotFilterSelection- accepted" << endl;

	bool haveEvent = false;

        EventSelection *newSelection = new EventSelection(*segment);
        EventSelection::eventcontainer &ec =
            existingSelection->getSegmentEvents();
        for (EventSelection::eventcontainer::iterator i =
             ec.begin(); i != ec.end(); ++i) {
            if (dialog.keepEvent(*i)) {
		haveEvent = true;
		newSelection->addEvent(*i);
	    }
        }

	if (haveEvent) setCurrentSelection(newSelection);
	else setCurrentSelection(0);
    }
}

void NotationView::slotToggleToolsToolBar()
{
    toggleNamedToolBar("Tools Toolbar");
}

void NotationView::slotToggleNotesToolBar()
{
    toggleNamedToolBar("Notes Toolbar");
}

void NotationView::slotToggleRestsToolBar()
{
    toggleNamedToolBar("Rests Toolbar");
}

void NotationView::slotToggleAccidentalsToolBar()
{
    toggleNamedToolBar("Accidentals Toolbar");
}

void NotationView::slotToggleClefsToolBar()
{
    toggleNamedToolBar("Clefs Toolbar");
}

void NotationView::slotToggleMetaToolBar()
{
    toggleNamedToolBar("Meta Toolbar");
}

void NotationView::slotToggleMarksToolBar()
{
    toggleNamedToolBar("Marks Toolbar");
}

void NotationView::slotToggleGroupToolBar()
{
    toggleNamedToolBar("Group Toolbar");
}

void NotationView::slotToggleLayoutToolBar()
{
    toggleNamedToolBar("Layout Toolbar");
}

void NotationView::slotToggleTransportToolBar()
{
    toggleNamedToolBar("Transport Toolbar");
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

    setSettingsDirty();

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
                        (*m_currentEventSelection));
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

    if (m_currentEventSelection) {

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

void NotationView::slotGroupUnTuplet()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Untupleting..."), this);

    addCommandToHistory(new GroupMenuUnTupletCommand
                        (*m_currentEventSelection));
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

void NotationView::slotGroupMakeChord()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Making chord..."), this);

    GroupMenuMakeChordCommand *command =
	new GroupMenuMakeChordCommand(*m_currentEventSelection);

    addCommandToHistory(command);
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

void NotationView::slotTransformsFixQuantization()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Fixing notation quantization..."), this);

    addCommandToHistory(new TransformsMenuFixNotationQuantizeCommand
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
                            (NoteStyleName(qstrtostr(name)),
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

    int pitch = 0;
    Rosegarden::Accidental accidental =
        Rosegarden::Accidentals::NoAccidental;
               
    try {

        pitch = getPitchFromNoteInsertAction(name, accidental);

    } catch (...) {
        
        KMessageBox::sorry
            (this, QString(i18n("Unknown note insert action %1").arg(name)));
        return;
    }

    KTmpStatusMsg msg(i18n("Inserting note"), this);
        
    NOTATION_DEBUG << "Inserting note at pitch " << pitch << endl;
    
    noteInserter->insertNote(segment, getInsertionTime(), pitch,
                             accidental);
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
    setMenuStates();
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
    setMenuStates();
}

void NotationView::slotRespellDoubleFlat()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::DoubleFlat,
					   *m_currentEventSelection));
}

void NotationView::slotRespellFlat()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::Flat,
					   *m_currentEventSelection));
}

void NotationView::slotRespellSharp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::Sharp,
					   *m_currentEventSelection));
}

void NotationView::slotRespellDoubleSharp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Set,
					   Rosegarden::Accidentals::DoubleSharp,
					   *m_currentEventSelection));
}

void NotationView::slotRespellUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Up,
					   Rosegarden::Accidentals::NoAccidental,
					   *m_currentEventSelection));
}

void NotationView::slotRespellDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Forcing accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Down,
					   Rosegarden::Accidentals::NoAccidental,
					   *m_currentEventSelection));
}

void NotationView::slotRespellRestore()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Restoring accidentals..."), this);

    addCommandToHistory(new RespellCommand(RespellCommand::Restore,
					   Rosegarden::Accidentals::NoAccidental,
					   *m_currentEventSelection));
}

void NotationView::slotTransformsQuantize()
{
    if (!m_currentEventSelection) return;

    QuantizeDialog *dialog = new QuantizeDialog(this, true);

    if (dialog->exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(i18n("Quantizing..."), this);
        addCommandToHistory(new EventQuantizeCommand
                            (*m_currentEventSelection,
                             dialog->getQuantizer()));
    }
}

void NotationView::slotTransformsInterpret()
{
    if (!m_currentEventSelection) return;

    InterpretDialog *dialog = new InterpretDialog(this);

    if (dialog->exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Interpreting selection..."), this);
	addCommandToHistory(new TransformsMenuInterpretCommand
			    (*m_currentEventSelection,
			     getDocument()->getComposition().getNotationQuantizer(),
			     dialog->getInterpretations()));
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
        bool pressedOK = false;
        
        QString txt = KLineEditDlg::getText(i18n("Text: "), "", &pressedOK, this);
        
        if (pressedOK) {
            addCommandToHistory(new MarksMenuAddTextMarkCommand
                                (qstrtostr(txt), *m_currentEventSelection));
        }
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
    Segment &segment = staff->getSegment();
    Rosegarden::Clef clef;
    Rosegarden::Key key;
    timeT insertionTime = getInsertionTime(clef, key);
    
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

void NotationView::slotEditAddKeySignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    Rosegarden::Clef clef;
    Rosegarden::Key key;
    timeT insertionTime = getInsertionTime(clef, key);

    //!!! experimental:
    Rosegarden::CompositionTimeSliceAdapter adapter
        (&getDocument()->getComposition(), insertionTime,
         getDocument()->getComposition().getDuration());
    Rosegarden::AnalysisHelper helper;
    key = helper.guessKey(adapter);

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
                 (getDocument()->getComposition(),
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
	    std::cerr << "\n" << n++ << " [" << (*i) << "]" << std::endl;
            (*i)->dump(std::cerr);
        }
    }
}


void
NotationView::slotSetPointerPosition(timeT time, bool scroll)
{
    Rosegarden::Composition &comp = getDocument()->getComposition();
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
NotationView::slotSetCurrentStaff(double x, int y)
{
    unsigned int staffNo;
    for (staffNo = 0; staffNo < m_staffs.size(); ++staffNo) {
        if (m_staffs[staffNo]->containsCanvasCoords(x, y)) break;
    }
    
    if (staffNo < m_staffs.size()) {
        if (m_currentStaff != signed(staffNo)) {
            m_staffs[m_currentStaff]->setCurrent(false);
            m_currentStaff = staffNo;
            m_staffs[m_currentStaff]->setCurrent(true);
        }
        m_chordNameRuler->setCurrentSegment
            (&m_staffs[m_currentStaff]->getSegment());
        m_rawNoteRuler->setCurrentSegment
            (&m_staffs[m_currentStaff]->getSegment());
        m_rawNoteRuler->repaint();
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
    slotSetCurrentStaff(x, y);

    NotationStaff *staff = m_staffs[m_currentStaff];
    Event *clefEvt, *keyEvt;
    NotationElementList::iterator i =
        staff->getElementUnderCanvasCoords(x, y, clefEvt, keyEvt);

    if (i == staff->getViewElementList()->end()) {
        slotSetInsertCursorPosition(staff->getSegment().getEndTime(), scroll,
                                    updateNow);
    } else {
        slotSetInsertCursorPosition((*i)->getViewAbsoluteTime(), scroll,
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
    NOTATION_DEBUG << "NotationView::doDeferredCursorMove: m_deferredCursorMove == " << m_deferredCursorMove << endl;

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

    while (i != staff->getViewElementList()->end() &&
           !static_cast<NotationElement*>(*i)->getCanvasItem()) ++i;

    if (i == staff->getViewElementList()->end()) {
        //!!! ???
        if (m_insertionTime >= staff->getSegment().getStartTime()) {
            i = staff->getViewElementList()->begin();
        }
        m_insertionTime = staff->getSegment().getStartTime();
    } else {
        m_insertionTime = static_cast<NotationElement*>(*i)->getViewAbsoluteTime();
    }

    if (i == staff->getViewElementList()->end() ||
        t == segment.getEndTime() ||
        t == segment.getBarStartForTime(t)) {

        staff->setInsertCursorPosition(*m_hlayout, t);

        if (m_deferredCursorMove == CursorMoveAndMakeVisible) {
            getCanvasView()->slotScrollHoriz(int(staff->getCanvasXForLayoutX
						 (m_hlayout->getXForTime(t))));
        }

    } else {

        // prefer a note or rest, if there is one, to a non-spacing event
        if (!static_cast<NotationElement*>(*i)->isNote() &&
	    !static_cast<NotationElement*>(*i)->isRest()) {
            NotationElementList::iterator j = i;
            while (j != staff->getViewElementList()->end()) {
		if (static_cast<NotationElement*>(*j)->getViewAbsoluteTime() !=
		    static_cast<NotationElement*>(*i)->getViewAbsoluteTime()) break;
		if (static_cast<NotationElement*>(*j)->getCanvasItem()) {
		    if (static_cast<NotationElement*>(*j)->isNote() ||
			static_cast<NotationElement*>(*j)->isRest()) {
			i = j;
			break;
		    }
		}
                ++j;
            }
        }

        staff->setInsertCursorPosition
            (static_cast<NotationElement*>(*i)->getCanvasX() - 2,
	     int(static_cast<NotationElement*>(*i)->getCanvasY()));

        if (m_deferredCursorMove == CursorMoveAndMakeVisible) {
            getCanvasView()->slotScrollHoriz
		(int(static_cast<NotationElement*>(*i)->getCanvasX()) - 4);
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
            ccx = static_cast<NotationElement*>(*i)->getCanvasX();
            static_cast<NotationElement*>(*i)->getLayoutAirspace(lx, lwidth);
            ccx += lwidth;
        } else {
            ccx = static_cast<NotationElement*>(*i)->getCanvasX();
        }
        
        QScrollBar* hbar = getCanvasView()->horizontalScrollBar();
        hbar->setValue(int(hbar->value() - (m_deferredCursorScrollToX - ccx)));
    }

    m_deferredCursorMove = NoCursorMoveNeeded;
    updateView();
}

void
NotationView::slotJumpCursorToPlayback()
{
    slotSetInsertCursorPosition(getDocument()->getComposition().getPosition());
}

void
NotationView::slotJumpPlaybackToCursor()
{
    emit jumpPlaybackTo(getInsertionTime());
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
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-treble")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Treble);
    setMenuStates();
}

void NotationView::slotTenorClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Tenor);
    setMenuStates();
}

void NotationView::slotAltoClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Alto);
    setMenuStates();
}

void NotationView::slotBassClef()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass")));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Rosegarden::Clef::Bass);
    setMenuStates();
}



void NotationView::slotText()
{
    m_currentNotePixmap->setPixmap
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("text")));
    setTool(m_toolBox->getTool(TextInserter::ToolName));
    setMenuStates();
}


//----------------------------------------
// Editing Tools
//----------------------------------------

void NotationView::slotEraseSelected()
{
    NOTATION_DEBUG << "NotationView::slotEraseSelected()" << endl;
    setTool(m_toolBox->getTool(NotationEraser::ToolName));
    setMenuStates();
}

void NotationView::slotSelectSelected()
{
    NOTATION_DEBUG << "NotationView::slotSelectSelected()" << endl;
    setTool(m_toolBox->getTool(NotationSelector::ToolName));
    setMenuStates();
}




void NotationView::slotLinearMode()
{
    setPageMode(LinedStaff::LinearMode);
}

void NotationView::slotContinuousPageMode()
{
    setPageMode(LinedStaff::ContinuousPageMode);
}

void NotationView::slotMultiPageMode()
{
    setPageMode(LinedStaff::MultiPageMode);
}

void NotationView::slotToggleChordsRuler()
{
    if (m_hlayout->isPageMode()) return;
    toggleWidget(m_chordNameRuler, "show_chords_ruler");
}

void NotationView::slotToggleRawNoteRuler()
{
    if (m_hlayout->isPageMode()) return;
    toggleWidget(m_rawNoteRuler, "show_raw_note_ruler");
}

void NotationView::slotToggleTempoRuler()
{
    if (m_hlayout->isPageMode()) return;
    toggleWidget(m_tempoRuler, "show_tempo_ruler");
}

void NotationView::slotToggleAnnotations()
{
    m_annotationsVisible = !m_annotationsVisible;
    slotUpdateAnnotationsStatus();
//!!! use refresh mechanism
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
                         << ")" << endl;

    ButtonState btnState = e->state();

    if (btnState & ControlButton) { // on ctrl-click, set cursor position

        slotSetInsertCursorPosition(e->x(), (int)e->y());

    } else {

        setActiveItem(0);

        timeT unknownTime = 0;

        // This won't work because a double click event is always
        // preceded by a single click event
        if (e->type() == QEvent::MouseButtonDblClick)
            m_tool->handleMouseDoubleClick(unknownTime, height,
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
        getDocument()->getComposition().getElapsedRealTime(t);
    long ms = rt.usec / 1000;

    QString message;
    QString format("%ld (%ld.%03lds)");
    format = i18n("Time: %1").arg(format);
    message.sprintf(format, t, rt.sec, ms);

    m_hoveredOverAbsoluteTime->setText(message);
}

void
NotationView::slotInsertableNoteEventReceived(int pitch, bool noteOn)
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    if (!action->isChecked()) return;

    Segment &segment = m_staffs[m_currentStaff]->getSegment();

    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>(m_tool);
    if (!noteInserter) {
	KMessageBox::sorry(this, i18n("Can't insert note: No note duration selected"));
        return;
    }

    KTmpStatusMsg msg(i18n("Inserting note"), this);

    // We need to ensure that multiple notes hit at once come out as
    // chords, without imposing the interpretation that overlapping
    // notes are always chords and without getting too involved with
    // the actual absolute times of the notes (this is still step
    // editing, not proper recording).

    // First, if we're in chord mode, there's no problem.

    if (isInChordMode()) {
	if (!noteOn) return;
	NOTATION_DEBUG << "Inserting note in chord at pitch " << pitch << endl;
	noteInserter->insertNote(segment, getInsertionTime(), pitch,
				 Rosegarden::Accidentals::NoAccidental,
				 true);

    } else {

	if (noteOn) {

	    // Rules:
	    //
	    // * If no other note event has turned up within half a
	    //   second, insert this note and advance.
	    // 
	    // * Relatedly, if this note is within half a second of
	    //   the previous one, they're chords.  Insert the previous
	    //   one, don't advance, and use the same rules for this.
	    // 
	    // * If a note event turns up before that time has elapsed,
	    //   we need to wait for the note-off events: if the second
	    //   note happened less than half way through the first,
	    //   it's a chord.

	    // We haven't implemented these yet... For now:
	    noteInserter->insertNote(segment, getInsertionTime(), pitch,
				     Rosegarden::Accidentals::NoAccidental,
				     true);
	}
    }
}

void
NotationView::slotInsertableNoteOnReceived(int pitch)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOnReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, true);
}

void
NotationView::slotInsertableNoteOffReceived(int pitch)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOffReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, false);
}

void
NotationView::slotInsertableTimerElapsed()
{
}

void
NotationView::slotToggleStepByStep()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    if (action->isChecked()) { // after toggling, that is
	emit stepByStepTargetRequested(this);
    } else {
	emit stepByStepTargetRequested(0);
    }
}

void
NotationView::slotStepByStepTargetRequested(QObject *obj)
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	NOTATION_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    action->setChecked(obj == this);
}

void
NotationView::slotCheckRendered(double cx0, double cx1)
{
    bool something = false;

    for (size_t i = 0; i < m_staffs.size(); ++i) {

	NotationStaff *staff = m_staffs[i];

	LinedStaff::LinedStaffCoords cc0 = staff->getLayoutCoordsForCanvasCoords
	    (cx0, 0);

	LinedStaff::LinedStaffCoords cc1 = staff->getLayoutCoordsForCanvasCoords
	    (cx1, staff->getTotalHeight() + staff->getY());

	timeT t0 = m_hlayout->getTimeForX(cc0.first);
	timeT t1 = m_hlayout->getTimeForX(cc1.first);

	if (staff->checkRendered(t0, t1)) something = true;
    }

    if (something) {
	emit renderComplete();
	QTimer *t = new QTimer(this);
	connect(t, SIGNAL(timeout()), SLOT(slotRenderSomething()));
	t->start(0, true);
    }

    if (m_deferredCursorMove != NoCursorMoveNeeded) doDeferredCursorMove();
}

void
NotationView::slotRenderSomething()
{
    static clock_t lastWork = 0;
    
    clock_t now = clock();
    long elapsed = ((now - lastWork) * 1000 / CLOCKS_PER_SEC);
    if (elapsed < 100) {
	QTimer *t = new QTimer(this);
	connect(t, SIGNAL(timeout()), SLOT(slotRenderSomething()));
	t->start(0, true);
	return;
    }
    lastWork = now;

    for (size_t i = 0; i < m_staffs.size(); ++i) {

	if (m_staffs[i]->doRenderWork(m_staffs[i]->getSegment().getStartTime(),
				      m_staffs[i]->getSegment().getEndTime())) {
	    QTimer *t = new QTimer(this);
	    connect(t, SIGNAL(timeout()), SLOT(slotRenderSomething()));
	    t->start(0, true);
	    return;
	}
    }
}

