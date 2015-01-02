/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MarkerEditor]"

#include "MarkerEditor.h"
#include "MarkerEditorViewItem.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Composition.h"
#include "base/Marker.h"
#include "base/RealTime.h"
#include "commands/edit/AddMarkerCommand.h"
#include "commands/edit/ModifyMarkerCommand.h"
#include "commands/edit/RemoveMarkerCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "document/Command.h"
#include "gui/dialogs/MarkerModifyDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "gui/general/IconLoader.h"

#include <QApplication>
#include <QMainWindow>
#include <QLayout>
#include <QVBoxLayout>
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
#include <QDesktopServices>

#include <QList>


namespace Rosegarden
{


MarkerEditor::MarkerEditor(QWidget *parent,
                           RosegardenDocument *doc):
    QMainWindow(parent),
    m_doc(doc),
    m_modified(false)
{
    this->setObjectName("markereditordialog");
    
    QWidget *mainFrame = new QWidget(this);
    QVBoxLayout *mainFrameLayout = new QVBoxLayout;
    setCentralWidget(mainFrame);

    setWindowTitle(tr("Manage Markers"));

    m_listView = new QTreeWidget(mainFrame);
    mainFrameLayout->addWidget(m_listView);
    
    QStringList sl;
    sl    << tr("Time  ")
        << tr("Text  ")
        << tr("Comment ");
    
    m_listView->setHeaderLabels(sl);
    
    QGroupBox *posGroup = new QGroupBox(tr("Pointer position"), mainFrame);
    mainFrameLayout->addWidget(posGroup);

    QGridLayout *posGroupLayout = new QGridLayout;

    posGroupLayout->addWidget(new QLabel(tr("Absolute time:")), 0, 0);
    m_absoluteTime = new QLabel;
    posGroupLayout->addWidget(m_absoluteTime, 0, 1);

    posGroupLayout->addWidget(new QLabel(tr("Real time:")), 1, 0);
    m_realTime = new QLabel;
    posGroupLayout->addWidget(m_realTime, 1, 1);

    posGroupLayout->addWidget(new QLabel(tr("In measure:")), 2, 0);
    m_barTime = new QLabel;
    posGroupLayout->addWidget(m_barTime, 2, 1);

    posGroup->setLayout(posGroupLayout);

    QFrame *btnBox = new QFrame(mainFrame);
    mainFrameLayout->addWidget(btnBox);
    mainFrame->setLayout(mainFrameLayout);

    btnBox->setSizePolicy(
        QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    btnBox->setContentsMargins(4, 4, 4, 4);
    QHBoxLayout* layout = new QHBoxLayout(btnBox);
    layout->setSpacing(10);

    m_addButton = new QPushButton(tr("Add"), btnBox);
    m_deleteButton = new QPushButton(tr("Delete"), btnBox);
    m_deleteAllButton = new QPushButton(tr("Delete All"), btnBox);

    m_closeButton = new QPushButton(tr("Close"), btnBox);

    m_addButton->setToolTip(tr("Add a Marker"));

    m_deleteButton->setToolTip(tr("Delete a Marker"));

    m_deleteAllButton->setToolTip(tr("Delete All Markers"));

    m_closeButton->setToolTip(tr("Close the Marker Editor"));

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

//     CommandHistory::getInstance()->attachView(actionCollection());    //&&&
    
    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    connect(m_listView, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
            SLOT(slotEdit(QTreeWidgetItem *, int)));
    
    // qt4 code:
    // on pressed
    connect( m_listView, SIGNAL(itemPressed( QTreeWidgetItem*, int)), //item,column
            this, SLOT(slotItemClicked(QTreeWidgetItem*, int)) );
//     // on clicked
//     connect( m_listView, SIGNAL(itemClicked( QTreeWidgetItem*, int)), //item,column
//             this, SLOT(slotItemClicked(QTreeWidgetItem*, int)) );
    
    
    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
//     m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
//     m_listView->setItemsRenameable(true);    
    QTreeWidgetItem* item;
    for(int i=0; i< m_listView->topLevelItemCount(); i++){
        item = m_listView->topLevelItem(i);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }
    
    initDialog();

//     setAutoSaveSettings(MarkerEditorConfigGroup, true);    //&&&

    m_shortcuts = new QShortcut(this);

    setAttribute(Qt::WA_DeleteOnClose);
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

    MarkerEditorViewItem *item;

    m_listView->clear();

    Composition::markercontainer markers =
        m_doc->getComposition().getMarkers();

    Composition::markerconstiterator it;

    QSettings settings;
    settings.beginGroup(MarkerEditorConfigGroup);

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
        QTreeWidgetItem *item = new MarkerEditorViewItem(m_listView, 0, QStringList(tr("<none>")));
        
        ((MarkerEditorViewItem *)item)->setFake(true);
        m_listView->addTopLevelItem(item);

        m_listView->setSelectionMode(QAbstractItemView::NoSelection);
    } else {
        m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }

    updatePosition();

    settings.endGroup();
}

void
MarkerEditor::slotDeleteAll()
{
    RG_DEBUG << "MarkerEditor::slotDeleteAll" << endl;
    MacroCommand *command = new MacroCommand(tr("Remove all markers"));

//     QTreeWidgetItem *item = m_listView->firstChild();
    QTreeWidgetItem *item;
    int cnt = m_listView->topLevelItemCount();

//     do {
    for(int i=0; i< cnt; i++){
        item = m_listView->topLevelItem(i);
        
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
    };    // while ((item = item->nextSibling()));

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
//         CommandHistory::getInstance()->detachView(actionCollection());    //&&&
    m_doc = 0;

    close();
}

void
MarkerEditor::setupActions()
{
    createAction("file_close", SLOT(slotClose())); //!!! uh-oh, file_close_discard in rc file
    
    m_closeButton->setText(tr("Close"));
    connect(m_closeButton, SIGNAL(released()), this, SLOT(slotClose()));

    QSettings settings;
    settings.beginGroup(MarkerEditorConfigGroup);

    int timeMode = settings.value("timemode", 0).toInt() ;
    
    QAction *a;
    a = createAction("time_musical", SLOT(slotMusicalTime()));
    a->setCheckable(true);
    if (timeMode == 0) a->setChecked(true);

    a = createAction("time_real", SLOT(slotRealTime()));
    a->setCheckable(true);
    if (timeMode == 1) a->setChecked(true);

    a = createAction("time_raw", SLOT(slotRawTime()));
    a->setCheckable(true);
    if (timeMode == 2) a->setChecked(true);
    createAction("marker_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    createGUI("markereditor.rc"); //@@@ JAS orig 0

    settings.endGroup();
}

void
MarkerEditor::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
    setModified(false);
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
MarkerEditor::slotEdit(QTreeWidgetItem *i, int)
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
    if(e){ };    // remove warning
    
    emit closing();
    close();
//     KMainWindow::closeEvent(e);
}

void
MarkerEditor::setDocument(RosegardenDocument *doc)
{
    // reset our pointers
    m_doc = doc;
    m_modified = false;

    slotUpdate();
}

void
MarkerEditor::slotItemClicked(QTreeWidgetItem *item, int column )
{
    if( ! item ){
        // no item clicked, ignore
        if( column ){ }; // removes warning
        return;
    }
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
    settings.beginGroup(MarkerEditorConfigGroup);

    settings.setValue("timemode", 0);
    findAction("time_musical")->setChecked(true);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(false);
    slotUpdate();

    settings.endGroup();
}

void
MarkerEditor::slotRealTime()
{
    std::cout << "scooby dooby dooo!!" << std::endl;
    QSettings settings;
    settings.beginGroup(MarkerEditorConfigGroup);

    settings.setValue("timemode", 1);
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(true);
    findAction("time_raw")->setChecked(false);
    slotUpdate();

    settings.endGroup();
}

void
MarkerEditor::slotRawTime()
{
    QSettings settings;
    settings.beginGroup(MarkerEditorConfigGroup);

    settings.setValue("timemode", 2);
    findAction("time_musical")->setChecked(false);
    findAction("time_real")->setChecked(false);
    findAction("time_raw")->setChecked(true);
    slotUpdate();

    settings.endGroup();
}



void
MarkerEditor::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:markerEditor-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:markerEditor-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
MarkerEditor::slotHelpAbout()
{
    new AboutDialog(this);
}
}
#include "MarkerEditor.moc"
