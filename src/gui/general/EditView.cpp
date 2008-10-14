/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include <Q3Canvas>
#include <Q3CanvasItem>
#include <Q3CanvasPixmap>
#include "EditView.h"
#include <QLayout>

#include "base/BaseProperties.h"
#include <klocale.h>
#include <QSettings>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "ActiveItem.h"
#include "document/CommandRegistry.h"
#include "base/AnalysisTypes.h"
#include "base/Composition.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/Controllable.h"
#include "base/ControlParameter.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/MidiTypes.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/Property.h"
#include "base/PropertyName.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/SoftSynthDevice.h"
#include "base/Staff.h"
#include "base/Studio.h"
#include "base/ViewElement.h"
#include "commands/edit/InvertCommand.h"
#include "commands/edit/MoveCommand.h"
#include "commands/edit/RescaleCommand.h"
#include "commands/edit/RetrogradeCommand.h"
#include "commands/edit/RetrogradeInvertCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "EditViewBase.h"
#include "gui/dialogs/RescaleDialog.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/rulers/StandardRuler.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "gui/kdeext/QCanvasGroupableItem.h"
#include "gui/rulers/ControllerEventsRuler.h"
#include "gui/rulers/ControlRuler.h"
#include "gui/rulers/PropertyControlRuler.h"
#include "RosegardenCanvasView.h"
#include <QAction>
#include "document/Command.h"
#include <QDockWidget>
//#include <kglobal.h>
//#include <kiconloader.h>
//#include <kstandarddirs.h>
#include <ktabwidget.h>
//#include <kxmlguiclient.h>
#include <QPushButton>
#include <QDialog>
#include <QFrame>
#include <QInputDialog>
#include <QLabel>
#include <QObjectList>
#include <QMenu>
#include <QSize>
#include <QString>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QMatrix>


namespace Rosegarden
{

const unsigned int EditView::CONTROLS_ROW         = 0;
const unsigned int EditView::RULERS_ROW           = CONTROLS_ROW + 1;
const unsigned int EditView::TOPBARBUTTONS_ROW    = RULERS_ROW + 1;
const unsigned int EditView::CANVASVIEW_ROW       = TOPBARBUTTONS_ROW + 1;
const unsigned int EditView::CONTROLRULER_ROW     = CANVASVIEW_ROW + 1;

// Just some simple features we might want to show - make them bit maskable
//
static int FeatureShowVelocity = 0x00001; // show the velocity ruler

EditView::EditView(RosegardenGUIDoc *doc,
                   std::vector<Segment *> segments,
                   unsigned int cols,
                   QWidget *parent, const char *name) :
        EditViewBase(doc, segments, cols, parent, name),
        m_currentEventSelection(0),
        m_activeItem(0),
        m_canvasView(0),
        m_rulerBox(new QVBoxLayout),  // top ruler box - added to grid later on
        m_rulerBoxFiller(0),          // On the left of m_rulerBox
        m_controlBox(new QVBoxLayout),  // top control ruler box - added to grid later on
        m_bottomBox(new QWidget(this)),  // bottom box - added to bottom of canvas view by setCanvasView()
        m_topStandardRuler(0),
        m_bottomStandardRuler(0),
        m_controlRuler(0),
        m_controlRulers(new KTabWidget(getBottomWidget(), "controlrulers"))
{
//!!!kiftsgate    m_commandRegistry = new CommandRegistry(this);
    m_bottomBox->setObjectName("bottomframe");
    m_bottomBox->setLayout(new QVBoxLayout);
    //@@@ Widget should be had to m_bottomBox using something as
    //@@@         getBottomWidget()->layout()->addWidget(widgetPtr);
    //@@@ All widgets added or some of them forgotten ?

    m_controlRulers->setHoverCloseButton(true);
    m_controlRulers->setHoverCloseButtonDelayed(false);
    connect(m_controlRulers, SIGNAL(closeRequest(QWidget*)),
            this, SLOT(slotRemoveControlRuler(QWidget*)));

    (dynamic_cast<QBoxLayout*>(m_bottomBox->layout()))->setDirection(QBoxLayout::BottomToTop);

    // m_rulerBoxFiller is a white label used to keep m_rulerBox exactly
    // above the scrolling part of the view (and never above the
    // RosegardenCanvasView::m_leftWidget).
    QGridLayout * gl = new QGridLayout; 
    gl->setColumnStretch(0, 0);
    gl->setColumnStretch(1, 1);
    gl->addLayout(m_rulerBox, 0, 1);
    m_rulerBoxFiller = new QLabel(getCentralWidget());
    gl->addWidget(m_rulerBoxFiller, 0, 0);
    m_rulerBoxFiller->hide();

    m_grid->addLayout(gl, RULERS_ROW, m_mainCol);

    m_grid->addLayout(m_controlBox, CONTROLS_ROW, 0, 1, 2);
    m_controlBox->setAlignment(Qt::AlignRight);
    //     m_grid->addWidget(m_controlRulers, CONTROLRULER_ROW, 2);

    m_controlRulers->hide();
    m_controlRulers->setTabPosition(QTabWidget::Bottom);
}

EditView::~EditView()
{
    delete m_currentEventSelection;
    m_currentEventSelection = 0;

    delete m_commandRegistry;
}

void EditView::updateBottomWidgetGeometry()
{
    getBottomWidget()->layout()->invalidate();
    getBottomWidget()->updateGeometry();
    getCanvasView()->updateBottomWidgetGeometry();
}

void EditView::paintEvent(QPaintEvent* e)
{
    RG_DEBUG << "EditView::paintEvent()\n";
    EditViewBase::paintEvent(e);

    if (m_needUpdate) {
        RG_DEBUG << "EditView::paintEvent() - calling updateView\n";
        updateView();
        getCanvasView()->slotUpdate();

        // update rulers
        QLayoutIterator it = m_rulerBox->iterator();
        QLayoutItem *child;
        while ( (child = it.current()) != 0 ) {
            if (child->widget())
                child->widget()->update();
            ++it;
        }

        updateControlRulers();

    } else {

        getCanvasView()->slotUpdate();
        updateControlRulers();

    }

    m_needUpdate = false;
}

void EditView::updateControlRulers(bool updateHPos)
{
    for (int i = 0; i < m_controlRulers->count(); ++i) {
        ControlRuler* ruler = dynamic_cast<ControlRuler*>(m_controlRulers->page(i));
        if (ruler) {
            if (updateHPos)
                ruler->slotUpdateElementsHPos();
            else
                ruler->slotUpdate();
        }
    }
}

void EditView::setControlRulersZoom(QMatrix zoomMatrix)
{
    m_currentRulerZoomMatrix = zoomMatrix;

    for (int i = 0; i < m_controlRulers->count(); ++i) {
        ControlRuler* ruler = dynamic_cast<ControlRuler*>(m_controlRulers->page(i));
        if (ruler)
            ruler->setWorldMatrix(zoomMatrix);
    }
}

void EditView::setControlRulersCurrentSegment()
{
    RG_DEBUG << "EditView::setControlRulersCurrentSegment: visible is " << m_controlRulers->isVisible() << endl;

    bool visible = m_controlRulers->isVisible();

    delete m_controlRulers;
    m_controlRulers = new KTabWidget(getBottomWidget(), "controlrulers");

    bool haveTabs = setupControllerTabs();
    setupAddControlRulerMenu();

    if (haveTabs)
        m_controlRulers->show();
    else
        m_controlRulers->hide();

    updateBottomWidgetGeometry();

    /*
        for (int i = 0; i < m_controlRulers->count(); ++i) {
     
            PropertyControlRuler *pcr = dynamic_cast<PropertyControlRuler *>
    	    (m_controlRulers->page(i));
     
            if (pcr) pcr->setStaff(getCurrentStaff());
    	else {
     
    	    ControllerEventsRuler *cer = dynamic_cast<ControllerEventsRuler *>
    		(m_controlRulers->page(i));
     
    	    if (cer) cer->setSegment(getCurrentSegment());
    	}
        }
    */
}

void EditView::setTopStandardRuler(StandardRuler* w, QWidget *leftBox)
{
    delete m_topStandardRuler;
    m_topStandardRuler = w;

    QGridLayout * gl = new QGridLayout;
    gl->setColumnStretch(0, 0);
    gl->setColumnStretch(1, 1);

    gl->addWidget(w, 0, 1);
    if (leftBox) {
        gl->addWidget(leftBox, 0, 0);
    }

    m_grid->addLayout(gl, TOPBARBUTTONS_ROW, m_mainCol);

    if (m_canvasView) {
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                m_topStandardRuler, SLOT(slotScrollHoriz(int)));
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                m_topStandardRuler, SLOT(slotScrollHoriz(int)));
    }
}

void EditView::setBottomStandardRuler(StandardRuler* w)
{
    delete m_bottomStandardRuler;
    m_bottomStandardRuler = w;

    //     m_bottomBox->insertWidget(0, w);

    if (m_canvasView) {
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                m_bottomStandardRuler, SLOT(slotScrollHoriz(int)));
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                m_bottomStandardRuler, SLOT(slotScrollHoriz(int)));
    }
}

void EditView::setRewFFwdToAutoRepeat()
{
    QWidget* transportToolbar = factory()->container("Transport Toolbar", this);

    if (transportToolbar) {
        QObjectList *l = transportToolbar->queryList();
        QObjectListIt it(*l); // iterate over the buttons
        QObject *obj;

        while ( (obj = it.current()) != 0 ) {
            // for each found object...
            ++it;
            //             RG_DEBUG << "EditView::setRewFFwdToAutoRepeat() : obj name : " << obj->objectName() << endl;
            QString objName = obj->objectName();

            if (objName.endsWith("playback_pointer_back_bar") || objName.endsWith("playback_pointer_forward_bar")) {
                QPushButton* btn = dynamic_cast<QPushButton*>(obj);
                if (!btn) {
                    RG_DEBUG << "Very strange - found widgets in Transport Toolbar which aren't buttons\n";

                    continue;
                }
                btn->setAutoRepeat(true);
            }


        }
        delete l;

    } else {
        RG_DEBUG << "transportToolbar == 0\n";
    }

}

void EditView::addRuler(QWidget* w)
{
    m_rulerBox->addWidget(w);

    if (m_canvasView) {
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                w, SLOT(slotScrollHoriz(int)));
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                w, SLOT(slotScrollHoriz(int)));
    }
}

void EditView::addPropertyBox(QWidget *w)
{
    m_controlBox->addWidget(w);
}

void EditView::addControlRuler(ControlRuler* ruler)
{
    ruler->setWorldMatrix(m_currentRulerZoomMatrix);
    m_controlRulers->addTab(ruler, KGlobal::iconLoader()->loadIconSet("fileclose", KIcon::Small),
                            ruler->getName());
    m_controlRulers->showPage(ruler);

    if (m_canvasView) {
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)),
                ruler->horizontalScrollBar(), SIGNAL(valueChanged(int)));
        connect(m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
                ruler->horizontalScrollBar(), SIGNAL(sliderMoved(int)));
    }

    connect(ruler, SIGNAL(stateChange(const QString&, bool)),
            this, SLOT(slotStateChanged(const QString&, bool)));
    
    //!!! This looks wrong -- surely we're entering this state?  But
    // this is definitely what the old code says
    leaveActionState("have_control_ruler");
//    stateChanged("have_control_ruler", KXMLGUIClient::StateReverse);
}

void EditView::readjustViewSize(QSize requestedSize, bool exact)
{
    Profiler profiler("EditView::readjustViewSize", true);

    if (exact) {
        RG_DEBUG << "EditView::readjustViewSize: exact size requested ("
        << requestedSize.width() << ", " << requestedSize.height()
        << ")\n";

        setViewSize(requestedSize);
        getCanvasView()->slotUpdate();
        return ;
    }

    int requestedWidth = requestedSize.width(),
        requestedHeight = requestedSize.height(),
        windowWidth = width(),
        windowHeight = height();

    QSize newSize;

    newSize.setWidth(((requestedWidth / windowWidth) + 1) * windowWidth);
    newSize.setHeight(((requestedHeight / windowHeight) + 1) * windowHeight);

    RG_DEBUG << "EditView::readjustViewSize: requested ("
    << requestedSize.width() << ", " << requestedSize.height()
    << "), getting (" << newSize.width() << ", "
    << newSize.height() << ")" << endl;

    setViewSize(newSize);

    getCanvasView()->slotUpdate();
}

void EditView::setCanvasView(RosegardenCanvasView *canvasView)
{
    delete m_canvasView;
    m_canvasView = canvasView;
    m_grid->addWidget(m_canvasView, CANVASVIEW_ROW, m_mainCol);
    m_canvasView->setBottomFixedWidget(m_bottomBox);

    // TODO : connect canvas view's horiz. scrollbar to top/bottom bars and rulers

    //     m_horizontalScrollBar->setRange(m_canvasView->horizontalScrollBar()->minimum(),
    //                                     m_canvasView->horizontalScrollBar()->maximum());

    //     m_horizontalScrollBar->setSteps(m_canvasView->horizontalScrollBar()->lineStep(),
    //                                     m_canvasView->horizontalScrollBar()->pageStep());

    //     connect(m_horizontalScrollBar, SIGNAL(valueChanged(int)),
    //             m_canvasView->horizontalScrollBar(), SIGNAL(valueChanged(int)));
    //     connect(m_horizontalScrollBar, SIGNAL(sliderMoved(int)),
    //             m_canvasView->horizontalScrollBar(), SIGNAL(sliderMoved(int)));

}

Device *
EditView::getCurrentDevice()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return 0;

    Studio &studio = getDocument()->getStudio();
    Instrument *instrument =
        studio.getInstrumentById
        (segment->getComposition()->getTrackById(segment->getTrack())->
         getInstrument());
    if (!instrument)
        return 0;

    return instrument->getDevice();
}

timeT
EditView::getInsertionTime(Clef &clef,
                           Rosegarden::Key &key)
{
    timeT t = getInsertionTime();
    Segment *segment = getCurrentSegment();

    if (segment) {
        clef = segment->getClefAtTime(t);
        key = segment->getKeyAtTime(t);
    } else {
        clef = Clef();
        key = ::Rosegarden::Key();
    }

    return t;
}

void EditView::slotActiveItemPressed(QMouseEvent* e,
                                     Q3CanvasItem* item)
{
    if (!item)
        return ;

    // Check if it's a groupable item, if so get its group
    //
    QCanvasGroupableItem *gitem = dynamic_cast<QCanvasGroupableItem*>(item);
    if (gitem)
        item = gitem->group();

    // Check if it's an active item
    //
    ActiveItem *activeItem = dynamic_cast<ActiveItem*>(item);

    if (activeItem) {

        setActiveItem(activeItem);
        activeItem->handleMousePress(e);
        updateView();

    }
}

void
EditView::slotStepBackward()
{
    Staff *staff = getCurrentStaff();
    if (!staff)
        return ;
    ViewElementList *vel = staff->getViewElementList();

    timeT time = getInsertionTime();
    ViewElementList::iterator i = vel->findTime(time);

    while (i != vel->begin() &&
            (i == vel->end() || (*i)->getViewAbsoluteTime() >= time))
        --i;

    if (i != vel->end())
        slotSetInsertCursorPosition((*i)->getViewAbsoluteTime());
}

void
EditView::slotStepForward()
{
    Staff *staff = getCurrentStaff();
    if (!staff)
        return ;
    ViewElementList *vel = staff->getViewElementList();

    timeT time = getInsertionTime();
    ViewElementList::iterator i = vel->findTime(time);

    while (i != vel->end() &&
            (*i)->getViewAbsoluteTime() <= time)
        ++i;

    if (i == vel->end()) {
        slotSetInsertCursorPosition(staff->getSegment().getEndMarkerTime());
    } else {
        slotSetInsertCursorPosition((*i)->getViewAbsoluteTime());
    }
}

void
EditView::slotJumpBackward()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return ;
    timeT time = getInsertionTime();
    time = segment->getBarStartForTime(time - 1);
    slotSetInsertCursorPosition(time);
}

void
EditView::slotJumpForward()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return ;
    timeT time = getInsertionTime();
    time = segment->getBarEndForTime(time);
    slotSetInsertCursorPosition(time);
}

void
EditView::slotJumpToStart()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return ;
    timeT time = segment->getStartTime();
    slotSetInsertCursorPosition(time);
}

void
EditView::slotJumpToEnd()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return ;
    timeT time = segment->getEndMarkerTime();
    slotSetInsertCursorPosition(time);
}

void EditView::slotExtendSelectionBackward()
{
    slotExtendSelectionBackward(false);
}

void EditView::slotExtendSelectionBackwardBar()
{
    slotExtendSelectionBackward(true);
}

void EditView::slotExtendSelectionBackward(bool bar)
{
    // If there is no current selection, or the selection is entirely
    // to the right of the cursor, move the cursor left and add to the
    // selection

    timeT oldTime = getInsertionTime();
    if (bar)
        slotJumpBackward();
    else
        slotStepBackward();
    timeT newTime = getInsertionTime();

    Staff *staff = getCurrentStaff();
    if (!staff)
        return ;
    Segment *segment = &staff->getSegment();
    ViewElementList *vel = staff->getViewElementList();

    EventSelection *es = new EventSelection(*segment);
    if (m_currentEventSelection &&
            &m_currentEventSelection->getSegment() == segment)
        es->addFromSelection(m_currentEventSelection);

    if (!m_currentEventSelection ||
            &m_currentEventSelection->getSegment() != segment ||
            m_currentEventSelection->getSegmentEvents().size() == 0 ||
            m_currentEventSelection->getStartTime() >= oldTime) {

        ViewElementList::iterator extendFrom = vel->findTime(oldTime);

        while (extendFrom != vel->begin() &&
                (*--extendFrom)->getViewAbsoluteTime() >= newTime) {
            if ((*extendFrom)->event()->isa(Note::EventType)) {
                es->addEvent((*extendFrom)->event());
            }
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

void EditView::slotExtendSelectionForward()
{
    slotExtendSelectionForward(false);
}

void EditView::slotExtendSelectionForwardBar()
{
    slotExtendSelectionForward(true);
}

void EditView::slotExtendSelectionForward(bool bar)
{
    // If there is no current selection, or the selection is entirely
    // to the left of the cursor, move the cursor right and add to the
    // selection

    timeT oldTime = getInsertionTime();
    if (bar)
        slotJumpForward();
    else
        slotStepForward();
    timeT newTime = getInsertionTime();

    Staff *staff = getCurrentStaff();
    if (!staff)
        return ;
    Segment *segment = &staff->getSegment();
    ViewElementList *vel = staff->getViewElementList();

    EventSelection *es = new EventSelection(*segment);
    if (m_currentEventSelection &&
            &m_currentEventSelection->getSegment() == segment)
        es->addFromSelection(m_currentEventSelection);

    if (!m_currentEventSelection ||
            &m_currentEventSelection->getSegment() != segment ||
            m_currentEventSelection->getSegmentEvents().size() == 0 ||
            m_currentEventSelection->getEndTime() <= oldTime) {

        ViewElementList::iterator extendFrom = vel->findTime(oldTime);

        while (extendFrom != vel->end() &&
                (*extendFrom)->getViewAbsoluteTime() < newTime) {
            if ((*extendFrom)->event()->isa(Note::EventType)) {
                es->addEvent((*extendFrom)->event());
            }
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

void
EditView::setupActions()
{
    createInsertPitchActionMenu();

    createAction("add_tempo", SLOT(slotAddTempo()));
    createAction("add_time_signature", SLOT(slotAddTimeSignature()));
    createAction("halve_durations", SLOT(slotHalveDurations()));
    createAction("double_durations", SLOT(slotDoubleDurations()));
    createAction("rescale", SLOT(slotRescale()));
    createAction("transpose_up", SLOT(slotTransposeUp()));
    createAction("transpose_up_octave", SLOT(slotTransposeUpOctave()));
    createAction("transpose_down", SLOT(slotTransposeDown()));
    createAction("transpose_down_octave", SLOT(slotTransposeDownOctave()));
    createAction("general_transpose", SLOT(slotTranspose()));
    createAction("general_diatonic_transpose", SLOT(slotDiatonicTranspose()));
    createAction("invert", SLOT(slotInvert()));
    createAction("retrograde", SLOT(slotRetrograde()));
    createAction("retrograde_invert", SLOT(slotRetrogradeInvert()));
    createAction("jog_left", SLOT(slotJogLeft()));
    createAction("jog_right", SLOT(slotJogRight()));
    createAction("show_velocity_control_ruler", SLOT(slotShowVelocityControlRuler()));
// was disabled in kde3 version:
// createAction("show_controller_events_ruler", SLOT(slotShowControllerEventsRuler()));
// was disabled in kde3 version:
// createAction("add_control_ruler", SLOT(slotShowPropertyControlRuler()));
    createAction("insert_control_ruler_item", SLOT(slotInsertControlRulerItem()));
    createAction("erase_control_ruler_item", SLOT(slotEraseControlRulerItem()));
    createAction("clear_control_ruler_item", SLOT(slotClearControlRulerItem()));
    createAction("start_control_line_item", SLOT(slotStartControlLineItem()));
    createAction("flip_control_events_forward", SLOT(slotFlipForwards()));
    createAction("flip_control_events_back", SLOT(slotFlipBackwards()));
    createAction("draw_property_line", SLOT(slotDrawPropertyLine()));
    createAction("select_all_properties", SLOT(slotSelectAllProperties()));

/*
    //
    // Tempo and time signature changes
    //
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    Q3CanvasPixmap pixmap(pixmapDir + "/toolbar/event-insert-tempo.png");
    QIcon icon = QIcon(pixmap);
    QAction* qa_add_tempo = new QAction(  AddTempoChangeCommand::getGlobalName(), dynamic_cast<QObject*>(0) );
			connect( qa_add_tempo, SIGNAL(toggled()), dynamic_cast<QObject*>(0), this, SLOT(slotAddTempo()) );
			qa_add_tempo->setObjectName( "add_tempo" );		//
			//qa_add_tempo->setCheckable( true );		//
			qa_add_tempo->setAutoRepeat( false );	//
			//qa_add_tempo->setActionGroup( 0 );		// QActionGroup*
			//qa_add_tempo->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    pixmap.load(pixmapDir + "/toolbar/event-insert-timesig.png");
    icon = QIcon(pixmap);
    QAction* qa_add_time_signature = new QAction(  AddTimeSignatureCommand::getGlobalName(), dynamic_cast<QObject*>(0) );
			connect( qa_add_time_signature, SIGNAL(toggled()), dynamic_cast<QObject*>(0), this, SLOT(slotAddTimeSignature()) );
			qa_add_time_signature->setObjectName( "add_time_signature" );		//
			//qa_add_time_signature->setCheckable( true );		//
			qa_add_time_signature->setAutoRepeat( false );	//
			//qa_add_time_signature->setActionGroup( 0 );		// QActionGroup*
			//qa_add_time_signature->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    //
    // Transforms
    //
    QAction* qa_halve_durations = new QAction(  i18n("&Halve Durations"), dynamic_cast<QObject*>(this) );
			connect( qa_halve_durations, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotHalveDurations()) );
			qa_halve_durations->setObjectName( "halve_durations" );		//
			//qa_halve_durations->setCheckable( true );		//
			qa_halve_durations->setAutoRepeat( false );	//
			//qa_halve_durations->setActionGroup( 0 );		// QActionGroup*
			//qa_halve_durations->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_double_durations = new QAction(  i18n("&Double Durations"), dynamic_cast<QObject*>(this) );
			connect( qa_double_durations, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotDoubleDurations()) );
			qa_double_durations->setObjectName( "double_durations" );		//
			//qa_double_durations->setCheckable( true );		//
			qa_double_durations->setAutoRepeat( false );	//
			//qa_double_durations->setActionGroup( 0 );		// QActionGroup*
			//qa_double_durations->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_rescale = new QAction(  RescaleCommand::getGlobalName(), dynamic_cast<QObject*>(this) );
			connect( qa_rescale, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRescale()) );
			qa_rescale->setObjectName( "rescale" );		//
			//qa_rescale->setCheckable( true );		//
			qa_rescale->setAutoRepeat( false );	//
			//qa_rescale->setActionGroup( 0 );		// QActionGroup*
			//qa_rescale->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_transpose_up = new QAction(  TransposeCommand::getGlobalName(1), dynamic_cast<QObject*>(Qt::Key_Up) );
			connect( qa_transpose_up, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_Up), this );
			qa_transpose_up->setObjectName( "transpose_up" );		//
			//qa_transpose_up->setCheckable( true );		//
			qa_transpose_up->setAutoRepeat( false );	//
			//qa_transpose_up->setActionGroup( 0 );		// QActionGroup*
			//qa_transpose_up->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_transpose_up_octave = new QAction(  TransposeCommand::getGlobalName(12), dynamic_cast<QObject*>(Qt::Key_Up + Qt::CTRL) );
			connect( qa_transpose_up_octave, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_Up + Qt::CTRL), this );
			qa_transpose_up_octave->setObjectName( "transpose_up_octave" );		//
			//qa_transpose_up_octave->setCheckable( true );		//
			qa_transpose_up_octave->setAutoRepeat( false );	//
			//qa_transpose_up_octave->setActionGroup( 0 );		// QActionGroup*
			//qa_transpose_up_octave->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_transpose_down = new QAction(  TransposeCommand::getGlobalName( -1), dynamic_cast<QObject*>(Qt::Key_Down) );
			connect( qa_transpose_down, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_Down), this );
			qa_transpose_down->setObjectName( "transpose_down" );		//
			//qa_transpose_down->setCheckable( true );		//
			qa_transpose_down->setAutoRepeat( false );	//
			//qa_transpose_down->setActionGroup( 0 );		// QActionGroup*
			//qa_transpose_down->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_transpose_down_octave = new QAction(  TransposeCommand::getGlobalName( -12), dynamic_cast<QObject*>(Qt::Key_Down + Qt::CTRL) );
			connect( qa_transpose_down_octave, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_Down + Qt::CTRL), this );
			qa_transpose_down_octave->setObjectName( "transpose_down_octave" );		//
			//qa_transpose_down_octave->setCheckable( true );		//
			qa_transpose_down_octave->setAutoRepeat( false );	//
			//qa_transpose_down_octave->setActionGroup( 0 );		// QActionGroup*
			//qa_transpose_down_octave->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_general_transpose = new QAction(  TransposeCommand::getGlobalName(0), dynamic_cast<QObject*>(this) );
			connect( qa_general_transpose, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotTranspose()) );
			qa_general_transpose->setObjectName( "general_transpose" );		//
			//qa_general_transpose->setCheckable( true );		//
			qa_general_transpose->setAutoRepeat( false );	//
			//qa_general_transpose->setActionGroup( 0 );		// QActionGroup*
			//qa_general_transpose->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_general_diatonic_transpose = new QAction(  TransposeCommand::getDiatonicGlobalName(0, dynamic_cast<QObject*>(0) );
			connect( qa_general_diatonic_transpose, SIGNAL(toggled()), dynamic_cast<QObject*>(0), this );
			qa_general_diatonic_transpose->setObjectName( "general_diatonic_transpose" );		//
			//qa_general_diatonic_transpose->setCheckable( true );		//
			qa_general_diatonic_transpose->setAutoRepeat( false );	//
			//qa_general_diatonic_transpose->setActionGroup( 0 );		// QActionGroup*
			//qa_general_diatonic_transpose->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_invert = new QAction(  InvertCommand::getGlobalName(0), dynamic_cast<QObject*>(this) );
			connect( qa_invert, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotInvert()) );
			qa_invert->setObjectName( "invert" );		//
			//qa_invert->setCheckable( true );		//
			qa_invert->setAutoRepeat( false );	//
			//qa_invert->setActionGroup( 0 );		// QActionGroup*
			//qa_invert->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_retrograde = new QAction(  RetrogradeCommand::getGlobalName(0), dynamic_cast<QObject*>(this) );
			connect( qa_retrograde, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRetrograde()) );
			qa_retrograde->setObjectName( "retrograde" );		//
			//qa_retrograde->setCheckable( true );		//
			qa_retrograde->setAutoRepeat( false );	//
			//qa_retrograde->setActionGroup( 0 );		// QActionGroup*
			//qa_retrograde->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_retrograde_invert = new QAction(  RetrogradeInvertCommand::getGlobalName(0), dynamic_cast<QObject*>(this) );
			connect( qa_retrograde_invert, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRetrogradeInvert()) );
			qa_retrograde_invert->setObjectName( "retrograde_invert" );		//
			//qa_retrograde_invert->setCheckable( true );		//
			qa_retrograde_invert->setAutoRepeat( false );	//
			//qa_retrograde_invert->setActionGroup( 0 );		// QActionGroup*
			//qa_retrograde_invert->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_jog_left = new QAction(  i18n("Jog &Left"), dynamic_cast<QObject*>(this) );
			connect( qa_jog_left, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotJogLeft()) );
			qa_jog_left->setObjectName( "jog_left" );		//
			//qa_jog_left->setCheckable( true );		//
			qa_jog_left->setAutoRepeat( false );	//
			//qa_jog_left->setActionGroup( 0 );		// QActionGroup*
			//qa_jog_left->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_jog_right = new QAction(  i18n("Jog &Right"), dynamic_cast<QObject*>(this) );
			connect( qa_jog_right, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotJogRight()) );
			qa_jog_right->setObjectName( "jog_right" );		//
			//qa_jog_right->setCheckable( true );		//
			qa_jog_right->setAutoRepeat( false );	//
			//qa_jog_right->setActionGroup( 0 );		// QActionGroup*
			//qa_jog_right->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    // Control rulers
    //
    QAction* qa_show_velocity_control_ruler = new QAction(  i18n("Show Velocity Property Ruler"), dynamic_cast<QObject*>(this) );
			connect( qa_show_velocity_control_ruler, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotShowVelocityControlRuler()) );
			qa_show_velocity_control_ruler->setObjectName( "show_velocity_control_ruler" );		//
			//qa_show_velocity_control_ruler->setCheckable( true );		//
			qa_show_velocity_control_ruler->setAutoRepeat( false );	//
			//qa_show_velocity_control_ruler->setActionGroup( 0 );		// QActionGroup*
			//qa_show_velocity_control_ruler->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    //
    // Control Ruler context menu
    //
    QAction* qa_insert_control_ruler_item = new QAction(  i18n("Insert item"), dynamic_cast<QObject*>(this) );
			connect( qa_insert_control_ruler_item, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotInsertControlRulerItem()) );
			qa_insert_control_ruler_item->setObjectName( "insert_control_ruler_item" );		//
			//qa_insert_control_ruler_item->setCheckable( true );		//
			qa_insert_control_ruler_item->setAutoRepeat( false );	//
			//qa_insert_control_ruler_item->setActionGroup( 0 );		// QActionGroup*
			//qa_insert_control_ruler_item->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    // This was on Qt::Key_Delete, but that conflicts with existing Delete commands
    // on individual edit views
    QAction* qa_erase_control_ruler_item = new QAction(  i18n("Erase selected items"), dynamic_cast<QObject*>(this) );
			connect( qa_erase_control_ruler_item, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotEraseControlRulerItem()) );
			qa_erase_control_ruler_item->setObjectName( "erase_control_ruler_item" );		//
			//qa_erase_control_ruler_item->setCheckable( true );		//
			qa_erase_control_ruler_item->setAutoRepeat( false );	//
			//qa_erase_control_ruler_item->setActionGroup( 0 );		// QActionGroup*
			//qa_erase_control_ruler_item->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_clear_control_ruler_item = new QAction(  i18n("Clear ruler"), dynamic_cast<QObject*>(this) );
			connect( qa_clear_control_ruler_item, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotClearControlRulerItem()) );
			qa_clear_control_ruler_item->setObjectName( "clear_control_ruler_item" );		//
			//qa_clear_control_ruler_item->setCheckable( true );		//
			qa_clear_control_ruler_item->setAutoRepeat( false );	//
			//qa_clear_control_ruler_item->setActionGroup( 0 );		// QActionGroup*
			//qa_clear_control_ruler_item->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_start_control_line_item = new QAction(  i18n("Insert line of controllers"), dynamic_cast<QObject*>(this) );
			connect( qa_start_control_line_item, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotStartControlLineItem()) );
			qa_start_control_line_item->setObjectName( "start_control_line_item" );		//
			//qa_start_control_line_item->setCheckable( true );		//
			qa_start_control_line_item->setAutoRepeat( false );	//
			//qa_start_control_line_item->setActionGroup( 0 );		// QActionGroup*
			//qa_start_control_line_item->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_flip_control_events_forward = new QAction(  i18n("Flip forward"), dynamic_cast<QObject*>(this) );
			connect( qa_flip_control_events_forward, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotFlipForwards()) );
			qa_flip_control_events_forward->setObjectName( "flip_control_events_forward" );		//
			//qa_flip_control_events_forward->setCheckable( true );		//
			qa_flip_control_events_forward->setAutoRepeat( false );	//
			//qa_flip_control_events_forward->setActionGroup( 0 );		// QActionGroup*
			//qa_flip_control_events_forward->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_flip_control_events_back = new QAction(  i18n("Flip backwards"), dynamic_cast<QObject*>(this) );
			connect( qa_flip_control_events_back, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotFlipBackwards()) );
			qa_flip_control_events_back->setObjectName( "flip_control_events_back" );		//
			//qa_flip_control_events_back->setCheckable( true );		//
			qa_flip_control_events_back->setAutoRepeat( false );	//
			//qa_flip_control_events_back->setActionGroup( 0 );		// QActionGroup*
			//qa_flip_control_events_back->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_draw_property_line = new QAction(  i18n("Draw property line"), dynamic_cast<QObject*>(this) );
			connect( qa_draw_property_line, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotDrawPropertyLine()) );
			qa_draw_property_line->setObjectName( "draw_property_line" );		//
			//qa_draw_property_line->setCheckable( true );		//
			qa_draw_property_line->setAutoRepeat( false );	//
			//qa_draw_property_line->setActionGroup( 0 );		// QActionGroup*
			//qa_draw_property_line->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_select_all_properties = new QAction(  i18n("Select all property values"), dynamic_cast<QObject*>(this) );
			connect( qa_select_all_properties, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSelectAllProperties()) );
			qa_select_all_properties->setObjectName( "select_all_properties" );		//
			//qa_select_all_properties->setCheckable( true );		//
			qa_select_all_properties->setAutoRepeat( false );	//
			//qa_select_all_properties->setActionGroup( 0 );		// QActionGroup*
			//qa_select_all_properties->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			*/
}

void
EditView::setupAddControlRulerMenu()
{
    RG_DEBUG << "EditView::setupAddControlRulerMenu" << endl;

    QMenu* addControlRulerMenu = dynamic_cast<QMenu*>
        (factory()->container("add_control_ruler", this));

    if (addControlRulerMenu) {

        addControlRulerMenu->clear();

        //!!! problem here with notation view -- current segment can
        // change after construction, but this function isn't used again

        Controllable *c =
            dynamic_cast<MidiDevice *>(getCurrentDevice());
        if (!c) {
            c = dynamic_cast<SoftSynthDevice *>(getCurrentDevice());
            if (!c)
                return ;
        }

        const ControlList &list = c->getControlParameters();

        int i = 0;
        QString itemStr;

        for (ControlList::const_iterator it = list.begin();
                it != list.end(); ++it) {
            if (it->getType() == Controller::EventType) {
                QString hexValue;
                hexValue.sprintf("(0x%x)", it->getControllerValue());

                itemStr = i18n("%1 Controller %2 %3", strtoqstr(it->getName()),
                           it->getControllerValue(),
                           hexValue);

            } else if (it->getType() == PitchBend::EventType)
                itemStr = i18n("Pitch Bend");
            else
                itemStr = i18n("Unsupported Event Type");

            addControlRulerMenu->addItem(itemStr, i++);
        }

        connect(addControlRulerMenu, SIGNAL(activated(int)),
                SLOT(slotAddControlRuler(int)));
    }

}

bool
EditView::setupControllerTabs()
{
    bool have = false;

    // Setup control rulers the Segment already has some stored against it.
    //
    Segment *segment = getCurrentSegment();
    Segment::EventRulerList list = segment->getEventRulerList();

    RG_DEBUG << "EditView::setupControllerTabs - got " << list.size() << " EventRulers" << endl;

    RG_DEBUG << "Segment view features: " << segment->getViewFeatures() << endl;
    if (segment->getViewFeatures() & FeatureShowVelocity) {
        showPropertyControlRuler(BaseProperties::VELOCITY);
        have = true;
    }

    if (list.size()) {
        Controllable *c =
            dynamic_cast<MidiDevice *>(getCurrentDevice());
        if (!c) {
            c = dynamic_cast<SoftSynthDevice *>(getCurrentDevice());
            if (!c)
                return have;
        }

        have = true;

        Segment::EventRulerListIterator it;

        for (it = list.begin(); it != list.end(); ++it) {
            // Get ControlParameter object from controller value
            //
            const ControlParameter *controlParameter =
                c->getControlParameter((*it)->m_type,
                                       MidiByte((*it)->m_controllerValue));

            RG_DEBUG << "EditView::setupControllerTabs - "
            << "Control Parameter type = " << (*it)->m_type << endl;

            if (controlParameter) {
                ControllerEventsRuler* controlRuler = makeControllerEventRuler(controlParameter);
                addControlRuler(controlRuler);
                RG_DEBUG << "EditView::setupControllerTabs - adding Ruler" << endl;
            }
        }

        if (!m_controlRulers->isVisible())
            m_controlRulers->show();

        updateBottomWidgetGeometry();
    }

    return have;
}

void
EditView::slotAddControlRuler(int controller)
{
    RG_DEBUG << "EditView::slotAddControlRuler - item = "
    << controller << endl;

    Controllable *c =
        dynamic_cast<MidiDevice *>(getCurrentDevice());
    if (!c) {
        c = dynamic_cast<SoftSynthDevice *>(getCurrentDevice());
        if (!c)
            return ;
    }

    const ControlList &list = c->getControlParameters();
    ControlParameter control = list[controller];

    int index = 0;

    ControlRuler* existingRuler = findRuler(control, index);

    if (existingRuler) {

        m_controlRulers->setCurrentPage(index);

    } else {

        // Create control ruler to a specific controller.  This duplicates
        // the control parameter in the supplied pointer.
        ControllerEventsRuler* controlRuler = makeControllerEventRuler(&control);

        addControlRuler(controlRuler);
    }

    if (!m_controlRulers->isVisible()) {
        m_controlRulers->show();
    }

    updateBottomWidgetGeometry();

    // Add the controller to the segment so the views can
    // remember what we've opened against it.
    //
    Staff *staff = getCurrentStaff();
    staff->getSegment().addEventRuler(control.getType(), control.getControllerValue());

    getDocument()->slotDocumentModified();
}

void EditView::slotRemoveControlRuler(QWidget* w)
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(w);

    if (ruler) {
        ControlParameter *controller = ruler->getControlParameter();

        // remove the control parameter from the "showing controllers" list on the segment
        //
        if (controller) {
            Staff *staff = getCurrentStaff();
            bool value = staff->getSegment().
                         deleteEventRuler(controller->getType(), controller->getControllerValue());

            if (value)
                RG_DEBUG << "slotRemoveControlRuler : removed controller from segment\n";
            else
                RG_DEBUG << "slotRemoveControlRuler : couldn't remove controller from segment - "
                << int(controller->getControllerValue())
                << endl;

        }
    } else { // else it's probably a velocity ruler
        PropertyControlRuler *propertyRuler = dynamic_cast<PropertyControlRuler*>(w);

        if (propertyRuler) {
            Segment &seg = getCurrentStaff()->getSegment();
            seg.setViewFeatures(0); // for the moment we only have one view feature so
            // we can just blank it out

            RG_DEBUG << "slotRemoveControlRuler : removed velocity ruler" << endl;
        }
    }

    delete w;

    if (m_controlRulers->count() == 0) {
        m_controlRulers->hide();
        updateBottomWidgetGeometry();
    }

    getDocument()->slotDocumentModified();
}

void
EditView::createInsertPitchActionMenu()
{
    QString notePitchNames[] = {
        i18n("I"), i18n("II"), i18n("III"), i18n("IV"),
        i18n("V"), i18n("VI"), i18n("VII"), i18n("VIII")
    };
    QString flat = i18n("%1 flat");
    QString sharp = i18n("%1 sharp");

    const Qt::Key notePitchKeys[3][7] = {
        {
            Qt::Key_A, Qt::Key_S, Qt::Key_D, Qt::Key_F, Qt::Key_J, Qt::Key_K, Qt::Key_L,
        },
        {
            Qt::Key_Q, Qt::Key_W, Qt::Key_E, Qt::Key_R, Qt::Key_U, Qt::Key_I, Qt::Key_O,
        },
        {
            Qt::Key_Z, Qt::Key_X, Qt::Key_C, Qt::Key_V, Qt::Key_B, Qt::Key_N, Qt::Key_M,
        },
    };

    QMenu *insertPitchActionMenu = new QMenu(i18n("&Insert Note"), this);
    insertPitchActionMenu->setObjectName("insert_note_actionmenu");

    for (int octave = 0; octave <= 2; ++octave) {

        QMenu *menu = insertPitchActionMenu;
        if (octave == 1) {
            menu = new QMenu(i18n("&Upper Octave"), this);
            menu->setObjectName("insert_note_actionmenu_upper_octave");
            insertPitchActionMenu->addSeparator();
            insertPitchActionMenu->addMenu(menu);
        } else if (octave == 2) {
            menu = new QMenu(i18n("&Lower Octave"), this);
            menu->setObjectName("insert_note_actionmenu_lower_octave");
            insertPitchActionMenu->addMenu(menu);
        }

        for (unsigned int i = 0; i < 7; ++i) {

            QAction *insertPitchAction = 0;

            QString octaveSuffix;
            if (octave == 1) octaveSuffix = "_high";
            else if (octave == 2) octaveSuffix = "_low";

            // do and fa lack a flat

            if (i != 0 && i != 3) {

                insertPitchAction = createAction
                    (QString("insert_%1_flat%2").arg(i).arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));

                insertPitchAction->setText(flat.arg(notePitchNames[i]));
                insertPitchAction->setShortcut
                    (Qt::CTRL + Qt::SHIFT + notePitchKeys[octave][i]);
                     
                menu->addAction(insertPitchAction);
            }

            insertPitchAction = createAction
                (QString("insert_%1%2").arg(i).arg(octaveSuffix),
                 SLOT(slotInsertNoteFromAction()));

            insertPitchAction->setText(notePitchNames[i]);
            insertPitchAction->setShortcut(notePitchKeys[octave][i]);
            
            menu->addAction(insertPitchAction);

            // and mi and ti lack a sharp

            if (i != 2 && i != 6) {

                insertPitchAction = createAction
                    (QString("insert_%1_sharp%2").arg(i).arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));

                insertPitchAction->setText(sharp.arg(notePitchNames[i]));
                insertPitchAction->setShortcut
                    (Qt::SHIFT + notePitchKeys[octave][i]);
                     
                menu->addAction(insertPitchAction);
            }

            if (i < 6) {
                menu->addSeparator();
            }
        }
    }

    //&&& Ensure insertPitchActionMenu goes into the proper super-menu
}

int
EditView::getPitchFromNoteInsertAction(QString name,
                                       Accidental &accidental,
                                       const Clef &clef,
                                       const ::Rosegarden::Key &key)
{
    using namespace Accidentals;

    accidental = NoAccidental;

    if (name.left(7) == "insert_") {

        name = name.right(name.length() - 7);

        int modify = 0;
        int octave = 0;

        if (name.right(5) == "_high") {

            octave = 1;
            name = name.left(name.length() - 5);

        } else if (name.right(4) == "_low") {

            octave = -1;
            name = name.left(name.length() - 4);
        }

        if (name.right(6) == "_sharp") {

            modify = 1;
            accidental = Sharp;
            name = name.left(name.length() - 6);

        } else if (name.right(5) == "_flat") {

            modify = -1;
            accidental = Flat;
            name = name.left(name.length() - 5);
        }

        int scalePitch = name.toInt();

        if (scalePitch < 0 || scalePitch > 7) {
            NOTATION_DEBUG << "EditView::getPitchFromNoteInsertAction: pitch "
            << scalePitch << " out of range, using 0" << endl;
            scalePitch = 0;
        }

	//
	// Note: middle-C is in octave 5 + octaveBase (default = -2) = 3 (hjj)
	//
        Pitch pitch
        (scalePitch, 3 + octave + clef.getOctave(), key, accidental);
        return pitch.getPerformancePitch();

    } else {

        throw Exception("Not an insert action",
                        __FILE__, __LINE__);
    }
}

void EditView::slotAddTempo()
{
    timeT insertionTime = getInsertionTime();

    TempoDialog tempoDlg(this, getDocument());

    connect(&tempoDlg,
            SIGNAL(changeTempo(timeT,
                               tempoT,
                               tempoT,
                               TempoDialog::TempoDialogAction)),
            this,
            SIGNAL(changeTempo(timeT,
                               tempoT,
                               tempoT,
                               TempoDialog::TempoDialogAction)));

    tempoDlg.setTempoPosition(insertionTime);
    tempoDlg.exec();
}

void EditView::slotAddTimeSignature()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return ;
    Composition *composition = segment->getComposition();
    timeT insertionTime = getInsertionTime();

    TimeSignatureDialog *dialog = 0;
    int timeSigNo = composition->getTimeSignatureNumberAt(insertionTime);

    if (timeSigNo >= 0) {

        dialog = new TimeSignatureDialog
                 (this, composition, insertionTime,
                  composition->getTimeSignatureAt(insertionTime));

    } else {

        timeT endTime = composition->getDuration();
        if (composition->getTimeSignatureCount() > 0) {
            endTime = composition->getTimeSignatureChange(0).first;
        }

        CompositionTimeSliceAdapter adapter
        (composition, insertionTime, endTime);
        AnalysisHelper helper;
        TimeSignature timeSig = helper.guessTimeSignature(adapter);

        dialog = new TimeSignatureDialog
                 (this, composition, insertionTime, timeSig, false,
                  i18n("Estimated time signature shown"));
    }

    if (dialog->exec() == QDialog::Accepted) {

        insertionTime = dialog->getTime();

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

void EditView::showPropertyControlRuler(PropertyName propertyName)
{
    int index = 0;

    ControlRuler* existingRuler = findRuler(propertyName, index);

    if (existingRuler) {

        m_controlRulers->setCurrentPage(index);

    } else {

        PropertyControlRuler* controlRuler = makePropertyControlRuler(propertyName);
        addControlRuler(controlRuler);
    }

    if (!m_controlRulers->isVisible()) {
        m_controlRulers->show();
    }

    updateBottomWidgetGeometry();
}

void EditView::slotShowVelocityControlRuler()
{
    showPropertyControlRuler(BaseProperties::VELOCITY);
    Segment &seg = getCurrentStaff()->getSegment();
    seg.setViewFeatures(seg.getViewFeatures() | FeatureShowVelocity);
    getDocument()->slotDocumentModified();
}

void EditView::slotShowControllerEventsRuler()
{

    //     int index = 0;

    //     ControlRuler* existingRuler = findRuler(propertyName, index);

    //     if (existingRuler) {

    //         m_controlRulers->setCurrentPage(index);

    //     } else {

    //         ControllerEventsRuler* controlRuler = makeControllerEventRuler();
    //         addControlRuler(controlRuler);
    //     }

    //     if (!m_controlRulers->isVisible()) {
    //         m_controlRulers->show();
    //     }

    //     updateBottomWidgetGeometry();
}

void EditView::slotShowPropertyControlRuler()
{
    /*
        KDialogBase propChooserDialog(this, "propertychooserdialog", true, i18n("Select event property"),
                                      KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok);
        
        KListBox* propList = new KListBox(propChooserDialog.makeVBoxMainWidget());
        new QListWidgetRGProperty(propList, BaseProperties::VELOCITY.c_str());
     
        int rc = propChooserDialog.exec();
        if (rc == QDialog::Accepted) {
            // fix for KDE 3.0
            //QListWidgetRGProperty* item = dynamic_cast<QListWidgetRGProperty*>(propList->selectedItem());
            QListWidgetRGProperty* item = dynamic_cast<QListWidgetRGProperty*>
                (propList->item(propList->currentIndex()));
     
            if (item) {
                PropertyName property = item->getPropertyName();
                showPropertyControlRuler(property);
            }
        }
    */
}

void
EditView::slotInsertControlRulerItem()
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler)
        ruler->insertControllerEvent();
}

void
EditView::slotEraseControlRulerItem()
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler)
        ruler->eraseControllerEvent();
}

void
EditView::slotStartControlLineItem()
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler)
        ruler->startControlLine();
}

void
EditView::slotDrawPropertyLine()
{
    int index = 0;
    PropertyControlRuler* ruler = dynamic_cast<PropertyControlRuler*>
                                  (findRuler(BaseProperties::VELOCITY, index));

    if (ruler)
        ruler->startPropertyLine();
}

void
EditView::slotSelectAllProperties()
{
    int index = 0;
    PropertyControlRuler* ruler = dynamic_cast<PropertyControlRuler*>
                                  (findRuler(BaseProperties::VELOCITY, index));

    if (ruler)
        ruler->selectAllProperties();
}

void
EditView::slotClearControlRulerItem()
{
    ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(getCurrentControlRuler());
    if (ruler)
        ruler->clearControllerEvents();
}

void
EditView::slotHalveDurations()
{
    if (!m_currentEventSelection)
        return ;

    KTmpStatusMsg msg(i18n("Halving durations..."), this);

    addCommandToHistory(
        new RescaleCommand(*m_currentEventSelection,
                           m_currentEventSelection->getTotalDuration() / 2,
                           false));
}

void
EditView::slotDoubleDurations()
{
    if (!m_currentEventSelection)
        return ;

    KTmpStatusMsg msg(i18n("Doubling durations..."), this);

    addCommandToHistory(
        new RescaleCommand(*m_currentEventSelection,
                           m_currentEventSelection->getTotalDuration() * 2,
                           false));
}

void
EditView::slotRescale()
{
    if (!m_currentEventSelection)
        return ;

    RescaleDialog dialog
    (this,
     &getDocument()->getComposition(),
     m_currentEventSelection->getStartTime(),
     m_currentEventSelection->getEndTime() -
     m_currentEventSelection->getStartTime(),
     true,
     true);

    if (dialog.exec() == QDialog::Accepted) {
        KTmpStatusMsg msg(i18n("Rescaling..."), this);
        addCommandToHistory(new RescaleCommand
                            (*m_currentEventSelection,
                             dialog.getNewDuration(),
                             dialog.shouldCloseGap()));
    }
}

void EditView::slotTranspose()
{
    if (!m_currentEventSelection)
        return ;

    QSettings settings;
    settings.beginGroup( EditViewConfigGroup );

    int dialogDefault = settings.value("lasttransposition", 0).toInt() ;

    bool ok = false;
    int semitones = QInputDialog::getInteger
                    (i18n("Transpose"),
                     i18n("By number of semitones: "),
                     dialogDefault, -127, 127, 1, &ok, this);
    if (!ok || semitones == 0) return;

    //### settings.beginGroup( EditViewConfigGroup );

    settings.setValue("lasttransposition", semitones);

    KTmpStatusMsg msg(i18n("Transposing..."), this);
    addCommandToHistory(new TransposeCommand
                        (semitones, *m_currentEventSelection));

    settings.endGroup();
}

void EditView::slotDiatonicTranspose()
{
    if (!m_currentEventSelection)
        return ;

    QSettings settings;
    settings.beginGroup( EditViewConfigGroup );

    IntervalDialog intervalDialog(this);
    int ok = intervalDialog.exec();
	//int dialogDefault = settings.value("lasttransposition", 0).toInt() ;
    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();
    settings.endGroup();

    if (!ok || (semitones == 0 && steps == 0)) return;

    KTmpStatusMsg msg(i18n("Transposing..."), this);
    if (intervalDialog.getChangeKey())
    {
		std::cout << "Transposing changing keys is not currently supported on selections" << std::endl;
    }
    else
    {
	// Transpose within key
		//std::cout << "Transposing semitones, steps: " << semitones << ", " << steps << std::endl;
		addCommandToHistory(new TransposeCommand
                        (semitones, steps, *m_currentEventSelection));
    }
}

void EditView::slotTransposeUp()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Transposing up one semitone..."), this);

    addCommandToHistory(new TransposeCommand(1, *m_currentEventSelection));
}

void EditView::slotTransposeUpOctave()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Transposing up one octave..."), this);

    addCommandToHistory(new TransposeCommand(12, *m_currentEventSelection));
}

void EditView::slotTransposeDown()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Transposing down one semitone..."), this);

    addCommandToHistory(new TransposeCommand( -1, *m_currentEventSelection));
}

void EditView::slotTransposeDownOctave()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Transposing down one octave..."), this);

    addCommandToHistory(new TransposeCommand( -12, *m_currentEventSelection));
}

void EditView::slotInvert()
{
    if (!m_currentEventSelection)
        return ;

    int semitones = 0;

    KTmpStatusMsg msg(i18n("Inverting..."), this);
    addCommandToHistory(new InvertCommand
                        (semitones, *m_currentEventSelection));
}

void EditView::slotRetrograde()
{
    if (!m_currentEventSelection)
        return ;

    int semitones = 0;

    KTmpStatusMsg msg(i18n("Retrograding..."), this);
    addCommandToHistory(new RetrogradeCommand
                        (semitones, *m_currentEventSelection));
}

void EditView::slotRetrogradeInvert()
{
    if (!m_currentEventSelection)
        return ;

    int semitones = 0;

    KTmpStatusMsg msg(i18n("Retrograde inverting..."), this);
    addCommandToHistory(new RetrogradeInvertCommand
                        (semitones, *m_currentEventSelection));
}

void EditView::slotJogLeft()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Jogging left..."), this);

    RG_DEBUG << "EditView::slotJogLeft" << endl;

    addCommandToHistory(
        new MoveCommand(*getCurrentSegment(),
                        -Note(Note::Demisemiquaver).getDuration(),
                        false,  // don't use notation timings
                        *m_currentEventSelection));
}

void EditView::slotJogRight()
{
    if (!m_currentEventSelection)
        return ;
    KTmpStatusMsg msg(i18n("Jogging right..."), this);

    RG_DEBUG << "EditView::slotJogRight" << endl;

    addCommandToHistory(
        new MoveCommand(*getCurrentSegment(),
                        Note(Note::Demisemiquaver).getDuration(),
                        false,  // don't use notation timings
                        *m_currentEventSelection));
}

void
EditView::slotFlipForwards()
{
    RG_DEBUG << "EditView::slotFlipForwards" << endl;
    ControlRuler* ruler = getCurrentControlRuler();
    if (ruler) ruler->flipForwards();
}

void
EditView::slotFlipBackwards()
{
    RG_DEBUG << "EditView::slotFlipBackwards" << endl;
    ControlRuler* ruler = getCurrentControlRuler();
    if (ruler) ruler->flipBackwards();
}

ControlRuler* EditView::getCurrentControlRuler()
{
    return dynamic_cast<ControlRuler*>(m_controlRulers->currentPage());
}

ControlRuler* EditView::findRuler(PropertyName propertyName, int &index)
{
    for(index = 0; index < m_controlRulers->count(); ++index) {
        PropertyControlRuler* ruler = dynamic_cast<PropertyControlRuler*>(m_controlRulers->page(index));
        if (ruler && ruler->getPropertyName() == propertyName) return ruler;
    }

    return 0;
}

ControlRuler* EditView::findRuler(const ControlParameter& controller, int &index)
{
    for(index = 0; index < m_controlRulers->count(); ++index) {
        ControllerEventsRuler* ruler = dynamic_cast<ControllerEventsRuler*>(m_controlRulers->page(index));
        if (ruler && *(ruler->getControlParameter()) == controller) return ruler;
    }

    return 0;
}

PropertyControlRuler* EditView::makePropertyControlRuler(PropertyName propertyName)
{
    Q3Canvas* controlRulerCanvas = new Q3Canvas(this);
    QSize viewSize = getViewSize();
    controlRulerCanvas->resize(viewSize.width(), ControlRuler::DefaultRulerHeight); // TODO - keep it in sync with main canvas size

//     Q3Canvas* controlRulerCanvas = ControlRulerCanvasRepository::getCanvas(getCurrentSegment(), propertyName,
//                                                                           getViewSize());

    PropertyControlRuler* controlRuler = new PropertyControlRuler
    (propertyName, getCurrentStaff(), getHLayout(), this,
     controlRulerCanvas, m_controlRulers);

    controlRuler->setMainHorizontalScrollBar(m_canvasView->horizontalScrollBar());

    return controlRuler;
}

ControllerEventsRuler* EditView::makeControllerEventRuler(const ControlParameter *controller)
{
    Q3Canvas* controlRulerCanvas = new Q3Canvas(this);
    QSize viewSize = getViewSize();
    controlRulerCanvas->resize(viewSize.width(), ControlRuler::DefaultRulerHeight); // TODO - keep it in sync with main canvas size
//     Q3Canvas* controlRulerCanvas = ControlRulerCanvasRepository::getCanvas(getCurrentSegment(), controller,
//                                                                           getViewSize());
    

    ControllerEventsRuler* controlRuler = new ControllerEventsRuler
    (getCurrentSegment(), getHLayout(), this,
     controlRulerCanvas, m_controlRulers, controller);

    controlRuler->setMainHorizontalScrollBar(m_canvasView->horizontalScrollBar());

    return controlRuler;
}

RosegardenCanvasView* EditView::getCanvasView()
{
    return m_canvasView;
}

}
#include "EditView.moc"
