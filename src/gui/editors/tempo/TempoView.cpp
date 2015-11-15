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

#define RG_MODULE_STRING "[TempoView]"

#include "TempoView.h"

#include "TempoListItem.h"

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
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/general/ListEditView.h"
#include "gui/general/IconLoader.h"
#include "gui/widgets/TmpStatusMsg.h"
#include "misc/Strings.h"

#include <QAction>
#include <QSettings>
#include <QTreeWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QDialog>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QList>
#include <QDesktopServices>


namespace Rosegarden
{

TempoView::TempoView(RosegardenDocument *doc, QWidget *parent, timeT openTime):
        ListEditView(doc, std::vector<Segment *>(), 2, parent),
        m_filter(Tempo | TimeSignature),
        m_ignoreUpdates(true)
{
    initStatusBar();
    setupActions();

    // define some note filtering buttons in a group
    //
    m_filterGroup = new QGroupBox(tr("Filter"), getCentralWidget());
    QVBoxLayout *filterGroupLayout = new QVBoxLayout;
    m_filterGroup->setLayout(filterGroupLayout);

    m_tempoCheckBox = new QCheckBox(tr("Tempo"), m_filterGroup);
    filterGroupLayout->addWidget(m_tempoCheckBox, 50, Qt::AlignTop);

    m_timeSigCheckBox = new QCheckBox(tr("Time Signature"), m_filterGroup);
    filterGroupLayout->addWidget(m_timeSigCheckBox, 50, Qt::AlignTop);

    // hard coded spacers are evil, but I can't find any other way to fix this
    filterGroupLayout->addSpacing(200);

    m_filterGroup->setLayout(filterGroupLayout);
    m_grid->addWidget(m_filterGroup, 2, 0);

    m_list = new QTreeWidget(getCentralWidget());
    
//     m_list->setItemsRenameable(true);    //&&&

    m_grid->addWidget(m_list, 2, 1);

    updateViewCaption();

    doc->getComposition().addObserver(this);

    // Connect double clicker
    //
    connect(m_list, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
            SLOT(slotPopupEditor(QTreeWidgetItem*, int)));

    m_list->setAllColumnsShowFocus(true);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    
    QStringList sl;
    sl << tr("Time  ")
       << tr("Type  ")
       << tr("Value  ")
       << tr("Properties  ");
    
    m_list->setColumnCount(4);
    m_list->setHeaderLabels(sl);
    

//     for (int col = 0; col < m_list->columnCount(); ++col)
//         m_list->setRenameable(col, true);    //&&&
    
    readOptions();
    setButtonsToFilter();

    connect(m_tempoCheckBox, SIGNAL(stateChanged(int)),
            SLOT(slotModifyFilter(int)));
    connect(m_timeSigCheckBox, SIGNAL(stateChanged(int)),
            SLOT(slotModifyFilter(int)));

    applyLayout();

    makeInitialSelection(openTime);

    m_ignoreUpdates = false;
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
    slotSaveOptions();
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
    // crashing selection garbage removed, and selection changed to single
    // selection, which is really the only sane thing to do with this anyway;
    // Classic had multiple selections for some reason, but you could only edit
    // one event at a time.

    // Ok, recreate list
    //
    m_list->clear();

    Composition *comp = &getDocument()->getComposition();

    QSettings settings;
    settings.beginGroup(TempoViewConfigGroup);

    int timeMode = settings.value("timemode", 0).toInt() ;
    settings.endGroup();

    if (m_filter & TimeSignature) {
        for (int i = 0; i < comp->getTimeSignatureCount(); ++i) {

            std::pair<timeT, Rosegarden::TimeSignature> sig =
                comp->getTimeSignatureChange(i);

            QString properties;
            if (sig.second.isHidden()) {
                if (sig.second.isCommon())
                    properties = tr("Common, hidden");
                else
                    properties = tr("Hidden");
            } else {
                if (sig.second.isCommon())
                    properties = tr("Common");
            }

            QString timeString = makeTimeString(sig.first, timeMode);

            new TempoListItem(comp, TempoListItem::TimeSignature,
                              sig.first, i, m_list, 
                            QStringList()
                            << timeString
                              << tr("Time Signature   ")
                              << QString("%1/%2   ").arg(sig.second.getNumerator()).
                                              arg(sig.second.getDenominator())
                              << properties);
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
                desc = tr("%1.%2%3")
                       .arg(qpmUnits).arg(qpmTenths).arg(qpmHundredths);
            } else {
                float bpm = (qpm *
                             Note(Note::Crotchet).getDuration()) /
                            sig.getBeatDuration();
                int bpmUnits = int(bpm + 0.001);
                int bpmTenths = int((bpm - bpmUnits) * 10 + 0.001);
                int bpmHundredths = int((bpm - bpmUnits - bpmTenths / 10.0) * 100 + 0.001);

                desc = tr("%1.%2%3 qpm (%4.%5%6 bpm)   ")
                       .arg(qpmUnits).arg(qpmTenths).arg(qpmHundredths)
                       .arg(bpmUnits).arg(bpmTenths).arg(bpmHundredths);
            }

            QString timeString = makeTimeString(tempo.first, timeMode);

            new TempoListItem(comp, TempoListItem::Tempo,
                              tempo.first, i, m_list, QStringList() << timeString
                              << tr("Tempo   ")
                              << desc);
        }
    }

    //!!! the <nothing at this filter level> does not work, and I'm ignoring it
    if (m_list->topLevelItemCount() == 0) {
        new QTreeWidgetItem(m_list, QStringList() << tr("<nothing at this filter level>"));
        m_list->setSelectionMode(QTreeWidget::NoSelection);
        leaveActionState("have_selection");
    } else {
        m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // If no selection then select the first event
        if (m_listSelection.size() == 0)
            m_listSelection.push_back(0);
        enterActionState("have_selection");
    }

    // Set a selection from a range of indexes
    //
    std::vector<int>::iterator sIt = m_listSelection.begin();
    int index = 0;

    for (; sIt != m_listSelection.end(); ++sIt) {
        index = *sIt;

        while (index > 0 && !m_list->topLevelItem(index))
            index--;

//         m_list->setSelected(m_list->topLevelItem(index), true);
        m_list->topLevelItem(index)->setSelected(true);
//         m_list->setCurrentIndex(m_list->topLevelItem(index));
        m_list->setCurrentItem( m_list->topLevelItem(index) );

        // ensure visible
//         m_list->ensureItemVisible(m_list->topLevelItem(index));
        m_list->scrollToItem(m_list->topLevelItem(index));
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

    for (int i = 0; m_list->topLevelItem(i); ++i) {

        TempoListItem *item = dynamic_cast<TempoListItem *>
                                (m_list->topLevelItem(i));

//         m_list->setSelected(item, false);
        item->setSelected(false);

        if (item) {
            if (item->getTime() > time)
                break;
            goodItem = item;
            goodItemNo = i;
        }
    }

    if (goodItem) {
        m_listSelection.push_back(goodItemNo);
//         m_list->setSelected(goodItem, true);
        goodItem->setSelected(true);
//         m_list->ensureItemVisible(goodItem);
        m_list->scrollToItem(goodItem);
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
            //    return QString("%1   ").arg(rt.toString().c_str());
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
    applyLayout();
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
    QList<QTreeWidgetItem*> selection = m_list->selectedItems();
    
    if (selection.count() == 0) return ;

    RG_DEBUG << "TempoView::slotEditDelete - deleting "
    << selection.count() << " items" << endl;

    // QTreeWidgetItem *listItem;

    TempoListItem *item;
    int itemIndex = -1;

    m_ignoreUpdates = true;
    bool haveSomething = false;

    // We want the Remove commands to be in reverse order, because
    // removing one item by index will affect the indices of
    // subsequent items.  So we'll stack them onto here and then pull
    // them off again.
    std::vector<Command *> commands;

    while (!selection.isEmpty()) {
        item = dynamic_cast<TempoListItem*>(selection.first());

        if (itemIndex == -1) itemIndex = m_list->indexOfTopLevelItem(selection.first());

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

        delete selection.takeFirst();
    }    
    
    if (haveSomething) {
        MacroCommand *command = new MacroCommand
                                 (tr("Delete Tempo or Time Signature"));
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
    QList<QTreeWidgetItem*> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.first());
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
    QList<QTreeWidgetItem*> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.first());
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

    QList<QTreeWidgetItem*> selection = m_list->selectedItems();

    if (selection.count() > 0) {
        TempoListItem *item =
            dynamic_cast<TempoListItem*>(selection.first());
        if (item)
            slotPopupEditor(item);
    }
}

void
TempoView::slotSelectAll()
{
    m_listSelection.clear();
    for (int i = 0; m_list->topLevelItem(i); ++i) {
        m_listSelection.push_back(i);
//         m_list->setSelected(m_list->topLevelItem(i), true);
        m_list->topLevelItem(i)->setSelected(true);
    }
}

void
TempoView::slotClearSelection()
{
    m_listSelection.clear();
    for (int i = 0; m_list->topLevelItem(i); ++i) {
//         m_list->setSelected(m_list->topLevelItem(i), false);
        m_list->topLevelItem(i)->setSelected(false);
    }
}

void
TempoView::setupActions()
{
    ListEditView::setupActions("tempoview.rc", false);

    createAction("insert_tempo", SLOT(slotEditInsertTempo()));
    createAction("insert_timesig", SLOT(slotEditInsertTimeSignature()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("edit", SLOT(slotEdit()));
    createAction("select_all", SLOT(slotSelectAll()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("tempo_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    QSettings settings;
    settings.beginGroup(TempoViewConfigGroup);
    int timeMode = settings.value("timemode", 0).toInt() ;
    settings.endGroup();

    QAction *a;
    a = createAction("time_musical", SLOT(slotMusicalTime()));
    if (timeMode == 0) { a->setCheckable(true); a->setChecked(true); }

    a = createAction("time_real", SLOT(slotRealTime()));
    if (timeMode == 1) { a->setCheckable(true); a->setChecked(true); }

    a = createAction("time_raw", SLOT(slotRawTime()));
    if (timeMode == 2) { a->setCheckable(true); a->setChecked(true); }

    createGUI(getRCFileName());
}

void
TempoView::initStatusBar()
{
    QStatusBar* sb = statusBar();

    sb->showMessage(TmpStatusMsg::getDefaultMsg());
    
//     sb->addItem(TmpStatusMsg::getDefaultMsg(),
//                    TmpStatusMsg::getDefaultId(), 1);
//     sb->setItemAlignment(TmpStatusMsg::getDefaultId(),
//                          AlignLeft | AlignVCenter);
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
    settings.beginGroup(TempoViewConfigGroup);
    EditViewBase::readOptions();
    m_filter = settings.value("filter", m_filter).toInt();
    settings.endGroup();
}

void
TempoView::slotSaveOptions()
{
    QSettings settings;
    settings.beginGroup(TempoViewConfigGroup);
    settings.setValue("filter", m_filter);
    settings.endGroup();
}

void
TempoView::slotModifyFilter(int)
{
    if (m_tempoCheckBox->isChecked()) m_filter |= Tempo;
    else m_filter ^= Tempo;

    if (m_timeSigCheckBox->isChecked()) m_filter |= TimeSignature;
    else m_filter ^= TimeSignature;

    applyLayout();
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
    settings.beginGroup(TempoViewConfigGroup);

    settings.setValue("timemode", 0);
    applyLayout();

    settings.endGroup();
}

void
TempoView::slotRealTime()
{
    QSettings settings;
    settings.beginGroup(TempoViewConfigGroup);

    settings.setValue("timemode", 1);
    applyLayout();

    settings.endGroup();
}

void
TempoView::slotRawTime()
{
    QSettings settings;
    settings.beginGroup(TempoViewConfigGroup);

    settings.setValue("timemode", 2);
    applyLayout();

    settings.endGroup();
}

void
TempoView::slotPopupEditor(QTreeWidgetItem *qitem, int)
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
    setWindowTitle(tr("%1 - Tempo and Time Signature Editor")
                .arg(getDocument()->getTitle()));
}

void
TempoView::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:tempoView-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:tempoView-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
TempoView::slotHelpAbout()
{
    new AboutDialog(this);
}
}
