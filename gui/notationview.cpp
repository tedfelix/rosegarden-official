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

#include <kstdaction.h>
#include <kapp.h>

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
#include "barbuttons.h"
#include "loopruler.h"

#include "rosedebug.h"

#include "NotationTypes.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "Quantizer.h"
#include "Selection.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "segmentcommands.h"
#include "dialogs.h"

#include "chordnameruler.h"

#include "CompositionTimeSliceAdapter.h"
#include "AnalysisTypes.h"


using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::Bool;
using Rosegarden::String;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::EventSelection;
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

class NoteActionData
{
public:
    NoteActionData();
    NoteActionData(const QString& _title,
               const char* _actionName,
               const char* _pixmapName,
               bool _rest,
               Note::Type _noteType,
               int _dots);
    
    QString title;
    const char* actionName;
    const char* pixmapName;
    bool rest;
    Note::Type noteType;
    int dots;
};

NoteActionData::NoteActionData()
    : actionName(0),
      pixmapName(0),
      rest(false),
      noteType(0),
      dots(0)
{
}


NoteActionData::NoteActionData(const QString& _title,
                       const char* _actionName,
                       const char* _pixmapName,
                       bool _rest,
                       Note::Type _noteType,
                       int _dots)
    : title(i18n(_title)),
      actionName(_actionName),
      pixmapName(_pixmapName),
      rest(_rest),
      noteType(_noteType),
      dots(_dots)
{
}


//////////////////////////////////////////////////////////////////////

NotationView::NotationView(RosegardenGUIDoc *doc,
                           vector<Segment *> segments,
                           QWidget *parent) :
    EditView(doc, segments, false, parent, "notationview"),
    m_currentEventSelection(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_lastFinishingStaff(-1),
    m_fontName(NotePixmapFactory::getDefaultFont()),
    m_fontSize(NotePixmapFactory::getDefaultSize(m_fontName)),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(&doc->getComposition(), m_notePixmapFactory),
    m_vlayout(&doc->getComposition()),
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_tupletMode(false),
    m_fontSizeSlider(0),
    m_selectDefaultNote(0)
{
    initNoteActionDataMap(); // does something only the 1st time it's called
    
    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    kdDebug(KDEBUG_AREA) << "NotationView ctor" << endl;

    setupActions();

    initFontToolbar
	(m_document->getComposition().getLegatoQuantizer()->getUnit());
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);
    
    setCanvasView(new NotationCanvasView(*this, m_horizontalScrollBar,
                                         tCanvas, getCentralFrame()));

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


    m_topBarButtons = new BarButtons(&m_hlayout, 25,
                                     false, getCentralFrame());
    setTopBarButtons(m_topBarButtons);

    m_topBarButtons->getLoopRuler()->setBackgroundColor
	(RosegardenGUIColours::InsertCursorRuler);

    m_chordNameRuler = new ChordNameRuler
	(&m_hlayout, &doc->getComposition(), 20, getCentralFrame());
    addRuler(m_chordNameRuler);
    m_chordNameRuler->hide();
    m_chordNamesVisible = false;

    m_bottomBarButtons = new BarButtons(&m_hlayout, 25,
                                        true, getCentralFrame());
    setBottomBarButtons(m_bottomBarButtons);
    
    show();
    kapp->processEvents();

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], i,
                                             false, width() - 50,
                                             m_fontName, m_fontSize));
    }

    positionStaffs();
    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    m_hlayout.setPageMode(false);
    m_hlayout.setPageWidth(width() - 50);

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply score layout"));
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            
            m_staffs[i]->renderAllElements();
            m_staffs[i]->positionAllElements();
            m_staffs[i]->getSegment().getRefreshStatus(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
	    canvas()->update();

        }
    }

    //
    // Connect signals
    //

    QObject::connect
	(m_topBarButtons->getLoopRuler(),
	 SIGNAL(setPointerPosition(Rosegarden::timeT)),
	 this, SLOT(slotSetInsertCursorPosition(Rosegarden::timeT)));

    m_bottomBarButtons->connectRulerToDocPointer(doc);

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

    m_selectDefaultNote->activate();
}

NotationView::~NotationView()
{
    kdDebug(KDEBUG_AREA) << "-> ~NotationView()\n";

    saveOptions();

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
 
    for(NoteActionDataMap::Iterator actionDataIter = m_noteActionDataMap->begin();
        actionDataIter != m_noteActionDataMap->end();
        ++actionDataIter) {

        NoteActionData noteActionData = *actionDataIter;
        
	string iconName = Note(noteActionData.noteType).getReferenceName();
        icon = QIconSet
	    (m_toolbarNotePixmapFactory.makeToolbarPixmap(iconName.c_str()));
        noteAction = new KRadioAction(noteActionData.title, icon, 0, this,
                                      SLOT(slotNoteAction()),
                                      actionCollection(), noteActionData.actionName);
        noteAction->setExclusiveGroup("notes");

        if (QString(noteActionData.actionName) == "quarter")
            m_selectDefaultNote = noteAction; // quarter is the default selected note

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
    noteAction = new KRadioAction(i18n("&Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor"));
    noteAction = new KRadioAction(i18n("Te&nor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto"));
    noteAction = new KRadioAction(i18n("&Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass"));
    noteAction = new KRadioAction(i18n("&Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");


    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("text"));
    noteAction = new KRadioAction(i18n("&Text"), icon, 0, this,
                                  SLOT(slotText()),
                                  actionCollection(), "text");
    noteAction->setExclusiveGroup("notes");


    //
    // Edition tools (eraser, selector...)
    //
    noteAction = new KRadioAction(i18n("&Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap("select"));
    noteAction = new KRadioAction(i18n("&Select"), icon, 0,
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

    new KAction(i18n("P&aste..."), 0, this,
		SLOT(slotEditGeneralPaste()), actionCollection(),
		"general_paste");

    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

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

    new KToggleAction
	(i18n("Label &Chords"), 0, this, SLOT(slotLabelChords()),
	 actionCollection(), "label_chords");

    // setup Group menu
    new KAction(GroupMenuBeamCommand::name(), 0, this,
                SLOT(slotGroupBeam()), actionCollection(), "beam");

    new KAction(GroupMenuAutoBeamCommand::name(), 0, this,
                SLOT(slotGroupAutoBeam()), actionCollection(), "auto_beam");

    new KAction(GroupMenuBreakCommand::name(), 0, this,
                SLOT(slotGroupBreak()), actionCollection(), "break_group");

    new KAction(GroupMenuTupletCommand::name(true), 0, this,
		SLOT(slotGroupSimpleTuplet()), actionCollection(), "simple_tuplet");

    new KAction(GroupMenuTupletCommand::name(false), 0, this,
		SLOT(slotGroupGeneralTuplet()), actionCollection(), "tuplet");

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

        toolbarAction->setChecked(true);
    }
    
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()), actionCollection());

    KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

    createGUI("notation.rc");
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
    kdDebug(KDEBUG_AREA) << "NotationView::setViewSize: resizing canvas from "
			 << canvas()->width() << "x" << canvas()->height()
			 << " to " << s.width() << "x" << s.height() << endl;

    canvas()->resize(s.width(), s.height());
}

void
NotationView::slotChangeStretch(int n)
{
    vector<double> spacings = m_hlayout.getAvailableSpacings();
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

    m_document->getComposition().setLegatoQuantizerDuration
	(m_legatoDurations[n]);
    
    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->positionAllElements();
    }

    updateView();
}


void
NotationView::slotChangeFont(const QString &newName)
{
    kdDebug(KDEBUG_AREA) << "changeFont: " << newName << endl;
    slotChangeFont(string(newName.latin1()));
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

    m_hlayout.setPageMode(pageMode);
    m_hlayout.setPageWidth(width() - 50);
    
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

    updateView();
}   


bool NotationView::applyLayout(int staffNo, timeT startTime, timeT endTime)
{
    START_TIMING;
    unsigned int i;

    for (i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo) continue;

        m_hlayout.resetStaff(*m_staffs[i], startTime, endTime);
        m_vlayout.resetStaff(*m_staffs[i], startTime, endTime);

        m_hlayout.scanStaff(*m_staffs[i], startTime, endTime);
        m_vlayout.scanStaff(*m_staffs[i], startTime, endTime);
    }

    m_hlayout.finishLayout(startTime, endTime);
    m_vlayout.finishLayout(startTime, endTime);

    // find the last finishing staff for future use

    timeT lastFinishingStaffEndTime = 0;
    bool haveEndTime = false;
    m_lastFinishingStaff = -1;

    timeT firstStartingStaffStartTime = 0;
    bool haveStartTime = false;
    int firstStartingStaff = -1;

    for (i = 0; i < m_staffs.size(); ++i) {

	timeT thisStartTime = m_staffs[i]->getSegment().getStartTime();
	if (thisStartTime < firstStartingStaffStartTime || !haveStartTime) {
	    firstStartingStaffStartTime = thisStartTime;
	    haveStartTime = true;
	    firstStartingStaff = i;
	}

        timeT thisEndTime = m_staffs[i]->getSegment().getEndTime();
        if (thisEndTime > lastFinishingStaffEndTime || !haveEndTime) {
            lastFinishingStaffEndTime = thisEndTime;
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

void NotationView::setCurrentSelectedNote(NoteActionData noteAction)
{
    setCurrentSelectedNote(noteAction.pixmapName,
                           noteAction.rest,
                           noteAction.noteType,
                           noteAction.dots);
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

    updateView();
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

void NotationView::previewNote(int staffNo, Rosegarden::timeT time,
			       int pitch, int height,
			       const Rosegarden::Note &note)
{ 
    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Studio &studio = m_document->getStudio();

    Rosegarden::Track *track = comp.getTrackByIndex
	(m_staffs[staffNo]->getSegment().getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0) return;

    // Send out note of half second duration
    //
    Rosegarden::MappedEvent *mE = 
        new Rosegarden::MappedEvent(ins->getID(),
                                    Rosegarden::MappedEvent::MidiNoteOneShot,
                                    pitch,
                                    Rosegarden::MidiMaxValue,
                                    Rosegarden::RealTime(0,0),
                                    Rosegarden::RealTime(0, 500000),
                                    Rosegarden::RealTime(0, 0));

    emit notePlayed(mE);

    //!!! and the rest
}


void NotationView::setNotePixmapFactory(NotePixmapFactory* f)
{
    delete m_notePixmapFactory;
    m_notePixmapFactory = f;
}


NotationCanvasView* NotationView::getCanvasView()
{
    return dynamic_cast<NotationCanvasView *>(m_canvasView);
}


timeT
NotationView::getInsertionTime()
{
    Rosegarden::Event *clef, *key;
    return getInsertionTime(clef, key);
}


timeT
NotationView::getInsertionTime(Event *&clefEvt,
			       Event *&keyEvt)
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX < 0) layoutX = 0;

    NotationElementList::iterator i = staff->getClosestElementToLayoutX
	(layoutX, clefEvt, keyEvt, false, -1);

    timeT insertionTime = segment.getEndTime();
    if (i != staff->getViewElementList()->end()) {
	insertionTime = (*i)->getAbsoluteTime();
    }

    return insertionTime;
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

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

void NotationView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
					m_document->getClipboard()));

    emit usedSelection();
}

void NotationView::slotEditPaste()
{
    if (m_document->getClipboard()->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }
    if (!m_document->getClipboard()->isSingleSegment()) {
        slotStatusHelpMsg(i18n("Can't paste multiple Segments into one"));
        return;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    
    // Paste at cursor position
    //
    PasteEventsCommand *command = new PasteEventsCommand
	(segment, m_document->getClipboard(), getInsertionTime());

    if (!command->isPossible()) {
	slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    } else {
	addCommandToHistory(command);
    }
}

void NotationView::slotEditGeneralPaste()
{
    if (m_document->getClipboard()->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }

    slotStatusHelpMsg(i18n("Inserting clipboard contents..."));

    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    
    PasteNotationDialog *dialog = new PasteNotationDialog
	(this, PasteEventsCommand::getDefaultPasteType());

    if (dialog->exec() == QDialog::Accepted) {

	PasteEventsCommand::PasteType type = dialog->getPasteType();
	if (dialog->setAsDefault()) {
	    PasteEventsCommand::setDefaultPasteType(type);
	}

	PasteEventsCommand *command = new PasteEventsCommand
	    (segment, m_document->getClipboard(), getInsertionTime(), type);

	if (!command->isPossible()) {
	    slotStatusHelpMsg(i18n("Couldn't paste at this point"));
	} else {
	    addCommandToHistory(command);
	}
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

    NotationSelector *selector = dynamic_cast<NotationSelector *>
	(m_toolBox->getTool(NotationSelector::ToolName));

    if (m_currentEventSelection &&
	selector && selector->isRectangleVisible()) {

	t = m_currentEventSelection->getBeginTime();

	timeT duration = m_currentEventSelection->getTotalDuration();
	Note::Type unitType =
	    Note::getNearestNote(duration / 3, 0).getNoteType();
	unit = Note(unitType).getDuration();

	if (!simple) {
	    TupletDialog *dialog = new TupletDialog(this, unitType, duration);
	    if (dialog->exec() != QDialog::Accepted) return;
	    unit = Note(dialog->getUnitType()).getDuration();
	    tupled = dialog->getTupledCount();
	    untupled = dialog->getUntupledCount();
	}

	kdDebug(KDEBUG_AREA) << "Got time and unit from selection; they're " << t << " and " << unit << " respectively"<< endl;

	segment = &m_currentEventSelection->getSegment();

    } else {

	t = getInsertionTime();
	kdDebug(KDEBUG_AREA) << "Got insertion time; it's " << t << endl;

	NoteInserter *currentInserter = dynamic_cast<NoteInserter *>
	    (m_toolBox->getTool(NoteInserter::ToolName));

	Note::Type unitType;

	if (currentInserter) {
	    unitType = currentInserter->getCurrentNote().getNoteType();
	} else {
	    unitType = Note::Quaver;
	}

	unit = Note(unitType).getDuration();
	kdDebug(KDEBUG_AREA) << "Got unit; it's " << unit
			     << endl;

	if (!simple) {
	    TupletDialog *dialog = new TupletDialog(this, unitType);
	    if (dialog->exec() != QDialog::Accepted) return;
	    unit = Note(dialog->getUnitType()).getDuration();
	    tupled = dialog->getTupledCount();
	    untupled = dialog->getUntupledCount();
	}

	segment = &m_staffs[m_currentStaff]->getSegment();
    }

    addCommandToHistory(new GroupMenuTupletCommand
			(*segment, t, unit, untupled, tupled));
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

	Rosegarden::Event *clefEvt = 0, *keyEvt = 0;
	Segment &segment = staff->getSegment();
	Rosegarden::Composition &composition = *segment.getComposition();
	timeT insertionTime = getInsertionTime(clefEvt, keyEvt);

	int barNo = composition.getBarNumber(insertionTime);
	bool atStartOfBar = (insertionTime == composition.getBarStart(barNo));
	TimeSignature timeSig = composition.getTimeSignatureAt(insertionTime);

	TimeSignatureDialog *dialog = new TimeSignatureDialog
	    (this, timeSig, barNo, atStartOfBar);

	if (dialog->exec() == QDialog::Accepted) {

	    TimeSignatureDialog::Location location = dialog->getLocation();
	    if (location == TimeSignatureDialog::StartOfBar) {
		insertionTime = composition.getBarStartForTime(insertionTime);
	    }

	    if (dialog->shouldNormalizeRests()) {
		
		addCommandToHistory(new AddTimeSignatureAndNormalizeCommand
				    (segment.getComposition(),
				     insertionTime,
				     dialog->getTimeSignature()));

	    } else {

		addCommandToHistory(new AddTimeSignatureCommand
				    (segment.getComposition(),
				     insertionTime,
				     dialog->getTimeSignature()));
	    }
	}

	delete dialog;
    }
}			

void NotationView::slotTransformsAddKeySignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX >= 0) {

	Rosegarden::Event *clefEvt = 0, *keyEvt = 0;
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
	    new KeySignatureDialog(this, m_notePixmapFactory, clef, key);

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
	if (barNo < m_hlayout.getFirstVisibleBarOnStaff(*m_staffs[i]) ||
	    barNo > m_hlayout. getLastVisibleBarOnStaff(*m_staffs[i])) {
	    m_staffs[i]->hidePointer();
	} else {
	    m_staffs[i]->setPointerPosition(m_hlayout, time);
	}
    }

    updateView();
}

void
NotationView::slotSetInsertCursorPosition(timeT time)
{
    //!!! For now.  Probably unlike slotSetPointerPosition this one
    // should snap to the nearest event.

    m_staffs[m_currentStaff]->setInsertCursorPosition(m_hlayout, time);
    updateView();
}

    


//////////////////////////////////////////////////////////////////////

//----------------------------------------
// Notes & Rests
//----------------------------------------

void NotationView::slotNoteAction()
{
    const QObject* sigSender = sender();

    NoteActionDataMap::Iterator noteAct = m_noteActionDataMap->find(sigSender->name());
    
    if (noteAct != m_noteActionDataMap->end())
        setCurrentSelectedNote(*noteAct);
    else
        kdDebug(KDEBUG_AREA) << "NotationView::slotNoteAction() : couldn't find NoteActionData named '"
                             << sigSender->name() << "'\n";
}

void NotationView::slotToggleTriplet()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotToggleTriplet()\n";
    
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
    if (m_hlayout.getPageMode()) return;
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

	updateView();

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
        updateView();
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

void NotationView::updateView()
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
        if (staff) applyLayout(staff->getId(), startTime, endTime);
    } else {
        applyLayout(-1, startTime, endTime);
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        Segment *ssegment = &m_staffs[i]->getSegment();
        bool thisStaff = (ssegment == segment || segment == 0);

        NotationElementList *notes = m_staffs[i]->getViewElementList();
        NotationElementList::iterator starti = notes->begin();
        NotationElementList::iterator endi = notes->end();

        timeT barStartTime = ssegment->getStartTime(),
	      barEndTime   = ssegment->getEndTime();

        if (startTime != endTime) {
	    barStartTime = ssegment->getBarStartForTime(startTime);
	    barEndTime = ssegment->getBarEndForTime(endTime);
	    starti = notes->findTime(barStartTime);
	    endi = notes->findTime(barEndTime);
	}

        kdDebug(KDEBUG_AREA) << "NotationView::refreshSegment: "
                             << "start = " << startTime << ", end = " << endTime << ", barStart = " << barStartTime << ", barEnd = " << barEndTime << endl;

        if (thisStaff) {
            m_staffs[i]->renderElements(starti, endi);
        }
        m_staffs[i]->positionElements(barStartTime, barEndTime);
    }

    PRINT_ELAPSED("NotationView::refreshSegment (without update/GC)");

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

        staff.sizeStaff(m_hlayout);

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

void NotationView::initNoteActionDataMap()
{
    static bool called = false;
    
    if (called) return;

    called = true;

    m_noteActionDataMap = new NoteActionDataMap;

    // Regular Notes
    //
    m_noteActionDataMap->insert("breve",      NoteActionData("Breve",   "breve",      "breve",        false, Note::Breve, 0));
    m_noteActionDataMap->insert("whole_note", NoteActionData("Whole",   "whole_note", "semibreve",    false, Note::WholeNote, 0));
    m_noteActionDataMap->insert("half",       NoteActionData("Half",    "half",       "minim",        false, Note::HalfNote, 0));
    m_noteActionDataMap->insert("quarter",    NoteActionData("Quarter", "quarter",    "crotchet",     false, Note::QuarterNote, 0));
    m_noteActionDataMap->insert("8th",        NoteActionData("8th",     "8th",        "quaver",       false, Note::EighthNote, 0));
    m_noteActionDataMap->insert("16th",       NoteActionData("16th",    "16th",       "semiquaver",   false, Note::SixteenthNote, 0));
    m_noteActionDataMap->insert("32nd",       NoteActionData("32nd",    "32nd",       "demisemi",     false, Note::ThirtySecondNote, 0));
    m_noteActionDataMap->insert("64th",       NoteActionData("64th",    "64th",       "hemidemisemi", false, Note::SixtyFourthNote, 0));

    // Dotted Notes
    //
    m_noteActionDataMap->insert("dotted_breve",      NoteActionData("Dotted Breve",   "dotted_breve",      "dotted-breve",        false, Note::Breve, 1));
    m_noteActionDataMap->insert("dotted_whole_note", NoteActionData("Dotted Whole",   "dotted_whole_note", "dotted-semibreve",    false, Note::WholeNote, 1));
    m_noteActionDataMap->insert("dotted_half",       NoteActionData("Dotted Half",    "dotted_half",       "dotted-minim",        false, Note::HalfNote, 1));
    m_noteActionDataMap->insert("dotted_quarter",    NoteActionData("Dotted Quarter", "dotted_quarter",    "dotted-crotchet",     false, Note::QuarterNote, 1));
    m_noteActionDataMap->insert("dotted_8th",        NoteActionData("Dotted 8th",     "dotted_8th",        "dotted-quaver",       false, Note::EighthNote, 1));
    m_noteActionDataMap->insert("dotted_16th",       NoteActionData("Dotted 16th",    "dotted_16th",       "dotted-semiquaver",   false, Note::SixteenthNote, 1));
    m_noteActionDataMap->insert("dotted_32nd",       NoteActionData("Dotted 32nd",    "dotted_32nd",       "dotted-demisemi",     false, Note::ThirtySecondNote, 1));
    m_noteActionDataMap->insert("dotted_64th",       NoteActionData("Dotted 64th",    "dotted_64th",       "dotted-hemidemisemi", false, Note::SixtyFourthNote, 1));


    // Rests
    //
    m_noteActionDataMap->insert("breve_rest",      NoteActionData("Breve Rest",   "breve_rest",      "rest-breve",        true, Note::Breve, 0));
    m_noteActionDataMap->insert("whole_note_rest", NoteActionData("Whole Rest",   "whole_note_rest", "rest-semibreve",    true, Note::WholeNote, 0));
    m_noteActionDataMap->insert("half_rest",       NoteActionData("Half Rest",    "half_rest",       "rest-minim",        true, Note::HalfNote, 0));
    m_noteActionDataMap->insert("quarter_rest",    NoteActionData("Quarter Rest", "quarter_rest",    "rest-crotchet",     true, Note::QuarterNote, 0));
    m_noteActionDataMap->insert("8th_rest",        NoteActionData("8th Rest",     "8th_rest",        "rest-quaver",       true, Note::EighthNote, 0));
    m_noteActionDataMap->insert("16th_rest",       NoteActionData("16th Rest",    "16th_rest",       "rest-semiquaver",   true, Note::SixteenthNote, 0));
    m_noteActionDataMap->insert("32nd_rest",       NoteActionData("32nd Rest",    "32nd_rest",       "rest-demisemi",     true, Note::ThirtySecondNote, 0));
    m_noteActionDataMap->insert("64th_rest",       NoteActionData("64th Rest",    "64th_rest",       "rest-hemidemisemi", true, Note::SixtyFourthNote, 0));

    // Dotted rests
    //
    m_noteActionDataMap->insert("dotted_breve_rest",      NoteActionData("Dotted Breve Rest",   "dotted_breve_rest",      "dotted-rest-breve",        true, Note::Breve, 1));
    m_noteActionDataMap->insert("dotted_whole_note_rest", NoteActionData("Dotted Whole Rest",   "dotted_whole_note_rest", "dotted-rest-semibreve",    true, Note::WholeNote, 1));
    m_noteActionDataMap->insert("dotted_half_rest",       NoteActionData("Dotted Half Rest",    "dotted_half_rest",       "dotted-rest-minim",        true, Note::HalfNote, 1));
    m_noteActionDataMap->insert("dotted_quarter_rest",    NoteActionData("Dotted Quarter Rest", "dotted_quarter_rest",    "dotted-rest-crotchet",     true, Note::QuarterNote, 1));
    m_noteActionDataMap->insert("dotted_8th_rest",        NoteActionData("Dotted 8th Rest",     "dotted_8th_rest",        "dotted-rest-quaver",       true, Note::EighthNote, 1));
    m_noteActionDataMap->insert("dotted_16th_rest",       NoteActionData("Dotted 16th Rest",    "dotted_16th_rest",       "dotted-rest-semiquaver",   true, Note::SixteenthNote, 1));
    m_noteActionDataMap->insert("dotted_32nd_rest",       NoteActionData("Dotted 32nd Rest",    "dotted_32nd_rest",       "dotted-rest-demisemi",     true, Note::ThirtySecondNote, 1));
    m_noteActionDataMap->insert("dotted_64th_rest",       NoteActionData("Dotted 64th Rest",    "dotted_64th_rest",       "dotted-rest-hemidemisemi", true, Note::SixtyFourthNote, 1));
}

    
NotationView::NoteActionDataMap* NotationView::m_noteActionDataMap = 0;
