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

#include <sys/times.h>

#include <qcanvas.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qinputdialog.h>
#include <qvbox.h>

#include <kmessagebox.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kcommand.h>
#include <kstdaction.h>
#include <kapp.h>

#include "eventselection.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "notationview.h"
#include "notationelement.h"
#include "notationproperties.h"

#include "notationstaff.h"
#include "notepixmapfactory.h"
#include "notationtool.h"
#include "qcanvassimplesprite.h"
#include "ktmpstatusmsg.h"
#include "trackeditor.h"
#include "barbuttons.h"
#include "loopruler.h"

#include "rosedebug.h"

#include "NotationTypes.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "Quantizer.h"
#include "notationcommands.h"
#include "segmentcommands.h"
#include "dialogs.h"
#include "staffruler.h" // for ActiveItem

#include "chordnameruler.h"


using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Clef;
using Rosegarden::Key;
using Rosegarden::TimeSignature;
using Rosegarden::Quantizer;
using Rosegarden::timeT;
using Rosegarden::Accidental;

using Rosegarden::Mark;
using namespace Rosegarden::Marks;

using std::vector;
using std::string;
using std::set;
#define ID_STATUS_MSG 1

using namespace Rosegarden::BaseProperties;

//////////////////////////////////////////////////////////////////////

NotationView::NotationView(RosegardenGUIDoc *doc,
                           vector<Segment *> segments,
                           QWidget *parent) :
    EditView(doc, segments, parent),
    m_currentEventSelection(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_lastFinishingStaff(-1),
    m_fontName(NotePixmapFactory::getDefaultFont()),
    m_fontSize(NotePixmapFactory::getDefaultSize(m_fontName)),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(new NotationHLayout(&doc->getComposition(),
				  *m_notePixmapFactory)),
    m_vlayout(new NotationVLayout()),
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_fontSizeSlider(0),
    m_selectDefaultNote(0)
{
    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    kdDebug(KDEBUG_AREA) << "NotationView ctor" << endl;

    setupActions();

    kdDebug(KDEBUG_AREA) << "NotationView: Quantizer status is:\n"
                         << "Unit = " << segments[0]->getQuantizer()->getUnit()
                         << "\nMax Dots = "
                         << segments[0]->getQuantizer()->getMaxDots() << endl;

    initFontToolbar(segments[0]->getQuantizer()->getUnit());
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);
    
    setCanvasView(new NotationCanvasView(*this, m_horizontalScrollBar,
                                         tCanvas, m_centralFrame));

    //
    // Connect signals
    //
    QObject::connect
        (getCanvasView(), SIGNAL(itemPressed(int, int, QMouseEvent*, NotationElement*)),
         this,            SLOT  (slotItemPressed(int, int, QMouseEvent*, NotationElement*)));

    QObject::connect
        (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
         this,         SLOT  (slotActiveItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseMoved(QMouseEvent*)),
         this,         SLOT  (slotMouseMoved(QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseReleased(QMouseEvent*)),
         this,         SLOT  (slotMouseReleased(QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverNoteChanged(const QString&)),
         this,         SLOT  (slotHoveredOverNoteChanged(const QString&)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
         this,         SLOT  (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
	(doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	 this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    //
    // Window appearance (options, title...)
    //
    readOptions();

    if (segments.size() == 1) {
        setCaption(QString("%1 - Segment Track #%2")
                   .arg(doc->getTitle())
                   .arg(segments[0]->getTrack()));
    } else if (segments.size() == doc->getComposition().getNbSegments()) {
        setCaption(QString("%1 - All Segments")
                   .arg(doc->getTitle()));
    } else {
        setCaption(QString("%1 - %2-Segment Partial View")
                   .arg(doc->getTitle())
                   .arg(segments.size()));
    }

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], i,
                                             false, width() - 50,
                                             m_fontName, m_fontSize));
    }

    positionStaffs();
    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply score layout"));
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            
            m_staffs[i]->renderAllElements();
            m_staffs[i]->positionAllElements();
        }
    }


    m_topBarButtons = new BarButtons(m_hlayout, 25,
                                     false, m_centralFrame);
    setTopBarButtons(m_topBarButtons);

    QObject::connect
	(m_topBarButtons->getLoopRuler(),
	 SIGNAL(setPointerPosition(Rosegarden::timeT)),
	 this, SLOT(slotSetInsertCursorPosition(Rosegarden::timeT)));

    m_topBarButtons->getLoopRuler()->setBackgroundColor
	(RosegardenGUIColours::InsertCursorRuler);

    m_chordNameRuler = new ChordNameRuler
	(m_hlayout, &doc->getComposition(), 20, m_centralFrame);
    addRuler(m_chordNameRuler);
    m_chordNameRuler->hide();
    m_chordNamesVisible = false;

    m_bottomBarButtons = new BarButtons(m_hlayout, 25,
                                        true, m_centralFrame);
    setBottomBarButtons(m_bottomBarButtons);

    m_bottomBarButtons->connectRulerToDocPointer(doc);

    m_selectDefaultNote->activate();
}

NotationView::~NotationView()
{
    kdDebug(KDEBUG_AREA) << "-> ~NotationView()\n";

    saveOptions();

    delete m_hlayout;
    delete m_vlayout;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    // Delete remaining canvas items.
    QCanvasItemList allItems = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = allItems.begin(); it != allItems.end(); ++it) delete *it;
    // delete canvas();

    kdDebug(KDEBUG_AREA) << "<- ~NotationView()\n";
}

void NotationView::positionStaffs()
{
    vector<int> staffHeights;
    int totalHeight = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        staffHeights.push_back(m_staffs[i]->getHeightOfRow());
        totalHeight += staffHeights[i];
    }

    int h = 0;
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->setRowSpacing(totalHeight + staffHeights[i] / 7);
        if (i < m_staffs.size() - 1) {
            m_staffs[i]->setConnectingLineLength(staffHeights[i]);
        }
        
//!!!        m_staffs[i]->setX(20);
        m_staffs[i]->setX(0);
        m_staffs[i]->setY(h);

        h += staffHeights[i];
    }
}    

void NotationView::saveOptions()
{        
    m_config->setGroup("Notation Options");
    m_config->writeEntry("Geometry", size());
    m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
    m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
    m_config->writeEntry("ToolBarPos", (int) toolBar()->barPos());
}

void NotationView::readOptions()
{
    m_config->setGroup("Notation Options");
        
    QSize size(m_config->readSizeEntry("Geometry"));

    if (!size.isEmpty()) {
        resize(size);
    }
}

void NotationView::setupActions()
{   
    KRadioAction* noteAction = 0;
    
    // setup Notes menu & toolbar
    QIconSet icon;
 
    //
    // Notes
    //
    static const char* actionsNote[][4] = 
        {   // i18n,     slotName,         action name,     pixmap
            { "Breve",   "1slotBreve()",   "breve",         "breve" },
            { "Whole",   "1slotWhole()",   "whole_note",    "semibreve" },
            { "Half",    "1slotHalf()",    "half",          "minim" },
            { "Quarter", "1slotQuarter()", "quarter",       "crotchet" },
            { "8th",     "1slot8th()",     "8th",           "quaver" },
            { "16th",    "1slot16th()",    "16th",          "semiquaver" },
            { "32nd",    "1slot32nd()",    "32nd",          "demisemi" },
            { "64th",    "1slot64th()",    "64th",          "hemidemisemi" }
        };
   
    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsNote[i][3]));
        noteAction = new KRadioAction(i18n(actionsNote[i][0]), icon, 0, this,
                                      actionsNote[i][1],
                                      actionCollection(), actionsNote[i][2]);
        noteAction->setExclusiveGroup("notes");

        if (i == 3)
            m_selectDefaultNote = noteAction; // quarter is the default selected note

    }

    //
    // Dotted Notes
    //
    static const char* actionsDottedNote[][4] = 
        {
            { "Dotted Breve",   "1slotDottedBreve()",   "dotted_breve",      "dotted-breve" },
            { "Dotted Whole",   "1slotDottedWhole()",   "dotted_whole_note", "dotted-semibreve" },
            { "Dotted Half",    "1slotDottedHalf()",    "dotted_half",       "dotted-minim" },
            { "Dotted Quarter", "1slotDottedQuarter()", "dotted_quarter",    "dotted-crotchet" },
            { "Dotted 8th",     "1slotDotted8th()",     "dotted_8th",        "dotted-quaver" },
            { "Dotted 16th",    "1slotDotted16th()",    "dotted_16th",       "dotted-semiquaver" },
            { "Dotted 32nd",    "1slotDotted32nd()",    "dotted_32nd",       "dotted-demisemi" },
            { "Dotted 64th",    "1slotDotted64th()",    "dotted_64th",       "dotted-hemidemisemi" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsDottedNote[i][3]));
        noteAction = new KRadioAction(i18n(actionsDottedNote[i][0]), icon, 0, this,
                                      actionsDottedNote[i][1],
                                      actionCollection(), actionsDottedNote[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Rests
    //
    static const char* actionsRest[][4] = 
        {
            { "Breve Rest",   "1slotRBreve()",   "breve_rest",      "rest-breve" },
            { "Whole Rest",   "1slotRWhole()",   "whole_note_rest", "rest-semibreve" },
            { "Half Rest",    "1slotRHalf()",    "half_rest",       "rest-minim" },
            { "Quarter Rest", "1slotRQuarter()", "quarter_rest",    "rest-crotchet" },
            { "8th Rest",     "1slotR8th()",     "8th_rest",        "rest-quaver" },
            { "16th Rest",    "1slotR16th()",    "16th_rest",       "rest-semiquaver" },
            { "32nd Rest",    "1slotR32nd()",    "32nd_rest",       "rest-demisemi" },
            { "64th Rest",    "1slotR64th()",    "64th_rest",       "rest-hemidemisemi" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsRest[i][3]));
        noteAction = new KRadioAction(i18n(actionsRest[i][0]), icon, 0, this,
                                      actionsRest[i][1],
                                      actionCollection(), actionsRest[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Dotted Rests
    //
    static const char* actionsDottedRest[][4] = 
        {
            { "Dotted Breve Rest",   "1slotDottedRBreve()",   "dotted_breve_rest",      "dotted-rest-breve" },
            { "Dotted Whole Rest",   "1slotDottedRWhole()",   "dotted_whole_note_rest", "dotted-rest-semibreve" },
            { "Dotted Half Rest",    "1slotDottedRHalf()",    "dotted_half_rest",       "dotted-rest-minim" },
            { "Dotted Quarter Rest", "1slotDottedRQuarter()", "dotted_quarter_rest",    "dotted-rest-crotchet" },
            { "Dotted 8th Rest",     "1slotDottedR8th()",     "dotted_8th_rest",        "dotted-rest-quaver" },
            { "Dotted 16th Rest",    "1slotDottedR16th()",    "dotted_16th_rest",       "dotted-rest-semiquaver" },
            { "Dotted 32nd Rest",    "1slotDottedR32nd()",    "dotted_32nd_rest",       "dotted-rest-demisemi" },
            { "Dotted 64th Rest",    "1slotDottedR64th()",    "dotted_64th_rest",       "dotted-rest-hemidemisemi" }
        };

    for (unsigned int i = 0, noteType = Note::Longest;
         i < 8 && noteType > 0; ++i, --noteType) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsDottedRest[i][3]));
        noteAction = new KRadioAction(i18n(actionsDottedRest[i][0]), icon, 0, this,
                                      actionsDottedRest[i][1],
                                      actionCollection(), actionsDottedRest[i][2]);
        noteAction->setExclusiveGroup("notes");

    }

    //
    // Accidentals
    //
    static const char* actionsAccidental[][4] = 
        {
            { "No accidental",  "1slotNoAccidental()",  "no_accidental",           "accidental-none" },
            { "Sharp",          "1slotSharp()",         "sharp_accidental",        "accidental-sharp" },
            { "Flat",           "1slotFlat()",          "flat_accidental",         "accidental-flat" },
            { "Natural",        "1slotNatural()",       "natural_accidental",      "accidental-natural" },
            { "Double sharp",   "1slotDoubleSharp()",   "double_sharp_accidental", "accidental-doublesharp" },
            { "Double flat",    "1slotDoubleFlat()",    "double_flat_accidental",  "accidental-doubleflat" }
        };

    for (unsigned int i = 0; i < 6; ++i) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsAccidental[i][3]));
        noteAction = new KRadioAction(i18n(actionsAccidental[i][0]), icon, 0, this,
                                      actionsAccidental[i][1],
                                      actionCollection(), actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }
    

    //
    // Clefs
    //

    // Treble
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-treble"));
    noteAction = new KRadioAction(i18n("Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor"));
    noteAction = new KRadioAction(i18n("Tenor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto"));
    noteAction = new KRadioAction(i18n("Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass"));
    noteAction = new KRadioAction(i18n("Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");

    //
    // Edition tools (eraser, selector...)
    //
    noteAction = new KRadioAction(i18n("Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("select"));
    noteAction = new KRadioAction(i18n("Select"), icon, 0,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    noteAction->setExclusiveGroup("notes");
    

    // File menu
    KStdAction::close (this, SLOT(closeWindow()),          actionCollection());

    // Edit menu
    new KAction(i18n("Select from St&art"), 0, this,
		SLOT(slotEditSelectFromStart()), actionCollection(),
		"select_from_start");

    new KAction(i18n("Select to &End"), 0, this,
		SLOT(slotEditSelectToEnd()), actionCollection(),
		"select_to_end");

    new KAction(i18n("Select &Whole Staff"), 0, this,
		SLOT(slotEditSelectWholeStaff()), actionCollection(),
		"select_whole_staff");

    // View menu
    KRadioAction *linearModeAction = new KRadioAction
        (i18n("&Linear Layout"), 0, this, SLOT(slotLinearMode()),
         actionCollection(), "linear_mode");
    linearModeAction->setExclusiveGroup("layoutMode");
    linearModeAction->setChecked(true);

    KRadioAction *pageModeAction = new KRadioAction
        (i18n("&Page Layout"), 0, this, SLOT(slotPageMode()),
         actionCollection(), "page_mode");
    pageModeAction->setExclusiveGroup("layoutMode");

    KToggleAction *labelChordsAction = new KToggleAction
	(i18n("Label &Chords"), 0, this, SLOT(slotLabelChords()),
	 actionCollection(), "label_chords");

    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

    // setup Group menu
    new KAction(GroupMenuBeamCommand::name(), 0, this,
                SLOT(slotGroupBeam()), actionCollection(), "beam");

    new KAction(GroupMenuAutoBeamCommand::name(), 0, this,
                SLOT(slotGroupAutoBeam()), actionCollection(), "auto_beam");

    new KAction(GroupMenuBreakCommand::name(), 0, this,
                SLOT(slotGroupBreak()), actionCollection(), "break_group");

    new KAction(GroupMenuAddIndicationCommand::name
                (Rosegarden::Indication::Slur), 0, this,
                SLOT(slotGroupSlur()), actionCollection(), "slur");

    new KAction(GroupMenuAddIndicationCommand::name
                (Rosegarden::Indication::Crescendo), 0, this,
                SLOT(slotGroupCrescendo()), actionCollection(), "crescendo");

    new KAction(GroupMenuAddIndicationCommand::name
                (Rosegarden::Indication::Decrescendo), 0, this,
                SLOT(slotGroupDecrescendo()), actionCollection(), "decrescendo");

    // setup Transforms menu
    new KAction(TransformsMenuNormalizeRestsCommand::name(), 0, this,
                SLOT(slotTransformsNormalizeRests()), actionCollection(),
                "normalize_rests");

    new KAction(TransformsMenuCollapseRestsCommand::name(), 0, this,
                SLOT(slotTransformsCollapseRests()), actionCollection(),
                "collapse_rests_aggressively");

    new KAction(TransformsMenuCollapseNotesCommand::name(), 0, this,
                SLOT(slotTransformsCollapseNotes()), actionCollection(),
                "collapse_notes");

    new KAction(TransformsMenuChangeStemsCommand::name(true), 0, this,
                SLOT(slotTransformsStemsUp()), actionCollection(),
                "stems_up");

    new KAction(TransformsMenuChangeStemsCommand::name(false), 0, this,
                SLOT(slotTransformsStemsDown()), actionCollection(),
                "stems_down");

    new KAction(TransformsMenuRestoreStemsCommand::name(), 0, this,
                SLOT(slotTransformsRestoreStems()), actionCollection(),
                "restore_stems");

    new KAction(TransformsMenuTransposeOneStepCommand::name(true), 0, this,
                SLOT(slotTransformsTransposeUp()), actionCollection(),
                "transpose_up");

    new KAction(TransformsMenuTransposeOctaveCommand::name(true), 0, this,
                SLOT(slotTransformsTransposeUpOctave()), actionCollection(),
                "transpose_up_octave");

    new KAction(TransformsMenuTransposeOneStepCommand::name(false), 0, this,
                SLOT(slotTransformsTransposeDown()), actionCollection(),
                "transpose_down");

    new KAction(TransformsMenuTransposeOctaveCommand::name(false), 0, this,
                SLOT(slotTransformsTransposeDownOctave()), actionCollection(),
                "transpose_down_octave");

    new KAction(TransformsMenuTransposeCommand::name(), 0, this,
                SLOT(slotTransformsTranspose()), actionCollection(),
                "general_transpose");

    new KAction(i18n("&Dump selected events to stderr"), 0, this,
		SLOT(slotDebugDump()), actionCollection(), "debug_dump");

    static const Mark marks[] = 
    { Accent, Tenuto, Staccato, Sforzando, Rinforzando,
      Trill, Turn, Pause, UpBow, DownBow };
    static const char *markSlots[] = 
    { "1slotTransformsAddAccent()",      "1slotTransformsAddTenuto()",
      "1slotTransformsAddStaccato()",    "1slotTransformsAddSforzando()",
      "1slotTransformsAddRinforzando()", "1slotTransformsAddTrill()",
      "1slotTransformsAddTurn()",        "1slotTransformsAddPause()",
      "1slotTransformsAddUpBow()",       "1slotTransformsAddDownBow()" };

    for (unsigned int i = 0; i < 10; ++i) {
        new KAction(TransformsMenuAddMarkCommand::name(marks[i]), 0, this,
                    markSlots[i], actionCollection(),
                    QString("add_%1").arg(marks[i].c_str()));
    }

    new KAction(TransformsMenuAddTextMarkCommand::name(), 0, this,
                SLOT(slotTransformsAddTextMark()), actionCollection(),
                "add_text_mark");

    new KAction(TransformsMenuRemoveMarksCommand::name(), 0, this,
                SLOT(slotTransformsRemoveMarks()), actionCollection(),
                "remove_marks");

    new KAction(AddTimeSignatureCommand::name(), 0, this,
                SLOT(slotTransformsAddTimeSignature()), actionCollection(),
                "add_time_signature");

    new KAction(KeyInsertionCommand::name(), 0, this,
                SLOT(slotTransformsAddKeySignature()), actionCollection(),
                "add_key_signature");

    // setup Settings menu
    KStdAction::showToolbar(this, SLOT(slotToggleToolBar()), actionCollection());

    static const char* actionsToolbars[][4] = 
        {
            { "Show &Notes Toolbar",  "1slotToggleNotesToolBar()",  "show_notes_toolbar",                    "palette-notes" },
            { "Show &Rests Toolbar",  "1slotToggleRestsToolBar()",  "show_rests_toolbar",                    "palette-rests" },
            { "Show &Accidentals Toolbar",   "1slotToggleAccidentalsToolBar()",  "show_accidentals_toolbar", "palette-accidentals" },
            { "Show Cle&fs Toolbar",         "1slotToggleClefsToolBar()",        "show_clefs_toolbar",       "palette-clefs" },
            { "Show &Layout Toolbar",         "1slotToggleFontToolBar()",        "show_font_toolbar",       "palette-font" }
        };

    for (unsigned int i = 0; i < 5; ++i) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap(actionsToolbars[i][3]));

        KToggleAction* toolbarAction = new KToggleAction
            (i18n(actionsToolbars[i][0]), icon, 0,
             this, actionsToolbars[i][1],
             actionCollection(), actionsToolbars[i][2]);

/*
        KToggleAction* toolbarAction = new KToggleAction(i18n(actionsToolbars[i][0]), 0,
                                                         this, actionsToolbars[i][1],
                                                         actionCollection(), actionsToolbars[i][2]);
*/
        toolbarAction->setChecked(true);
    }
    
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()), actionCollection());

    KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

    createGUI("notation.rc");
}

template<class T>
NotationView::ZoomSlider<T>::ZoomSlider(const vector<T> &sizes,
                                        T initialSize, Orientation o,
                                        QWidget *parent, const char *name) :
    QSlider(0, sizes.size()-1, 1,
            getIndex(sizes, initialSize), o, parent, name),
    m_sizes(sizes)
{
    setTracking(false);
    setFixedWidth(150);
    setFixedHeight(15);
    setLineStep(1);
    setTickmarks(Below);
}

template<class T>
NotationView::ZoomSlider<T>::~ZoomSlider() { }

template<class T>
int
NotationView::ZoomSlider<T>::getIndex(const vector<T> &sizes, T size)
{
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size) return i;
    }
    return sizes.size()/2;
}

template<class T>
void
NotationView::ZoomSlider<T>::reinitialise(const vector<T> &sizes, T size)
{ 
    m_sizes = sizes;
    setMinValue(0);
    setMaxValue(sizes.size()-1);
    setValue(getIndex(sizes, size));
    setLineStep(1);
    setTickmarks(Below);
}

NotationStaff *
NotationView::getStaff(const Segment &segment)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (&(m_staffs[i]->getSegment()) == &segment) return m_staffs[i];
    }
    return 0;
}

void NotationView::initFontToolbar(int legatoUnit)
{
    KToolBar *fontToolbar = toolBar("fontToolBar");

    kdDebug(KDEBUG_AREA) << "NotationView::initFontToolbar: legatoUnit is "
                         << legatoUnit << endl;
    
    if (!fontToolbar) {
        kdDebug(KDEBUG_AREA)
            << "NotationView::initFontToolbar() : font toolbar not found\n";
        return;
    }

    new QLabel(i18n("  Font:  "), fontToolbar);

    QComboBox *fontCombo = new QComboBox(fontToolbar);
    fontCombo->setEditable(false);

    set<string> fs(NotePixmapFactory::getAvailableFontNames());
    vector<string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    for (vector<string>::iterator i = f.begin(); i != f.end(); ++i) {
        fontCombo->insertItem(QString(i->c_str()));
        if (*i == m_fontName) {
            fontCombo->setCurrentItem(fontCombo->count() - 1);
        }
    }

    connect(fontCombo, SIGNAL(activated(const QString &)),
            this,        SLOT(slotChangeFont(const QString &)));

    new QLabel(i18n("  Size:  "), fontToolbar);

    vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    m_fontSizeSlider = new ZoomSlider<int>
        (sizes, m_fontSize, QSlider::Horizontal, fontToolbar);
    connect(m_fontSizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeFontSizeFromIndex(int)));

    new QLabel(i18n("  Spacing:  "), fontToolbar);

    vector<double> spacings = NotationHLayout::getAvailableSpacings();
    QSlider *stretchSlider = new ZoomSlider<double>
        (spacings, 1.0, QSlider::Horizontal, fontToolbar);
    connect(stretchSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeStretch(int)));

    new QLabel(i18n("  Legato:  "), fontToolbar);

    if (m_legatoDurations.size() == 0) {
        for (int type = Note::Shortest; type <= Note::Longest; ++type) {
            m_legatoDurations.push_back
                ((int)(Note(type).getDuration()));
        }
    }
    QSlider *quantizeSlider = new ZoomSlider<int>
        (m_legatoDurations, legatoUnit, QSlider::Horizontal, fontToolbar);
    connect(quantizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeLegato(int)));
}

void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();
    
    m_currentNotePixmap       = new QLabel(sb);
    m_hoveredOverNoteName     = new QLabel(sb);
    m_hoveredOverAbsoluteTime = new QLabel(sb);

    m_currentNotePixmap->setMinimumWidth(20);
    m_hoveredOverNoteName->setMinimumWidth(32);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);
    sb->addWidget(m_currentNotePixmap);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
                         AlignLeft | AlignVCenter);
}

QSize NotationView::getViewSize()
{
    return canvas()->size();
}

void NotationView::setViewSize(QSize s)
{
    canvas()->resize(s.width(), s.height());
}

void
NotationView::slotChangeStretch(int n)
{
    vector<double> spacings = m_hlayout->getAvailableSpacings();
    if (n >= (int)spacings.size()) n = spacings.size() - 1;
    m_hlayout->setSpacing(spacings[n]);

    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->positionAllElements();
    }

    update();
}

void NotationView::slotChangeLegato(int n)
{
    if (n >= (int)m_legatoDurations.size())
        n = m_legatoDurations.size() - 1;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->setLegatoDuration(m_legatoDurations[n]);
    }

    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->positionAllElements();
    }

    update();
}


void
NotationView::slotChangeFont(const QString &newName)
{
    kdDebug(KDEBUG_AREA) << "changeFont: " << newName << endl;
    slotChangeFont(std::string(newName.latin1()));
}


void
NotationView::slotChangeFont(string newName)
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
    vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    if (n >= (int)sizes.size()) n = sizes.size()-1;
    slotChangeFont(m_fontName, sizes[n]);
}


void
NotationView::slotChangeFont(string newName, int newSize)
{
    kdDebug(KDEBUG_AREA) << "NotationView::changeResolution(" << newSize << ")\n";

    if (newName == m_fontName && newSize == m_fontSize) return;

    NotePixmapFactory* npf = 0;
    
    try {
        npf = new NotePixmapFactory(newName, newSize);
    } catch (...) {
        return;
    }

    bool changedFont = (newName != m_fontName);

    m_fontName = newName;
    m_fontSize = newSize;
    setNotePixmapFactory(npf);

    if (changedFont) {
        vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
        m_fontSizeSlider->reinitialise(sizes, m_fontSize);
    }

    double spacing = m_hlayout->getSpacing();
    setHLayout(new NotationHLayout(&m_document->getComposition(),
				   *m_notePixmapFactory));
    m_hlayout->setSpacing(spacing);

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

    update();
}


void
NotationView::setPageMode(bool pageMode)
{
    if (pageMode) {
	if (m_topBarButtons) m_topBarButtons->hide();
	if (m_bottomBarButtons) m_bottomBarButtons->hide();
	if (m_chordNameRuler) m_chordNameRuler->hide();
    } else {
	if (m_topBarButtons) m_topBarButtons->show();
	if (m_bottomBarButtons) m_bottomBarButtons->show();
	if (m_chordNameRuler && m_chordNamesVisible) m_chordNameRuler->show();
    }

    m_hlayout->setPageMode(pageMode);
    m_hlayout->setPageWidth(width() - 50);
    
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->setPageMode(pageMode);
        m_staffs[i]->setPageWidth(width() - 50);
    }

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
//            m_staffs[i]->renderAllElements();
            m_staffs[i]->positionAllElements();
        }
    }

    update();
}   


bool NotationView::applyLayout(int staffNo)
{
    START_TIMING;
    unsigned int i;

    for (i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo) continue;

        m_hlayout->resetStaff(*m_staffs[i]);
        m_vlayout->resetStaff(*m_staffs[i]);

        m_hlayout->scanStaff(*m_staffs[i]);
        m_vlayout->scanStaff(*m_staffs[i]);
    }

    m_hlayout->finishLayout();
    m_vlayout->finishLayout();

    // find the last finishing staff for future use

    timeT endTime = 0;
    bool haveEndTime = false;
    m_lastFinishingStaff = -1;

    timeT startTime = 0;
    bool haveStartTime = false;
    int firstStartingStaff = -1;

    for (i = 0; i < m_staffs.size(); ++i) {

	timeT thisStartTime = m_staffs[i]->getSegment().getStartTime();
	if (thisStartTime < startTime || !haveStartTime) {
	    startTime = thisStartTime;
	    haveStartTime = true;
	    firstStartingStaff = i;
	}

        timeT thisEndTime = m_staffs[i]->getSegment().getEndTime();
        if (thisEndTime > endTime || !haveEndTime) {
            endTime = thisEndTime;
	    haveEndTime = true;
            m_lastFinishingStaff = i;
        }
    }

    readjustCanvasSize();
    if (m_topBarButtons) {
	m_topBarButtons->update();
    }
    if (m_bottomBarButtons) {
	m_bottomBarButtons->update();
    }

    PRINT_ELAPSED("NotationView::applyLayout");
    return true;
}


void NotationView::setCurrentSelectedNote(const char *pixmapName,
                                          bool rest, Note::Type n, int dots)
{
    NoteInserter* inserter = 0;

    if (rest)
        inserter = dynamic_cast<NoteInserter*>(m_toolBox->getTool(RestInserter::ToolName));
    else
        inserter = dynamic_cast<NoteInserter*>(m_toolBox->getTool(NoteInserter::ToolName));

    inserter->slotSetNote(n);
    inserter->slotSetDots(dots);

    setTool(inserter);

    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap(pixmapName));

    emit changeCurrentNote(rest, n);
}

void NotationView::setCurrentSelection(EventSelection* s)
{
    if (m_currentEventSelection) {
        m_currentEventSelection->removeSelectionFromSegment();
        getStaff(m_currentEventSelection->getSegment())->positionElements
            (m_currentEventSelection->getBeginTime(),
             m_currentEventSelection->getEndTime());
    }

    delete m_currentEventSelection;
    m_currentEventSelection = s;

    if (s) {
        s->recordSelectionOnSegment();
        getStaff(s->getSegment())->positionElements(s->getBeginTime(),
                                                    s->getEndTime());
    }

    update();
}

void NotationView::setSingleSelectedEvent(int staffNo, Event *event)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event);
}

void NotationView::setSingleSelectedEvent(Segment &segment, Event *event)
{
    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);
    setCurrentSelection(selection);
}

void NotationView::setNotePixmapFactory(NotePixmapFactory* f)
{
    delete m_notePixmapFactory;
    m_notePixmapFactory = f;
}

void NotationView::setHLayout(NotationHLayout* l)
{
    if (m_hlayout) {
        l->setPageMode(m_hlayout->getPageMode());
        l->setPageWidth(m_hlayout->getPageWidth());
    }
    delete m_hlayout;
    m_hlayout = l;
}

//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

//
// Cut, Copy, Paste
//
void NotationView::slotEditCut()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotEditCut()\n";

    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection..."), statusBar());

    kdDebug(KDEBUG_AREA) << "NotationView::slotEditCut() : cutting selection\n";

    m_currentEventSelection->cut();

    emit usedSelection();

    refreshSegment(&m_currentEventSelection->getSegment(),
		   m_currentEventSelection->getBeginTime(),
		   m_currentEventSelection->getEndTime());

    kdDebug(KDEBUG_AREA) << "NotationView::slotEditCut() : selection duration = "
                         << m_currentEventSelection->getTotalDuration() << endl;
}

void NotationView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());

    m_currentEventSelection->copy();

    emit usedSelection();
}

void NotationView::slotEditPaste()
{
    if (!m_currentEventSelection) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        slotQuarter();
        return;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    kdDebug(KDEBUG_AREA) << "NotationView::slotEditPaste() : selection duration = "
                         << m_currentEventSelection->getTotalDuration() << endl;

    // Paste at cursor position
    //

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX < 0) {
	slotStatusHelpMsg(i18n("Couldn't paste at this point"));
	return;
    }

    Rosegarden::Event *timeSig, *clef, *key;

    NotationElementList::iterator i = staff->getClosestElementToLayoutX
	(layoutX, timeSig, clef, key, false, -1);

    timeT insertionTime = segment.getEndTime();
    if (i != staff->getViewElementList()->end()) {
	insertionTime = (*i)->getAbsoluteTime();
    }

    if (m_currentEventSelection->pasteToSegment(segment, insertionTime)) {

	refreshSegment
	    (&segment, 0,
	     insertionTime + m_currentEventSelection->getTotalDuration() + 1);

    } else {
        
        slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    }
}

void NotationView::slotEditSelectFromStart()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();

    NotationElementList *notes = staff->getViewElementList();
    EventSelection *selection = new EventSelection(staff->getSegment());

    for (NotationElementList::iterator i = notes->begin();
	 i != notes->end(); ++i) {
	if ((*i)->getLayoutX() < layoutX) selection->addEvent((*i)->event());
    }

    setCurrentSelection(selection);
}

void NotationView::slotEditSelectToEnd()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();

    NotationElementList *notes = staff->getViewElementList();
    EventSelection *selection = new EventSelection(staff->getSegment());

    for (NotationElementList::iterator i = notes->begin();
	 i != notes->end(); ++i) {
	if ((*i)->getLayoutX() > layoutX) selection->addEvent((*i)->event());
    }

    setCurrentSelection(selection);
}

void NotationView::slotEditSelectWholeStaff()
{
    //!!! The problem with this is it only selects those events that are
    // visible in this notation view.  We should really select every
    // event in the segment, not every event that has an element in the
    // notation element list.  Unfortunately while that would be fine
    // for this method, it can't be done efficiently in the select-from-
    // start and select-to-end methods, where it's also the right thing
    // to do.  Needs thought.

    NotationStaff *staff = m_staffs[m_currentStaff];
    NotationElementList *notes = staff->getViewElementList();
    EventSelection *selection = new EventSelection(staff->getSegment());

    for (NotationElementList::iterator i = notes->begin();
	 i != notes->end(); ++i) {
	selection->addEvent((*i)->event());
    }

    setCurrentSelection(selection);
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

void NotationView::toggleNamedToolBar(const QString& toolBarName)
{
    KToolBar *namedToolBar = toolBar(toolBarName);

    if (!namedToolBar) {
        kdDebug(KDEBUG_AREA) << "NotationView::toggleNamedToolBar() : toolBar "
                             << toolBarName << " not found" << endl;
        return;
    }
    
    if (namedToolBar->isVisible())
        namedToolBar->hide();
    else
        namedToolBar->show();
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
                        (*m_currentEventSelection));
}

void NotationView::slotGroupBreak()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Breaking groups..."), statusBar());

    addCommandToHistory(new GroupMenuBreakCommand
                        (*m_currentEventSelection));
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

    addCommandToHistory(new TransformsMenuTransposeOneStepCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotTransformsTransposeUpOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one octave..."), statusBar());

    addCommandToHistory(new TransformsMenuTransposeOctaveCommand
                        (true, *m_currentEventSelection));
}

void NotationView::slotTransformsTransposeDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one semitone..."), statusBar());

    addCommandToHistory(new TransformsMenuTransposeOneStepCommand
                        (false, *m_currentEventSelection));
}

void NotationView::slotTransformsTransposeDownOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one octave..."), statusBar());

    addCommandToHistory(new TransformsMenuTransposeOctaveCommand
                        (false, *m_currentEventSelection));
}

void NotationView::slotTransformsAddAccent()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (Accent, *m_currentEventSelection));
}

void NotationView::slotTransformsAddTenuto()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (Tenuto, *m_currentEventSelection));
}

void NotationView::slotTransformsAddStaccato()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (Staccato, *m_currentEventSelection));
}

void NotationView::slotTransformsAddSforzando()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (Sforzando, *m_currentEventSelection));
}

void NotationView::slotTransformsAddRinforzando()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (Rinforzando, *m_currentEventSelection));
}

void NotationView::slotTransformsAddTrill()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (Trill, *m_currentEventSelection));
}

void NotationView::slotTransformsAddTurn()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (Turn, *m_currentEventSelection));
}

void NotationView::slotTransformsAddPause()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (Pause, *m_currentEventSelection));
}

void NotationView::slotTransformsAddUpBow()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (UpBow, *m_currentEventSelection));
}

void NotationView::slotTransformsAddDownBow()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuAddMarkCommand
                            (DownBow, *m_currentEventSelection));
}

void NotationView::slotTransformsAddTextMark()
{
    if (m_currentEventSelection) {
	SimpleTextDialog *dialog = new SimpleTextDialog(this, 20);
	if (dialog->exec() == QDialog::Accepted) {
	    addCommandToHistory(new TransformsMenuAddTextMarkCommand
				(dialog->getText(), *m_currentEventSelection));
	}
	delete dialog;
    }
}

void NotationView::slotTransformsRemoveMarks()
{
    if (m_currentEventSelection)
        addCommandToHistory(new TransformsMenuRemoveMarksCommand
                            (*m_currentEventSelection));
}

void NotationView::slotTransformsAddTimeSignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX >= 0) {

	Rosegarden::Event *timeSigEvt = 0, *clefEvt = 0, *keyEvt = 0;
	Segment &segment = staff->getSegment();

	NotationElementList::iterator i = staff->getClosestElementToLayoutX
	    (layoutX, timeSigEvt, clefEvt, keyEvt, false, -1);

	timeT insertionTime = segment.getEndTime();
	if (i != staff->getViewElementList()->end()) {
	    insertionTime = (*i)->getAbsoluteTime();
	}

	TimeSignature timeSig;
	if (timeSigEvt) timeSig = TimeSignature(*timeSigEvt);

	TimeSignatureDialog *dialog = new TimeSignatureDialog(this, timeSig);
	if (dialog->exec() == QDialog::Accepted) {
	    addCommandToHistory
		(new AddTimeSignatureCommand
		 (m_staffs[m_currentStaff]->getSegment().getComposition(),
		  insertionTime, dialog->getTimeSignature()));
	}
	delete dialog;
    }
}			

void NotationView::slotTransformsAddKeySignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX >= 0) {

	Rosegarden::Event *timeSigEvt = 0, *clefEvt = 0, *keyEvt = 0;
	Segment &segment = staff->getSegment();

	NotationElementList::iterator i = staff->getClosestElementToLayoutX
	    (layoutX, timeSigEvt, clefEvt, keyEvt, false, -1);

	timeT insertionTime = segment.getEndTime();
	if (i != staff->getViewElementList()->end()) {
	    insertionTime = (*i)->getAbsoluteTime();
	}

	Rosegarden::Key key;
	if (keyEvt) key = Rosegarden::Key(*keyEvt);

	Rosegarden::Clef clef;
	if (clefEvt) clef = Rosegarden::Clef(*clefEvt);

	KeySignatureDialog *dialog =
	    new KeySignatureDialog(this, m_notePixmapFactory, clef, key);

	if (dialog->exec() == QDialog::Accepted &&
	    dialog->isValid()) {

	    KeySignatureDialog::ConversionType conversion =
		dialog->getConversionType();

	    addCommandToHistory
		(new KeyInsertionCommand
		 (m_staffs[m_currentStaff]->getSegment(),
		  insertionTime, dialog->getKey(),
		  conversion == KeySignatureDialog::Convert,
		  conversion == KeySignatureDialog::Transpose));
	}

	delete dialog;
    }
}			


void NotationView::slotDebugDump()
{
    if (m_currentEventSelection) {
	EventSelection::eventcontainer &ec = m_currentEventSelection->getSegmentEvents();
	int n = 0;
	for (EventSelection::eventcontainer::iterator i = ec.begin();
	     i != ec.end(); ++i) {
	    cerr << "\n" << n++ << " [" << (*i) << "]" << endl;
	    (*i)->dump(cerr);
	}
    }
}


void
NotationView::slotSetPointerPosition(timeT time)
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

    update();
}

void
NotationView::slotSetInsertCursorPosition(timeT time)
{
    //!!! For now.  Probably unlike slotSetPointerPosition this one
    // should snap to the nearest event.

    m_staffs[m_currentStaff]->setInsertCursorPosition(*m_hlayout, time);
    update();
}

    


//////////////////////////////////////////////////////////////////////

//----------------------------------------
// Notes
//----------------------------------------

void NotationView::slotBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotBreve()\n";
    setCurrentSelectedNote("breve", false, Note::Breve);
}

void NotationView::slotWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotWhole()\n";
    setCurrentSelectedNote("semibreve", false, Note::WholeNote);
}

void NotationView::slotHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotHalf()\n";
    setCurrentSelectedNote("minim", false, Note::HalfNote);
}

void NotationView::slotQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotQuarter()\n";
    setCurrentSelectedNote("crotchet", false, Note::QuarterNote);
}

void NotationView::slot8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot8th()\n";
    setCurrentSelectedNote("quaver", false, Note::EighthNote);
}

void NotationView::slot16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot16th()\n";
    setCurrentSelectedNote("semiquaver", false, Note::SixteenthNote);
}

void NotationView::slot32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot32nd()\n";
    setCurrentSelectedNote("demisemi", false, Note::ThirtySecondNote);
}

void NotationView::slot64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slot64th()\n";
    setCurrentSelectedNote("hemidemisemi", false, Note::SixtyFourthNote);
}

void NotationView::slotDottedBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedBreve()\n";
    setCurrentSelectedNote("dotted-breve", false, Note::Breve, 1);
}

void NotationView::slotDottedWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedWhole()\n";
    setCurrentSelectedNote("dotted-semibreve", false, Note::WholeNote, 1);
}

void NotationView::slotDottedHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedHalf()\n";
    setCurrentSelectedNote("dotted-minim", false, Note::HalfNote, 1);
}

void NotationView::slotDottedQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedQuarter()\n";
    setCurrentSelectedNote("dotted-crotchet", false, Note::QuarterNote, 1);
}

void NotationView::slotDotted8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted8th()\n";
    setCurrentSelectedNote("dotted-quaver", false, Note::EighthNote, 1);
}

void NotationView::slotDotted16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted16th()\n";
    setCurrentSelectedNote("dotted-semiquaver", false, Note::SixteenthNote, 1);
}

void NotationView::slotDotted32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted32nd()\n";
    setCurrentSelectedNote("dotted-demisemi", false, Note::ThirtySecondNote, 1);
}

void NotationView::slotDotted64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDotted64th()\n";
    setCurrentSelectedNote("dotted-hemidemisemi", false, Note::SixtyFourthNote, 1);
}

//----------------------------------------
// Rests
//----------------------------------------

void NotationView::slotRBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRBreve()\n";
    setCurrentSelectedNote("rest-breve", true, Note::Breve);
}

void NotationView::slotRWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRWhole()\n";
    setCurrentSelectedNote("rest-semibreve", true, Note::WholeNote);
}

void NotationView::slotRHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRHalf()\n";
    setCurrentSelectedNote("rest-minim", true, Note::HalfNote);
}

void NotationView::slotRQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotRQuarter()\n";
    setCurrentSelectedNote("rest-crotchet", true, Note::QuarterNote);
}

void NotationView::slotR8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR8th()\n";
    setCurrentSelectedNote("rest-quaver", true, Note::EighthNote);
}

void NotationView::slotR16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR16th()\n";
    setCurrentSelectedNote("rest-semiquaver", true, Note::SixteenthNote);
}

void NotationView::slotR32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR32nd()\n";
    setCurrentSelectedNote("rest-demisemi", true, Note::ThirtySecondNote);
}

void NotationView::slotR64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotR64th()\n";
    setCurrentSelectedNote("rest-hemidemisemi", true, Note::SixtyFourthNote);
}

void NotationView::slotDottedRBreve()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRBreve()\n";
    setCurrentSelectedNote("dotted-rest-breve", true, Note::Breve, 1);
}

void NotationView::slotDottedRWhole()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRWhole()\n";
    setCurrentSelectedNote("dotted-rest-semibreve", true, Note::WholeNote, 1);
}

void NotationView::slotDottedRHalf()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRHalf()\n";
    setCurrentSelectedNote("dotted-rest-minim", true, Note::HalfNote, 1);
}

void NotationView::slotDottedRQuarter()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedRQuarter()\n";
    setCurrentSelectedNote("dotted-rest-crotchet", true, Note::QuarterNote, 1);
}

void NotationView::slotDottedR8th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR8th()\n";
    setCurrentSelectedNote("dotted-rest-quaver", true, Note::EighthNote, 1);
}

void NotationView::slotDottedR16th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR16th()\n";
    setCurrentSelectedNote("dotted-rest-semiquaver", true, Note::SixteenthNote, 1);
}

void NotationView::slotDottedR32nd()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR32nd()\n";
    setCurrentSelectedNote("dotted-rest-demisemi", true, Note::ThirtySecondNote, 1);
}

void NotationView::slotDottedR64th()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDottedR64th()\n";
    setCurrentSelectedNote("dotted-rest-hemidemisemi", true, Note::SixtyFourthNote, 1);
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

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Clef::Treble);
}

void NotationView::slotTenorClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Clef::Tenor);
}

void NotationView::slotAltoClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Clef::Alto);
}

void NotationView::slotBassClef()
{
    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass"));
    setTool(m_toolBox->getTool(ClefInserter::ToolName));

    dynamic_cast<ClefInserter*>(m_tool)->setClef(Clef::Bass);
}


//----------------------------------------
// Time sigs.
//----------------------------------------
// TODO: time sig :-)

//----------------------------------------
// Edition Tools
//----------------------------------------

void NotationView::slotEraseSelected()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotEraseSelected()\n";
    setTool(m_toolBox->getTool(NotationEraser::ToolName));
}

void NotationView::slotSelectSelected()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotSelectSelected()\n";
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
    if (m_hlayout->getPageMode()) return;
    m_chordNamesVisible = !m_chordNamesVisible;

    if (!m_chordNamesVisible) {
	m_chordNameRuler->hide();
    } else {
	m_chordNameRuler->show();
    }
}

//----------------------------------------------------------------------

void NotationView::slotItemPressed(int height, int staffNo,
                               QMouseEvent* e,
                               NotationElement* el)
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotItemPressed(height = "
                         << height << ", staffNo = " << staffNo
                         << ")\n";

    ButtonState btnState = e->state();

    if (btnState & ShiftButton) { // on shift-click, set cursor position

	unsigned int staffNo;
	for (staffNo = 0; staffNo < m_staffs.size(); ++staffNo) {
	    if (m_staffs[staffNo]->containsCanvasY((int)e->y())) break;
	}

	if (staffNo < m_staffs.size()) {
	    
	    if (m_currentStaff != signed(staffNo)) {
		m_staffs[m_currentStaff]->setCurrent(false);
		m_currentStaff = staffNo;
		m_staffs[m_currentStaff]->setCurrent(true, (int)e->y());
	    }

	    m_staffs[m_currentStaff]->setInsertCursorPosition
		(e->x(), (int)e->y());
	}

	update();

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
        update();
    }
    else 
        m_tool->handleMouseMove(0, 0, // unknown time and height
                                e);
}

void NotationView::slotMouseReleased(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        update();
    }
    else
        m_tool->handleMouseRelease(0, 0, // unknown time and height
                                   e);
}


NotationStaff *
NotationView::getStaffForCanvasY(int y) const
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (m_staffs[i]->containsCanvasY(y)) return m_staffs[i];
    }
    return 0;
}

void NotationView::update()
{
    canvas()->update();
}

void NotationView::print(QPainter* printpainter)
{
    getCanvasView()->print(printpainter);
}


void NotationView::refreshSegment(Segment *segment,
				  timeT startTime, timeT endTime)
{
    START_TIMING;

    emit usedSelection(); //!!! hardly right

    if (segment) {
        NotationStaff *staff = getStaff(*segment);
        if (staff) applyLayout(staff->getId());
    } else {
        applyLayout();
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        Segment *ssegment = &m_staffs[i]->getSegment();
        bool thisStaff = (ssegment == segment || segment == 0);

        //        if (thisStaff && (segment != 0)) applyLayout(i);

        NotationElementList *notes = m_staffs[i]->getViewElementList();
        NotationElementList::iterator starti = notes->begin();
        NotationElementList::iterator endi = notes->end();

        timeT barStartTime = -1, barEndTime = -1;
        
	barStartTime = ssegment->getBarStartForTime(startTime);
        starti = notes->findTime(barStartTime);
	//!!!???starti = notes->findTime(startTime);

	barEndTime = ssegment->getBarEndForTime(endTime);
	endi = notes->findTime(barEndTime);
	//!!!???endi = notes->findTime(endTime);

        kdDebug(KDEBUG_AREA) << "NotationView::refreshSegment: "
                             << "start = " << startTime << ", end = " << endTime << ", barStart = " << barStartTime << ", barEnd = " << barEndTime << endl;

        if (thisStaff) {
            m_staffs[i]->renderElements(starti, endi);
        }
        m_staffs[i]->positionElements(barStartTime, barEndTime);
    }

    PRINT_ELAPSED("NotationView::refreshSegment (without update/GC)");

    update();
    PixmapArrayGC::deleteAll();

    Event::dumpStats(cerr);

    PRINT_ELAPSED("NotationView::refreshSegment (including update/GC)");
}


void NotationView::readjustCanvasSize()
{
    START_TIMING;

    double maxWidth = 0.0;
    int maxHeight = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        NotationStaff &staff = *m_staffs[i];

        staff.sizeStaff(*m_hlayout);

        if (staff.getTotalWidth() + staff.getX() > maxWidth) {
            maxWidth = staff.getTotalWidth() + staff.getX() + 1;
        }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
            maxHeight = staff.getTotalHeight() + staff.getY() + 1;
        }

	PRINT_ELAPSED("NotationView::readjustCanvasSize checkpoint");
    }

    // now get the EditView to do the biz
    readjustViewSize(QSize(int(maxWidth), maxHeight));

    PRINT_ELAPSED("NotationView::readjustCanvasSize total");
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

