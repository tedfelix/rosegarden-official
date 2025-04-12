/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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

#include <QApplication>
#include <QMainWindow>
#include <QLayout>
#include <QVBoxLayout>
#include <QAction>
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

    connect(m_addButton, &QAbstractButton::released,
            this, &MarkerEditor::slotAdd);

    connect(m_deleteButton, &QAbstractButton::released,
            this, &MarkerEditor::slotDelete);

    connect(m_closeButton, &QAbstractButton::released,
            this, &MarkerEditor::slotClose);

    connect(m_deleteAllButton, &QAbstractButton::released,
            this, &MarkerEditor::slotDeleteAll);

    setupActions();

//     CommandHistory::getInstance()->attachView(actionCollection());    //&&&

    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &MarkerEditor::slotUpdate);

    connect(m_listView, &QTreeWidget::itemDoubleClicked,
            this, &MarkerEditor::slotEdit);

    // qt4 code:
    // on pressed
    connect( m_listView, &QTreeWidget::itemPressed, //item,column
            this, &MarkerEditor::slotItemClicked );
//     // on clicked
//     connect( m_listView, SIGNAL(itemClicked( QTreeWidgetItem*, int)), //item,column
//             this, SLOT(slotItemClicked(QTreeWidgetItem*, int)) );


    // Highlight all columns - enable extended selection mode
    //
    m_listView->setAllColumnsShowFocus(true);
//     m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);

//     m_listView->setItemsRenameable(true);
    for(int i=0; i< m_listView->topLevelItemCount(); i++){
        QTreeWidgetItem* item = m_listView->topLevelItem(i);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }

    initDialog();

//     setAutoSaveSettings(MarkerEditorConfigGroup, true);    //&&&

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

    QString realTime;
    if (hours)
        realTime += QString("%1h ").arg(hours);
    if (mins)
        realTime += QString("%1m ").arg(mins);
    const QString secsStr = QString::asprintf("%ld.%03lds", secs, msecs);
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
    RG_DEBUG << "MarkerEditor::~MarkerEditor";

}

void
MarkerEditor::initDialog()
{
    RG_DEBUG << "MarkerEditor::initDialog";
    slotUpdate();
}

void
MarkerEditor::slotUpdate()
{
    RG_DEBUG << "MarkerEditor::slotUpdate";

    m_listView->clear();

    Composition &comp = m_doc->getComposition();

    Composition::MarkerVector markers = comp.getMarkers();

    Composition::MarkerVector::const_iterator it;

    QSettings settings;
    settings.beginGroup(MarkerEditorConfigGroup);

    int timeMode = settings.value("timemode", 0).toInt() ;

    for (it = markers.begin(); it != markers.end(); ++it) {
        const QString timeString = comp.makeTimeString(
                (*it)->getTime(), static_cast<Composition::TimeMode>(timeMode));

        MarkerEditorViewItem *item =
            new MarkerEditorViewItem(
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
        MarkerEditorViewItem *newItem = new MarkerEditorViewItem(m_listView, 0, QStringList(tr("<none>")));

        newItem->setFake(true);
        m_listView->addTopLevelItem(newItem);

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
    RG_DEBUG << "MarkerEditor::slotDeleteAll";
    MacroCommand *command = new MacroCommand(tr("Remove all markers"));

//     QTreeWidgetItem *item = m_listView->firstChild();
    int cnt = m_listView->topLevelItemCount();

//     do {
    for(int i=0; i< cnt; i++){
        QTreeWidgetItem *item = m_listView->topLevelItem(i);

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
    RG_DEBUG << "MarkerEditor::slotAdd";

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
    RG_DEBUG << "MarkerEditor::slotDelete";
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
    RG_DEBUG << "MarkerEditor::slotClose";

//     if (m_doc)
//         CommandHistory::getInstance()->detachView(actionCollection());    //&&&
    m_doc = nullptr;

    close();
}

void
MarkerEditor::setupActions()
{
    createAction("file_close", SLOT(slotClose())); //!!! uh-oh, file_close_discard in rc file

    m_closeButton->setText(tr("Close"));
    connect(m_closeButton, &QAbstractButton::released, this, &MarkerEditor::slotClose);

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

    createMenusAndToolbars("markereditor.rc"); //@@@ JAS orig 0

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
    RG_DEBUG << "MarkerEditor::setModified(" << modified << ")";

    if (modified) {}
    else {}

    m_modified = modified;
}

/* unused
void
MarkerEditor::checkModified()
{
    RG_DEBUG << "MarkerEditor::checkModified(" << m_modified << ")";
}
*/

void
MarkerEditor::slotEdit(QTreeWidgetItem *i, int)
{
    RG_DEBUG << "MarkerEditor::slotEdit";

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
    RG_DEBUG << "MarkerEditor::slotItemClicked";
    MarkerEditorViewItem *ei =
        dynamic_cast<MarkerEditorViewItem *>(item);

    if (ei && !ei->isFake()) {
        RG_DEBUG << "MarkerEditor::slotItemClicked - "
        << "jump to marker at " << ei->getRawTime();

        emit jumpToMarker(timeT(ei->getRawTime()));
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
