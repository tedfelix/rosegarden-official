// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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
#include <qhbox.h>

#include <kmessagebox.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaction.h>
#include <kcommand.h>
#include <kstdaction.h>
#include <kapp.h>

#include "eventselection.h"
#include "rosegardenguidoc.h"
#include "notationview.h"
#include "notationelement.h"
#include "notationproperties.h"

#include "notationstaff.h"
#include "notepixmapfactory.h"
#include "notationtool.h"
#include "qcanvassimplesprite.h"
#include "ktmpstatusmsg.h"

#include "rosedebug.h"

#include "NotationTypes.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "Quantizer.h"
#include "staffruler.h"
#include "notationcommands.h"
#include "multiviewcommandhistory.h"

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

using std::vector;
using std::string;
using std::set;
#define ID_STATUS_MSG 1

using namespace Rosegarden::BaseProperties;

//////////////////////////////////////////////////////////////////////

NotationView::NotationViewSet NotationView::m_viewsExtant;

NotationView::NotationView(RosegardenGUIDoc* doc,
                           vector<Segment *> segments,
                           QWidget *parent) :
    KMainWindow(parent),
    m_config(kapp->config()),
    m_document(doc),
    m_currentEventSelection(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_canvasView(new NotationCanvasView(*this,
					new QCanvas(width() * 2, height() * 2),
                                        this)),
    m_lastFinishingStaff(-1),
    m_ruler(new StaffRuler(20, 0, canvas())),
    m_activeItem(0),
    m_hlayout(0),
    m_vlayout(0),
    m_tool(0),
    m_toolBox(new NotationToolBox(this)),
    m_fontSizeSlider(0),
    m_selectDefaultNote(0),
    m_pointer(0)
{
    assert(segments.size() > 0);
    kdDebug(KDEBUG_AREA) << "NotationView ctor" << endl;

    m_fontName = NotePixmapFactory::getDefaultFont();
    m_fontSize = NotePixmapFactory::getDefaultSize(m_fontName);
    m_notePixmapFactory = new NotePixmapFactory(m_fontName, m_fontSize);

    setupActions();

    kdDebug(KDEBUG_AREA) << "NotationView: Quantizer status is:\n"
                         << "Unit = " << segments[0]->getQuantizer()->getUnit()
                         << "\nMax Dots = "
                         << segments[0]->getQuantizer()->getMaxDots() << endl;

    initFontToolbar(segments[0]->getQuantizer()->getUnit());
    initStatusBar();
    
    setBackgroundMode(PaletteBase);

    setCentralWidget(m_canvasView);

    QObject::connect
        (m_canvasView, SIGNAL(itemPressed(int, int, QMouseEvent*, NotationElement*)),
         this,         SLOT  (itemPressed(int, int, QMouseEvent*, NotationElement*)));

    QObject::connect
        (m_canvasView, SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
         this,         SLOT  (activeItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
        (m_canvasView, SIGNAL(mouseMove(QMouseEvent*)),
         this,         SLOT  (mouseMove(QMouseEvent*)));

    QObject::connect
        (m_canvasView, SIGNAL(mouseRelease(QMouseEvent*)),
         this,         SLOT  (mouseRelease(QMouseEvent*)));

    QObject::connect
        (m_canvasView, SIGNAL(hoveredOverNoteChange (const QString&)),
         this,         SLOT  (hoveredOverNoteChanged(const QString&)));

    QObject::connect
        (m_canvasView, SIGNAL(hoveredOverAbsoluteTimeChange(unsigned int)),
         this,         SLOT  (hoveredOverAbsoluteTimeChange(unsigned int)));

    readOptions();

    if (segments.size() == 1) {
        setCaption(QString("%1 - Segment Instrument #%2")
                   .arg(doc->getTitle())
                   .arg(segments[0]->getInstrument()));
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
					     false,
					     width() - 50,
					     300, //!!!
                                             m_fontName, m_fontSize));
        m_staffs[i]->move(20, m_staffs[i]->getStaffHeight() * i + 45);
        m_staffs[i]->show();
    }
    m_currentStaff = 0;

    m_vlayout = new NotationVLayout();
    m_hlayout = new NotationHLayout(*m_notePixmapFactory);

    // Position pointer
    //
    m_pointer = new QCanvasLine(canvas());
    m_pointer->setPen(Qt::darkBlue);
    m_pointer->setPoints(0, 0, 0, canvas()->height());
    // m_pointer->show();

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->renderElements();
	    m_staffs[i]->positionElements();
            showBars(i);
        }
    }

    m_selectDefaultNote->activate();

    m_viewsExtant.insert(this);
}

NotationView::~NotationView()
{
    m_viewsExtant.erase(this);
    getCommandHistory()->detachView(actionCollection());

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

    // setup edit menu
    getCommandHistory()->attachView(actionCollection());

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

    new KAction(TransformsMenuTransposeOneStepCommand::name(false), 0, this,
                SLOT(slotTransformsTransposeDown()), actionCollection(),
                "transpose_down");

    // setup Settings menu
    KStdAction::showToolbar(this, SLOT(slotToggleToolBar()), actionCollection());

    static const char* actionsToolbars[][4] = 
        {
            { "Show Notes Toolbar",  "1slotToggleNotesToolBar()",  "show_notes_toolbar",                    "palette-notes" },
            { "Show Rests Toolbar",  "1slotToggleRestsToolBar()",  "show_rests_toolbar",                    "palette-rests" },
            { "Show Accidentals Toolbar",   "1slotToggleAccidentalsToolBar()",  "show_accidentals_toolbar", "palette-accidentals" },
            { "Show Clefs Toolbar",         "1slotToggleClefsToolBar()",        "show_clefs_toolbar",       "palette-clefs" }
        };

    for (unsigned int i = 0; i < 4; ++i) {

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
            this,        SLOT(changeFont(const QString &)));

    new QLabel(i18n("  Size:  "), fontToolbar);

    vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    m_fontSizeSlider = new ZoomSlider<int>
        (sizes, m_fontSize, QSlider::Horizontal, fontToolbar);
    connect(m_fontSizeSlider, SIGNAL(valueChanged(int)),
            this, SLOT(changeFontSizeFromIndex(int)));

    new QLabel(i18n("  Spacing:  "), fontToolbar);

    vector<double> spacings = NotationHLayout::getAvailableSpacings();
    QSlider *stretchSlider = new ZoomSlider<double>
        (spacings, 1.0, QSlider::Horizontal, fontToolbar);
    connect(stretchSlider, SIGNAL(valueChanged(int)),
            this, SLOT(changeStretch(int)));

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
            this, SLOT(changeLegato(int)));
}

void NotationView::initStatusBar()
{
    KStatusBar* sb = statusBar();
    
    m_currentNotePixmap       = new QLabel(sb);
    m_hoveredOverNoteName     = new QLabel(sb);
    m_hoveredOverAbsoluteTime = new QLabel(sb);

    m_currentNotePixmap->setMinimumWidth(20);
    m_hoveredOverNoteName->setMinimumWidth(32);
    m_hoveredOverAbsoluteTime->setMinimumWidth(80);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);
    sb->addWidget(m_currentNotePixmap);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
			 AlignLeft | AlignVCenter);
}

MultiViewCommandHistory *
NotationView::getCommandHistory()
{
    return getDocument()->getCommandHistory();
}

//!!! This should probably be unnecessary here and done in
//NotationStaff instead (it should be intelligent enough to query the
//notationhlayout itself)

void NotationView::showBars(int staffNo)
{
    NotationStaff &staff = *m_staffs[staffNo];
    staff.deleteBars();
    staff.deleteTimeSignatures();

    unsigned int barCount = m_hlayout->getBarLineCount(staff);

    for (unsigned int i = 0; i < barCount; ++i) {

        if (m_hlayout->isBarLineVisible(staff, i)) {

            staff.insertBar(unsigned(m_hlayout->getBarLineX(staff, i)),
                            m_hlayout->isBarLineCorrect(staff, i));

            int x;
            Event *timeSig = m_hlayout->getTimeSignatureInBar(staff, i, x);
            if (timeSig && i < barCount-1) {
                staff.insertTimeSignature(x, TimeSignature(*timeSig));
            }
        }
    }

    updateRuler();
}

void NotationView::updateRuler()
{
    if (m_lastFinishingStaff < 0 ||
	unsigned(m_lastFinishingStaff) >= m_staffs.size()) return;

    NotationStaff &staff = *m_staffs[m_lastFinishingStaff];
    TimeSignature timeSignature;

    m_ruler->clearSteps();
    
    for (unsigned int i = 0; i < m_hlayout->getBarLineCount(staff); ++i) {

        int x;
        Event *timeSigEvent = m_hlayout->getTimeSignatureInBar(staff, i, x);
        if (timeSigEvent) timeSignature = TimeSignature(*timeSigEvent);

        m_ruler->addStep(m_hlayout->getBarLineX(staff, i),
                         timeSignature.getBeatsPerBar());
    }

    m_ruler->update();
}


void
NotationView::changeStretch(int n)
{
    vector<double> spacings = m_hlayout->getAvailableSpacings();
    if (n >= (int)spacings.size()) n = spacings.size() - 1;
    m_hlayout->setSpacing(spacings[n]);

    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->positionElements();
        showBars(i);
    }

    canvas()->update();
}

void NotationView::changeLegato(int n)
{
    if (n >= (int)m_legatoDurations.size())
        n = m_legatoDurations.size() - 1;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->setLegatoDuration(m_legatoDurations[n]);
    }

    applyLayout();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->positionElements();
        showBars(i);
    }

    canvas()->update();
}

void NotationView::setCursorPosition(unsigned int n)
{
    m_ruler->setCursorPosition(n);
    canvas()->update();
}

unsigned int NotationView::getCursorPosition() const
{
    return m_ruler->getCursorPosition();
}


void
NotationView::changeFont(const QString &newName)
{
    kdDebug(KDEBUG_AREA) << "changeFont: " << newName << endl;
    changeFont(std::string(newName.latin1()));
}


void
NotationView::changeFont(string newName)
{
    changeFont(newName, NotePixmapFactory::getDefaultSize(newName));
}


void
NotationView::changeFontSize(int newSize)
{
    changeFont(m_fontName, newSize);
}


void
NotationView::changeFontSizeFromIndex(int n)
{
    vector<int> sizes = NotePixmapFactory::getAvailableSizes(m_fontName);
    if (n >= (int)sizes.size()) n = sizes.size()-1;
    changeFont(m_fontName, sizes[n]);
}


void
NotationView::changeFont(string newName, int newSize)
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
    setHLayout(new NotationHLayout(*m_notePixmapFactory));
    m_hlayout->setSpacing(spacing);

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->move(0, 0);
        m_staffs[i]->changeFont(m_fontName, m_fontSize);
        m_staffs[i]->move(20, m_staffs[i]->getStaffHeight() * i + 45);
    }

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, "Couldn't apply layout");
    else {
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {
            m_staffs[i]->renderElements();
            m_staffs[i]->positionElements();
            showBars(i);
        }
    }

    canvas()->update();
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

    timeT endTime = -1;
    m_lastFinishingStaff = -1;

    for (i = 0; i < m_staffs.size(); ++i) {
	timeT thisEndTime = m_staffs[i]->getSegment().getEndIndex();
	if (thisEndTime > endTime) {
	    endTime = thisEndTime;
	    m_lastFinishingStaff = i;
	}
    }

    readjustCanvasSize();

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

    inserter->setNote(n);
    inserter->setDots(dots);

    setTool(inserter);

    m_currentNotePixmap->setPixmap
        (m_toolbarNotePixmapFactory.makeToolbarPixmap(pixmapName));

    emit changeCurrentNote(rest, n);
}

void NotationView::setCurrentSelection(EventSelection* s)
{
    if (/* s && */ m_currentEventSelection /* &&
	s->getSegment() != m_currentEventSelection->getSegment() */) {
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

    canvas()->update();
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

void NotationView::setTool(NotationTool* tool)
{
    if (m_tool)
        m_tool->stow();

    m_tool = tool;

    if (m_tool)
        m_tool->ready();

}

void NotationView::setNotePixmapFactory(NotePixmapFactory* f)
{
    delete m_notePixmapFactory;
    m_notePixmapFactory = f;
}

void NotationView::setHLayout(NotationHLayout* l)
{
    delete m_hlayout;
    m_hlayout = l;
}

PositionCursor* NotationView::getCursor()
{
    return getRuler()->getCursor();
}

string NotationView::getNoteNameAtCoordinates(int x, int y) const
{
//!!!
    int i = ((NotationView *)this)->findClosestStaff(y);
    if (i < 0) return "";

    Rosegarden::Clef clef;
    Rosegarden::Key key;
    m_staffs[i]->getClefAndKeyAtX(x, clef, key);

    return
	Rosegarden::NotationDisplayPitch
	(getHeightAtY(y), m_currentAccidental).getAsString(clef, key);
}



//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

void NotationView::closeWindow()
{
    close();
}

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

    redoLayout(&m_currentEventSelection->getSegment(),
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
    Event *tsig = 0, *clef = 0, *key = 0;

    int staffNo = 0; // TODO : how to select on which staff we're pasting ?
    
    NotationElementList::iterator closestNote =
        findClosestNote(getCursorPosition(),
                        tsig, clef, key, staffNo);

    if (closestNote == getStaff(staffNo)->getViewElementList()->end()) {
        slotStatusHelpMsg(i18n("Couldn't paste at this point"));
        return;
    }

    timeT time = (*closestNote)->getAbsoluteTime();

    Segment& segment = getStaff(staffNo)->getSegment();

    if (m_currentEventSelection->pasteToSegment(segment, time)) {

        redoLayout(&segment,
                   0,
                   time + m_currentEventSelection->getTotalDuration() + 1);

    } else {
        
        slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    }
}

//
// Toolbar and statusbar toggling
//
void NotationView::slotToggleToolBar()
{
    KTmpStatusMsg msg(i18n("Toggle the toolbar..."), statusBar());

    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();
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

void NotationView::slotToggleStatusBar()
{
    KTmpStatusMsg msg(i18n("Toggle the statusbar..."), statusBar());

    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();
}

//
// Group stuff
//

void NotationView::slotGroupBeam()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Beaming group..."), statusBar());

    getCommandHistory()->addCommand(new GroupMenuBeamCommand
				    (*m_currentEventSelection));
}

void NotationView::slotGroupAutoBeam()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Auto-beaming selection..."), statusBar());

    getCommandHistory()->addCommand(new GroupMenuAutoBeamCommand
				    (*m_currentEventSelection));
}

void NotationView::slotGroupBreak()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Breaking groups..."), statusBar());

    getCommandHistory()->addCommand(new GroupMenuBreakCommand
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
    
    getCommandHistory()->addCommand(command);

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
    
    getCommandHistory()->addCommand(command);

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
    
    getCommandHistory()->addCommand(command);

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

    getCommandHistory()->addCommand(new TransformsMenuNormalizeRestsCommand
				    (*m_currentEventSelection));
}

void NotationView::slotTransformsCollapseRests()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Collapsing rests..."), statusBar());

    getCommandHistory()->addCommand(new TransformsMenuCollapseRestsCommand
				    (*m_currentEventSelection));
}

void NotationView::slotTransformsStemsUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pointing stems up..."), statusBar());

    getCommandHistory()->addCommand(new TransformsMenuChangeStemsCommand
				    (true, *m_currentEventSelection));
}

void NotationView::slotTransformsStemsDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Pointing stems down..."), statusBar());

    getCommandHistory()->addCommand(new TransformsMenuChangeStemsCommand
				    (false, *m_currentEventSelection));
}

void NotationView::slotTransformsRestoreStems()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Restoring computed stem directions..."), statusBar());

    getCommandHistory()->addCommand(new TransformsMenuRestoreStemsCommand
				    (*m_currentEventSelection));
}

void NotationView::slotTransformsTransposeUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one semitone..."), statusBar());

    getCommandHistory()->addCommand(new TransformsMenuTransposeOneStepCommand
				    (true, *m_currentEventSelection));
}

void NotationView::slotTransformsTransposeDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one semitone..."), statusBar());

    getCommandHistory()->addCommand(new TransformsMenuTransposeOneStepCommand
				    (false, *m_currentEventSelection));
}



//
// Status messages
//
void NotationView::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void NotationView::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}


// Code required to work out where to put the pointer
// (i.e. where is the nearest note) and also if indeed
// it should be currently shown at all for this view
// (is it within scope)
//!!! No consideration of scope yet
// 
void
NotationView::setPositionPointer(const int& position)
{
    if (m_lastFinishingStaff < 0 ||
	unsigned(m_lastFinishingStaff) >= m_staffs.size()) return;

    kdDebug(KDEBUG_AREA) << "NotationView::setPositionPointer: position is "
			 << position << endl;

    Rosegarden::Composition &comp = m_document->getComposition();

    int barNo = comp.getBarNumber(position, true);
    pair<timeT, timeT> times = comp.getBarRange(position);

    double canvasPosition = m_hlayout->getTotalWidth();

    NotationStaff &staff = *m_staffs[m_lastFinishingStaff];

    if (unsigned(barNo) < m_hlayout->getBarLineCount(staff)) {

	canvasPosition = m_hlayout->getBarLineX(staff, barNo);

	if (times.first != times.second) {

	    double barWidth;
	    if (unsigned(barNo) + 1 < m_hlayout->getBarLineCount(staff)) {
		barWidth =
		    m_hlayout->getBarLineX(staff, barNo + 1) - canvasPosition;
	    } else {
		barWidth = m_hlayout->getTotalWidth() - canvasPosition;
	    }

	    canvasPosition += barWidth * (position - times.first) /
		(times.second - times.first);
	}
    }

    m_pointer->setX(canvasPosition);
    canvas()->update();
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

//----------------------------------------------------------------------

void NotationView::itemPressed(int height, int staffNo,
                               QMouseEvent* e,
                               NotationElement* el)
{
    kdDebug(KDEBUG_AREA) << "NotationView::itemPressed(height = "
                         << height << ", staffNo = " << staffNo
                         << ")\n";

    ButtonState btnState = e->state();

    if (btnState & ShiftButton) { // on shift-click, set cursor position

        setCursorPosition(e->x());

    } else {

        setActiveItem(0);

        // This won't work because a double click event is always
        // preceded by a single click event

        if (e->type() == QEvent::MouseButtonDblClick)
            m_tool->handleMouseDblClick(height, staffNo, e, el);
        else
            m_tool->handleMousePress(height, staffNo, e, el);
    }
    
}

void NotationView::activeItemPressed(QMouseEvent* e,
                                     QCanvasItem* item)
{
    if (!item) return;

    // Check if it's a groupable item, if so get its group
    //
    QCanvasGroupableItem *gitem = dynamic_cast<QCanvasGroupableItem*>(item);
    if (gitem) item = gitem->group();
        
    // Check if it's an active item
    //
    ActiveItem *activeItem = dynamic_cast<ActiveItem*>(item);
        
    if (activeItem) {

        setActiveItem(activeItem);
        activeItem->handleMousePress(e);
        canvas()->update();

    }

}

void NotationView::mouseMove(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseMove(e);
        canvas()->update();
    }
    else
        m_tool->handleMouseMove(e);
}

void NotationView::mouseRelease(QMouseEvent *e)
{
    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        canvas()->update();
    }
    else
        m_tool->handleMouseRelease(e);
}

int NotationView::findClosestStaff(double eventY)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        int top = m_staffs[i]->getStaffHeight() * i;
        int bottom = top + m_staffs[i]->getStaffHeight() + 45;
        if (eventY >= top && eventY < bottom) return i;
    }
    return -1;
}


NotationElementList::iterator
NotationView::findClosestNote(double eventX, Event *&timeSignature,
                              Event *&clef, Event *&key, int staffNo,
                              unsigned int proximityThreshold)
{
    START_TIMING;

    double minDist = 10e9,
        prevDist = 10e9;

    NotationElementList *notes = m_staffs[staffNo]->getViewElementList();

    NotationElementList::iterator it, res;

    // TODO: this is grossly inefficient
    //
    // Possible optimization : make a QRect of width = 2*proximityThreshold,
    // height = staffHeight, centered at eventX. Get canvas items in this
    // rectangle (QCanvas::collisions(QRect), and scan this item list only
    // (though we'd still have to scan back, potentially to the start of
    // the segment, to find the clef and key).
    //
    for (it = notes->begin();
         it != notes->end(); ++it) 
{
        if (!(*it)->isNote() && !(*it)->isRest()) {
            if ((*it)->event()->isa(Clef::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found clef: type is "
                                     << (*it)->event()->get<String>(Clef::ClefPropertyName) << endl;
                clef = (*it)->event();
            } else if ((*it)->event()->isa(TimeSignature::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found time sig " << endl;
                timeSignature = (*it)->event();
            } else if ((*it)->event()->isa(Rosegarden::Key::EventType)) {
                kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : found key: type is "
                                     << (*it)->event()->get<String>(Rosegarden::Key::KeyPropertyName) << endl;
                key = (*it)->event();
            }
            continue;
        }

        double dist;
        
        if ( (*it)->getEffectiveX() >= eventX )
            dist = (*it)->getEffectiveX() - eventX;
        else
            dist = eventX - (*it)->getEffectiveX();

        if (dist < minDist) {
            kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : minDist was "
                                 << minDist << " now = " << dist << endl;
            minDist = dist;
            res = it;
        }
        
        // not sure about this
        if (dist > prevDist) break; // we can stop right now

        prevDist = dist;
    }

    if (minDist > proximityThreshold) {
        kdDebug(KDEBUG_AREA) << "NotationView::findClosestNote() : element is too far away : "
                             << minDist << endl;
        return notes->end();
    }
        
    PRINT_ELAPSED("NotationView::findClosestNote");

    return res;
}


void NotationView::redoLayout(Segment *segment, timeT startTime, timeT endTime)
{
    for (NotationViewSet::iterator i = m_viewsExtant.begin();
         i != m_viewsExtant.end(); ++i) {
        (*i)->redoLayoutAdvised(segment, startTime, endTime);
    }
}


void NotationView::redoLayoutAdvised(Segment *segment,
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
	
        if (startTime > 0) {
            barStartTime = ssegment->getBarStart(startTime);
//            starti = notes->findTime(barStartTime);
            starti = notes->findTime(startTime);
        }

        if (endTime >= 0) {
	    barEndTime = ssegment->getBarEnd(endTime);
//            endi = notes->findTime(barEndTime);
            endi = notes->findTime(endTime);
        }

	kdDebug(KDEBUG_AREA) << "NotationView::redoLayoutAdvised: "
			     << "start = " << startTime << ", end = " << endTime << ", barStart = " << barStartTime << ", barEnd = " << barEndTime << endl;

	if (thisStaff) {
	    m_staffs[i]->renderElements(starti, endi);
	}
	m_staffs[i]->positionElements(barStartTime, barEndTime);
        showBars(i);
    }

    PRINT_ELAPSED("NotationView::redoLayoutAdvised (without update/GC)");

    canvas()->update();
    PixmapArrayGC::deleteAll();

    Event::dumpStats(cerr);

    PRINT_ELAPSED("NotationView::redoLayoutAdvised (including update/GC)");
}


//!!! Some of this should be in NotationStaff (at the very least it
//should be able to report how much width and height it needs based on
//its own bar line positions which it's calculated by querying the
//notationhlayout)

void NotationView::readjustCanvasSize()
{
    START_TIMING;

    double totalHeight = 0.0, totalWidth = m_hlayout->getTotalWidth();

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        double xleft = 0, xright = totalWidth;

        NotationStaff &staff = *m_staffs[i];
        unsigned int barCount = m_hlayout->getBarLineCount(staff);

        for (unsigned int j = 0; j < barCount; ++j) {
            if (m_hlayout->isBarLineVisible(staff, j)) {
                xleft = m_hlayout->getBarLineX(staff, j);
                break;
            }
        }

        if (barCount > 0) {
            xright = m_hlayout->getBarLineX(staff, barCount - 1);
        }

        kdDebug(KDEBUG_AREA) << "Setting staff lines to " << xleft <<
            " -> " << xright << endl;
        staff.setLines(xleft, xright);

        totalHeight += staff.getStaffHeight() + 15;
    }

    // We want to avoid resizing the canvas too often, so let's make
    // sure it's always an integer multiple of the view's dimensions
    
    int iWidth = (int)width(), iHeight = (int)height();
    int widthMultiple = ((int)(totalWidth + 50) / iWidth) + 1;
    int heightMultiple = ((int)(totalHeight + 50) / iHeight) + 1;

    if (canvas()->width() < (widthMultiple * iWidth) ||
	canvas()->height() < (heightMultiple * iHeight)) {

	cerr/*        kdDebug(KDEBUG_AREA)*/ << "NotationView::readjustCanvasWidth() to "
					     << totalWidth << " (iWidth is " << iWidth << " so need width of " << (widthMultiple * iWidth) << ")" << endl;

	canvas()->resize(std::max(widthMultiple * iWidth,
				  (int)canvas()->width()),
			 std::max(heightMultiple * iHeight,
				  (int)canvas()->height()));

//        canvas()->resize(int(totalWidth) + 50, int(totalHeight) + 50);
        m_ruler->resize();
    }

    PRINT_ELAPSED("NotationView::readjustCanvasWidth total");
}

void
NotationView::hoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(QString(" ") + noteName);
}

void
NotationView::hoveredOverAbsoluteTimeChange(unsigned int time)
{
    m_hoveredOverAbsoluteTime->setText(QString(" Time: %1").arg(time));
}

