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
#include <Q3Canvas>

#include "MarkerEditor.h"
#include "MarkerEditorViewItem.h"

// #include <kglobal.h>
// #include <kmainwindow.h>
// #include <kstandardshortcut.h>
// #include <kstandardaction.h>
#include <klocale.h>
// #include <kstandarddirs.h>
// #include <kstandardshortcut.h>

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Marker.h"
#include "base/RealTime.h"
#include "commands/edit/AddMarkerCommand.h"
#include "commands/edit/ModifyMarkerCommand.h"
#include "commands/edit/RemoveMarkerCommand.h"
#include "document/MultiViewCommandHistory.h"
#include "document/RosegardenGUIDoc.h"
#include "document/ConfigGroups.h"
#include "document/Command.h"
#include "gui/dialogs/MarkerModifyDialog.h"
#include "gui/kdeext/KTmpStatusMsg.h"
#include "gui/general/IconLoader.h"

#include <QApplication>
#include <QMainWindow>
#include <QLayout>
#include <QVBoxLayout>
// #include <QHBoxLayout>
#include <QAction>
#include <QShortcut>
#include <QDialog>
#include <QFrame>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QSettings>
#include <QStringList>

#include <QList>
// #include <qptrlist.h>


namespace Rosegarden
{

MarkerEditor::MarkerEditor(QWidget *parent,
                                       RosegardenGUIDoc *doc):
        QMainWindow(parent),
        m_doc(doc),
        m_modified(false)
{
	this->setObjectName( "markereditordialog" );
	
    QWidget *mainFrame = new QWidget(this);
    QVBoxLayout *mainFrameLayout = new QVBoxLayout;
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage Markers"));

    m_listView = new QTreeWidget( mainFrame );
    mainFrameLayout->addWidget(m_listView);
	
	QStringList sl;
	sl	<< i18n("Marker time  ")
		<< i18n("Marker text  ")
		<< i18n("Marker description ");
	
	m_listView->setHeaderLabels( sl );
	/*
    m_listView->addColumn(i18n("Marker time  "));
    m_listView->addColumn(i18n("Marker text  "));
    m_listView->addColumn(i18n("Marker description "));
	*/
    
	// Align centrally
//     for (int i = 0; i < 3; ++i)
//         m_listView->setColumnAlignment(i, Qt::AlignHCenter);	//&&& align items now
	
	
    QGroupBox *posGroup = new QGroupBox(i18n("Pointer position"), mainFrame);
    mainFrameLayout->addWidget(posGroup);

    QGridLayout *posGroupLayout = new QGridLayout;

    posGroupLayout->addWidget(new QLabel(i18n("Absolute time:")), 0, 0);
    m_absoluteTime = new QLabel;
    posGroupLayout->addWidget(m_absoluteTime, 0, 1);

    posGroupLayout->addWidget(new QLabel(i18n("Real time:")), 1, 0);
    m_realTime = new QLabel;
    posGroupLayout->addWidget(m_realTime, 1, 1);

    posGroupLayout->addWidget(new QLabel(i18n("In measure:")), 2, 0);
    m_barTime = new QLabel;
    posGroupLayout->addWidget(m_barTime, 2, 1);

    posGroup->setLayout(posGroupLayout);

    QFrame *btnBox = new QFrame( mainFrame );
    mainFrameLayout->addWidget(btnBox);
    mainFrame->setLayout(mainFrameLayout);

    btnBox->setSizePolicy(
        QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    btnBox->setContentsMargins(4, 4, 4, 4);
    QHBoxLayout* layout = new QHBoxLayout(btnBox);
    layout->setSpacing(10);

    m_addButton = new QPushButton(i18n("Add"), btnBox);
    m_deleteButton = new QPushButton(i18n("Delete"), btnBox);
    m_deleteAllButton = new QPushButton(i18n("Delete All"), btnBox);

    m_closeButton = new QPushButton(i18n("Close"), btnBox);

    m_addButton->setToolTip(i18n("Add a Marker"));

    m_deleteButton->setToolTip(i18n("Delete a Marker"));

    m_deleteAllButton->setToolTip(i18n("Delete All Markers"));

    m_closeButton->setToolTip(i18n("Close the Marker Editor"));

    layout->addStretch(10);
    layout->addWidget(m_addButton);
    layout->addWidget(m_deleteButton);
    layout->addWidget(m_deleteAllButton);
    layout->addSpacing(30);

    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    btnBox->setLayout(layout);

    connect(m_addButton, SIGNAL(released()),
            SLOT(slotAdd()));

    connect(m_deleteButton, SIGNAL(released()),
            SLOT(slotDelete()));

    connect(m_closeButton, SIGNAL(released()),
            SLOT(slotClose()));

    connect(m_deleteAllButton, SIGNAL(released()),
            SLOT(slotDeleteAll()));

    setupActions();

//     m_doc->getCommandHistory()->attachView(actionCollection());	//&&&
	
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    connect(m_listView, SIGNAL(doubleClicked(QTreeWidgetItem *)),
            SLOT(slotEdit(QTreeWidgetItem *)));

    connect(m_listView, SIGNAL(pressed(QTreeWidgetItem *)),
            this, SLOT(slotItemClicked(QTreeWidgetItem *)));

    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
// 	m_listView->setSelectionBehavior( QAbstractItemView::SelectRows );
	m_listView->setSelectionMode( QAbstractItemView::ExtendedSelection );
	
// 	m_listView->setItemsRenameable(true);	
	QTreeWidgetItem* item;
	for( int i=0; i< m_listView->topLevelItemCount(); i++ ){
		item = m_listView->topLevelItem( i );
		item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
	}
	
    initDialog();

//     setAutoSaveSettings(MarkerEditorConfigGroup, true);	//&&&

    m_shortcuterators = new QShortcut(this);
}

void
MarkerEditor::updatePosition()
{
    timeT pos = m_doc->getComposition().getPosition();
    m_absoluteTime->setText(QString("%1").arg(pos));

    RealTime rT = m_doc->getComposition().getElapsedRealTime(pos);
    long hours = rT.sec / (60 * 60);
    long mins = rT.sec / 60;
    long secs = rT.sec;
    long msecs = rT.msec();

    QString realTime, secsStr;
    if (hours)
        realTime += QString("%1h ").arg(hours);
    if (mins)
        realTime += QString("%1m ").arg(mins);
    secsStr.sprintf("%ld.%03lds", secs, msecs);
    realTime += secsStr;

    // only update if we need to to try and avoid flickering
    if (m_realTime->text() != realTime)
        m_realTime->setText(realTime);

    QString barTime =
        QString("%1").arg(m_doc->getComposition().getBarNumber(pos) + 1);

    // again only update if needed
    if (m_barTime->text() != barTime)
        m_barTime->setText(barTime);

    /*
    // Don't allow us to add another marker if there's already one
    // at the current position.
    //
    if (m_doc->getComposition().
            isMarkerAtPosition(m_doc->getComposition().getPosition()))
        m_addButton->setEnabled(false);
    else
        m_addButton->setEnabled(true);
        */
}

MarkerEditor::~MarkerEditor()
{
    RG_DEBUG << "MarkerEditor::~MarkerEditor" << endl;

//     m_listView->saveLayout(MarkerEditorConfigGroup);	//&&&

//     if (m_doc)
//         m_doc->getCommandHistory()->detachView(actionCollection());	//&&&
}

void
MarkerEditor::initDialog()
{
    RG_DEBUG << "MarkerEditor::initDialog" << endl;
    slotUpdate();
}

void
MarkerEditor::slotUpdate()
{
    RG_DEBUG << "MarkerEditor::slotUpdate" << endl;

    //QPtrList<QTreeWidgetItem> selection = m_listView->selectedItems();

    MarkerEditorViewItem *item;

    m_listView->clear();

    Composition::markercontainer markers =
        m_doc->getComposition().getMarkers();

    Composition::markerconstiterator it;

    QSettings settings;
    settings.beginGroup( MarkerEditorConfigGroup );

    int timeMode = settings.value("timemode", 0).toInt() ;

    for (it = markers.begin(); it != markers.end(); ++it) {
        QString timeString = makeTimeString((*it)->getTime(), timeMode);

        item = new MarkerEditorViewItem(
									m_listView,
                                    (*it)->getID(),
									QStringList()
										<< timeString
                                    	<< strtoqstr((*it)->getName())
                                    	<< strtoqstr((*it)->getDescription())
									);

        // Set this for the MarkerEditor
        //
        item->setRawTime((*it)->getTime());

        m_listView->addTopLevelItem(item);
    }

    if (m_listView->topLevelItemCount() == 0) {
        QTreeWidgetItem *item = new MarkerEditorViewItem(m_listView, 0, QStringList(i18n("<none>")) );
		
		((MarkerEditorViewItem *)item)->setFake(true);
        m_listView->addTopLevelItem(item);

		m_listView->setSelectionMode( QAbstractItemView::NoSelection );
    } else {
		m_listView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    }

    updatePosition();

    settings.endGroup();
}

void
MarkerEditor::slotDeleteAll()
{
    RG_DEBUG << "MarkerEditor::slotDeleteAll" << endl;
    MacroCommand *command = new MacroCommand(i18n("Remove all markers"));

// 	QTreeWidgetItem *item = m_listView->firstChild();
	QTreeWidgetItem *item;
	int cnt = m_listView->topLevelItemCount();

//     do {
	for( int i=0; i< cnt; i++ ){
		item = m_listView->topLevelItem( i );
		
        MarkerEditorViewItem *ei =
                dynamic_cast<MarkerEditorViewItem *>(item);
		
        if (!ei || ei->isFake())
                continue;

        RemoveMarkerCommand *rc =
            new RemoveMarkerCommand(&m_doc->getComposition(),
                                    ei->getID(),
                                    ei->getRawTime(),
                                    qstrtostr(item->text(1)),
                                    qstrtostr(item->text(2)));
        command->addCommand(rc);
    };	// while ((item = item->nextSibling()));

    addCommandToHistory(command);
}

void
MarkerEditor::slotAdd()
{
    RG_DEBUG << "MarkerEditor::slotAdd" << endl;

    AddMarkerCommand *command =
        new AddMarkerCommand(&m_doc->getComposition(),
                             m_doc->getComposition().getPosition(),
                             std::string("new marker"),
                             std::string("no description"));

    addCommandToHistory(command);
}

void
MarkerEditor::slotDelete()
{
    RG_DEBUG << "MarkerEditor::slotDelete" << endl;
    QTreeWidgetItem *item = m_listView->currentItem();

    MarkerEditorViewItem *ei =
        dynamic_cast<MarkerEditorViewItem *>(item);

    if (!ei || ei->isFake())
        return ;

    RemoveMarkerCommand *command =
        new RemoveMarkerCommand(&m_doc->getComposition(),
                                ei->getID(),
                                ei->getRawTime(),
                                qstrtostr(item->text(1)),
                                qstrtostr(item->text(2)));

    addCommandToHistory(command);

}

void
MarkerEditor::slotClose()
{
    RG_DEBUG << "MarkerEditor::slotClose" << endl;

//     if (m_doc)
//         m_doc->getCommandHistory()->detachView(actionCollection());	//&&&
    m_doc = 0;

    close();
}

void
MarkerEditor::setupActions()
{
//     KAction* close = KStandardAction::close(this,
//                                        SLOT(slotClose()),
//                                        actionCollection());
	QAction *close = new QAction( i18n("&Close"), this );
// 	connect( close, SIGNAL(), this, SLOT(slotClose()) );
	
    m_closeButton->setText( close->text() );
    connect(m_closeButton, SIGNAL(released()), this, SLOT(slotClose()));
	
    // some adjustments
//     new KToolBarPopupAction(i18n("Und&o"),
//                             "undo",
//                             KStandardShortcut::key(KStandardShortcut::Undo),
//                             actionCollection(),
//                             KStandardAction::stdName(KStandardAction::Undo));
	QAction* tac;
	tac = new QAction( i18n("Und&o"), this );
	tac->setObjectName( "undo" );
	tac->setShortcut( QKeySequence::Undo );
	
//     new KToolBarPopupAction(i18n("Re&do"),
//                             "redo",
//                             KStandardShortcut::key(KStandardShortcut::Redo),
//                             actionCollection(),
//                             KStandardAction::stdName(KStandardAction::Redo));
	tac = new QAction( i18n("Re&do"), this );
	tac->setObjectName( "redo" );
	tac->setShortcut( QKeySequence::Redo );


    QSettings settings;
    settings.beginGroup( MarkerEditorConfigGroup );

    int timeMode = settings.value("timemode", 0).toInt() ;

//     KRadioAction *action;

// 	QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
// 	Q3CanvasPixmap pixmap(pixmapDir + "/toolbar/time-musical.png");
//     QIcon icon(pixmap);
	
	QWidget* qa_parent = this;
	QActionGroup *qag_timeMode	= new QActionGroup( this );
	
	IconLoader il;
	QIcon icon;
	icon = il.load( "time-musical" );
	
//     action = 
	QAction* qa_time_musical = new QAction( icon, i18n("&Musical Times"), qa_parent );
			connect( qa_time_musical, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotMusicalTime()) );
			qa_time_musical->setObjectName( "time_musical" );
			qa_time_musical->setCheckable( true );		//
			qa_time_musical->setChecked( false );			//
			qa_time_musical->setAutoRepeat( false );		//
			qa_time_musical->setActionGroup( qag_timeMode );	// QActionGroup*
			//### FIX: deallocate QAction ptr

    if (timeMode == 0)
		qa_time_musical->setChecked(true);

//     pixmap.load(pixmapDir + "/toolbar/time-real.png");
//     icon = QIcon(pixmap);
	icon = il.load( "real-musical" );	

//     action = 
	QAction* qa_time_real = new QAction( icon, i18n("&Real Times"), qa_parent );
			connect( qa_time_real, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRealTime()) );
			qa_time_real->setObjectName( "time_real" );
			qa_time_real->setCheckable( true );		//
			qa_time_real->setChecked( false );			//
			qa_time_real->setAutoRepeat( false );		//
			qa_time_real->setActionGroup( qag_timeMode );	// QActionGroup*
			//### FIX: deallocate QAction ptr

    if (timeMode == 1)
		qa_time_real->setChecked(true);

//     pixmap.load(pixmapDir + "/toolbar/time-raw.png");
//     icon = QIcon(pixmap);
	icon = il.load( "time-raw" );

	QAction* qa_time_raw = new QAction( icon, i18n("Ra&w Times"), qa_parent );
			connect( qa_time_raw, SIGNAL(toggled()), dynamic_cast<QObject*>(this), SLOT(slotRawTime()) );
			qa_time_raw->setObjectName( "time_raw" );
			qa_time_raw->setCheckable( true );		//
			qa_time_raw->setChecked( false );			//
			qa_time_raw->setAutoRepeat( false );		//
			qa_time_raw->setActionGroup( qag_timeMode );	// QActionGroup*
			//### FIX: deallocate QAction ptr

    if (timeMode == 2)
		qa_time_raw->setChecked(true);

    rgTempQtIV->createGUI("markereditor.rc", 0);

    settings.endGroup();
}

void
MarkerEditor::addCommandToHistory(Command *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}

MultiViewCommandHistory*
MarkerEditor::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

void
MarkerEditor::setModified(bool modified)
{
    RG_DEBUG << "MarkerEditor::setModified(" << modified << ")" << endl;

    if (modified) {}
    else {}

    m_modified = modified;
}

void
MarkerEditor::checkModified()
{
    RG_DEBUG << "MarkerEditor::checkModified(" << m_modified << ")"
    << endl;

}

void
MarkerEditor::slotEdit(QTreeWidgetItem *i)
{
    RG_DEBUG << "MarkerEditor::slotEdit" << endl;

    if (m_listView->selectionMode() == QTreeWidget::NoSelection) {
        // The marker list is empty, so we shouldn't allow editing the
        // <none> placeholder
        return ;
    }

    // Need to get the raw time from the ListViewItem
    //
    MarkerEditorViewItem *item =
        dynamic_cast<MarkerEditorViewItem*>(i);

    if (!item || item->isFake())
        return ;

    MarkerModifyDialog dialog(this,
                              &m_doc->getComposition(),
                              item->getRawTime(),
                              item->text(1),
                              item->text(2));

    if (dialog.exec() == QDialog::Accepted) {
        ModifyMarkerCommand *command =
            new ModifyMarkerCommand(&m_doc->getComposition(),
                                    item->getID(),
                                    dialog.getOriginalTime(),
                                    dialog.getTime(),
                                    qstrtostr(dialog.getName()),
                                    qstrtostr(dialog.getDescription()));

        addCommandToHistory(command);
    }


}

void
MarkerEditor::closeEvent(QCloseEvent *e)
{
    emit closing();
	close();
//     KMainWindow::closeEvent(e);
}

void
MarkerEditor::setDocument(RosegardenGUIDoc *doc)
{
    // reset our pointers
    m_doc = doc;
    m_modified = false;

    slotUpdate();
}

void
MarkerEditor::slotItemClicked(QTreeWidgetItem *item)
{
    RG_DEBUG << "MarkerEditor::slotItemClicked" << endl;
    MarkerEditorViewItem *ei =
        dynamic_cast<MarkerEditorViewItem *>(item);

    if (ei && !ei->isFake()) {
        RG_DEBUG << "MarkerEditor::slotItemClicked - "
        << "jump to marker at " << ei->getRawTime() << endl;

        emit jumpToMarker(timeT(ei->getRawTime()));
    }
}

QString
MarkerEditor::makeTimeString(timeT time, int timeMode)
{
    switch (timeMode) {

    case 0:  // musical time
        {
            int bar, beat, fraction, remainder;
            m_doc->getComposition().getMusicalTimeForAbsoluteTime
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
                m_doc->getComposition().getElapsedRealTime(time);
            //        return QString("%1   ").arg(rt.toString().c_str());
            return QString("%1   ").arg(rt.toText().c_str());
        }

    default:
        return QString("%1   ").arg(time);
    }
}

void
MarkerEditor::slotMusicalTime()
{
    QSettings settings;
    settings.beginGroup( MarkerEditorConfigGroup );

    settings.setValue("timemode", 0);
    slotUpdate();

    settings.endGroup();
}

void
MarkerEditor::slotRealTime()
{
    QSettings settings;
    settings.beginGroup( MarkerEditorConfigGroup );

    settings.setValue("timemode", 1);
    slotUpdate();

    settings.endGroup();
}

void
MarkerEditor::slotRawTime()
{
    QSettings settings;
    settings.beginGroup( MarkerEditorConfigGroup );

    settings.setValue("timemode", 2);
    slotUpdate();

    settings.endGroup();
}

}
#include "MarkerEditor.moc"
