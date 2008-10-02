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


#include <Q3CanvasPixmap>
#include "TempoView.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "commands/segment/RemoveTempoChangeCommand.h"
#include "commands/segment/RemoveTimeSignatureCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/general/EditViewBase.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "TempoListItem.h"
#include <QAction>
#include <kglobal.h>
#include <QSettings>
#include <QListWidget>
#include <kxmlguiclient.h>
#include <QGroupBox>
#include <QCheckBox>
#include <QDialog>
#include <QIcon>
#include <QListWidget>
#include <QPixmap>
#include <qptrlist.h>
#include <QSize>
#include <QString>
#include <QLayout>
#include <QVBoxLayout>
#include <Q3Canvas>
#include <kstatusbar.h>


namespace Rosegarden
{

int
TempoView::m_lastSetFilter = -1;


TempoView::TempoView(RosegardenGUIDoc *doc, QWidget *parent, timeT openTime):
        EditViewBase(doc, std::vector<Segment *>(), 2, parent, "tempoview"),
        m_filter(Tempo | TimeSignature),
        m_ignoreUpdates(true)
{
    if (m_lastSetFilter < 0)
        m_lastSetFilter = m_filter;
    else
        m_filter = m_lastSetFilter;

    initStatusBar();
    setupActions();

    // define some note filtering buttons in a group
    //
    m_filterGroup =
        new QGroupBox(1, Horizontal, i18n("Filter"), getCentralWidget());
    QVBoxLayout *filterGroupLayout = new QVBoxLayout;

    m_tempoCheckBox = new QCheckBox(i18n("Tempo"), m_filterGroup);
    filterGroupLayout->addWidget(m_tempoCheckBox);

    m_timeSigCheckBox = new QCheckBox(i18n("Time Signature"), m_filterGroup);
    filterGroupLayout->addWidget(m_timeSigCheckBox);

    m_filterGroup->setLayout(filterGroupLayout);
    m_grid->addWidget(m_filterGroup, 2, 0);

    // Connect up
    //
    connect(m_filterGroup, SIGNAL(released(int)),
            SLOT(slotModifyFilter(int)));

    m_list = new QListWidget(getCentralWidget());
    m_list->setItemsRenameable(true);

    m_grid->addWidget(m_list, 2, 1);

    updateViewCaption();

    doc->getComposition().addObserver(this);

    // Connect double clicker
    //
    connect(m_list, SIGNAL(doubleClicked(QListWidgetItem*)),
            SLOT(slotPopupEditor(QListWidgetItem*)));

    m_list->setAllColumnsShowFocus(true);
    m_list->setSelectionMode(QListWidget::Extended);

    m_list->addColumn(i18n("Time  "));
    m_list->addColumn(i18n("Type  "));
    m_list->addColumn(i18n("Value  "));
    m_list->addColumn(i18n("Properties  "));

    for (int col = 0; col < m_list->columns(); ++col)
        m_list->setRenameable(col, true);

    readOptions();
    setButtonsToFilter();
    applyLayout();

    makeInitialSelection(openTime);

    m_ignoreUpdates = false;
    setOutOfCtor();
}

TempoView::~TempoView()
{
    if (!getDocument()->isBeingDestroyed() && !isCompositionDeleted()) {
        getDocument()->getComposition().removeObserver(this);
    }
}

void
TempoView::closeEvent(QCloseEvent *e)
{
    emit closing();
    EditViewBase::closeEvent(e);
}

void
TempoView::tempoChanged(const Composition *comp)
{
    if (m_ignoreUpdates)
        return ;
    if (comp == &getDocument()->getComposition()) {
        applyLayout();
    }
}

void
TempoView::timeSignatureChanged(const Composition *comp)
{
    if (m_ignoreUpdates)
        return ;
    if (comp == &getDocument()->getComposition()) {
        applyLayout();
    }
}

bool
TempoView::applyLayout(int /*staffNo*/)
{
    // If no selection has already been set then we copy what's
    // already set and try to replicate this after the rebuild
    // of the view.  This code borrowed from EventView.
    //
    if (m_listSelection.size() == 0) {
        QPtrList<QListWidgetItem> selection = m_list->selectedItems();

        if (selection.count()) {
            QPtrListIterator<QListWidgetItem> it(selection);
            QListWidgetItem *listItem;

            while ((listItem = it.current()) != 0) {
                m_listSelection.push_back(m_list->itemIndex(*it));
                ++it;
            }
        }
    }

    // Ok, recreate list
    //
    m_list->clear();

    Composition *comp = &getDocument()->getComposition();

    QSettings settings;
    settings.beginGroup( TempoViewConfigGroup );

    int timeMode = settings.value("timemode", 0).toInt() ;
    settings.endGroup();

    if (m_filter & TimeSignature) {
        for (int i = 0; i < comp->getTimeSignatureCount(); ++i) {

            std::pair<timeT, Rosegarden::TimeSignature> sig =
                comp->getTimeSignatureChange(i);

            QString properties;
            if (sig.second.isHidden()) {
                if (sig.second.isCommon())
                    properties = i18n("Common, hidden");
                else
                    properties = i18n("Hidden");
            } else {
                if (sig.second.isCommon())
                    properties = i18n("Common");
            }

            QString timeString = makeTimeString(sig.first, timeMode);

            new TempoListItem(comp, TempoListItem::TimeSignature,
                              sig.first, i, m_list, timeString,
                              i18n("Time Signature   "),
                              QString("%1/%2   ").arg(sig.second.getNumerator()).
                              arg(sig.second.getDenominator()),
                              properties);
        }
    }

    if (m_filter & Tempo) {
        for (int i = 0; i < comp->getTempoChangeCount(); ++i) {

            std::pair<timeT, tempoT> tempo =
                comp->getTempoChange(i);

            QString desc;

            //!!! imprecise -- better to work from tempoT directly

            float qpm = comp->getTempoQpm(tempo.second);
            int qpmUnits = int(qpm + 0.001);
            int qpmTenths = int((qpm - qpmUnits) * 10 + 0.001);
            int qpmHundredths = int((qpm - qpmUnits - qpmTenths / 10.0) * 100 + 0.001);

            Rosegarden::TimeSignature sig = comp->getTimeSignatureAt(tempo.first);
            if (sig.getBeatDuration() ==
                    Note(Note::Crotchet).getDuration()) {
                desc = i18n("%1.%2%3", 
                       qpmUnits, qpmTenths, qpmHundredths);
            } else {
                float bpm = (qpm *
                             Note(Note::Crotchet).getDuration()) /
                            sig.getBeatDuration();
                int bpmUnits = int(bpm + 0.001);
                int bpmTenths = int((bpm - bpmUnits) * 10 + 0.001);
                int bpmHundredths = int((bpm - bpmUnits - bpmTenths / 10.0) * 100 + 0.001);

                desc = i18n("%1.%2%3 qpm (%4.%5%6 bpm)   ", 
                       qpmUnits, qpmTenths, qpmHundredths, 
                       bpmUnits, bpmTenths, bpmHundredths);
            }

            QString timeString = makeTimeString(tempo.first, timeMode);

            new TempoListItem(comp, TempoListItem::Tempo,
                              tempo.first, i, m_list, timeString,
                              i18n("Tempo   "),
                              desc);
        }
    }

    if (m_list->childCount() == 0) {
        new QListWidgetItem(m_list,
                          i18n("<nothing at this filter level>"));
        m_list->setSelectionMode(QListWidget::NoSelection);
        stateChanged("have_selection", KXMLGUIClient::StateReverse);
    } else {
        m_list->setSelectionMode(QListWidget::Extended);

        // If no selection then select the first event
        if (m_listSelection.size() == 0)
            m_listSelection.push_back(0);
        stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
    }

    // Set a selection from a range of indexes
    //
    std::vector<int>::iterator sIt = m_listSelection.begin();
    int index = 0;

    for (; sIt != m_listSelection.end(); ++sIt) {
        index = *sIt;

        while (index > 0 && !m_list->itemAtIndex(index))
            index--;

        m_list->setSelected(m_list->itemAtIndex(index), true);
        m_list->setCurrentIndex(m_list->itemAtIndex(index));

        // ensure visible
        m_list->ensureItemVisible(m_list->itemAtIndex(index));
    }

    m_listSelection.clear();

    return true;
}

void
TempoView::makeInitialSelection(timeT time)
{
    m_listSelection.clear();

    TempoListItem *goodItem = 0;
    int goodItemNo = 0;

    for (int i = 0; m_list->itemAtIndex(i); ++i) {

        TempoListItem *item = dynamic_cast<TempoListItem *>
                              (m_list->itemAtIndex(i));

        m_list->setSelected(item, false);

        if (item) {
            if (item->getTime() > time)
                break;
            goodItem = item;
            goodItemNo = i;
        }
    }

    if (goodItem) {
        m_listSelection.push_back(goodItemNo);
        m_list->setSelected(goodItem, true);
        m_list->ensureItemVisible(goodItem);
    }
}

Segment *
TempoView::getCurrentSegment()
{
    if (m_segments.empty())
        return 0;
    else
        return *m_segments.begin();
}

QString
TempoView::makeTimeString(timeT time, int timeMode)
{
    switch (timeMode) {

    case 0:  // musical time
        {
            int bar, beat, fraction, remainder;
            getDocument()->getComposition().getMusicalTimeForAbsoluteTime
            (time, bar, beat, fraction, remainder);
            ++bar;
            return QString("%1%2%3-%4%5-%6%7-%8%9   ")
                   .arg(bar / 100)
                   .arg((bar % 100) / 10)
                   .arg(bar % 10)
                   .arg(beat / 10)
                   .arg(beat % 10)
                   .arg(fraction / 10)
                   .arg(fraction % 10)
                   .arg(remainder / 10)
                   .arg(remainder % 10);
        }

    case 1:  // real time
        {
            RealTime rt =
                getDocument()->getComposition().getElapsedRealTime(time);
            //	return QString("%1   ").arg(rt.toString().c_str());
            return QString("%1   ").arg(rt.toText().c_str());
        }

    default:
        return QString("%1   ").arg(time);
    }
}

void
TempoView::refreshSegment(Segment * /*segment*/,
                          timeT /*startTime*/,
                          timeT /*endTime*/)
{
    RG_DEBUG << "TempoView::refreshSegment" << endl;
    applyLayout(0);
}

void
TempoView::updateView()
{
    m_list->update();
}

void
TempoView::slotEditCut()
{
    // not implemented yet -- can't use traditional clipboard (which
    // only holds events from segments, or segments)
}

void
TempoView::slotEditCopy()
{
    // likewise
}

void
TempoView::slotEditPaste()
{
    // likewise
}

void
TempoView::slotEditDelete()
{
    QPtrList<QListWidgetItem> selection = m_list->selectedItems();
    if (selection.count() == 0)
        return ;

    RG_DEBUG << "TempoView::slotEditDelete - deleting "
    << selection.count() << " items" << endl;

    QPtrListIterator<QListWidgetItem> it(selection);
    QListWidgetItem *listItem;
    TempoListItem *item;
    int itemIndex = -1;

    m_ignoreUpdates = true;
    bool haveSomething = false;

    // We want the Remove commands to be in reverse order, because
    // removing one item by index will affect the indices of
    // subsequent items.  So we'll stack them onto here and then pull
    // them off again.
    std::vector<Command *> commands;

    while ((listItem = it.current()) != 0) {
        item = dynamic_cast<TempoListItem*>((*it));

        if (itemIndex == -1)
            itemIndex = m_list->itemIndex(*it);

        if (item) {
            if (item->getType() == TempoListItem::TimeSignature) {
                commands.push_back(new RemoveTimeSignatureCommand
                                   (item->getComposition(),
                                    item->getIndex()));
                haveSomething = true;
            } else {
                commands.push_back(new RemoveTempoChangeCommand
                                   (item->getComposition(),
                                    item->getIndex()));
                haveSomething = true;
            }
        }
        ++it;
    }

    if (haveSomething) {
        MacroCommand *command = new MacroCommand
                                 (i18n("Delete Tempo or Time Signature"));
        for (std::vector<Command *>::iterator i = commands.end();
                i != commands.begin();) {
            command->addCommand(*--i);
        }
        addCommandToHistory(command);
    }

    applyLayout();
    m_ignoreUpdates = false;
}

void
TempoView::slotEditInsertTempo()
{
    timeT insertTime = 0;
    QPtrList<QListWidgetItem> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.getFirst());
        if (item)
            insertTime = item->getTime();
    }

    TempoDialog dialog(this, getDocument(), true);
    dialog.setTempoPosition(insertTime);

    connect(&dialog,
            SIGNAL(changeTempo(timeT,
                               tempoT,
                               tempoT,
                               TempoDialog::TempoDialogAction)),
            this,
            SIGNAL(changeTempo(timeT,
                               tempoT,
                               tempoT,
                               TempoDialog::TempoDialogAction)));

    dialog.exec();
}

void
TempoView::slotEditInsertTimeSignature()
{
    timeT insertTime = 0;
    QPtrList<QListWidgetItem> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.getFirst());
        if (item)
            insertTime = item->getTime();
    }

    Composition &composition(m_doc->getComposition());
    Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(insertTime);

    TimeSignatureDialog dialog(this, &composition, insertTime, sig, true);

    if (dialog.exec() == QDialog::Accepted) {

        insertTime = dialog.getTime();

        if (dialog.shouldNormalizeRests()) {
            addCommandToHistory
            (new AddTimeSignatureAndNormalizeCommand
             (&composition, insertTime, dialog.getTimeSignature()));
        } else {
            addCommandToHistory
            (new AddTimeSignatureCommand
             (&composition, insertTime, dialog.getTimeSignature()));
        }
    }
}

void
TempoView::slotEdit()
{
    RG_DEBUG << "TempoView::slotEdit" << endl;

    QPtrList<QListWidgetItem> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.getFirst());
        if (item)
            slotPopupEditor(item);
    }
}

void
TempoView::slotSelectAll()
{
    m_listSelection.clear();
    for (int i = 0; m_list->itemAtIndex(i); ++i) {
        m_listSelection.push_back(i);
        m_list->setSelected(m_list->itemAtIndex(i), true);
    }
}

void
TempoView::slotClearSelection()
{
    m_listSelection.clear();
    for (int i = 0; m_list->itemAtIndex(i); ++i) {
        m_list->setSelected(m_list->itemAtIndex(i), false);
    }
}

void
TempoView::setupActions()
{
    EditViewBase::setupActions("tempoview.rc", false);

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIcon icon(QPixmap(pixmapDir + "/toolbar/event-insert-tempo.png"));

    QAction* qa_insert_tempo = new QAction(  AddTempoChangeCommand::getGlobalName(), dynamic_cast<QObject*>(Qt::Key_I) );
			connect( qa_insert_tempo, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_I), this );
			qa_insert_tempo->setObjectName( "insert_tempo" );		//
			//qa_insert_tempo->setCheckable( true );		//
			qa_insert_tempo->setAutoRepeat( false );	//
			//qa_insert_tempo->setActionGroup( 0 );		// QActionGroup*
			//qa_insert_tempo->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    Q3CanvasPixmap pixmap(pixmapDir + "/toolbar/event-insert-timesig.png");
    icon = QIcon(pixmap);

    QAction* qa_insert_timesig = new QAction(  AddTimeSignatureCommand::getGlobalName(), dynamic_cast<QObject*>(Qt::Key_G) );
			connect( qa_insert_timesig, SIGNAL(toggled()), dynamic_cast<QObject*>(Qt::Key_G), this );
			qa_insert_timesig->setObjectName( "insert_timesig" );		//
			//qa_insert_timesig->setCheckable( true );		//
			qa_insert_timesig->setAutoRepeat( false );	//
			//qa_insert_timesig->setActionGroup( 0 );		// QActionGroup*
			//qa_insert_timesig->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    pixmap.load(pixmapDir + "/toolbar/event-delete.png");
    icon = QIcon(pixmap);

    QAction *qa_delete = new QAction( "&Delete", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_delete->setIcon(icon); 
			connect( qa_delete, SIGNAL(triggered()), this, SLOT(slotEditDelete())  );

    pixmap.load(pixmapDir + "/toolbar/event-edit.png");
    icon = QIcon(pixmap);

    QAction *qa_edit = new QAction( "&Edit Item", dynamic_cast<QObject*>(this) ); //### deallocate action ptr 
			qa_edit->setIcon(icon); 
			connect( qa_edit, SIGNAL(triggered()), this, SLOT(slotEdit())  );

    QAction* qa_select_all = new QAction(  i18n("Select &All"), dynamic_cast<QObject*>(this) );
			connect( qa_select_all, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotSelectAll()) );
			qa_select_all->setObjectName( "select_all" );		//
			//qa_select_all->setCheckable( true );		//
			qa_select_all->setAutoRepeat( false );	//
			//qa_select_all->setActionGroup( 0 );		// QActionGroup*
			//qa_select_all->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QAction* qa_clear_selection = new QAction(  i18n("Clear Selection"), dynamic_cast<QObject*>(this) );
			connect( qa_clear_selection, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotClearSelection()) );
			qa_clear_selection->setObjectName( "clear_selection" );		//
			//qa_clear_selection->setCheckable( true );		//
			qa_clear_selection->setAutoRepeat( false );	//
			//qa_clear_selection->setActionGroup( 0 );		// QActionGroup*
			//qa_clear_selection->setChecked( false );		//
			//### FIX: deallocate QAction ptr
			

    QSettings settings;
    settings.beginGroup( TempoViewConfigGroup );

    int timeMode = settings.value("timemode", 0).toInt() ;
    settings.endGroup();

    KRadioAction *action;

    pixmap.load(pixmapDir + "/toolbar/time-musical.png");
    icon = QIcon(pixmap);

    action = QAction* qa_time_musical = new QAction( icon, i18n("&Musical Times"), qa_parent );
			connect( qa_time_musical, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotMusicalTime()) );
			qa_time_musical->setObjectName( "time_musical" );
			qa_time_musical->setCheckable( true );		//
			qa_time_musical->setChecked( false );			//
			qa_time_musical->setAutoRepeat( false );		//
			qa_time_musical->setActionGroup( qag_timeMode );	// QActionGroup*
			//### FIX: deallocate QAction ptr

    if (timeMode == 0)
        action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-real.png");
    icon = QIcon(pixmap);

    action = QAction* qa_time_real = new QAction( icon, i18n("&Real Times"), qa_parent );
			connect( qa_time_real, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRealTime()) );
			qa_time_real->setObjectName( "time_real" );
			qa_time_real->setCheckable( true );		//
			qa_time_real->setChecked( false );			//
			qa_time_real->setAutoRepeat( false );		//
			qa_time_real->setActionGroup( qag_timeMode );	// QActionGroup*
			//### FIX: deallocate QAction ptr

    if (timeMode == 1)
        action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-raw.png");
    icon = QIcon(pixmap);

    action = QAction* qa_time_raw = new QAction( icon, i18n("Ra&w Times"), qa_parent );
			connect( qa_time_raw, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRawTime()) );
			qa_time_raw->setObjectName( "time_raw" );
			qa_time_raw->setCheckable( true );		//
			qa_time_raw->setChecked( false );			//
			qa_time_raw->setAutoRepeat( false );		//
			qa_time_raw->setActionGroup( qag_timeMode );	// QActionGroup*
			//### FIX: deallocate QAction ptr

    if (timeMode == 2)
        action->setChecked(true);

    createGUI(getRCFileName());
}

void
TempoView::initStatusBar()
{
    KStatusBar* sb = statusBar();

    sb->addItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(),
                         AlignLeft | AlignVCenter);
}

QSize
TempoView::getViewSize()
{
    return m_list->size();
}

void
TempoView::setViewSize(QSize s)
{
    m_list->setFixedSize(s);
}

void
TempoView::readOptions()
{
    QSettings settings;
    settings.beginGroup( TempoViewConfigGroup );

    EditViewBase::readOptions();
    m_filter = settings.value("filter", m_filter).toInt() ;
    m_list->restoreLayout(TempoViewLayoutConfigGroupName);

    settings.endGroup();
}

void
TempoView::slotSaveOptions()
{
    QSettings settings;
    settings.beginGroup( TempoViewConfigGroup );

    settings.setValue("filter", m_filter);
    m_list->saveLayout(TempoViewLayoutConfigGroupName);

    settings.endGroup();
}

void
TempoView::slotModifyFilter(int button)
{
    QCheckBox *checkBox = dynamic_cast<QCheckBox*>(m_filterGroup->find(button));

    if (checkBox == 0)
        return ;

    if (checkBox->isChecked()) {
        switch (button) {
        case 0:
            m_filter |= Tempo;
            break;

        case 1:
            m_filter |= TimeSignature;
            break;

        default:
            break;
        }

    } else {
        switch (button) {
        case 0:
            m_filter ^= Tempo;
            break;

        case 1:
            m_filter ^= TimeSignature;
            break;

        default:
            break;
        }
    }

    m_lastSetFilter = m_filter;

    applyLayout(0);
}

void
TempoView::setButtonsToFilter()
{
    if (m_filter & Tempo)
        m_tempoCheckBox->setChecked(true);
    else
        m_tempoCheckBox->setChecked(false);

    if (m_filter & TimeSignature)
        m_timeSigCheckBox->setChecked(true);
    else
        m_timeSigCheckBox->setChecked(false);
}

void
TempoView::slotMusicalTime()
{
    QSettings settings;
    settings.beginGroup( TempoViewConfigGroup );

    settings.setValue("timemode", 0);
    applyLayout();

    settings.endGroup();
}

void
TempoView::slotRealTime()
{
    QSettings settings;
    settings.beginGroup( TempoViewConfigGroup );

    settings.setValue("timemode", 1);
    applyLayout();

    settings.endGroup();
}

void
TempoView::slotRawTime()
{
    QSettings settings;
    settings.beginGroup( TempoViewConfigGroup );

    settings.setValue("timemode", 2);
    applyLayout();

    settings.endGroup();
}

void
TempoView::slotPopupEditor(QListWidgetItem *qitem)
{
    TempoListItem *item = dynamic_cast<TempoListItem *>(qitem);
    if (!item)
        return ;

    timeT time = item->getTime();

    switch (item->getType()) {

    case TempoListItem::Tempo:
    {
        TempoDialog dialog(this, getDocument(), true);
        dialog.setTempoPosition(time);
        
        connect(&dialog,
                SIGNAL(changeTempo(timeT,
                                   tempoT,
                                   tempoT,
                                   TempoDialog::TempoDialogAction)),
                this,
                SIGNAL(changeTempo(timeT,
                                   tempoT,
                                   tempoT,
                                   TempoDialog::TempoDialogAction)));
        
        dialog.exec();
        break;
    }

    case TempoListItem::TimeSignature:
    {
        Composition &composition(getDocument()->getComposition());
        Rosegarden::TimeSignature sig = composition.getTimeSignatureAt(time);
        
        TimeSignatureDialog dialog(this, &composition, time, sig, true);
        
        if (dialog.exec() == QDialog::Accepted) {
            
            time = dialog.getTime();
            
            if (dialog.shouldNormalizeRests()) {
                addCommandToHistory
                    (new AddTimeSignatureAndNormalizeCommand
                     (&composition, time, dialog.getTimeSignature()));
            } else {
                addCommandToHistory
                    (new AddTimeSignatureCommand
                     (&composition, time, dialog.getTimeSignature()));
            }
        }
    }
    
    default:
        break;
    }
}

void
TempoView::updateViewCaption()
{
    setWindowTitle(i18n("%1 - Tempo and Time Signature Editor",
                getDocument()->getTitle()));
}

}
#include "TempoView.moc"
