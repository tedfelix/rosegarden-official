// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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


#include <kconfig.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kstddirs.h>

#include <qvbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qaccel.h>
#include <qiconset.h>

#include "markereditor.h"
#include "rosegardenguidoc.h"
#include "rosedebug.h"
#include "rosestrings.h"
#include "editcommands.h"
#include "widgets.h"


// ------- MarkerEditorViewItem --------
//
//

class MarkerEditorViewItem : public QListViewItem
{
public:
    MarkerEditorViewItem(QListView * parent, QString label1, 
                         QString label2 = QString::null, 
                         QString label3 = QString::null,
                         QString label4 = QString::null, 
                         QString label5 = QString::null, 
                         QString label6 = QString::null, 
                         QString label7 = QString::null, 
                         QString label8 = QString::null):
        QListViewItem(parent, label1, label2, label3, label4,
                      label5, label6, label7, label8) { ; }

    virtual int compare(QListViewItem * i, int col, bool ascending) const;

    void setRawTime(Rosegarden::timeT rawTime) { m_rawTime = rawTime; }
    Rosegarden::timeT getRawTime() const { return m_rawTime; }

protected:
    Rosegarden::timeT   m_rawTime;
};

int
MarkerEditorViewItem::compare(QListViewItem * i, int col, bool ascending) const
{
    MarkerEditorViewItem *ei = 
        dynamic_cast<MarkerEditorViewItem *>(i);

    if (!ei) return QListViewItem::compare(i, col, ascending);

    // Raw time sorting on time column
    //
    if (col == 0) {  

        if (m_rawTime < ei->getRawTime()) return -1;
        else if (ei->getRawTime() < m_rawTime) return 1;
        else return 0;

    } else {
        return QListViewItem::compare(i, col, ascending);
    }
}


// -------- MarkerEditorDialog --------
//
//

MarkerEditorDialog::MarkerEditorDialog(QWidget *parent,
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
    m_listView->addColumn(i18n("Marker name  "));
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

    QToolTip::add(m_addButton,
                  i18n("Add a Marker"));

    QToolTip::add(m_deleteButton,
                  i18n("Delete a Marker"));

    QToolTip::add(m_deleteAllButton,
                  i18n("Delete All Markers"));

    QToolTip::add(m_closeButton,
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
MarkerEditorDialog::updatePosition()
{
    Rosegarden::timeT pos = m_doc->getComposition().getPosition();
    m_absoluteTime->setText(QString("%1").arg(pos));

    Rosegarden::RealTime rT = m_doc->getComposition().getElapsedRealTime(pos);
    long hours = rT.sec / (60 * 60);
    long mins = rT.sec / 60;
    long secs = rT.sec;
    long msecs = rT.msec();

    QString realTime, secsStr;
    if (hours) realTime += QString("%1h ").arg(hours);
    if (mins) realTime += QString("%1m ").arg(hours);
    secsStr.sprintf("%ld.%03lds", secs, msecs);
    realTime += secsStr;

    // only update if we need to to try and avoid flickering
    if (m_realTime->text() != realTime) m_realTime->setText(realTime);

    QString barTime = 
        QString("%1").arg(m_doc->getComposition().getBarNumber(pos) + 1);

    // again only update if needed
    if (m_barTime->text() != barTime) m_barTime->setText(barTime);

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



MarkerEditorDialog::~MarkerEditorDialog()
{
    RG_DEBUG << "MarkerEditorDialog::~MarkerEditorDialog" << endl;

    m_listView->saveLayout(kapp->config(), MarkerEditorConfigGroup);

    if (m_doc)
        m_doc->getCommandHistory()->detachView(actionCollection());
}

void 
MarkerEditorDialog::initDialog()
{
    RG_DEBUG << "MarkerEditorDialog::initDialog" << endl;
    slotUpdate();
}

void
MarkerEditorDialog::slotUpdate()
{
    RG_DEBUG << "MarkerEditorDialog::slotUpdate" << endl;

    //QPtrList<QListViewItem> selection = m_listView->selectedItems();

    MarkerEditorViewItem *item;

    m_listView->clear();

    Rosegarden::Composition::markercontainer markers =
       m_doc->getComposition().getMarkers();

    Rosegarden::Composition::markerconstiterator it;

    kapp->config()->setGroup(MarkerEditorConfigGroup);
    int timeMode = kapp->config()->readNumEntry("timemode", 0);

    for (it = markers.begin(); it != markers.end(); ++it)
    {
        QString timeString = makeTimeString((*it)->getTime(), timeMode);

        item = new 
            MarkerEditorViewItem(m_listView,
                                 timeString,
                                 strtoqstr((*it)->getName()),
                                 strtoqstr((*it)->getDescription()));

        // Set this for the MarkerEditorDialog
        //
        item->setRawTime((*it)->getTime());

        m_listView->insertItem(item);
    }

    if (m_listView->childCount() == 0)
    {
        QListViewItem *item = 
            new MarkerEditorViewItem(m_listView, i18n("<none>"));
        m_listView->insertItem(item);

        m_listView->setSelectionMode(QListView::NoSelection);
    }
    else
    {
        m_listView->setSelectionMode(QListView::Extended);
    }

    updatePosition();

}

void 
MarkerEditorDialog::slotDeleteAll()
{
    RG_DEBUG << "MarkerEditorDialog::slotDeleteAll" << endl;
    KMacroCommand *command = new KMacroCommand(i18n("Remove all markers"));

    QListViewItem *item = m_listView->firstChild();

    do
    {
	MarkerEditorViewItem *ei = 
	    dynamic_cast<MarkerEditorViewItem *>(item);
	if (!ei) continue;

        RemoveMarkerCommand *rc = 
            new RemoveMarkerCommand(&m_doc->getComposition(),
                                    ei->getRawTime(),
                                    qstrtostr(item->text(1)),
                                    qstrtostr(item->text(2)));
        command->addCommand(rc);
    }
    while((item = item->nextSibling()));

    addCommandToHistory(command);
}

void
MarkerEditorDialog::slotAdd()
{
    RG_DEBUG << "MarkerEditorDialog::slotAdd" << endl;

    AddMarkerCommand *command =
        new AddMarkerCommand(&m_doc->getComposition(),
                             m_doc->getComposition().getPosition(),
                             std::string("new marker"),
                             std::string("no description"));

    addCommandToHistory(command);
}


void
MarkerEditorDialog::slotDelete()
{
    RG_DEBUG << "MarkerEditorDialog::slotDelete" << endl;
    QListViewItem *item = m_listView->currentItem();

    MarkerEditorViewItem *ei = 
        dynamic_cast<MarkerEditorViewItem *>(item);

    if (!ei) return;

    RemoveMarkerCommand *command =
        new RemoveMarkerCommand(&m_doc->getComposition(),
                                ei->getRawTime(),
                                qstrtostr(item->text(1)),
                                qstrtostr(item->text(2)));

    addCommandToHistory(command);
                                             
}

void
MarkerEditorDialog::slotClose()
{
    RG_DEBUG << "MarkerEditorDialog::slotClose" << endl;

    if (m_doc) m_doc->getCommandHistory()->detachView(actionCollection());
    m_doc = 0;

    close();
}

void
MarkerEditorDialog::setupActions()
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
    if (timeMode == 0) action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-real.png");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("&Real Times"), icon, 0, this,
                              SLOT(slotRealTime()),
                              actionCollection(), "time_real");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 1) action->setChecked(true);

    pixmap.load(pixmapDir + "/toolbar/time-raw.png");
    icon = QIconSet(pixmap);

    action = new KRadioAction(i18n("Ra&w Times"), icon, 0, this,
                              SLOT(slotRawTime()),
                              actionCollection(), "time_raw");
    action->setExclusiveGroup("timeMode");
    if (timeMode == 2) action->setChecked(true);

    createGUI("markereditor.rc");
}

void 
MarkerEditorDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}

MultiViewCommandHistory* 
MarkerEditorDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}


void
MarkerEditorDialog::setModified(bool modified)
{
    RG_DEBUG << "MarkerEditorDialog::setModified(" << modified << ")" << endl;

    if (modified)
    {
    }
    else
    {
    }

    m_modified = modified;
}

void
MarkerEditorDialog::checkModified()
{
    RG_DEBUG << "MarkerEditorDialog::checkModified(" << m_modified << ")" 
             << endl;

}

void
MarkerEditorDialog::slotEdit(QListViewItem *i)
{
    RG_DEBUG << "MarkerEditorDialog::slotEdit" << endl;

    if (m_listView->selectionMode() == QListView::NoSelection) {
	// The marker list is empty, so we shouldn't allow editing the
	// <none> placeholder
	return;
    }

    // Need to get the raw time from the ListViewItem
    //
    MarkerEditorViewItem *item = 
        dynamic_cast<MarkerEditorViewItem*>(i);

    if (!item) return;

    MarkerModifyDialog dialog(this,
			      &m_doc->getComposition(),
			      item->getRawTime(),
			      item->text(1), 
                              item->text(2));

    if (dialog.exec() == QDialog::Accepted)
    {
        ModifyMarkerCommand *command =
            new ModifyMarkerCommand(&m_doc->getComposition(),
                                    dialog.getOriginalTime(),
                                    dialog.getTime(),
                                    qstrtostr(dialog.getName()),
                                    qstrtostr(dialog.getDescription()));

        addCommandToHistory(command);
    }


}

void
MarkerEditorDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
    KMainWindow::closeEvent(e);
}

// Reset the document
//
void
MarkerEditorDialog::setDocument(RosegardenGUIDoc *doc)
{
    // reset our pointers
    m_doc = doc;
    m_modified = false;

    slotUpdate();
}


MarkerModifyDialog::MarkerModifyDialog(QWidget *parent,
				       Rosegarden::Composition *composition,
                                       int time,
                                       const QString &name,
                                       const QString &des):
    KDialogBase(parent, 0, true, i18n("Edit Marker"), Ok | Cancel),
    m_originalTime(time)
{
    QVBox *vbox = makeVBoxMainWidget();

    m_timeEdit = new RosegardenTimeWidget(i18n("Marker Time"), vbox, composition,
					  time);
					  
/*!!!

    layout->addWidget(new QLabel(i18n("Absolute Time:"), frame), 0, 0);
    m_timeEdit = new QSpinBox(frame);
    layout->addWidget(m_timeEdit, 0, 1);

    m_timeEdit->setMinValue(INT_MIN);
    m_timeEdit->setMaxValue(INT_MAX);
    m_timeEdit->setLineStep(
            Rosegarden::Note(Rosegarden::Note::Shortest).getDuration());
    m_timeEdit->setValue(time);
*/
    QGroupBox *groupBox = new QGroupBox
        (1, Horizontal, i18n("Marker Properties"), vbox);

    QFrame *frame = new QFrame(groupBox);

    QGridLayout *layout = new QGridLayout(frame, 2, 2, 5, 5);

    layout->addWidget(new QLabel(i18n("Name:"), frame), 0, 0);
    m_nameEdit = new QLineEdit(name, frame);
    layout->addWidget(m_nameEdit, 0, 1);

    layout->addWidget(new QLabel(i18n("Description:"), frame), 1, 0);
    m_desEdit = new QLineEdit(des, frame);
    layout->addWidget(m_desEdit, 1, 1);
}

void
MarkerEditorDialog::slotItemClicked(QListViewItem *item)
{
    RG_DEBUG << "MarkerEditorDialog::slotItemClicked" << endl;
    MarkerEditorViewItem *ei = 
        dynamic_cast<MarkerEditorViewItem *>(item);

    if (ei)
    {
        RG_DEBUG << "MarkerEditorDialog::slotItemClicked - "
                 << "jump to marker at " << ei->getRawTime() << endl;

        emit jumpToMarker(Rosegarden::timeT(ei->getRawTime()));
    }
}

QString
MarkerEditorDialog::makeTimeString(Rosegarden::timeT time, int timeMode)
{
    switch (timeMode) {

    case 0: // musical time
    {
        int bar, beat, fraction, remainder;
        m_doc->getComposition().getMusicalTimeForAbsoluteTime
            (time, bar, beat, fraction, remainder);
        ++bar;
        return QString("%1%2%3-%4%5-%6%7-%8%9   ")
            .arg(bar/100)
            .arg((bar%100)/10)
            .arg(bar%10)
            .arg(beat/10)
            .arg(beat%10)
            .arg(fraction/10)
            .arg(fraction%10)
            .arg(remainder/10)
            .arg(remainder%10);
    }

    case 1: // real time
    {
        Rosegarden::RealTime rt =
            m_doc->getComposition().getElapsedRealTime(time);
        return QString("%1   ").arg(rt.toString().c_str());
    }

    default:
        return QString("%1   ").arg(time);
    }
}

void
MarkerEditorDialog::slotMusicalTime()
{
    kapp->config()->setGroup(MarkerEditorConfigGroup);
    kapp->config()->writeEntry("timemode", 0);
    slotUpdate();
}

void
MarkerEditorDialog::slotRealTime()
{
    kapp->config()->setGroup(MarkerEditorConfigGroup);
    kapp->config()->writeEntry("timemode", 1);
    slotUpdate();
}

void
MarkerEditorDialog::slotRawTime()
{
    kapp->config()->setGroup(MarkerEditorConfigGroup);
    kapp->config()->writeEntry("timemode", 2);
    slotUpdate();
}



const char* const MarkerEditorDialog::MarkerEditorConfigGroup = "Marker Editor";
#include "markereditor.moc"
