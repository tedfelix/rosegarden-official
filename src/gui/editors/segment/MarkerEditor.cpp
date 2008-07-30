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


#include "MarkerEditor.h"
#include "MarkerEditorViewItem.h"
#include <qlayout.h>
#include <kapplication.h>

#include <klocale.h>
#include <kstddirs.h>
#include <kstdaccel.h>
#include <kconfig.h>
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
#include "gui/dialogs/MarkerModifyDialog.h"
#include <kaction.h>
#include <kcommand.h>
#include <kglobal.h>
#include <klistview.h>
#include <kmainwindow.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <qaccel.h>
#include <qdialog.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qiconset.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qptrlist.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qcanvas.h>


namespace Rosegarden
{

MarkerEditor::MarkerEditor(QWidget *parent,
                                       RosegardenGUIDoc *doc):
        KMainWindow(parent, "markereditordialog"),
        m_doc(doc),
        m_modified(false)
{
    QVBox* mainFrame = new QVBox(this);
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage Markers"));

    m_listView = new KListView(mainFrame);
    m_listView->addColumn(i18n("Marker time  "));
    m_listView->addColumn(i18n("Marker text  "));
    m_listView->addColumn(i18n("Marker description "));

    // Align centrally
    for (int i = 0; i < 3; ++i)
        m_listView->setColumnAlignment(i, Qt::AlignHCenter);

    QGroupBox *posGroup = new QGroupBox(2, Horizontal,
                                        i18n("Pointer position"), mainFrame);

    new QLabel(i18n("Absolute time:"), posGroup);
    m_absoluteTime = new QLabel(posGroup);

    new QLabel(i18n("Real time:"), posGroup);
    m_realTime = new QLabel(posGroup);

    new QLabel(i18n("In measure:"), posGroup);
    m_barTime = new QLabel(posGroup);

    QFrame* btnBox = new QFrame(mainFrame);

    btnBox->setSizePolicy(
        QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 4, 10);

    m_addButton = new QPushButton(i18n("Add"), btnBox);
    m_deleteButton = new QPushButton(i18n("Delete"), btnBox);
    m_deleteAllButton = new QPushButton(i18n("Delete All"), btnBox);

    m_closeButton = new QPushButton(i18n("Close"), btnBox);

    QToolTip::add
        (m_addButton,
                i18n("Add a Marker"));

    QToolTip::add
        (m_deleteButton,
                i18n("Delete a Marker"));

    QToolTip::add
        (m_deleteAllButton,
                i18n("Delete All Markers"));

    QToolTip::add
        (m_closeButton,
                i18n("Close the Marker Editor"));

    layout->addStretch(10);
    layout->addWidget(m_addButton);
    layout->addWidget(m_deleteButton);
    layout->addWidget(m_deleteAllButton);
    layout->addSpacing(30);

    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_addButton, SIGNAL(released()),
            SLOT(slotAdd()));

    connect(m_deleteButton, SIGNAL(released()),
            SLOT(slotDelete()));

    connect(m_closeButton, SIGNAL(released()),
            SLOT(slotClose()));

    connect(m_deleteAllButton, SIGNAL(released()),
            SLOT(slotDeleteAll()));

    setupActions();

    m_doc->getCommandHistory()->attachView(actionCollection());
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    connect(m_listView, SIGNAL(doubleClicked(QListViewItem *)),
            SLOT(slotEdit(QListViewItem *)));

    connect(m_listView, SIGNAL(pressed(QListViewItem *)),
            this, SLOT(slotItemClicked(QListViewItem *)));

    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
    m_listView->setSelectionMode(QListView::Extended);
    m_listView->setItemsRenameable(true);

    initDialog();

    setAutoSaveSettings(MarkerEditorConfigGroup, true);

    m_accelerators = new QAccel(this);
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

    m_listView->saveLayout(kapp->config(), MarkerEditorConfigGroup);

    if (m_doc)
        m_doc->getCommandHistory()->detachView(actionCollection());
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

    //QPtrList<QListViewItem> selection = m_listView->selectedItems();

    MarkerEditorViewItem *item;

    m_listView->clear();

    Composition::markercontainer markers =
        m_doc->getComposition().getMarkers();

    Composition::markerconstiterator it;

    kapp->config()->setGroup(MarkerEditorConfigGroup);
    int timeMode = kapp->config()->readNumEntry("timemode", 0);

    for (it = markers.begin(); it != markers.end(); ++it) {
        QString timeString = makeTimeString((*it)->getTime(), timeMode);

        item = new
               MarkerEditorViewItem(m_listView,
                                    (*it)->getID(),
                                    timeString,
                                    strtoqstr((*it)->getName()),
                                    strtoqstr((*it)->getDescription()));

        // Set this for the MarkerEditor
        //
        item->setRawTime((*it)->getTime());

        m_listView->insertItem(item);
    }

    if (m_listView->childCount() == 0) {
        QListViewItem *item =
            new MarkerEditorViewItem(m_listView, 0, i18n("<none>"));
        ((MarkerEditorViewItem *)item)->setFake(true);
        m_listView->insertItem(item);

        m_listView->setSelectionMode(QListView::NoSelection);
    } else {
        m_listView->setSelectionMode(QListView::Extended);
    }

    updatePosition();

}

void
MarkerEditor::slotDeleteAll()
{
    RG_DEBUG << "MarkerEditor::slotDeleteAll" << endl;
    KMacroCommand *command = new KMacroCommand(i18n("Remove all markers"));

    QListViewItem *item = m_listView->firstChild();

    do {
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
    } while ((item = item->nextSibling()));

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
    QListViewItem *item = m_listView->currentItem();

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

    if (m_doc)
        m_doc->getCommandHistory()->detachView(actionCollection());
    m_doc = 0;

    close();
}

void
MarkerEditor::setupActions()
{
    KAction* close = KStdAction::close(this,
                                       SLOT(slotClose()),
                                       actionCollection());

    m_closeButton->setText(close->text());
    connect(m_closeButton, SIGNAL(released()), this, SLOT(slotClose()));

    // some adjustments
    new KToolBarPopupAction(i18n("Und&o"),
                            "undo",
                            KStdAccel::key(KStdAccel::Undo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Undo));

    new KToolBarPopupAction(i18n("Re&do"),
                            "redo",
                            KStdAccel::key(KStdAccel::Redo),
                            actionCollection(),
                            KStdAction::stdName(KStdAction::Redo));

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    kapp->config()->setGroup(MarkerEditorConfigGroup);
    int timeMode = kapp->config()->readNumEntry("timemode", 0);

    KRadioAction *action;

    QCanvasPixmap pixmap(pixmapDir + "/toolbar/time-musical.png");
    QIconSet icon(pixmap);

    action = new KRadioAction(i18n("&Musical Times"), icon, 0, this,
                              SLOT(slotMusicalTime()),
                              actionCollection(), "time_musical");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 0)
        action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-real.png");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("&Real Times"), icon, 0, this,
                              SLOT(slotRealTime()),
                              actionCollection(), "time_real");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 1)
        action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-raw.png");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("Ra&w Times"), icon, 0, this,
                              SLOT(slotRawTime()),
                              actionCollection(), "time_raw");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 2)
        action->setChecked(true);

    createGUI("markereditor.rc");
}

void
MarkerEditor::addCommandToHistory(KCommand *command)
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
MarkerEditor::slotEdit(QListViewItem *i)
{
    RG_DEBUG << "MarkerEditor::slotEdit" << endl;

    if (m_listView->selectionMode() == QListView::NoSelection) {
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
    KMainWindow::closeEvent(e);
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
MarkerEditor::slotItemClicked(QListViewItem *item)
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
    kapp->config()->setGroup(MarkerEditorConfigGroup);
    kapp->config()->writeEntry("timemode", 0);
    slotUpdate();
}

void
MarkerEditor::slotRealTime()
{
    kapp->config()->setGroup(MarkerEditorConfigGroup);
    kapp->config()->writeEntry("timemode", 1);
    slotUpdate();
}

void
MarkerEditor::slotRawTime()
{
    kapp->config()->setGroup(MarkerEditorConfigGroup);
    kapp->config()->writeEntry("timemode", 2);
    slotUpdate();
}

}
#include "MarkerEditor.moc"
