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
#include <qregexp.h>

#include <kmessagebox.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>

#include <kstdaction.h>
#include <kapp.h>
#include <kstatusbar.h>

#include "rosestrings.h"
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
#include "widgets.h"
#include "notestyle.h"

#include "chordnameruler.h"
#include "temporuler.h"

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
                           vector<Segment *> segments,
                           QWidget *parent,
			   bool showProgressive) :
    EditView(doc, segments, false, parent, "notationview"),
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
    m_fontSizeSlider(0),
    m_selectDefaultNote(0),
    m_progressDlg(0),
    m_inhibitRefresh(true),
    m_documentDestroyed(false)
{
    initActionDataMaps(); // does something only the 1st time it's called
    
    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    kdDebug(KDEBUG_AREA) << "NotationView ctor" << endl;

    setupActions();

    initFontToolbar(m_legatoQuantizer->getUnit(), segments.size() > 1);
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
	(&m_hlayout, &doc->getComposition(), 20.0, 20, getCentralFrame());
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
	    (i18n("Starting..."), i18n("Cancel"), 100, this,
	     i18n("Notation progress"), true);
	m_progressDlg->setAutoClose(false);
	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->setProgressDialog(m_progressDlg);
	}
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
NotationView::slotDocumentDestroyed()
{
    kdDebug(KDEBUG_AREA) << "NotationView::slotDocumentDestroyed()\n";
    m_documentDestroyed = true;
    m_inhibitRefresh = true;
}

void
NotationView::slotTestClipboard()
{
#ifdef RGKDE3
    if (m_document->getClipboard()->isEmpty()) {
	stateChanged("have_clipboard", KXMLGUIClient::StateReverse);
	stateChanged("have_clipboard_single_segment",
		     KXMLGUIClient::StateReverse);
    } else {
	stateChanged("have_clipboard", KXMLGUIClient::StateNoReverse);
	stateChanged("have_clipboard_single_segment",
		     (m_document->getClipboard()->isSingleSegment() ?
		      KXMLGUIClient::StateNoReverse :
		      KXMLGUIClient::StateReverse));
    }
#endif
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
     (i18n("Label &Chords"), 0, this, SLOT(slotLabelChords()),
      actionCollection(), "label_chords"))->setChecked(true);

    new KToggleAction
	(i18n("Display &Tempo Changes"), 0, this, SLOT(slotShowTempos()),
	 actionCollection(), "display_tempo_changes");

    // setup Group menu
    new KAction(GroupMenuBeamCommand::getGlobalName(), 0, this,
                SLOT(slotGroupBeam()), actionCollection(), "beam");

    new KAction(GroupMenuAutoBeamCommand::getGlobalName(), 0, this,
                SLOT(slotGroupAutoBeam()), actionCollection(), "auto_beam");

    new KAction(GroupMenuBreakCommand::getGlobalName(), 0, this,
                SLOT(slotGroupBreak()), actionCollection(), "break_group");

    new KAction(GroupMenuTupletCommand::getGlobalName(true), 0, this,
		SLOT(slotGroupSimpleTuplet()), actionCollection(), "simple_tuplet");

    new KAction(GroupMenuTupletCommand::getGlobalName(false), 0, this,
		SLOT(slotGroupGeneralTuplet()), actionCollection(), "tuplet");

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Slur), 0, this,
                SLOT(slotGroupSlur()), actionCollection(), "slur");

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Crescendo), 0, this,
                SLOT(slotGroupCrescendo()), actionCollection(), "crescendo");

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Decrescendo), 0, this,
                SLOT(slotGroupDecrescendo()), actionCollection(), "decrescendo");

    // setup Transforms menu
    new KAction(TransformsMenuNormalizeRestsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsNormalizeRests()), actionCollection(),
                "normalize_rests");

    new KAction(TransformsMenuCollapseRestsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsCollapseRests()), actionCollection(),
                "collapse_rests_aggressively");

    new KAction(TransformsMenuCollapseNotesCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsCollapseNotes()), actionCollection(),
                "collapse_notes");

    new KAction(TransformsMenuTieNotesCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsTieNotes()), actionCollection(),
                "tie_notes");

    new KAction(TransformsMenuUntieNotesCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsUntieNotes()), actionCollection(),
                "untie_notes");

    new KAction(TransformsMenuChangeStemsCommand::getGlobalName(true), 0, this,
                SLOT(slotTransformsStemsUp()), actionCollection(),
                "stems_up");

    new KAction(TransformsMenuChangeStemsCommand::getGlobalName(false), 0, this,
                SLOT(slotTransformsStemsDown()), actionCollection(),
                "stems_down");

    new KAction(TransformsMenuRestoreStemsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsRestoreStems()), actionCollection(),
                "restore_stems");

    new KAction(TransformsMenuChangeStyleCommand::getGlobalName
		(StandardNoteStyleNames::Classical),
		0, this,
		SLOT(slotTransformsClassicalStyle()), actionCollection(),
		"style_classical");

    new KAction(TransformsMenuChangeStyleCommand::getGlobalName
		(StandardNoteStyleNames::Cross),
		0, this,
		SLOT(slotTransformsCrossStyle()), actionCollection(),
		"style_x");

    new KAction(TransformsMenuChangeStyleCommand::getGlobalName
		(StandardNoteStyleNames::Mensural),
		0, this,
		SLOT(slotTransformsMensuralStyle()), actionCollection(),
		"style_mensural");

    new KAction(TransformsMenuChangeStyleCommand::getGlobalName
		(StandardNoteStyleNames::Triangle),
		0, this,
		SLOT(slotTransformsTriangleStyle()), actionCollection(),
		"style_triangle");

    new KAction(TransformsMenuTransposeCommand::getGlobalName(1), 0,
		Key_Up, this,
                SLOT(slotTransformsTransposeUp()), actionCollection(),
                "transpose_up");

    new KAction(TransformsMenuTransposeCommand::getGlobalName(12), 0,
		Key_Up + CTRL, this,
                SLOT(slotTransformsTransposeUpOctave()), actionCollection(),
                "transpose_up_octave");

    new KAction(TransformsMenuTransposeCommand::getGlobalName(-1), 0,
		Key_Down, this,
                SLOT(slotTransformsTransposeDown()), actionCollection(),
                "transpose_down");

    new KAction(TransformsMenuTransposeCommand::getGlobalName(-12), 0,
		Key_Down + CTRL, this,
                SLOT(slotTransformsTransposeDownOctave()), actionCollection(),
                "transpose_down_octave");

    new KAction(TransformsMenuTransposeCommand::getGlobalName(0), 0, this,
                SLOT(slotTransformsTranspose()), actionCollection(),
                "general_transpose");

    new KAction(EventQuantizeCommand::getGlobalName(), 0, this,
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

	new KAction(markActionData.title,
		    icon,
		    markActionData.keycode,
		    this,
		    SLOT(slotAddMark()),
		    actionCollection(),
		    markActionData.actionName);
    }

    new KAction(MarksMenuAddTextMarkCommand::getGlobalName(), 0, this,
                SLOT(slotMarksAddTextMark()), actionCollection(),
                "add_text_mark");

    new KAction(MarksMenuRemoveMarksCommand::getGlobalName(), 0, this,
                SLOT(slotMarksRemoveMarks()), actionCollection(),
                "remove_marks");

    static const char *slashTitles[] = {
	"&None", "&1", "&2", "&3", "&4", "&5"
    };
    for (int i = 0; i <= 5; ++i) {
	new KAction(slashTitles[i], 0, this,
		    SLOT(slotAddSlashes()), actionCollection(),
		    QString("slashes_%1").arg(i));
    }

    new KAction(ClefInsertionCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddClef()), actionCollection(),
                "add_clef");

    new KAction(AddTempoChangeCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddTempo()), actionCollection(),
                "add_tempo");

    new KAction(AddTimeSignatureCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddTimeSignature()), actionCollection(),
                "add_time_signature");

    new KAction(KeyInsertionCommand::getGlobalName(), 0, this,
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

NotationStaff *
NotationView::getStaff(const Segment &segment)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        if (&(m_staffs[i]->getSegment()) == &segment) return m_staffs[i];
    }
    return 0;
}

void NotationView::initFontToolbar(int legatoUnit, bool multiStaff)
{
    KToolBar *fontToolbar = toolBar("fontToolBar");

    kdDebug(KDEBUG_AREA) << "NotationView::initFontToolbar: legatoUnit is "
                         << legatoUnit << endl;
    
    if (!fontToolbar) {
        kdDebug(KDEBUG_AREA)
            << "NotationView::initFontToolbar() : font toolbar not found\n";
        return;
    }

    KConfig *config = kapp->config();
    config->setGroup("Notation Options");

    m_fontName = qstrtostr(config->readEntry
			   ("notefont",
			    strtoqstr(NotePixmapFactory::getDefaultFont())));

    m_fontSize = config->readUnsignedNumEntry
	((multiStaff ? "multistaffnotesize" : "singlestaffnotesize"),
	 NotePixmapFactory::getDefaultSize(m_fontName));

    new QLabel(i18n("  Font:  "), fontToolbar);

    QComboBox *fontCombo = new QComboBox(fontToolbar);
    fontCombo->setEditable(false);

    set<string> fs(NotePixmapFactory::getAvailableFontNames());
    vector<string> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    bool foundFont = false;
    for (vector<string>::iterator i = f.begin(); i != f.end(); ++i) {
        fontCombo->insertItem(strtoqstr(*i));
        if (*i == m_fontName) {
            fontCombo->setCurrentItem(fontCombo->count() - 1);
	    foundFont = true;
        }
    }
    if (!foundFont) {
	KMessageBox::sorry
	    (this, QString(i18n("Unknown font \"%1\", using default")).arg
	     (strtoqstr(m_fontName)));
	m_fontName = NotePixmapFactory::getDefaultFont();
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

    new QLabel(i18n("  Smoothing:  "), fontToolbar);

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

    Quantizer q(Quantizer::RawEventData,
		getViewLocalPropertyPrefix() + "Q",
		Quantizer::LegatoQuantize, m_legatoDurations[n]);
    *m_legatoQuantizer = q;
    
    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->renderAllElements();
        m_staffs[i]->positionAllElements();
    }

    updateView();
}


void
NotationView::slotChangeFont(const QString &newName)
{
    kdDebug(KDEBUG_AREA) << "changeFont: " << newName << endl;
    slotChangeFont(string(newName.utf8()));
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


#define UPDATE_PROGRESS(n) \
	progressCount += (n); \
	if (m_progressDlg && (progressTotal > 0)) { \
	    m_progressDlg->setCompleted(progressCount * 100 / progressTotal); \
	    m_progressDlg->processEvents(); \
	}


bool NotationView::applyLayout(int staffNo, timeT startTime, timeT endTime)
{
    if (m_progressDlg) {
	m_progressDlg->setLabelText(i18n("Laying out score..."));
	m_progressDlg->processEvents();
    }

    START_TIMING;
    unsigned int i;

    int progressTotal = m_staffs.size() * 3 + 5;
    int progressCount = 0;

    for (i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo) continue;

        m_hlayout.resetStaff(*m_staffs[i], startTime, endTime);
	UPDATE_PROGRESS(2);

        m_vlayout.resetStaff(*m_staffs[i], startTime, endTime);
	UPDATE_PROGRESS(1);

        m_hlayout.scanStaff(*m_staffs[i], startTime, endTime);
	UPDATE_PROGRESS(2);

        m_vlayout.scanStaff(*m_staffs[i], startTime, endTime);
	UPDATE_PROGRESS(1);
    }

    m_hlayout.finishLayout(startTime, endTime);
    UPDATE_PROGRESS(3);

    m_vlayout.finishLayout(startTime, endTime);
    UPDATE_PROGRESS(2);

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

    if (s) {
        s->recordSelectionOnSegment(m_properties.SELECTED);
        getStaff(s->getSegment())->positionElements(s->getStartTime(),
                                                    s->getEndTime());
#ifdef RGKDE3
	stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
#endif
    } else {
#ifdef RGKDE3
	stateChanged("have_selection", KXMLGUIClient::StateReverse);
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
        new Rosegarden::MappedEvent(ins->getID(),
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

//
// Cut, Copy, Paste
//
void NotationView::slotEditCut()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), statusBar());

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
				       m_document->getClipboard()));
    setCurrentSelection(0);
}

void NotationView::slotEditDelete()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Deleting selection..."), statusBar());

    addCommandToHistory(new EraseCommand(*m_currentEventSelection));
    setCurrentSelection(0);
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
    setCurrentSelection(0);
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
    Segment &segment = staff->getSegment();
    
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
	    setCurrentSelection(new EventSelection
				(segment, insertionTime, endTime));
	}
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

void NotationView::toggleNamedToolBar(const QString& toolBarName, bool* force)
{
    KToolBar *namedToolBar = toolBar(toolBarName);

    if (!namedToolBar) {
        kdDebug(KDEBUG_AREA) << "NotationView::toggleNamedToolBar() : toolBar "
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

	t = m_currentEventSelection->getStartTime();

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

	segment = &m_currentEventSelection->getSegment();

    } else {

	t = getInsertionTime();

	NoteInserter *currentInserter = dynamic_cast<NoteInserter *>
	    (m_toolBox->getTool(NoteInserter::ToolName));

	Note::Type unitType;

	if (currentInserter) {
	    unitType = currentInserter->getCurrentNote().getNoteType();
	} else {
	    unitType = Note::Quaver;
	}

	unit = Note(unitType).getDuration();

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
    setCurrentSelection(0);
}

void NotationView::slotTransformsCollapseRests()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing rests..."), statusBar());

    addCommandToHistory(new TransformsMenuCollapseRestsCommand
                        (*m_currentEventSelection));
    setCurrentSelection(0);
}

void NotationView::slotTransformsCollapseNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing notes..."), statusBar());

    addCommandToHistory(new TransformsMenuCollapseNotesCommand
                        (*m_currentEventSelection));
    setCurrentSelection(0);
}

void NotationView::slotTransformsTieNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Tying notes..."), statusBar());

    addCommandToHistory(new TransformsMenuTieNotesCommand
                        (*m_currentEventSelection));
    setCurrentSelection(0);
}

void NotationView::slotTransformsUntieNotes()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Untying notes..."), statusBar());

    addCommandToHistory(new TransformsMenuUntieNotesCommand
                        (*m_currentEventSelection));
    setCurrentSelection(0);
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

void NotationView::slotTransformsClassicalStyle()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Changing to Classical style..."), statusBar());

    addCommandToHistory(new TransformsMenuChangeStyleCommand
                        (StandardNoteStyleNames::Classical,
			 *m_currentEventSelection));
}

void NotationView::slotTransformsCrossStyle()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Changing to Cross style..."), statusBar());

    addCommandToHistory(new TransformsMenuChangeStyleCommand
                        (StandardNoteStyleNames::Cross,
			 *m_currentEventSelection));
}

void NotationView::slotTransformsTriangleStyle()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Changing to Triangle style..."), statusBar());

    addCommandToHistory(new TransformsMenuChangeStyleCommand
                        (StandardNoteStyleNames::Triangle,
			 *m_currentEventSelection));
}

void NotationView::slotTransformsMensuralStyle()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Changing to Mensural style..."), statusBar());

    addCommandToHistory(new TransformsMenuChangeStyleCommand
                        (StandardNoteStyleNames::Mensural,
			 *m_currentEventSelection));
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

    QuantizeDialog *dialog = new QuantizeDialog(this,
						Quantizer::RawEventData,
						Quantizer::RawEventData);

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
    TimeSignature timeSig = helper.guessTimeSignature(adapter);


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

	setCurrentSelection(0);
    }
    
    delete dialog;
}			

void NotationView::slotEditAddKeySignature()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
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

	setCurrentSelection(0);
    }

    delete dialog;
}			


void NotationView::slotDebugDump()
{
    if (m_currentEventSelection) {
	EventSelection::eventcontainer &ec =
	    m_currentEventSelection->getSegmentEvents();
	int n = 0;
	for (EventSelection::eventcontainer::iterator i = ec.begin();
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
    Segment &segment = staff->getSegment();
    timeT time = m_insertionTime;
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
    timeT time = m_insertionTime;
    Segment::iterator i = segment.findTime(time);

    while (i != segment.end() && (*i)->getAbsoluteTime() == time) ++i;
    if (i == segment.end()) slotSetInsertCursorPosition(segment.getEndTime());
    else slotSetInsertCursorPosition((*i)->getAbsoluteTime());
}

void
NotationView::slotJumpBackward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
    timeT time = segment.getBarStartForTime(m_insertionTime - 1);
    slotSetInsertCursorPosition(time);
}

void
NotationView::slotJumpForward()
{
    NotationStaff *staff = m_staffs[m_currentStaff];
    Segment &segment = staff->getSegment();
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

void NotationView::slotShowTempos()
{
    if (m_hlayout.getPageMode()) return;
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
    kdDebug(KDEBUG_AREA) << "NotationView::slotItemPressed(height = "
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

    Event::dumpStats(cerr);
    doDeferredCursorMove();
//!!!    slotSetInsertCursorPosition(m_insertionTime, false);
    slotSetPointerPosition(m_document->getComposition().getPosition(), false);

    PRINT_ELAPSED("NotationView::refreshSegment (including update/GC)");
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

