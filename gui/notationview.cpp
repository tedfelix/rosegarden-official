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
#include "Quantizer.h"
#include "Selection.h"

#include "notepixmapfactory.h"
#include "notestyle.h"
#include "notationtool.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "rosedebug.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "segmentcommands.h"
#include "widgets.h"
#include "chordnameruler.h"
#include "temporuler.h"

#include <qstring.h>
#include <qregexp.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kapp.h>

#include "qcanvassimplesprite.h"
#include "ktmpstatusmsg.h"

using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::EventSelection;
using Rosegarden::Quantizer;
using Rosegarden::timeT;
using Rosegarden::Mark;
using namespace Rosegarden::Marks;


class NoteActionData
{
public:
    NoteActionData();
    NoteActionData(const QString& _title,
		   QString _actionName,
		   QString _pixmapName,
		   int _keycode,
		   bool _rest,
		   Note::Type _noteType,
		   int _dots);
    
    QString title;
    QString actionName;
    QString pixmapName;
    int keycode;
    bool rest;
    Note::Type noteType;
    int dots;
};

NoteActionData::NoteActionData()
    : title(0),
      actionName(0),
      pixmapName(0),
      keycode(0),
      rest(false),
      noteType(0),
      dots(0)
{
}

NoteActionData::NoteActionData(const QString& _title,
			       QString _actionName,
			       QString _pixmapName,
			       int _keycode,
			       bool _rest,
			       Note::Type _noteType,
			       int _dots)
    : title(i18n(_title)),
      actionName(_actionName),
      pixmapName(_pixmapName),
      keycode(_keycode),
      rest(_rest),
      noteType(_noteType),
      dots(_dots)
{
}


class MarkActionData
{
public:
    MarkActionData() :
	title(0),
	actionName(0),
	pixmapName(0),
	keycode(0) { }

    MarkActionData(const QString &_title,
		   QString _actionName,
		   QString _pixmapName,
		   int _keycode,
		   Mark _mark) :
	title(_title),
	actionName(_actionName),
	pixmapName(_pixmapName),
	keycode(_keycode),
	mark(_mark) { }

    QString title;
    QString actionName;
    QString pixmapName;
    int keycode;
    Mark mark;
};




//////////////////////////////////////////////////////////////////////

NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           QWidget *parent,
			   bool showProgressive) :
    EditView(doc, segments, 1, parent, "notationview"),
    m_properties(getViewLocalPropertyPrefix()),
    m_currentEventSelection(0),
    m_legatoQuantizer(new Quantizer(Quantizer::RawEventData,
				    getViewLocalPropertyPrefix() + "Q",
				    Quantizer::LegatoQuantize)),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_lastFinishingStaff(-1),
    m_insertionTime(0),
    m_fontName(NotePixmapFactory::getDefaultFont()),
    m_fontSize(NotePixmapFactory::getDefaultSize(m_fontName)),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(&doc->getComposition(), m_notePixmapFactory,
	      m_legatoQuantizer, m_properties),
    m_vlayout(&doc->getComposition(),
	      m_legatoQuantizer, m_properties),
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_tupletMode(false),
    m_selectDefaultNote(0),
    m_fontCombo(0),
    m_fontSizeSlider(0),
    m_spacingSlider(0),
    m_smoothingSlider(0),
    m_fontSizeActionMenu(0),
    m_progressDlg(0),
    m_inhibitRefresh(true),
    m_documentDestroyed(false)
{
    initActionDataMaps(); // does something only the 1st time it's called
    
    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    kdDebug(KDEBUG_AREA) << "NotationView ctor" << endl;

    KConfig *config = kapp->config();
    config->setGroup("Notation Options");

    m_fontName = qstrtostr(config->readEntry
			   ("notefont",
			    strtoqstr(NotePixmapFactory::getDefaultFont())));

    m_fontSize = config->readUnsignedNumEntry
	((segments.size() > 1 ? "multistaffnotesize" : "singlestaffnotesize"),
	 NotePixmapFactory::getDefaultSize(m_fontName));

    setupActions();
    initFontToolbar(m_legatoQuantizer->getUnit());
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

    m_topBarButtons = new BarButtons(&m_hlayout, 20.0, 25,
                                     false, getCentralFrame());
    setTopBarButtons(m_topBarButtons);

    m_topBarButtons->getLoopRuler()->setBackgroundColor
	(RosegardenGUIColours::InsertCursorRuler);

    m_chordNameRuler = new ChordNameRuler
	(&m_hlayout, 0, 20.0, 20, getCentralFrame());
    addRuler(m_chordNameRuler);
    if (showProgressive) m_chordNameRuler->show();
    m_chordNamesVisible = true;

    m_tempoRuler = new TempoRuler
	(&m_hlayout, &doc->getComposition(),
	 20.0, 20, false, getCentralFrame());
    addRuler(m_tempoRuler);
    m_tempoRuler->hide();
    m_temposVisible = false;

    m_bottomBarButtons = new BarButtons(&m_hlayout, 20.0, 25,
                                        true, getCentralFrame());
    setBottomBarButtons(m_bottomBarButtons);

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], i,
					     m_legatoQuantizer,
					     m_properties, false, width() - 50,
                                             m_fontName, m_fontSize));
    }

    if (showProgressive) {
	show();
	m_progressDlg = new RosegardenProgressDialog
	    (i18n("Starting..."), 0, 100, this,
	     i18n("Notation progress"), true);
	m_progressDlg->setAutoClose(false);
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressDialog(m_progressDlg);
	}
	m_progressDlg->hide();
    }

    m_chordNameRuler->setComposition(&doc->getComposition());

    positionStaffs();
    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    m_hlayout.setPageMode(false);
    m_hlayout.setPageWidth(width() - 50);

    bool layoutApplied = applyLayout();
    if (!layoutApplied) {
	KMessageBox::sorry(0, i18n("Couldn't apply score layout"));
    } else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

            m_staffs[i]->renderAllElements();
            m_staffs[i]->positionAllElements();
            m_staffs[i]->getSegment().getRefreshStatus
		(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);

	    canvas()->update();
        }
    }

    if (showProgressive) {
	delete m_progressDlg;
	m_progressDlg = 0;
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressDialog(0);
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

    QObject::connect
	(doc, SIGNAL(destroyed()), this, SLOT(slotDocumentDestroyed()));

#ifdef RGKDE3
    QObject::connect
	(doc->getCommandHistory(), SIGNAL(commandExecuted()),
	 this, SLOT(slotTestClipboard()));

    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_multiple_staffs",
		 (m_staffs.size() > 1 ? KXMLGUIClient::StateNoReverse :
		                        KXMLGUIClient::StateReverse));
    slotTestClipboard();
#endif

    m_selectDefaultNote->activate();
    slotSetInsertCursorPosition(0);
    slotSetPointerPosition(doc->getComposition().getPosition());
    m_chordNameRuler->repaint();
    m_inhibitRefresh = false;
}

NotationView::~NotationView()
{
    kdDebug(KDEBUG_AREA) << "-> ~NotationView()\n";

    saveOptions();
    if (m_documentDestroyed) return;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	for (Segment::iterator j = m_staffs[i]->getSegment().begin();
	     j != m_staffs[i]->getSegment().end(); ++j) {
	    removeViewLocalProperties(*j);
	}
	delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    // Delete remaining canvas items.
    QCanvasItemList allItems = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = allItems.begin(); it != allItems.end(); ++it) delete *it;
    // delete canvas();

    kdDebug(KDEBUG_AREA) << "<- ~NotationView()\n";
}
    
void
NotationView::removeViewLocalProperties(Rosegarden::Event *e)
{
    Event::PropertyNames names(e->getPropertyNames());
    std::string prefix(getViewLocalPropertyPrefix());

    for (Event::PropertyNames::iterator i = names.begin();
	 i != names.end(); ++i) {
	if (i->getName().substr(0, prefix.size()) == prefix) {
	    e->unset(*i);
	}
    }
}
    

void NotationView::positionStaffs()
{
    Rosegarden::TrackId minTrack = 0, maxTrack = 0;
    bool haveMinTrack = false;
    typedef std::map<Rosegarden::TrackId, int> TrackIntMap;
    TrackIntMap trackHeights;
    TrackIntMap trackCoords;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	int height = m_staffs[i]->getHeightOfRow();
	Rosegarden::TrackId track = m_staffs[i]->getSegment().getTrack();

	TrackIntMap::iterator hi = trackHeights.find(track);
	if (hi == trackHeights.end()) {
	    trackHeights.insert(TrackIntMap::value_type(track, height));
	} else if (height > hi->second) {
	    hi->second = height;
	}
	
	if (track < minTrack || !haveMinTrack) {
	    minTrack = track;
	    haveMinTrack = true;
	}
	if (track > maxTrack) {
	    maxTrack = track;
	}
    }

    int accumulatedHeight = 0;
    for (Rosegarden::TrackId i = minTrack; i <= maxTrack; ++i) {
	TrackIntMap::iterator hi = trackHeights.find(i);
	if (hi != trackHeights.end()) {
	    trackCoords[i] = accumulatedHeight;
	    accumulatedHeight += hi->second;
	}
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	Rosegarden::TrackId track = m_staffs[i]->getSegment().getTrack();
        m_staffs[i]->setRowSpacing(accumulatedHeight + trackHeights[track] / 7);
        if (track < maxTrack) {
            m_staffs[i]->setConnectingLineLength(trackHeights[track]);
        }
        
        m_staffs[i]->setX(20);
	m_staffs[i]->setY(trackCoords[track]);
    }
}    

void NotationView::saveOptions()
{
    m_config->setGroup("Notation Options");
    EditView::saveOptions();

    m_config->writeEntry("Show Notes Toolbar",       getToggleAction("show_notes_toolbar")->isChecked());
    m_config->writeEntry("Show Rests Toolbar",       getToggleAction("show_rests_toolbar")->isChecked());
    m_config->writeEntry("Show Clefs Toolbar",       getToggleAction("show_clefs_toolbar")->isChecked());
    m_config->writeEntry("Show Font Toolbar",        getToggleAction("show_font_toolbar")->isChecked());
    m_config->writeEntry("Show Accidentals Toolbar", getToggleAction("show_accidentals_toolbar")->isChecked());

    toolBar("notesToolBar")->saveSettings(m_config, "Notation Options notesToolBar");
    toolBar("restsToolBar")->saveSettings(m_config, "Notation Options restsToolBar");
    toolBar("clefsToolBar")->saveSettings(m_config, "Notation Options clefsToolBar");
    toolBar("fontToolBar")->saveSettings(m_config, "Notation Options fontToolBar");
    toolBar("accidentalsToolBar")->saveSettings(m_config, "Notation Options accidentalsToolBar");
}

void NotationView::readOptions()
{
    m_config->setGroup("Notation Options");
    EditView::readOptions();

    bool opt;

    opt = m_config->readBoolEntry("Show Notes Toolbar", true);
    getToggleAction("show_notes_toolbar")->setChecked(opt);
    toggleNamedToolBar("notesToolBar", &opt);

    opt = m_config->readBoolEntry("Show Rests Toolbar", true);
    getToggleAction("show_rests_toolbar")->setChecked(opt);
    toggleNamedToolBar("restsToolBar", &opt);

    opt = m_config->readBoolEntry("Show Clefs Toolbar", false);
    getToggleAction("show_clefs_toolbar")->setChecked(opt);
    toggleNamedToolBar("clefsToolBar", &opt);

    opt = m_config->readBoolEntry("Show Font Toolbar", true);
    getToggleAction("show_font_toolbar")->setChecked(opt);
    toggleNamedToolBar("fontToolBar", &opt);

    opt = m_config->readBoolEntry("Show Accidentals Toolbar", true);
    getToggleAction("show_accidentals_toolbar")->setChecked(opt);
    toggleNamedToolBar("accidentalsToolBar", &opt);

    toolBar("notesToolBar")->applySettings(m_config, "Notation Options notesToolBar");
    toolBar("restsToolBar")->applySettings(m_config, "Notation Options restsToolBar");
    toolBar("clefsToolBar")->applySettings(m_config, "Notation Options clefsToolBar");
    toolBar("fontToolBar")->applySettings(m_config, "Notation Options fontToolBar");
    toolBar("accidentalsToolBar")->applySettings(m_config, "Notation Options accidentalsToolBar");
}

void NotationView::setupActions()
{   
    KRadioAction* noteAction = 0;
    
    // View menu stuff
    
    KActionMenu *fontActionMenu =
	new KActionMenu(i18n("Note &Font"), this, "note_font_actionmenu");

    std::set<std::string> fs(NotePixmapFactory::getAvailableFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());
    
    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {

	QString fontQName(strtoqstr(*i));

	KToggleAction *fontAction = 
	    new KToggleAction
	    (fontQName, 0, this, SLOT(slotChangeFontFromAction()),
	     actionCollection(), "note_font_" + fontQName);

	fontAction->setChecked(*i == m_fontName);
	fontActionMenu->insert(fontAction);
    }

    actionCollection()->insert(fontActionMenu);

    m_fontSizeActionMenu =
	new KActionMenu(i18n("Si&ze"), this, "note_font_size_actionmenu");
    setupFontSizeMenu();

    actionCollection()->insert(m_fontSizeActionMenu);


    KActionMenu *styleActionMenu =
	new KActionMenu(i18n("Note &Style"), this, "note_style_actionmenu");

    std::vector<NoteStyleName> styles
	    (NoteStyleFactory::getAvailableStyleNames());
    
    for (std::vector<NoteStyleName>::iterator i = styles.begin();
	 i != styles.end(); ++i) {

	QString styleQName(strtoqstr(*i));

	KAction *styleAction = 
	    new KAction
	    (styleQName, 0, this, SLOT(slotSetStyleFromAction()),
	     actionCollection(), "style_" + styleQName);

	styleActionMenu->insert(styleAction);
    }

    actionCollection()->insert(styleActionMenu);


    // setup Notes menu & toolbar
    QIconSet icon;
 
    for (NoteActionDataMap::Iterator actionDataIter = m_noteActionDataMap->begin();
	 actionDataIter != m_noteActionDataMap->end();
	 ++actionDataIter) {

        NoteActionData noteActionData = *actionDataIter;
        
        icon = QIconSet
	    (m_toolbarNotePixmapFactory.makeToolbarPixmap
	     (noteActionData.pixmapName));
        noteAction = new KRadioAction(noteActionData.title,
				      icon,
				      noteActionData.keycode,
				      this,
                                      SLOT(slotNoteAction()),
                                      actionCollection(),
				      noteActionData.actionName);
        noteAction->setExclusiveGroup("notes");

        if (noteActionData.noteType == Note::Crotchet &&
	    noteActionData.dots == 0 && !noteActionData.rest) {
            m_selectDefaultNote = noteAction;
	}
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

    for (unsigned int i = 0;
	 i < sizeof(actionsAccidental)/sizeof(actionsAccidental[0]); ++i) {

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
    KStdAction::close (this, SLOT(slotCloseWindow()),      actionCollection());

    // Edit menu
    new KAction(i18n("Select from Sta&rt"), 0, this,
		SLOT(slotEditSelectFromStart()), actionCollection(),
		"select_from_start");

    new KAction(i18n("Select to &End"), 0, this,
		SLOT(slotEditSelectToEnd()), actionCollection(),
		"select_to_end");

    new KAction(i18n("Select Whole St&aff"), 0, this,
		SLOT(slotEditSelectWholeStaff()), actionCollection(),
		"select_whole_staff");

    new KAction(i18n("C&ut and Close"), CTRL + SHIFT + Key_X, this,
		SLOT(slotEditCutAndClose()), actionCollection(),
		"cut_and_close");

    new KAction(i18n("Pa&ste..."), CTRL + SHIFT + Key_V, this,
		SLOT(slotEditGeneralPaste()), actionCollection(),
		"general_paste");

    new KAction(i18n("De&lete"), Key_Delete, this,
		SLOT(slotEditDelete()), actionCollection(),
		"delete");

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

    (new KToggleAction
     (i18n("Show &Chord Name Ruler"), 0, this, SLOT(slotLabelChords()),
      actionCollection(), "label_chords"))->setChecked(true);

    new KToggleAction
	(i18n("Show &Tempo Ruler"), 0, this, SLOT(slotShowTempos()),
	 actionCollection(), "display_tempo_changes");

    // setup Group menu
    new KAction(i18n(GroupMenuBeamCommand::getGlobalName()), 0, this,
                SLOT(slotGroupBeam()), actionCollection(), "beam");

    new KAction(i18n(GroupMenuAutoBeamCommand::getGlobalName()), 0, this,
                SLOT(slotGroupAutoBeam()), actionCollection(), "auto_beam");

    new KAction(i18n(GroupMenuBreakCommand::getGlobalName()), 0, this,
                SLOT(slotGroupBreak()), actionCollection(), "break_group");

    new KAction(i18n(GroupMenuTupletCommand::getGlobalName(true)), 0, this,
		SLOT(slotGroupSimpleTuplet()), actionCollection(), "simple_tuplet");

    new KAction(i18n(GroupMenuTupletCommand::getGlobalName(false)), 0, this,
		SLOT(slotGroupGeneralTuplet()), actionCollection(), "tuplet");

    new KAction(i18n(GroupMenuGraceCommand::getGlobalName()), 0, this,
		SLOT(slotGroupGrace()), actionCollection(), "grace");

    new KAction(i18n(GroupMenuUnGraceCommand::getGlobalName()), 0, this,
		SLOT(slotGroupUnGrace()), actionCollection(), "ungrace");

    new KAction(i18n(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Slur)), 0, this,
                SLOT(slotGroupSlur()), actionCollection(), "slur");

    new KAction(i18n(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Crescendo)), 0, this,
                SLOT(slotGroupCrescendo()), actionCollection(), "crescendo");

    new KAction(i18n(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Decrescendo)), 0, this,
                SLOT(slotGroupDecrescendo()), actionCollection(), "decrescendo");

    // setup Transforms menu
    new KAction(i18n(TransformsMenuNormalizeRestsCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsNormalizeRests()), actionCollection(),
                "normalize_rests");

    new KAction(i18n(TransformsMenuCollapseRestsCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsCollapseRests()), actionCollection(),
                "collapse_rests_aggressively");

    new KAction(i18n(TransformsMenuCollapseNotesCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsCollapseNotes()), actionCollection(),
                "collapse_notes");

    new KAction(i18n(TransformsMenuTieNotesCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsTieNotes()), actionCollection(),
                "tie_notes");

    new KAction(i18n(TransformsMenuUntieNotesCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsUntieNotes()), actionCollection(),
                "untie_notes");

    new KAction(i18n(TransformsMenuChangeStemsCommand::getGlobalName(true)), 0,Key_Up + CTRL, this,
                SLOT(slotTransformsStemsUp()), actionCollection(),
                "stems_up");

    new KAction(i18n(TransformsMenuChangeStemsCommand::getGlobalName(false)), 0, Key_Down + CTRL, this,
                SLOT(slotTransformsStemsDown()), actionCollection(),
                "stems_down");

    new KAction(i18n(TransformsMenuRestoreStemsCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsRestoreStems()), actionCollection(),
                "restore_stems");

    new KAction(i18n(TransformsMenuChangeStyleCommand::getGlobalName
		(StandardNoteStyleNames::Classical)),
		0, this,
		SLOT(slotTransformsClassicalStyle()), actionCollection(),
		"style_classical");

    new KAction(i18n(TransformsMenuChangeStyleCommand::getGlobalName
		(StandardNoteStyleNames::Cross)),
		0, this,
		SLOT(slotTransformsCrossStyle()), actionCollection(),
		"style_x");

    new KAction(i18n(TransformsMenuChangeStyleCommand::getGlobalName
		(StandardNoteStyleNames::Mensural)),
		0, this,
		SLOT(slotTransformsMensuralStyle()), actionCollection(),
		"style_mensural");

    new KAction(i18n(TransformsMenuChangeStyleCommand::getGlobalName
		(StandardNoteStyleNames::Triangle)),
		0, this,
		SLOT(slotTransformsTriangleStyle()), actionCollection(),
		"style_triangle");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(1)), 0,
		Key_Up, this,
                SLOT(slotTransformsTransposeUp()), actionCollection(),
                "transpose_up");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(12)), 0,
		Key_Up + CTRL, this,
                SLOT(slotTransformsTransposeUpOctave()), actionCollection(),
                "transpose_up_octave");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(-1)), 0,
		Key_Down, this,
                SLOT(slotTransformsTransposeDown()), actionCollection(),
                "transpose_down");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(-12)), 0,
		Key_Down + CTRL, this,
                SLOT(slotTransformsTransposeDownOctave()), actionCollection(),
                "transpose_down_octave");

    new KAction(i18n(TransformsMenuTransposeCommand::getGlobalName(0)), 0, this,
                SLOT(slotTransformsTranspose()), actionCollection(),
                "general_transpose");

    new KAction(i18n(EventQuantizeCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsQuantize()), actionCollection(),
                "quantize");

    new KAction(i18n("&Dump selected events to stderr"), 0, this,
		SLOT(slotDebugDump()), actionCollection(), "debug_dump");

    for (MarkActionDataMap::Iterator i = m_markActionDataMap->begin();
	 i != m_markActionDataMap->end(); ++i) {

        const MarkActionData &markActionData = *i;
        
        icon = QIconSet
	    (m_toolbarNotePixmapFactory.makeToolbarPixmap
	     (markActionData.pixmapName));

	new KAction(i18n(markActionData.title),
		    icon,
		    markActionData.keycode,
		    this,
		    SLOT(slotAddMark()),
		    actionCollection(),
		    markActionData.actionName);
    }

    new KAction(i18n(MarksMenuAddTextMarkCommand::getGlobalName()), 0, this,
                SLOT(slotMarksAddTextMark()), actionCollection(),
                "add_text_mark");

    new KAction(i18n(MarksMenuRemoveMarksCommand::getGlobalName()), 0, this,
                SLOT(slotMarksRemoveMarks()), actionCollection(),
                "remove_marks");

    static const char *slashTitles[] = {
	"&None", "&1", "&2", "&3", "&4", "&5"
    };
    for (int i = 0; i <= 5; ++i) {
	new KAction(i18n(slashTitles[i]), 0, this,
		    SLOT(slotAddSlashes()), actionCollection(),
		    QString("slashes_%1").arg(i));
    }

    new KAction(i18n(ClefInsertionCommand::getGlobalName()), 0, this,
                SLOT(slotEditAddClef()), actionCollection(),
                "add_clef");

    new KAction(i18n(AddTempoChangeCommand::getGlobalName()), 0, this,
                SLOT(slotEditAddTempo()), actionCollection(),
                "add_tempo");

    new KAction(i18n(AddTimeSignatureCommand::getGlobalName()), 0, this,
                SLOT(slotEditAddTimeSignature()), actionCollection(),
                "add_time_signature");

    new KAction(i18n(KeyInsertionCommand::getGlobalName()), 0, this,
                SLOT(slotEditAddKeySignature()), actionCollection(),
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

    for (unsigned int i = 0;
	 i < sizeof(actionsToolbars)/sizeof(actionsToolbars[0]); ++i) {

        icon = QIconSet(m_toolbarNotePixmapFactory.makeToolbarPixmap(actionsToolbars[i][3]));

        KToggleAction* toolbarAction = new KToggleAction
            (i18n(actionsToolbars[i][0]), icon, 0,
             this, actionsToolbars[i][1],
             actionCollection(), actionsToolbars[i][2]);

        if (i == 3)
            toolbarAction->setChecked(false);
        else
            toolbarAction->setChecked(true);
    }

    new KAction(i18n("Cursor &Back"), 0, Key_Left, this,
		SLOT(slotStepBackward()), actionCollection(),
		"cursor_back");

    new KAction(i18n("Cursor &Forward"), 0, Key_Right, this,
		SLOT(slotStepForward()), actionCollection(),
		"cursor_forward");

    new KAction(i18n("Cursor Ba&ck Bar"), 0, Key_Left + CTRL, this,
		SLOT(slotJumpBackward()), actionCollection(),
		"cursor_back_bar");

    new KAction(i18n("Cursor For&ward Bar"), 0, Key_Right + CTRL, this,
		SLOT(slotJumpForward()), actionCollection(),
		"cursor_forward_bar");

    new KAction(i18n("Cursor &Up Staff"), 0, Key_Up + SHIFT, this,
		SLOT(slotCurrentStaffUp()), actionCollection(),
		"cursor_up_staff");

    new KAction(i18n("Cursor &Down Staff"), 0, Key_Down + SHIFT, this,
		SLOT(slotCurrentStaffDown()), actionCollection(),
		"cursor_down_staff");

    new KAction(i18n("Cursor to &Playback Pointer"), 0, this,
		SLOT(slotJumpCursorToPlayback()), actionCollection(),
		"cursor_to_playback_pointer");

    new KAction(i18n("Re&wind"), 0, Key_End, this,
		SIGNAL(rewindPlayback()), actionCollection(),
		"playback_pointer_back_bar");

    new KAction(i18n("&Fast Forward"), 0, Key_PageDown, this,
		SIGNAL(fastForwardPlayback()), actionCollection(),
		"playback_pointer_forward_bar");

    new KAction(i18n("Rewind to &Beginning"), 0, this,
		SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
		"playback_pointer_start");

    new KAction(i18n("Fast Forward to &End"), 0, this,
		SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
		"playback_pointer_end");

    new KAction(i18n("Fast Forward to &End"), 0, this,
		SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
		"playback_pointer_end");

    new KAction(i18n("Playback Pointer to &Cursor"), 0, this,
		SLOT(slotJumpPlaybackToCursor()), actionCollection(),
		"playback_pointer_to_cursor");

    QAccel *accelerators(getAccelerators());

    accelerators->connectItem(accelerators->insertItem(Key_Left + SHIFT),
			      this, SLOT(slotExtendSelectionBackward()));
    accelerators->connectItem(accelerators->insertItem(Key_Right + SHIFT),
			      this, SLOT(slotExtendSelectionForward()));

    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()), actionCollection());

    KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

    createGUI("notation.rc");
}

void
NotationView::setupFontSizeMenu(std::string oldFontName)
{
    if (oldFontName != "") {
	
	std::vector<int> sizes = NotePixmapFactory::getAvailableSizes
	    (oldFontName);
	
	for (unsigned int i = 0; i < sizes.size(); ++i) {
	    KAction *action =
		actionCollection()->action
		(QString("note_font_size_%1").arg(sizes[i]));
	    m_fontSizeActionMenu->remove(action);
	    delete action;
	}
    }

    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);

    for (unsigned int i = 0; i < sizes.size(); ++i) {

	KToggleAction *sizeAction = 
	    new KToggleAction
	    (i18n("%1 pixels").arg(sizes[i]), 0, this,
	     SLOT(slotChangeFontSizeFromAction()),
	     actionCollection(), QString("note_font_size_%1").arg(sizes[i]));

	sizeAction->setChecked(sizes[i] == m_fontSize);
	m_fontSizeActionMenu->insert(sizeAction);
    }
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

    m_fontCombo = new QComboBox(fontToolbar);
    m_fontCombo->setEditable(false);

    std::set<std::string> fs(NotePixmapFactory::getAvailableFontNames());
    std::vector<std::string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    bool foundFont = false;

    for (std::vector<std::string>::iterator i = f.begin(); i != f.end(); ++i) {

	QString fontQName(strtoqstr(*i));

        m_fontCombo->insertItem(fontQName);
        if (*i == m_fontName) {
            m_fontCombo->setCurrentItem(m_fontCombo->count() - 1);
	    foundFont = true;
        }
    }

    if (!foundFont) {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown font \"%1\", using default")).arg
	     (strtoqstr(m_fontName)));
	m_fontName = NotePixmapFactory::getDefaultFont();
    }
    
    connect(m_fontCombo, SIGNAL(activated(const QString &)),
            this,        SLOT(slotChangeFont(const QString &)));

    new QLabel(i18n("  Size:  "), fontToolbar);

    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    m_fontSizeSlider = new ZoomSlider<int>
        (sizes, m_fontSize, QSlider::Horizontal, fontToolbar);
    connect(m_fontSizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeFontSizeFromIndex(int)));

    new QLabel(i18n("  Spacing:  "), fontToolbar);

    std::vector<double> spacings = NotationHLayout::getAvailableSpacings();
    m_spacingSlider = new ZoomSlider<double>
        (spacings, 1.0, QSlider::Horizontal, fontToolbar);
    connect(m_spacingSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeSpacing(int)));

    new QLabel(i18n("  Smoothing:  "), fontToolbar);

    if (m_legatoDurations.size() == 0) {
        for (int type = Note::Shortest; type <= Note::Longest; ++type) {
            m_legatoDurations.push_back
                ((int)(Note(type).getDuration()));
        }
    }
    QSlider *m_smoothingSlider = new ZoomSlider<int>
        (m_legatoDurations, legatoUnit, QSlider::Horizontal, fontToolbar);
    connect(m_smoothingSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeLegato(int)));

    slotChangeFont(m_fontName, m_fontSize);
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
NotationView::setPageMode(bool pageMode)
{
    if (pageMode) {
	if (m_topBarButtons) m_topBarButtons->hide();
	if (m_bottomBarButtons) m_bottomBarButtons->hide();
	if (m_chordNameRuler) m_chordNameRuler->hide();
	if (m_tempoRuler) m_tempoRuler->hide();
    } else {
	if (m_topBarButtons) m_topBarButtons->show();
	if (m_bottomBarButtons) m_bottomBarButtons->show();
	if (m_chordNameRuler && m_chordNamesVisible) m_chordNameRuler->show();
	if (m_tempoRuler && m_temposVisible) m_tempoRuler->show();
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


void
NotationView::paintEvent(QPaintEvent *e)
{
    if (m_hlayout.isPageMode()) {
	int diff = width() - 50 - m_hlayout.getPageWidth();
	if (diff > -10 && diff < 10) {
	    m_hlayout.setPageWidth(width() - 50);
	    refreshSegment(0, 0, 0);
	}
    }
    EditView::paintEvent(e);
}


bool NotationView::applyLayout(int staffNo, timeT startTime, timeT endTime)
{
    if (m_progressDlg) {
	m_progressDlg->setLabelText(i18n("Laying out score..."));
	m_progressDlg->processEvents();
	m_hlayout.setProgressDialog(m_progressDlg);
	m_hlayout.setStaffCount(m_staffs.size());
    }

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
    m_hlayout.setProgressDialog(0);

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
        m_currentEventSelection->removeSelectionFromSegment
	    (m_properties.SELECTED);
        getStaff(m_currentEventSelection->getSegment())->positionElements
            (m_currentEventSelection->getStartTime(),
             m_currentEventSelection->getEndTime());
    }

    delete m_currentEventSelection;
    m_currentEventSelection = s;

#ifdef RGKDE3
    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);
#endif

    if (s) {
        s->recordSelectionOnSegment(m_properties.SELECTED);
        getStaff(s->getSegment())->positionElements(s->getStartTime(),
                                                    s->getEndTime());
#ifdef RGKDE3
	stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
	if (s->contains(Rosegarden::Note::EventType)) {
	    stateChanged("have_notes_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
	if (s->contains(Rosegarden::Note::EventRestType)) {
	    stateChanged("have_rests_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
#endif
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

void NotationView::showPreviewNote(int staffNo, double layoutX,
				   int pitch, int height,
				   const Rosegarden::Note &note)
{ 
    m_staffs[staffNo]->showPreviewNote(layoutX, height, note);

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
        new Rosegarden::MappedEvent(ins->getId(),
                                    Rosegarden::MappedEvent::MidiNoteOneShot,
                                    pitch,
                                    Rosegarden::MidiMaxValue,
                                    Rosegarden::RealTime(0,0),
                                    Rosegarden::RealTime(0, 500000),
                                    Rosegarden::RealTime(0, 0));

    emit notePlayed(mE);
}

void NotationView::clearPreviewNote()
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	m_staffs[i]->clearPreviewNote();
    }
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
    return m_insertionTime;
}


timeT
NotationView::getInsertionTime(Event *&clefEvt,
			       Event *&keyEvt)
{
    // This fuss is solely to recover the clef and key: we already
    // set m_insertionTime to the right value when we first placed
    // the insert cursor.  We could get clef and key directly from
    // the segment but the staff has a more efficient lookup

    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX < 0) layoutX = 0;
    (void)staff->getElementUnderLayoutX(layoutX, clefEvt, keyEvt);

    return m_insertionTime;
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////



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
    if (m_inhibitRefresh) return;

    bool ownProgressDlg = false;
    if (!m_progressDlg) {
	m_progressDlg = new RosegardenProgressDialog
	    (i18n("Updating..."), 0, 100, this,
	     i18n("Notation progress"), true);
	m_progressDlg->setAutoClose(false);
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressDialog(m_progressDlg);
	}
	ownProgressDlg = true;
    }

    emit usedSelection();

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
    if (ownProgressDlg) {
	delete m_progressDlg;
	m_progressDlg = 0;
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressDialog(0);
	}
    }	

    Event::dumpStats(cerr);
    doDeferredCursorMove();
//!!!    slotSetInsertCursorPosition(m_insertionTime, false);
    slotSetPointerPosition(m_document->getComposition().getPosition(), false);

    PRINT_ELAPSED("NotationView::refreshSegment (including update/GC)");
}



#define UPDATE_PROGRESS(n) \
	progressCount += (n); \
	if (m_progressDlg && (progressTotal > 0)) { \
	    m_progressDlg->setCompleted(progressCount * 100 / progressTotal); \
	    m_progressDlg->processEvents(); \
	}

void NotationView::readjustCanvasSize()
{
    START_TIMING;

    double maxWidth = 0.0;
    int maxHeight = 0;

    if (m_progressDlg) {
	m_progressDlg->setLabelText(i18n("Sizing and allocating canvas..."));
	m_progressDlg->processEvents();
    }

    int progressTotal = m_staffs.size() + 2;
    int progressCount = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        NotationStaff &staff = *m_staffs[i];

        staff.sizeStaff(m_hlayout);
	UPDATE_PROGRESS(1);

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
    UPDATE_PROGRESS(2);

    PRINT_ELAPSED("NotationView::readjustCanvasSize total");
}


// Slots: These are here because they use the note action data map
// or mark action data map


void NotationView::slotNoteAction()
{
    const QObject* sigSender = sender();

    NoteActionDataMap::Iterator noteAct =
	m_noteActionDataMap->find(sigSender->name());
    
    if (noteAct != m_noteActionDataMap->end())
        setCurrentSelectedNote(*noteAct);
    else
        kdDebug(KDEBUG_AREA) << "NotationView::slotNoteAction() : couldn't find NoteActionData named '"
                             << sigSender->name() << "'\n";
}
    

void NotationView::slotAddMark()
{
    const QObject *s = sender();
    if (!m_currentEventSelection) return;

    MarkActionDataMap::Iterator i = m_markActionDataMap->find(s->name());

    if (i != m_markActionDataMap->end()) {
	addCommandToHistory(new MarksMenuAddMarkCommand
			    ((*i).mark, *m_currentEventSelection));
    }
}



void NotationView::initActionDataMaps()
{
    static bool called = false;
    static int keys[] =
    { Key_0, Key_3, Key_6, Key_8, Key_4, Key_2, Key_1, Key_5 };
    
    if (called) return;
    called = true;

    m_noteActionDataMap = new NoteActionDataMap;

    for (int rest = 0; rest < 2; ++rest) {
	for (int dots = 0; dots < 2; ++dots) {
	    for (int type = Note::Longest; type >= Note::Shortest; --type) {
		if (dots && (type == Note::Longest)) continue;

		Note note(type, dots);

		QString refName(strtoqstr(note.getReferenceName(rest)));
		QString shortName(strtoqstr(note.getShortName()));
		QString titleName(strtoqstr(note.getAmericanName()));
		titleName = titleName.left(1).upper() +
		            titleName.right(titleName.length()-1);

		shortName.replace(QRegExp(" "), "_");

		if (rest) {
		    shortName += "_rest";
		    titleName.replace(QRegExp("note"), "rest");
		}

		int keycode = keys[type - Note::Shortest];
		if (dots) keycode += CTRL;
		if (rest) keycode += SHIFT;

		m_noteActionDataMap->insert
		    (shortName, NoteActionData
		     (titleName, shortName, refName, keycode,
		      rest > 0, type, dots));
	    }
	}
    }

    m_markActionDataMap = new MarkActionDataMap;

    std::vector<Mark> marks = Rosegarden::Marks::getStandardMarks();
    for (unsigned int i = 0; i < marks.size(); ++i) {

	Mark mark = marks[i];
	QString markName(strtoqstr(mark));
	QString actionName = QString("add_%1").arg(markName);

	m_markActionDataMap->insert
	    (actionName, MarkActionData
	     (MarksMenuAddMarkCommand::getGlobalName(mark),
	      actionName, markName, 0, mark));
    }
	     
}

    
NotationView::NoteActionDataMap* NotationView::m_noteActionDataMap = 0;
NotationView::MarkActionDataMap* NotationView::m_markActionDataMap = 0;

