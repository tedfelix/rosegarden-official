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

#define RG_MODULE_STRING "[ControlEditorDialog]"
#define RG_NO_DEBUG_PRINT

#include "ControlEditorDialog.h"

#include "misc/Debug.h"
#include "misc/Strings.h"  // for strtoqstr() etc...
#include "base/Composition.h"
#include "base/ControlParameter.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"  // for PitchBend::EventType
#include "base/Studio.h"
#include "commands/studio/AddControlParameterCommand.h"
#include "commands/studio/ModifyControlParameterCommand.h"
#include "commands/studio/RemoveControlParameterCommand.h"
#include "ControlParameterEditDialog.h"
#include "ControlParameterItem.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"  // for WindowGeometryConfigGroup
#include "document/Command.h"
#include "document/CommandHistory.h"
#include "gui/dialogs/AboutDialog.h"

#include <QTreeWidget>
#include <QColor>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QSettings>
#include <QDesktopServices>


namespace Rosegarden
{


ControlEditorDialog::ControlEditorDialog(QWidget *parent,
                                         RosegardenDocument *doc,
                                         DeviceId device) :
    QMainWindow(parent),
    m_doc(doc),
    m_studio(&doc->getStudio()),
    m_device(device),
    m_modified(false)
{
    RG_DEBUG << "ControlEditorDialog::ControlEditorDialog: device is " << m_device;

    QWidget *mainFrame = new QWidget(this);
    QVBoxLayout *mainFrameLayout = new QVBoxLayout;
    setCentralWidget(mainFrame);
    setAttribute(Qt::WA_DeleteOnClose);

    // everything else failed, so screw it, let's just set the fscking minimum
    // width the brute force way
    setMinimumWidth(935);

    setWindowTitle(tr("Manage Controllers"));

    QString deviceName(tr("<no device>"));
    MidiDevice *md =
        dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));
    if (md)
        deviceName = strtoqstr(md->getName());

    // spacing hack!
    new QLabel("", mainFrame);
    new QLabel(tr("  Controllers for %1 (device %2)")
           .arg(deviceName)
           .arg(device), mainFrame);
    new QLabel("", mainFrame);
    
    QStringList sl;
    sl  << tr("Name  ")
        << tr("Type  ")
        << tr("Number  ")
        << tr("Description  ")
        << tr("Min. value  ")
        << tr("Max. value  ")
        << tr("Default value  ")
        << tr("Color  ")
        << tr("Position on instrument panel");
    
    m_treeWidget = new QTreeWidget(mainFrame);
    m_treeWidget->setHeaderLabels(sl);
    m_treeWidget->setSortingEnabled(true);
    
    mainFrameLayout->addWidget(m_treeWidget);
    
    QFrame *btnBox = new QFrame(mainFrame);
    mainFrameLayout->addWidget(btnBox);
    mainFrame->setLayout(mainFrameLayout);

    btnBox->setSizePolicy(
        QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    // QT3: I don't think it's necessary to replace the following ",4, 10" with
    // anything to explicitly set the dimensions of the HBox, but there might be
    // some compatibility trickery I'm not remembering, etc.  Leaving as a
    // reminder in case the layout turns out broken:
    QHBoxLayout* layout = new QHBoxLayout(btnBox /*, 4, 10 */);

    m_addButton = new QPushButton(tr("Add"), btnBox);
    m_deleteButton = new QPushButton(tr("Delete"), btnBox);

    m_closeButton = new QPushButton(tr("Close"), btnBox);

    m_addButton->setToolTip(tr("Add a Control Parameter to the Studio"));

    m_deleteButton->setToolTip(tr("Delete a Control Parameter from the Studio"));

    m_closeButton->setToolTip(tr("Close the Control Parameter editor"));

    layout->addStretch(10);
    layout->addWidget(m_addButton);
    layout->addWidget(m_deleteButton);
    layout->addSpacing(30);

    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_addButton, &QAbstractButton::released,
            this, &ControlEditorDialog::slotAdd);

    connect(m_deleteButton, &QAbstractButton::released,
            this, &ControlEditorDialog::slotDelete);

    setupActions();

    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this,
            static_cast<void(ControlEditorDialog::*)()>(
                    &ControlEditorDialog::slotUpdate));

    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
            this, &ControlEditorDialog::slotEdit);

    // Highlight all columns - enable extended selection mode
    //
    m_treeWidget->setAllColumnsShowFocus(true);
    
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    initDialog();
    
    // Set the top item in the list, if able.
    if (m_treeWidget->topLevelItemCount()) {
        m_treeWidget->setCurrentItem(m_treeWidget->topLevelItem(0));
    }
}

ControlEditorDialog::~ControlEditorDialog()
{
    RG_DEBUG << "\n*** ControlEditorDialog::~ControlEditorDialog\n";

    // Save window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    RG_DEBUG << "[geometry] storing window geometry for ControlEditorDialog";
    settings.setValue("Control_Editor_Dialog_Geometry", saveGeometry());
    settings.setValue("Control_Editor_Dialog_State", saveState());
    settings.endGroup();
}

void
ControlEditorDialog::initDialog()
{
    RG_DEBUG << "ControlEditorDialog::initDialog";
    slotUpdate();

    // Restore window geometry and toolbar/dock state
    RG_DEBUG << "[geometry] ControlEditorDialog - Restoring saved geometry...";
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Control_Editor_Dialog_Geometry").toByteArray());
    restoreState(settings.value("Control_Editor_Dialog_State").toByteArray());
    settings.endGroup();
}

void
ControlEditorDialog::slotUpdate(bool added)
{
    RG_DEBUG << "ControlEditorDialog::slotUpdate";

    MidiDevice *md =
        dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));
    if (!md)
        return ;

    ControlList::const_iterator it = md->beginControllers();
    ControlParameterItem *item = nullptr;
    int i = 0;

    // Attempt to track last controller selected so we can reselect it
    int lastControllerId = -1;
    ControlParameterItem *lastItem = dynamic_cast<ControlParameterItem *>(m_treeWidget->currentItem());
    if (lastItem) {
        lastControllerId = lastItem->getId();
    }
    
    m_treeWidget->clear();

    for (; it != md->endControllers(); ++it) {
        Composition &comp = m_doc->getComposition();

        QString colour =
            strtoqstr(comp.getGeneralColourMap().getName(it->getColourIndex()));

        if (colour == "")
            colour = tr("<default>");

        QString position = QString("%1").arg(it->getIPBPosition());
        if (position.toInt() == -1)
            position = tr("<not showing>");

        const QString value =
            QString::asprintf("%d (0x%x)", it->getControllerNumber(),
                      it->getControllerNumber());

        if (it->getType() == PitchBend::EventType) {
            item = new ControlParameterItem(
                                            i++,
                                            m_treeWidget,
                                            QStringList()
                                                << strtoqstr(it->getName())
                                                << strtoqstr(it->getType())
                                                << QString("-")
                                                << strtoqstr(it->getDescription())
                                                << QString("%1").arg(it->getMin())
                                                << QString("%1").arg(it->getMax())
                                                << QString("%1").arg(it->getDefault())
                                                << colour
                                                << position 
                                          );
        } else {
            item = new ControlParameterItem(
                            i++,
                            m_treeWidget,
                            QStringList()
                                << strtoqstr(it->getName())
                                << strtoqstr(it->getType())
                                << value
                                << strtoqstr(it->getDescription())
                                << QString("%1").arg(it->getMin())
                                << QString("%1").arg(it->getMax())
                                << QString("%1").arg(it->getDefault())
                                << colour
                                << position
                           );
        }

        if (item->getId() == lastControllerId) {
            m_treeWidget->setCurrentItem(item);
        }


        // create and set a colour pixmap
        //
        QPixmap colourPixmap(16, 16);
        QColor c = comp.getGeneralColourMap().getColour(it->getColourIndex());
        colourPixmap.fill(QColor(c.red(), c.green(), c.blue()));
        
        item->setIcon(7, QIcon(colourPixmap));

        m_treeWidget->addTopLevelItem(item);
    }

    if(m_treeWidget->topLevelItemCount() == 0) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_treeWidget, QStringList(tr("<none>")));
        m_treeWidget->addTopLevelItem(item);

        m_treeWidget->setSelectionMode(QAbstractItemView::NoSelection);
    } else {
        m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
    
    // This logic is kind of frigged up, and may be too fragile.  It assumes
    // that if you added an item, the last thing iterated through will be that
    // new item, so the value of the variable item will be the last thing
    // iterated through, and therefore the newest item you just added and want
    // to edit.
    //
    // I got substantially far along the way to making this quick and dirty hack
    // work before I thought it would be a lot cleaner to have the
    // AddControlParameterCommand itself launch the dialog and allow the user to
    // edit the parameters before ever adding it to the list at all.  That would
    // be a lot cleaner, but it would also require going against the flow of how
    // this logic always worked, so it would require a lot more thought to
    // achieve the same end result that way.  Instead, I just used this hack
    // overloaded slotUpdate() to tell it when a new controller was added, so we
    // could grab it here and launch the dialog on the generic blah we just added
    // to the list right before this slot got called with the optional bool set
    // true.
    //
    // (so much for verbose comments being helpful...  I wrote that not too long
    // ago, and reading it now, I have NO fscking idea what I was talking about)
    //
    if (added) {
        RG_DEBUG << "ControlEditorDialog: detected new item entered; launching editor";
        m_treeWidget->setCurrentItem(item);
        slotEdit(item, 0);
    }
}

void
ControlEditorDialog::slotAdd()
{
    RG_DEBUG << "ControlEditorDialog::slotAdd to device " << m_device;

    AddControlParameterCommand *command =
        new AddControlParameterCommand(m_studio, m_device,
                                       ControlParameter());

    addCommandToHistory(command);
    slotUpdate(true);
}

void
ControlEditorDialog::slotDelete()
{
    RG_DEBUG << "ControlEditorDialog::slotDelete";

    if(! m_treeWidget->currentItem())
        return ;

    ControlParameterItem *item =
        dynamic_cast<ControlParameterItem*>(m_treeWidget->currentItem());

    if (item) {
        RemoveControlParameterCommand *command =
            new RemoveControlParameterCommand(m_studio, m_device, item->getId());

        addCommandToHistory(command);
    }
}

void
ControlEditorDialog::slotClose()
{
    RG_DEBUG << "ControlEditorDialog::slotClose";

    m_doc = nullptr;

    close();
}

void
ControlEditorDialog::setupActions()
{
    createAction("file_close", SLOT(slotClose()));
    m_closeButton->setText(tr("Close"));
    connect(m_closeButton, &QAbstractButton::released, this, &ControlEditorDialog::slotClose);
    createAction("remove_all_from_ip", SLOT(slotRemoveAllFromInstrumentPanel()));
    createAction("control_help", SLOT(slotHelpRequested()));
    createAction("help_about_app", SLOT(slotHelpAbout()));

    createMenusAndToolbars("controleditor.rc");
}

void
ControlEditorDialog::addCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
    setModified(false);
}

void
ControlEditorDialog::setModified(bool modified)
{
    RG_DEBUG << "setModified(" << modified << ")";

    m_modified = modified;
}

void
ControlEditorDialog::slotEdit(QTreeWidgetItem *i, int)
{
    RG_DEBUG << "ControlEditorDialog::slotEdit";

    ControlParameterItem *item =
        dynamic_cast<ControlParameterItem*>(i);

    MidiDevice *md =
        dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));

    if (item && md) {
        ControlParameterEditDialog dialog(
                this,  // parent
                md->getControlParameter(item->getId()),  // control
                m_doc);  // doc

        if (dialog.exec() == QDialog::Accepted) {
            ModifyControlParameterCommand *command =
                new ModifyControlParameterCommand(m_studio,
                                                  m_device,
                                                  dialog.getControl(),
                                                  item->getId());

            addCommandToHistory(command);
        }
    }
}

void
ControlEditorDialog::closeEvent(QCloseEvent*)
{
    emit closing();
    close();
}

void
ControlEditorDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:controlEditorDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:controlEditorDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
ControlEditorDialog::slotHelpAbout()
{
    new AboutDialog(this);
}

void ControlEditorDialog::slotRemoveAllFromInstrumentPanel()
{
    MacroCommand *macroCommand =
            new MacroCommand("Remove All Controllers From Instrument Panel");

    MidiDevice *md = dynamic_cast<MidiDevice *>(m_studio->getDevice(m_device));
    if (!md)
        return;

    const ControlList &controlList = md->getControlParameters();

    // for each controller
    for (size_t controllerIndex = 0;
         controllerIndex < controlList.size();
         ++controllerIndex) {

        // If the controller is already not showing, try the next.
        if (controlList[controllerIndex].getIPBPosition() == -1)
            continue;

        // Controller is showing.  Remove it.

        // Make a copy to modify.
        ControlParameter controlParameter = controlList[controllerIndex];
        controlParameter.setIPBPosition(-1);

        // Create a ModifyControlParameterCommand with ippos set to -1.
        ModifyControlParameterCommand *command =
                new ModifyControlParameterCommand(m_studio,  // studio
                                                  m_device,  // device
                                                  controlParameter,  // control
                                                  controllerIndex);  // id

        // Add the command to the macro command.
        macroCommand->addCommand(command);
    }

    // If the macro command has more than 0 entries, send it off.
    if (macroCommand->hasCommands())
        addCommandToHistory(macroCommand);
    else
        delete macroCommand;
}


}
