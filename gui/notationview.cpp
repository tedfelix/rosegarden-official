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


#include <qstring.h>
#include <qregexp.h>
#include <qpaintdevicemetrics.h>
#include <qpixmap.h>

#include <qtimer.h> // delete me

#include <kmessagebox.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <kapp.h>
#include <kprinter.h>

#include "NotationTypes.h"
#include "Quantizer.h"
#include "Selection.h"
#include "BaseProperties.h"

#include "Profiler.h"

#include "notationview.h"
#include "constants.h"
#include "colours.h"
#include "notationstrings.h"
#include "notepixmapfactory.h"
#include "notestyle.h"
#include "notationtool.h"
#include "notationstrings.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "rosedebug.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "segmentcommands.h"
#include "widgets.h"
#include "chordnameruler.h"
#include "temporuler.h"
#include "rawnoteruler.h"
#include "studiocontrol.h"
#include "notationhlayout.h"
#include "notationvlayout.h"
#include "sequencemanager.h"

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
    : title(_title),
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
    m_selectionCounter(0),
    m_insertModeLabel(0),
    m_annotationsLabel(0),
    m_progressBar(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_currentStaff(-1),
    m_lastFinishingStaff(-1),
    m_insertionTime(0),
    m_fontName(NotePixmapFactory::getDefaultFont()),
    m_fontSize(NotePixmapFactory::getDefaultSize(m_fontName)),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(new NotationHLayout(&doc->getComposition(), m_notePixmapFactory,
                                  m_properties, this)),
    m_vlayout(new NotationVLayout(&doc->getComposition(),
                                  m_properties, this)),
    m_chordNameRuler(0),
    m_tempoRuler(0),
    m_rawNoteRuler(0),
    m_annotationsVisible(false),
    m_selectDefaultNote(0),
    m_fontCombo(0),
    m_fontSizeSlider(0),
    m_spacingSlider(0),
    m_fontSizeActionMenu(0),
    m_progressDisplayer(PROGRESS_NONE),
    m_progressEventFilterInstalled(false),
    m_inhibitRefresh(true),
    m_ok(false),
    m_printMode(false)
{
    initActionDataMaps(); // does something only the 1st time it's called
    
    m_toolBox = new NotationToolBox(this);

    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the layout toolbar

    m_config->setGroup(NotationView::ConfigGroup);

    m_fontName = qstrtostr(m_config->readEntry
			   ("notefont",
			    strtoqstr(NotePixmapFactory::getDefaultFont())));

    m_fontSize = m_config->readUnsignedNumEntry
	((segments.size() > 1 ? "multistaffnotesize" : "singlestaffnotesize"),
	 NotePixmapFactory::getDefaultSize(m_fontName));

    int defaultSpacing = m_config->readNumEntry("spacing", 100);
    m_hlayout->setSpacing(defaultSpacing);

    setupActions();
    initLayoutToolbar();
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);

    m_config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    if (m_config->readBoolEntry("backgroundtextures", false)) {
	QPixmap background;
	QString pixmapDir =
	    KGlobal::dirs()->findResource("appdata", "pixmaps/");
	if (background.load(QString("%1/misc/bg-paper-white.xpm").
			    arg(pixmapDir))) {
	    tCanvas->setBackgroundPixmap(background);
	}
    }
    m_config->setGroup(NotationView::ConfigGroup);


    setCanvasView(new NotationCanvasView(*this, m_horizontalScrollBar,
                                         tCanvas, getCentralFrame()));

    if (segments.size() == 1) {
        setCaption(QString("%1 - Segment Track #%2 - Notation")
                   .arg(doc->getTitle())
                   .arg(segments[0]->getTrack()));
    } else if (segments.size() == doc->getComposition().getNbSegments()) {
        setCaption(QString("%1 - All Segments - Notation")
                   .arg(doc->getTitle()));
    } else {
        setCaption(QString("%1 - %2 Segments - Notation")
                   .arg(doc->getTitle())
                   .arg(segments.size()));
    }

    setTopBarButtons(new BarButtons(getDocument(),
                                    m_hlayout, 20.0, 25,
				    false, getCentralFrame()));

    m_topBarButtons->getLoopRuler()->setBackgroundColor
	(RosegardenGUIColours::InsertCursorRuler);

    m_chordNameRuler = new ChordNameRuler
	(m_hlayout, 0, 20.0, 20, getCentralFrame());
    addRuler(m_chordNameRuler);
    if (showProgressive) m_chordNameRuler->show();

    m_tempoRuler = new TempoRuler
	(m_hlayout, &doc->getComposition(),
	 20.0, 20, false, getCentralFrame());
    addRuler(m_tempoRuler);
    m_tempoRuler->hide();

    m_rawNoteRuler = new RawNoteRuler
	(m_hlayout, segments[0], 20.0, 20, getCentralFrame());
    addRuler(m_rawNoteRuler);
    m_rawNoteRuler->show();

    // All toolbars should be created before this is called
    setAutoSaveSettings("NotationView", true);

    // All rulers must have been created before this is called,
    // or the program will crash
    readOptions();


    setBottomBarButtons(new BarButtons(getDocument(), m_hlayout, 20.0, 25,
				       true, getCentralFrame()));

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], 0, // snap
                                             i, this, false, getPageWidth(),
                                             m_fontName, m_fontSize));
    }

    //
    // layout
    //
    RosegardenProgressDialog* progressDlg = 0;

    if (showProgressive) {
	show();
        kapp->processEvents();

        RG_DEBUG << "NotationView : setting up progress dialog\n";

        progressDlg = new RosegardenProgressDialog(i18n("Starting..."),
                                                   100, this);
	progressDlg->setAutoClose(false);
        progressDlg->setAutoReset(true);
        progressDlg->setMinimumDuration(1000);
        setupProgress(progressDlg);

        m_progressDisplayer = PROGRESS_DIALOG;
    }

    m_chordNameRuler->setComposition(&getDocument()->getComposition());
    m_chordNameRuler->setStudio(&getDocument()->getStudio());

    positionStaffs();
    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    m_config->setGroup(NotationView::ConfigGroup);
    int layoutMode = m_config->readNumEntry("layoutmode", 0);

    try {

        setPageMode(layoutMode == 1); // also renders the staffs!

	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->getSegment().getRefreshStatus
		(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
	}

        m_ok = true;

    } catch (ProgressReporter::Cancelled c) {
	// when cancelled, m_ok is false -- checked by calling method
        NOTATION_DEBUG << "NotationView ctor : layout Cancelled\n";
    }
    
    NOTATION_DEBUG << "NotationView ctor : m_ok = " << m_ok << endl;
    
    delete progressDlg;

    // at this point we can return if operation was cancelled
    if (!isOK()) return;

    // otherwise, carry on
    setupDefaultProgress();

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

    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_multiple_staffs",
		 (m_staffs.size() > 1 ? KXMLGUIClient::StateNoReverse :
		                        KXMLGUIClient::StateReverse));
    stateChanged("rest_insert_tool_current", KXMLGUIClient::StateReverse);
    slotTestClipboard();

    if (getSegmentsOnlyRests()) {
        m_selectDefaultNote->activate();
	stateChanged("note_insert_tool_current", 
		     KXMLGUIClient::StateNoReverse);
    } else {
        actionCollection()->action("select")->activate();
	stateChanged("note_insert_tool_current",
		     KXMLGUIClient::StateReverse);
    }

    slotSetInsertCursorPosition(0);
    slotSetPointerPosition(doc->getComposition().getPosition());
    setCurrentSelection(0, false, true);
    slotUpdateInsertModeStatus();
    m_chordNameRuler->repaint();
    m_rawNoteRuler->repaint();
    m_inhibitRefresh = false;

    setConfigDialogPageIndex(1);

    NOTATION_DEBUG << "NotationView ctor exiting\n";
}

//
// Notation Print mode
//
NotationView::NotationView(RosegardenGUIDoc *doc,
                           std::vector<Segment *> segments,
                           KPrinter *printer,
			   QWidget *parent)
    : EditView(doc, segments, 1, 0, "printview"),
    m_properties(getViewLocalPropertyPrefix()),
    m_selectionCounter(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_lastFinishingStaff(-1),
    m_insertionTime(0),
    m_fontName(NotePixmapFactory::getDefaultFont()),
    m_fontSize(NotePixmapFactory::getDefaultSize(m_fontName)),
    m_notePixmapFactory(new NotePixmapFactory(m_fontName, m_fontSize)),
    m_hlayout(new NotationHLayout(&doc->getComposition(), m_notePixmapFactory,
				  m_properties, this)),
    m_vlayout(new NotationVLayout(&doc->getComposition(),
                                  m_properties, this)),
    m_chordNameRuler(0),
    m_tempoRuler(0),
    m_rawNoteRuler(0),
    m_annotationsVisible(false),
    m_selectDefaultNote(0),
    m_fontCombo(0),
    m_fontSizeSlider(0),
    m_spacingSlider(0),
    m_fontSizeActionMenu(0),
    m_progressDisplayer(PROGRESS_NONE),
    m_progressEventFilterInstalled(false),
    m_inhibitRefresh(true),
    m_ok(false),
    m_printMode(true)
{
    assert(segments.size() > 0);
    NOTATION_DEBUG << "NotationView print ctor" << endl;


    // Initialise the display-related defaults that will be needed
    // by both the actions and the layout toolbar

    m_config->setGroup(NotationView::ConfigGroup);

    m_fontName = qstrtostr(m_config->readEntry
			   ("notefont",
			    strtoqstr(NotePixmapFactory::getDefaultFont())));


    // Force largest font size
    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    m_fontSize = sizes[sizes.size()-1];

    setBackgroundMode(PaletteBase);

    QPaintDeviceMetrics pdm(printer);

    QCanvas *tCanvas = new QCanvas(this);

    RG_DEBUG << "Print area size : "
             << pdm.width() << ", " << pdm.height()
             << " - printer resolution : " << printer->resolution() << "\n";

    unsigned int scaleFactor = 3;//!!! need to know
    tCanvas->resize(pdm.width() / scaleFactor, pdm.height() / scaleFactor);
    
    setCanvasView(new NotationCanvasView(*this, m_horizontalScrollBar,
                                         tCanvas, getCentralFrame()));

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new NotationStaff(canvas(), segments[i], 0, // snap
                                             i, this, false, getPageWidth(),
                                             m_fontName, m_fontSize));
    }

    positionStaffs();
    m_currentStaff = 0;
    m_staffs[0]->setCurrent(true);

    RosegardenProgressDialog* progressDlg = 0;

    if (parent) {

        kapp->processEvents();

        RG_DEBUG << "NotationView : setting up progress dialog\n";

        progressDlg = new RosegardenProgressDialog(i18n("Printing..."),
                                                   100, parent);
	progressDlg->setAutoClose(false);
        progressDlg->setAutoReset(true);
        progressDlg->setMinimumDuration(1000);
        setupProgress(progressDlg);

        m_progressDisplayer = PROGRESS_DIALOG;
    }

    try {

        setPageMode(true);

	for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	    m_staffs[i]->getSegment().getRefreshStatus
		(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
	}

	m_ok = true;

    } catch (ProgressReporter::Cancelled c) {
	// when cancelled, m_ok is false -- checked by calling method
        NOTATION_DEBUG << "NotationView ctor : layout Cancelled\n";
    }

    NOTATION_DEBUG << "NotationView ctor : m_ok = " << m_ok << endl;
    
    delete progressDlg;

    if (!isOK()) return; // In case more code is added there later

}


NotationView::~NotationView()
{
    NOTATION_DEBUG << "-> ~NotationView()\n";

    // Give the sequencer something to suck on while we close
    //
    if (!getDocument()->isBeingDestroyed()) {
	getDocument()->getSequenceManager()->
	    setTemporarySequencerSliceSize(Rosegarden::RealTime(2, 0));
    }

    if (!m_printMode) slotSaveOptions();

    delete m_chordNameRuler;

    delete m_currentEventSelection;
    m_currentEventSelection = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
	for (Segment::iterator j = m_staffs[i]->getSegment().begin();
	     j != m_staffs[i]->getSegment().end(); ++j) {
	    removeViewLocalProperties(*j);
	}
	delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    Rosegarden::Profiles::getInstance()->dump();

    NOTATION_DEBUG << "<- ~NotationView()\n";
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
    
const NotationProperties &
NotationView::getProperties() const 
{
    return m_properties;
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

void NotationView::slotSaveOptions()
{
    m_config->setGroup(NotationView::ConfigGroup);

    m_config->writeEntry("Show Chord Name Ruler", getToggleAction("show_chords_ruler")->isChecked());
    m_config->writeEntry("Show Raw Note Ruler", getToggleAction("show_raw_note_ruler")->isChecked());
    m_config->writeEntry("Show Tempo Ruler",      getToggleAction("show_tempo_ruler")->isChecked());
    m_config->writeEntry("Show Annotations", m_annotationsVisible);

    m_config->sync();
}

void NotationView::setOneToolbar(const char *actionName,
				 const char *toolbarName)
{
    KToggleAction *action = getToggleAction(actionName);
    if (!action) {
	std::cerr << "WARNING: No such action as " << actionName << std::endl;
	return;
    }
    QWidget *toolbar = toolBar(toolbarName);
    if (!toolbar) {
	std::cerr << "WARNING: No such toolbar as " << toolbarName << std::endl;
	return;
    }
    action->setChecked(!toolbar->isHidden());
}
	

void NotationView::readOptions()
{
    EditView::readOptions();

    setOneToolbar("show_tools_toolbar", "Tools Toolbar");
    setOneToolbar("show_notes_toolbar", "Notes Toolbar");
    setOneToolbar("show_rests_toolbar", "Rests Toolbar");
    setOneToolbar("show_clefs_toolbar", "Clefs Toolbar");
    setOneToolbar("show_group_toolbar", "Group Toolbar");
    setOneToolbar("show_marks_toolbar", "Marks Toolbar");
    setOneToolbar("show_layout_toolbar", "Layout Toolbar");
    setOneToolbar("show_transport_toolbar", "Transport Toolbar");
    setOneToolbar("show_accidentals_toolbar", "Accidentals Toolbar");
    setOneToolbar("show_meta_toolbar", "Meta Toolbar");

    m_config->setGroup(NotationView::ConfigGroup);

    bool opt;

    opt = m_config->readBoolEntry("Show Chord Name Ruler", true);
    getToggleAction("show_chords_ruler")->setChecked(opt);
    slotToggleChordsRuler();

    opt = m_config->readBoolEntry("Show Raw Note Ruler", true);
    getToggleAction("show_raw_note_ruler")->setChecked(opt);
    slotToggleRawNoteRuler();

    opt = m_config->readBoolEntry("Show Tempo Ruler", false);
    getToggleAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = m_config->readBoolEntry("Show Annotations", true);
    m_annotationsVisible = opt;
    getToggleAction("show_annotations")->setChecked(opt);
    //slotUpdateAnnotationsStatus();
    slotToggleAnnotations();
}

void NotationView::setupActions()
{
    EditViewBase::setupActions("notation.rc");
    EditView::setupActions();

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

    
    KActionMenu *spacingActionMenu =
	new KActionMenu(i18n("Spa&cing"), this, "stretch_actionmenu");

    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    for (std::vector<int>::iterator i = spacings.begin();
	 i != spacings.end(); ++i) {
	
	KToggleAction *spacingAction =
	    new KToggleAction
	    (QString("%1%").arg(*i), 0, this,
	     SLOT(slotChangeSpacingFromAction()),
	     actionCollection(), QString("spacing_%1").arg(*i));

	spacingAction->setExclusiveGroup("spacing");
	spacingAction->setChecked(*i == defaultSpacing);
	spacingActionMenu->insert(spacingAction);
    }

    actionCollection()->insert(spacingActionMenu);

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


    new KAction
	(i18n("Insert Rest"), Key_P, this, SLOT(slotInsertRest()),
	 actionCollection(), QString("insert_rest"));

    new KAction
	(i18n("Switch from Note to Rest"), Key_T, this,
	 SLOT(slotSwitchFromNoteToRest()),
	 actionCollection(), QString("switch_from_note_to_rest"));

    new KAction
	(i18n("Switch from Rest to Note"), Key_Y, this,
	 SLOT(slotSwitchFromRestToNote()),
	 actionCollection(), QString("switch_from_rest_to_note"));


    // setup Notes menu & toolbar
    QIconSet icon;
 
    for (NoteActionDataMap::Iterator actionDataIter = m_noteActionDataMap->begin();
	 actionDataIter != m_noteActionDataMap->end();
	 ++actionDataIter) {

        NoteActionData noteActionData = *actionDataIter;
        
        icon = QIconSet
	    (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
	     (noteActionData.pixmapName)));
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
    static QString actionsAccidental[][4] = 
        {
            { i18n("No accidental"),  "1slotNoAccidental()",  "no_accidental",           "accidental-none" },
            { i18n("Sharp"),          "1slotSharp()",         "sharp_accidental",        "accidental-sharp" },
            { i18n("Flat"),           "1slotFlat()",          "flat_accidental",         "accidental-flat" },
            { i18n("Natural"),        "1slotNatural()",       "natural_accidental",      "accidental-natural" },
            { i18n("Double sharp"),   "1slotDoubleSharp()",   "double_sharp_accidental", "accidental-doublesharp" },
            { i18n("Double flat"),    "1slotDoubleFlat()",    "double_flat_accidental",  "accidental-doubleflat" }
        };

    for (unsigned int i = 0;
	 i < sizeof(actionsAccidental)/sizeof(actionsAccidental[0]); ++i) {

        icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
                        (actionsAccidental[i][3])));
        noteAction = new KRadioAction(actionsAccidental[i][0], icon, 0, this,
                                      actionsAccidental[i][1],
                                      actionCollection(), actionsAccidental[i][2]);
        noteAction->setExclusiveGroup("accidentals");
    }
    

    //
    // Clefs
    //

    // Treble
    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-treble")));
    noteAction = new KRadioAction(i18n("&Treble Clef"), icon, 0, this,
                                  SLOT(slotTrebleClef()),
                                  actionCollection(), "treble_clef");
    noteAction->setExclusiveGroup("notes");

    // Tenor
    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-tenor")));
    noteAction = new KRadioAction(i18n("Te&nor Clef"), icon, 0, this,
                                  SLOT(slotTenorClef()),
                                  actionCollection(), "tenor_clef");
    noteAction->setExclusiveGroup("notes");

    // Alto
    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-alto")));
    noteAction = new KRadioAction(i18n("&Alto Clef"), icon, 0, this,
                                  SLOT(slotAltoClef()),
                                  actionCollection(), "alto_clef");
    noteAction->setExclusiveGroup("notes");

    // Bass
    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("clef-bass")));
    noteAction = new KRadioAction(i18n("&Bass Clef"), icon, 0, this,
                                  SLOT(slotBassClef()),
                                  actionCollection(), "bass_clef");
    noteAction->setExclusiveGroup("notes");


    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("text")));
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

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("select")));
    noteAction = new KRadioAction(i18n("&Select"), icon, 0,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    noteAction->setExclusiveGroup("notes");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("step_by_step")));
    new KToggleAction(i18n("Ste&p Recording"), icon, 0, this,
                SLOT(slotToggleStepByStep()), actionCollection(),
                "toggle_step_by_step");


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

    //
    // Settings menu
    //
    int layoutMode = m_config->readNumEntry("layoutmode", 0);

    KRadioAction *linearModeAction = new KRadioAction
        (i18n("&Linear Layout"), 0, this, SLOT(slotLinearMode()),
         actionCollection(), "linear_mode");
    linearModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 0) linearModeAction->setChecked(true);

    KRadioAction *pageModeAction = new KRadioAction
        (i18n("&Page Layout"), 0, this, SLOT(slotPageMode()),
         actionCollection(), "page_mode");
    pageModeAction->setExclusiveGroup("layoutMode");
    if (layoutMode == 1) pageModeAction->setChecked(true);

    new KToggleAction(i18n("Show Ch&ord Name Ruler"), 0, this,
                      SLOT(slotToggleChordsRuler()),
                      actionCollection(), "show_chords_ruler");

    new KToggleAction(i18n("Show Ra&w Note Ruler"), 0, this,
                      SLOT(slotToggleRawNoteRuler()),
                      actionCollection(), "show_raw_note_ruler");

    new KToggleAction(i18n("Show &Tempo Ruler"), 0, this,
                      SLOT(slotToggleTempoRuler()),
                      actionCollection(), "show_tempo_ruler");

    new KToggleAction(i18n("Show &Annotations"), 0, this,
                      SLOT(slotToggleAnnotations()),
                      actionCollection(), "show_annotations");

    new KAction(i18n("Open L&yric Editor"), 0, this, SLOT(slotEditLyrics()),
		actionCollection(), "lyric_editor");

    //
    // Group menu
    //
    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("group-beam")));

    new KAction(GroupMenuBeamCommand::getGlobalName(), icon, 0, this,
                SLOT(slotGroupBeam()), actionCollection(), "beam");

    new KAction(GroupMenuAutoBeamCommand::getGlobalName(), 0, this,
                SLOT(slotGroupAutoBeam()), actionCollection(), "auto_beam");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("group-unbeam")));

    new KAction(GroupMenuBreakCommand::getGlobalName(), icon, 0, this,
                SLOT(slotGroupBreak()), actionCollection(), "break_group");
    
    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("group-simple-tuplet")));

    new KAction(GroupMenuTupletCommand::getGlobalName(true), icon, 0, this,
		SLOT(slotGroupSimpleTuplet()), actionCollection(), "simple_tuplet");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("group-tuplet")));

    new KAction(GroupMenuTupletCommand::getGlobalName(false), icon, 0, this,
		SLOT(slotGroupGeneralTuplet()), actionCollection(), "tuplet");

    new KAction(GroupMenuUnTupletCommand::getGlobalName(), 0, this,
                SLOT(slotGroupUnTuplet()), actionCollection(), "break_tuplets");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("triplet")));
    (new KToggleAction(i18n("Trip&let Insert Mode"), icon, Key_G,
		       this, SLOT(slotUpdateInsertModeStatus()),
                       actionCollection(), "triplet_mode"))->
	setChecked(false);

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap("chord")));
    (new KToggleAction(i18n("C&hord Insert Mode"), icon, Key_H,
		       this, SLOT(slotUpdateInsertModeStatus()),
		      actionCollection(), "chord_mode"))->
	setChecked(false);

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("group-grace")));

    new KAction(GroupMenuGraceCommand::getGlobalName(), icon, 0, this,
		SLOT(slotGroupGrace()), actionCollection(), "grace");

    new KAction(GroupMenuUnGraceCommand::getGlobalName(), 0, this,
		SLOT(slotGroupUnGrace()), actionCollection(), "ungrace");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("group-slur")));

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Slur), icon, 0, this,
                SLOT(slotGroupSlur()), actionCollection(), "slur");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("group-crescendo")));

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Crescendo), icon, 0, this,
                SLOT(slotGroupCrescendo()), actionCollection(), "crescendo");

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("group-decrescendo")));

    new KAction(GroupMenuAddIndicationCommand::getGlobalName
                (Rosegarden::Indication::Decrescendo), icon, 0, this,
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

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("transforms-tie")));

    new KAction(TransformsMenuTieNotesCommand::getGlobalName(), icon, 0, this,
                SLOT(slotTransformsTieNotes()), actionCollection(),
                "tie_notes");

    new KAction(TransformsMenuUntieNotesCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsUntieNotes()), actionCollection(),
                "untie_notes");

    new KAction(TransformsMenuMakeNotesViableCommand::getGlobalName(), 0, this,
		SLOT(slotTransformsMakeNotesViable()), actionCollection(),
		"make_notes_viable");

    new KAction(TransformsMenuDeCounterpointCommand::getGlobalName(), 0, this,
		SLOT(slotTransformsDeCounterpoint()), actionCollection(),
		"de_counterpoint");

    new KAction(TransformsMenuChangeStemsCommand::getGlobalName(true),
		0, Key_PageUp + CTRL, this,
                SLOT(slotTransformsStemsUp()), actionCollection(),
                "stems_up");

    new KAction(TransformsMenuChangeStemsCommand::getGlobalName(false),
		0, Key_PageDown + CTRL, this,
                SLOT(slotTransformsStemsDown()), actionCollection(),
                "stems_down");

    new KAction(TransformsMenuRestoreStemsCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsRestoreStems()), actionCollection(),
                "restore_stems");

    new KAction(TransposeCommand::getGlobalName(1), 0,
		Key_Up, this,
                SLOT(slotTransposeUp()), actionCollection(),
                "transpose_up");

    new KAction(TransposeCommand::getGlobalName(12), 0,
		Key_Up + CTRL, this,
                SLOT(slotTransposeUpOctave()), actionCollection(),
                "transpose_up_octave");

    new KAction(TransposeCommand::getGlobalName(-1), 0,
		Key_Down, this,
                SLOT(slotTransposeDown()), actionCollection(),
                "transpose_down");

    new KAction(TransposeCommand::getGlobalName(-12), 0,
		Key_Down + CTRL, this,
                SLOT(slotTransposeDownOctave()), actionCollection(),
                "transpose_down_octave");

    new KAction(TransposeCommand::getGlobalName(0), 0, this,
                SLOT(slotTranspose()), actionCollection(),
                "general_transpose");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::DoubleFlat),
		0, this,
		SLOT(slotRespellDoubleFlat()), actionCollection(),
                "respell_doubleflat");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::Flat),
		0, this,
		SLOT(slotRespellFlat()), actionCollection(),
                "respell_flat");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::Sharp),
		0, this,
		SLOT(slotRespellSharp()), actionCollection(),
                "respell_sharp");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Set, Rosegarden::Accidentals::DoubleSharp),
		0, this,
		SLOT(slotRespellDoubleSharp()), actionCollection(),
                "respell_doublesharp");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Up, Rosegarden::Accidentals::NoAccidental),
		Key_Up + CTRL + SHIFT, this,
		SLOT(slotRespellUp()), actionCollection(),
                "respell_up");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Down, Rosegarden::Accidentals::NoAccidental),
		Key_Down + CTRL + SHIFT, this,
		SLOT(slotRespellDown()), actionCollection(),
                "respell_down");

    new KAction(RespellCommand::getGlobalName
		(RespellCommand::Restore, Rosegarden::Accidentals::NoAccidental),
		0, this,
		SLOT(slotRespellRestore()), actionCollection(),
                "respell_restore");

    new KAction(EventQuantizeCommand::getGlobalName(), 0, this,
                SLOT(slotTransformsQuantize()), actionCollection(),
                "quantize");

    new KAction(TransformsMenuFixNotationQuantizeCommand::getGlobalName(), 0,
		this, SLOT(slotTransformsFixQuantization()), actionCollection(),
                "fix_quantization");

    new KAction(TransformsMenuInterpretCommand::getGlobalName(), 0,
		this, SLOT(slotTransformsInterpret()), actionCollection(),
		"interpret");

    new KAction(i18n("&Dump selected events to stderr"), 0, this,
		SLOT(slotDebugDump()), actionCollection(), "debug_dump");

    for (MarkActionDataMap::Iterator i = m_markActionDataMap->begin();
	 i != m_markActionDataMap->end(); ++i) {

        const MarkActionData &markActionData = *i;
        
        icon = QIconSet
	    (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
	     (markActionData.pixmapName)));

	new KAction(markActionData.title,
		    icon,
		    markActionData.keycode,
		    this,
		    SLOT(slotAddMark()),
		    actionCollection(),
		    markActionData.actionName);
    }

    icon = QIconSet
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
        ("text-mark")));

    new KAction(MarksMenuAddTextMarkCommand::getGlobalName(), icon, 0, this,
                SLOT(slotMarksAddTextMark()), actionCollection(),
                "add_text_mark");

    new KAction(MarksMenuRemoveMarksCommand::getGlobalName(), 0, this,
                SLOT(slotMarksRemoveMarks()), actionCollection(),
                "remove_marks");

    static QString slashTitles[] = {
	i18n("&None"), "&1", "&2", "&3", "&4", "&5"
    };
    for (int i = 0; i <= 5; ++i) {
	new KAction(slashTitles[i], 0, this,
		    SLOT(slotAddSlashes()), actionCollection(),
		    QString("slashes_%1").arg(i));
    }

    new KAction(ClefInsertionCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddClef()), actionCollection(),
                "add_clef");

    new KAction(KeyInsertionCommand::getGlobalName(), 0, this,
                SLOT(slotEditAddKeySignature()), actionCollection(),
                "add_key_signature");

    // setup Settings menu
    static QString actionsToolbars[][4] = 
        {
            { i18n("Show T&ools Toolbar"),  "1slotToggleToolsToolBar()",  "show_tools_toolbar",                    "palette-tools" },
            { i18n("Show &Notes Toolbar"),  "1slotToggleNotesToolBar()",  "show_notes_toolbar",                    "palette-notes" },
            { i18n("Show &Rests Toolbar"),  "1slotToggleRestsToolBar()",  "show_rests_toolbar",                    "palette-rests" },
            { i18n("Show &Accidentals Toolbar"),   "1slotToggleAccidentalsToolBar()",  "show_accidentals_toolbar", "palette-accidentals" },
            { i18n("Show Cle&fs Toolbar"),  "1slotToggleClefsToolBar()",  "show_clefs_toolbar",
          "palette-clefs" },
            { i18n("Show &Marks Toolbar"), "1slotToggleMarksToolBar()",   "show_marks_toolbar",
          "palette-marks" },
            { i18n("Show &Group Toolbar"), "1slotToggleGroupToolBar()",   "show_group_toolbar",
          "palette-group" },            
            { i18n("Show &Layout Toolbar"), "1slotToggleLayoutToolBar()", "show_layout_toolbar",
          "palette-font" },
            { i18n("Show Trans&port Toolbar"), "1slotToggleTransportToolBar()", "show_transport_toolbar",
	  "palette-transport" },
            { i18n("Show M&eta Toolbar"), "1slotToggleMetaToolBar()", "show_meta_toolbar",
          "palette-meta" }
        };

    for (unsigned int i = 0;
	 i < sizeof(actionsToolbars)/sizeof(actionsToolbars[0]); ++i) {

        icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap(actionsToolbars[i][3])));

        new KToggleAction(actionsToolbars[i][0], icon, 0,
                          this, actionsToolbars[i][1],
                          actionCollection(), actionsToolbars[i][2]);
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

    new KAction(i18n("Cursor Back and Se&lect"), SHIFT + Key_Left, this,
		SLOT(slotExtendSelectionBackward()), actionCollection(),
		"extend_selection_backward");

    new KAction(i18n("Cursor Forward and &Select"), SHIFT + Key_Right, this,
		SLOT(slotExtendSelectionForward()), actionCollection(),
		"extend_selection_forward");

    new KAction(i18n("Cursor Back Bar and Select"), SHIFT + CTRL + Key_Left, this,
		SLOT(slotExtendSelectionBackwardBar()), actionCollection(),
		"extend_selection_backward_bar");

    new KAction(i18n("Cursor Forward Bar and Select"), SHIFT + CTRL + Key_Right, this,
		SLOT(slotExtendSelectionForwardBar()), actionCollection(),
		"extend_selection_forward_bar");

/*!!! not here yet
    new KAction(i18n("Move Selection Left"), Key_Minus, this,
		SLOT(slotMoveSelectionLeft()), actionCollection(),
		"move_selection_left");
*/

    new KAction(i18n("Cursor to St&art"), 0, Key_A + CTRL, this,
		SLOT(slotJumpToStart()), actionCollection(),
		"cursor_start");

    new KAction(i18n("Cursor to &End"), 0, Key_E + CTRL, this,
		SLOT(slotJumpToEnd()), actionCollection(),
		"cursor_end");

    new KAction(i18n("Cursor &Up Staff"), 0, Key_Up + SHIFT, this,
		SLOT(slotCurrentStaffUp()), actionCollection(),
		"cursor_up_staff");

    new KAction(i18n("Cursor &Down Staff"), 0, Key_Down + SHIFT, this,
		SLOT(slotCurrentStaffDown()), actionCollection(),
		"cursor_down_staff");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-cursor-to-pointer")));
    new KAction(i18n("Cursor to &Playback Pointer"), icon, 0, this,
		SLOT(slotJumpCursorToPlayback()), actionCollection(),
		"cursor_to_playback_pointer");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-play")));
    new KAction(i18n("&Play"), icon, Key_Enter, this,
		SIGNAL(play()), actionCollection(), "play");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
		SIGNAL(stop()), actionCollection(), "stop");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
		SIGNAL(rewindPlayback()), actionCollection(),
		"playback_pointer_back_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
		SIGNAL(fastForwardPlayback()), actionCollection(),
		"playback_pointer_forward_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
		SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
		"playback_pointer_start");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
		SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
		"playback_pointer_end");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
		    ("transport-pointer-to-cursor")));
    new KAction(i18n("Playback Pointer to &Cursor"), icon, 0, this,
		SLOT(slotJumpPlaybackToCursor()), actionCollection(),
		"playback_pointer_to_cursor");

    icon = QIconSet(NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap
                                                 ("transport-solo")));
    new KToggleAction(i18n("&Solo"), icon, 0, this,
                SLOT(slotToggleSolo()), actionCollection(),
                "toggle_solo");

    new KAction(i18n("Set Loop to Selection"), Key_Semicolon + CTRL, this,
		SLOT(slotPreviewSelection()), actionCollection(),
		"preview_selection");

    new KAction(i18n("Clear L&oop"), Key_Colon + CTRL, this,
		SLOT(slotClearLoop()), actionCollection(),
		"clear_loop");

    new KAction(i18n("Clear Selection"), Key_Escape, this,
		SLOT(slotClearSelection()), actionCollection(),
		"clear_selection");

    QString pixmapDir =
	KGlobal::dirs()->findResource("appdata", "pixmaps/");
    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/eventfilter.xpm"));
    new KAction(i18n("&Filter Selection"), icon, 0, this,
                SLOT(slotFilterSelection()), actionCollection(),
                "filter_selection");

    createGUI(getRCFileName());
}

bool
NotationView::isInChordMode()
{
    return ((KToggleAction *)actionCollection()->action("chord_mode"))->
	isChecked();
}

bool
NotationView::isInTripletMode()
{
    return ((KToggleAction *)actionCollection()->action("triplet_mode"))->
	isChecked();
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

void NotationView::initLayoutToolbar()
{
    KToolBar *layoutToolbar = toolBar("Layout Toolbar");
    
    if (!layoutToolbar) {
        NOTATION_DEBUG
            << "NotationView::initLayoutToolbar() : layout toolbar not found\n";
        return;
    }

    new QLabel(i18n("  Font:  "), layoutToolbar, "kde toolbar widget");

    m_fontCombo = new QComboBox(layoutToolbar);
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

    new QLabel(i18n("  Size:  "), layoutToolbar, "kde toolbar widget");

    std::vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    m_fontSizeSlider = new ZoomSlider<int>
        (sizes, m_fontSize, QSlider::Horizontal, layoutToolbar, "kde toolbar widget");
    connect(m_fontSizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeFontSizeFromIndex(int)));

    new QLabel(i18n("  Spacing:  "), layoutToolbar, "kde toolbar widget");

    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();
    m_spacingSlider = new ZoomSlider<int>
        (spacings, defaultSpacing, QSlider::Horizontal, layoutToolbar, "kde toolbar widget");
    connect(m_spacingSlider, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeSpacingFromIndex(int)));
}

void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    m_hoveredOverNoteName = new QLabel(sb);
    m_hoveredOverNoteName->setMinimumWidth(32);

    m_hoveredOverAbsoluteTime = new QLabel(sb);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);

    QHBox *hbox = new QHBox(sb);
    m_currentNotePixmap = new QLabel(hbox);
    m_currentNotePixmap->setMinimumWidth(20);
    m_insertModeLabel = new QLabel(hbox);
    m_annotationsLabel = new QLabel(hbox);
    sb->addWidget(hbox);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
                         AlignLeft | AlignVCenter);

    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);

    m_progressBar = new RosegardenProgressBar(100, true, sb);
    sb->addWidget(m_progressBar);
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
	if (m_rawNoteRuler) m_rawNoteRuler->hide();
	if (m_tempoRuler) m_tempoRuler->hide();
    } else {
	if (m_topBarButtons) m_topBarButtons->show();
	if (m_bottomBarButtons) m_bottomBarButtons->show();
	if (m_chordNameRuler && getToggleAction("show_chords_ruler")->isChecked()) m_chordNameRuler->show();
	if (m_rawNoteRuler && getToggleAction("show_raw_note_ruler")->isChecked()) m_rawNoteRuler->show();
	if (m_tempoRuler && getToggleAction("show_tempo_ruler")->isChecked()) m_tempoRuler->show();
    }

    int pageWidth = getPageWidth();

    m_hlayout->setPageMode(pageMode);
    m_hlayout->setPageWidth(pageWidth);

    RG_DEBUG << "Page mode : page width = " << pageWidth << endl;
    
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->setPageMode(pageMode);
        m_staffs[i]->setPageWidth(pageWidth);
    }

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->positionAllElements();
        }
    }

    updateView();
    Rosegarden::Profiles::getInstance()->dump();
}   

int
NotationView::getPageWidth()
{
    if (isInPrintMode() && getCanvasView() && getCanvasView()->canvas())
        return getCanvasView()->canvas()->width();

    return width() - 50;
}

/// Scrolls the view such that the given time is centered
void
NotationView::scrollToTime(timeT t) {

    double notationViewLayoutCoord = m_hlayout->getXForTime(t);

    // Doesn't appear to matter which staff we use
    //!!! actually it probably does matter, if they don't have the same extents
    double notationViewCanvasCoord =
	getStaff(0)->getCanvasCoordsForLayoutCoords
	(notationViewLayoutCoord, 0).first;

    // HK: I could have sworn I saw a hard-coded scroll happen somewhere
    // (i.e. a default extra scroll to make up for the staff not beginning on
    // the left edge) but now I can't find it.
    getCanvasView()->slotScrollHorizSmallSteps
	(int(notationViewCanvasCoord));// + DEFAULT_STAFF_OFFSET);
}

Rosegarden::RulerScale*
NotationView::getHLayout()
{
    return m_hlayout;
}

Rosegarden::Staff*
NotationView::getFirstStaff()
{
    return m_staffs[0];
}

void
NotationView::paintEvent(QPaintEvent *e)
{
    if (m_hlayout->isPageMode()) {
	int diff = int(getPageWidth() - m_hlayout->getPageWidth());
	if (diff > -10 && diff < 10) {
	    m_hlayout->setPageWidth(getPageWidth());
	    refreshSegment(0, 0, 0);
	}
    }
    EditView::paintEvent(e);
}

bool NotationView::applyLayout(int staffNo, timeT startTime, timeT endTime)
{
    emit setOperationName(i18n("Laying out score..."));
    kapp->processEvents();

    m_hlayout->setStaffCount(m_staffs.size());

    START_TIMING;
    unsigned int i;

    for (i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo) continue;

        emit setOperationName(i18n("Laying out staff %1...").arg(i + 1));
        kapp->processEvents();

        m_hlayout->resetStaff(*m_staffs[i], startTime, endTime);
        m_vlayout->resetStaff(*m_staffs[i], startTime, endTime);
        m_hlayout->scanStaff(*m_staffs[i], startTime, endTime);
        m_vlayout->scanStaff(*m_staffs[i], startTime, endTime);
    }

    emit setOperationName(i18n("Reconciling staffs..."));
    kapp->processEvents();

    m_hlayout->finishLayout(startTime, endTime);
    m_vlayout->finishLayout(startTime, endTime);

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
        (NotePixmapFactory::toQPixmap(m_toolbarNotePixmapFactory.makeToolbarPixmap(pixmapName)));

    emit changeCurrentNote(rest, n);
}

void NotationView::setCurrentSelectedNote(NoteActionData noteAction)
{
    setCurrentSelectedNote(noteAction.pixmapName,
                           noteAction.rest,
                           noteAction.noteType,
                           noteAction.dots);
}


void NotationView::setCurrentSelection(EventSelection* s, bool preview,
				       bool redrawNow)
{
    //!!! rather too much here shared with matrixview -- could much of
    // this be in editview?

    if (!m_currentEventSelection && !s) return;
    NOTATION_DEBUG << "XXX " << endl;

    installProgressEventFilter();

    EventSelection *oldSelection = m_currentEventSelection;
    m_currentEventSelection = s;

    // positionElements is overkill here, but we hope it's not too
    // much overkill (if that's not a contradiction)

    timeT startA, endA, startB, endB;

    if (oldSelection) {
	startA = oldSelection->getStartTime();
	endA   = oldSelection->getEndTime();
	startB = s ? s->getStartTime() : startA;
	endB   = s ? s->getEndTime()   : endA;
    } else {
	// we know they can't both be null -- first thing we tested above
	startA = startB = s->getStartTime();
	endA   = endB   = s->getEndTime();
    }

    bool updateRequired = true;

    // play previews if appropriate -- also permits an optimisation
    // for the case where the selection is unchanged (quite likely
    // when sweeping) 

    if (s && preview) {

	bool foundNewEvent = false;

	for (EventSelection::eventcontainer::iterator i =
		 s->getSegmentEvents().begin();
	     i != s->getSegmentEvents().end(); ++i) {
	    
	    if (oldSelection && oldSelection->getSegment() == s->getSegment()
		&& oldSelection->contains(*i)) continue;
		
	    foundNewEvent = true;

	    long pitch;
	    if (!(*i)->get<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH,
					    pitch)) continue;

	    playNote(s->getSegment(), pitch);
	}

	if (!foundNewEvent) {
	    if (oldSelection &&
		oldSelection->getSegment() == s->getSegment() &&
		oldSelection->getSegmentEvents().size() ==
		s->getSegmentEvents().size()) updateRequired = false;
	}
    }

    if (updateRequired) {

	if ((endA >= startB && endB >= startA) &&
	    (!s || !oldSelection ||
	     oldSelection->getSegment() == s->getSegment())) {
	    
	    // the regions overlap: use their union and just do one refresh

	    Segment &segment(s ? s->getSegment() :
			     oldSelection->getSegment());

	    if (redrawNow) {
		// recolour the events now
		getStaff(segment)->positionElements(std::min(startA, startB),
						    std::max(endA, endB));
	    } else {
		// mark refresh status and then request a repaint
		segment.getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(segment)->getId()]).
		    push(std::min(startA, startB), std::max(endA, endB));
	    }

	} else {
	    // do two refreshes, one for each -- here we know neither is null

	    if (redrawNow) {
		// recolour the events now
		getStaff(oldSelection->getSegment())->positionElements(startA,
								       endA);
		
		getStaff(s->getSegment())->positionElements(startB, endB);
	    } else {
		// mark refresh status and then request a repaint

		oldSelection->getSegment().getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(oldSelection->getSegment())->getId()]).
		    push(startA, endA);
		
		s->getSegment().getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(s->getSegment())->getId()]).
		    push(startB, endB);
	    }
	}
    }

    delete oldSelection;

    removeProgressEventFilter();

    if (s) {
	int eventsSelected = s->getSegmentEvents().size();
	m_selectionCounter->setText
	    (i18n("  %1 event%2 selected ").
	     arg(eventsSelected).arg(eventsSelected == 1 ? "" : "s"));
    } else {
	m_selectionCounter->setText(i18n("  No selection "));
    }
    m_selectionCounter->update();

    setMenuStates();

    if (redrawNow) updateView();
    else update();

    NOTATION_DEBUG << "XXX " << endl;
}

void NotationView::setSingleSelectedEvent(int staffNo, Event *event,
					  bool preview, bool redrawNow)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event,
			   preview, redrawNow);
}

void NotationView::setSingleSelectedEvent(Segment &segment, Event *event,
					  bool preview, bool redrawNow)
{
    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);
    setCurrentSelection(selection, preview, redrawNow);
}

bool NotationView::canPreviewAnotherNote()
{
    static clock_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    clock_t now = clock();
    ++sinceLastCutOff;

    if ((((now - lastCutOff) * 1000) / CLOCKS_PER_SEC) >= 1) {
	sinceLastCutOff = 0;
	lastCutOff = now;
	NOTATION_DEBUG << "NotationView::canPreviewAnotherNote: reset" << endl;
    } else {
	if (sinceLastCutOff >= 20) {
	    // don't permit more than 20 notes per second, to avoid
	    // gungeing up the sound drivers
	    NOTATION_DEBUG << "Rejecting preview (too busy)" << endl;
	    return false;
	}
	NOTATION_DEBUG << "NotationView::canPreviewAnotherNote: ok" << endl;
    }

    return true;
}

void NotationView::playNote(Rosegarden::Segment &s, int pitch)
{
    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Track *track = comp.getTrackById(s.getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0) return;

    if (!canPreviewAnotherNote()) return;

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

    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void NotationView::showPreviewNote(int staffNo, double layoutX,
				   int pitch, int height,
				   const Rosegarden::Note &note)
{ 
    m_staffs[staffNo]->showPreviewNote(layoutX, height, note);
    playNote(m_staffs[staffNo]->getSegment(), pitch);
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

Rosegarden::Segment *
NotationView::getCurrentSegment()
{
    NotationStaff *staff = getStaff(m_currentStaff);
    return (staff ? &staff->getSegment() : 0);
}

timeT
NotationView::getInsertionTime()
{
    return m_insertionTime;
}


timeT
NotationView::getInsertionTime(Rosegarden::Clef &clef,
			       Rosegarden::Key &key)
{
    // This fuss is solely to recover the clef and key: we already
    // set m_insertionTime to the right value when we first placed
    // the insert cursor.  We could get clef and key directly from
    // the segment but the staff has a more efficient lookup

    NotationStaff *staff = m_staffs[m_currentStaff];
    double layoutX = staff->getLayoutXOfInsertCursor();
    if (layoutX < 0) layoutX = 0;
    Rosegarden::Event *clefEvt = 0, *keyEvt = 0;
    (void)staff->getElementUnderLayoutX(layoutX, clefEvt, keyEvt);
    
    if (clefEvt) clef = Rosegarden::Clef(*clefEvt);
    else clef = Rosegarden::Clef();

    if (keyEvt) key = Rosegarden::Key(*keyEvt);
    else key = Rosegarden::Key();

    return m_insertionTime;
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////



LinedStaff*
NotationView::getStaffForCanvasCoords(int x, int y) const
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	NotationStaff *s = m_staffs[i];

        if (s->containsCanvasY(y)) {
	    
	    NotationStaff::LinedStaffCoords coords = 
		s->getLayoutCoordsForCanvasCoords(x, y);

	    int barNo = m_hlayout->getBarForX(coords.first);
	    if (barNo >= m_hlayout->getFirstVisibleBarOnStaff(*s) &&
		barNo <= m_hlayout->getLastVisibleBarOnStaff(*s)) {
		return m_staffs[i];
	    }
	}
    }

    return 0;
}

void NotationView::updateView()
{
    canvas()->update();
}

void NotationView::print(KPrinter* printer)
{
    if (m_staffs.size() == 0) {
        KMessageBox::error(0, "Nothing to print");
        return;
    }

    //!!! need to know the printer resolution, I guess?
    unsigned int scaleFactor = 3;

    QPaintDeviceMetrics pdm(printer);

    unsigned int pageHeight = pdm.height(),
        staffRowSpacing = m_staffs[0]->getRowSpacing() * scaleFactor;

    RG_DEBUG << "Page height : " << pageHeight << endl;

    if (staffRowSpacing > pageHeight) {
        KMessageBox::error(0, "Too many staffs - Don't know how to print that yet");
        return;
    }

    unsigned int nbStaffRowsPerPage = pageHeight / staffRowSpacing;
    int printSliceHeight = staffRowSpacing * nbStaffRowsPerPage / scaleFactor;

    //!!! this isn't necessarily the last staff; staffs are in
    //arbitrary order

    NotationStaff* lastStaff = m_staffs[getStaffCount() - 1];

    int printY = 0,
        fullHeight = (lastStaff->getTotalHeight() + lastStaff->getY());
    
    QPainter printpainter(printer);

    RG_DEBUG << "Printing total height "  << fullHeight
             << ", nbStaffRowsPerPage = " << nbStaffRowsPerPage
             << ", printSliceHeight = "   << printSliceHeight
             << ", printer Resolution = " << printer->resolution()
             << endl;

    unsigned int pageNum = 1;

    unsigned int canvasWidth = getCanvasView()->canvas()->width();

    QWMatrix scale;

    scale.scale(scaleFactor, scaleFactor);
    printpainter.scale(scaleFactor, scaleFactor);

    // Stuff text items for debugging
//     for (int y = 0; y < canvasHeight; y += 10) {
//         QCanvasText* t = new QCanvasText(QString("y = %1").arg(y),
//                                          getCanvasView()->canvas());
//         t->setY(y);
//         t->show();
//     }
//     getCanvasView()->canvas()->update();
    // end of stuffing

    while (printY < fullHeight) {

        RG_DEBUG << "Printing from " << printY << " to "
                 << printY + printSliceHeight
                 << endl;

        QRect printRect(0, printY,
                        canvasWidth, printSliceHeight);

	// supplying doublebuffer==true to this method appears to
        // slow down printing considerably but without it we get
	// all sorts of horrible artifacts (possibly related to
	// mishandling of pixmap masks?)
        getCanvasView()->canvas()->drawArea(printRect,
                                            &printpainter, true);

        printpainter.translate(0, -printSliceHeight);

        printY += printSliceHeight;
        ++pageNum;

        if (printY < fullHeight) printer->newPage();
    }

}

void NotationView::refreshSegment(Segment *segment,
				  timeT startTime, timeT endTime)
{
    START_TIMING;
    NOTATION_DEBUG << "*** " << endl;
    Rosegarden::Profiler foo("NotationView::refreshSegment()");

    if (m_inhibitRefresh) return;

    getDocument()->getSequenceManager()->
	setTemporarySequencerSliceSize(Rosegarden::RealTime(3, 0));

    installProgressEventFilter();

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

        NOTATION_DEBUG << "NotationView::refreshSegment: "
                             << "start = " << startTime << ", end = " << endTime << ", barStart = " << barStartTime << ", barEnd = " << barEndTime << endl;

        if (thisStaff) {
            m_staffs[i]->renderElements(starti, endi);
        }
        m_staffs[i]->positionElements(barStartTime, barEndTime);
    }

    PRINT_ELAPSED("NotationView::refreshSegment (without update/GC)");

    PixmapArrayGC::deleteAll();

    removeProgressEventFilter();

    Event::dumpStats(std::cerr);
    doDeferredCursorMove();
    slotSetPointerPosition(getDocument()->getComposition().getPosition(), false);

    if (m_currentEventSelection &&
	m_currentEventSelection->getSegmentEvents().size() == 0) {
	delete m_currentEventSelection;
	m_currentEventSelection = 0;
	//!!!??? was that the right thing to do?
    }

    setMenuStates();

    PRINT_ELAPSED("NotationView::refreshSegment (including update/GC)");
    NOTATION_DEBUG << "*** " << endl;
}


void NotationView::setMenuStates()
{
    // 1. set selection-related states

    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);

    if (m_currentEventSelection) {

	NOTATION_DEBUG << "NotationView::setMenuStates: Have selection; it's " << m_currentEventSelection << " covering range from " << m_currentEventSelection->getStartTime() << " to " << m_currentEventSelection->getEndTime() << " (" << m_currentEventSelection->getSegmentEvents().size() << " events)" << endl;

	stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
	if (m_currentEventSelection->contains
	    (Rosegarden::Note::EventType)) {
	    stateChanged("have_notes_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
	if (m_currentEventSelection->contains
	    (Rosegarden::Note::EventRestType)) {
	    stateChanged("have_rests_in_selection",
			 KXMLGUIClient::StateNoReverse);
	}
    }

    // 2. set inserter-related states

    if (dynamic_cast<NoteInserter *>(m_tool)) {
	NOTATION_DEBUG << "Have note inserter " << endl;
	stateChanged("note_insert_tool_current", StateNoReverse);
	stateChanged("rest_insert_tool_current", StateReverse);
    } else if (dynamic_cast<RestInserter *>(m_tool)) {
	NOTATION_DEBUG << "Have rest inserter " << endl;
	stateChanged("note_insert_tool_current", StateReverse);
	stateChanged("rest_insert_tool_current", StateNoReverse);
    } else {
	NOTATION_DEBUG << "Have neither inserter " << endl;
	stateChanged("note_insert_tool_current", StateReverse);
	stateChanged("rest_insert_tool_current", StateReverse);
    }
}


#define UPDATE_PROGRESS(n) \
	progressCount += (n); \
	if (progressTotal > 0) { \
	    emit setProgress(progressCount * 100 / progressTotal); \
	    kapp->processEvents(); \
	}

void NotationView::readjustCanvasSize()
{
    START_TIMING;

    double maxWidth = 0.0;
    int maxHeight = 0;

    emit setOperationName(i18n("Sizing and allocating canvas..."));
    kapp->processEvents();

    int progressTotal = m_staffs.size() + 2;
    int progressCount = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        NotationStaff &staff = *m_staffs[i];

        staff.sizeStaff(*m_hlayout);
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
    
    if (noteAct != m_noteActionDataMap->end()) {
        setCurrentSelectedNote(*noteAct);
	setMenuStates();
    } else {
        NOTATION_DEBUG << "NotationView::slotNoteAction() : couldn't find NoteActionData named '"
                             << sigSender->name() << "'\n";
    }
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

    Note referenceNote(Note::Crotchet); // type doesn't matter
    for (int rest = 0; rest < 2; ++rest) {
	for (int dots = 0; dots < 2; ++dots) {
	    for (int type = Note::Longest; type >= Note::Shortest; --type) {
		if (dots && (type == Note::Longest)) continue;

		QString refName
		    (NotationStrings::getReferenceName(Note(type, dots), rest == 1));

		QString shortName(refName);
		shortName.replace(QRegExp("-"), "_");

		QString titleName
		    (NotationStrings::getNoteName(Note(type, dots)));

		titleName = titleName.left(1).upper() +
		            titleName.right(titleName.length()-1);

		if (rest) {
		    titleName.replace(QRegExp(i18n("note")), i18n("rest"));
		}

		int keycode = keys[type - Note::Shortest];
		if (dots) keycode += CTRL;
		if (rest) // keycode += SHIFT; -- can't do shift+numbers
		    keycode = 0;

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

void NotationView::setupProgress(KProgress* bar)
{
    if (bar) {
        RG_DEBUG << "NotationView::setupProgress(bar)\n";

        connect(m_hlayout, SIGNAL(setProgress(int)),
                bar,       SLOT(setValue(int)));

        connect(m_hlayout, SIGNAL(incrementProgress(int)),
                bar,       SLOT(advance(int)));

        connect(this,      SIGNAL(setProgress(int)),
                bar,       SLOT(setValue(int)));

        connect(this,      SIGNAL(incrementProgress(int)),
                bar,       SLOT(advance(int)));

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            connect(m_staffs[i], SIGNAL(setProgress(int)),
                    bar,         SLOT(setValue(int)));

            connect(m_staffs[i], SIGNAL(incrementProgress(int)),
                    bar,         SLOT(advance(int)));
        }
    }

}

void NotationView::setupProgress(RosegardenProgressDialog* dialog)
{
    RG_DEBUG << "NotationView::setupProgress(dialog)\n";
    disconnectProgress();
    
    if (dialog) {
        setupProgress(dialog->progressBar());

        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            connect(m_staffs[i], SIGNAL(setOperationName(QString)),
                    dialog,      SLOT(slotSetOperationName(QString)));

            connect(dialog, SIGNAL(cancelClicked()),
                    m_staffs[i], SLOT(slotCancel()));
                    
        }
    
        connect(this, SIGNAL(setOperationName(QString)),
                dialog, SLOT(slotSetOperationName(QString)));
        m_progressDisplayer = PROGRESS_DIALOG;
    }
    
}

void NotationView::disconnectProgress()
{
    RG_DEBUG << "NotationView::disconnectProgress()\n";

    m_hlayout->disconnect();
    disconnect(SIGNAL(setProgress(int)));
    disconnect(SIGNAL(incrementProgress(int)));
    disconnect(SIGNAL(setOperationName(QString)));

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->disconnect();
    }
}

void NotationView::setupDefaultProgress()
{
    if (m_progressDisplayer != PROGRESS_BAR) {
        RG_DEBUG << "NotationView::setupDefaultProgress()\n";
        disconnectProgress();
        setupProgress(m_progressBar);
        m_progressDisplayer = PROGRESS_BAR;
    }
}

void NotationView::installProgressEventFilter()
{
    if (m_progressDisplayer == PROGRESS_BAR &&
        !m_progressEventFilterInstalled) {
        RG_DEBUG << "NotationView::installProgressEventFilter()\n";
        kapp->installEventFilter(m_progressBar);
        m_progressEventFilterInstalled = true;
    } else {
        RG_DEBUG << "NotationView::installProgressEventFilter() - skipping install : "
                 << m_progressDisplayer << "," << PROGRESS_BAR << endl;
    }
}

void NotationView::removeProgressEventFilter()
{
    if (m_progressDisplayer == PROGRESS_BAR &&
        m_progressEventFilterInstalled) {
        RG_DEBUG << "NotationView::removeProgressEventFilter()\n";
        kapp->removeEventFilter(m_progressBar);
        m_progressBar->setValue(0);
        m_progressEventFilterInstalled = false;
    } else {
        RG_DEBUG << "NotationView::removeProgressEventFilter() - skipping remove : "
                 << m_progressDisplayer << "," << PROGRESS_BAR << endl;
    }
}
    
NotationView::NoteActionDataMap* NotationView::m_noteActionDataMap = 0;
NotationView::MarkActionDataMap* NotationView::m_markActionDataMap = 0;
const char* const NotationView::ConfigGroup = "Notation Options";

