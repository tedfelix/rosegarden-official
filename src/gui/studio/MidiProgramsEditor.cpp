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

#define RG_MODULE_STRING "[MidiProgramsEditor]"
#define RG_NO_DEBUG_PRINT

#include "MidiProgramsEditor.h"

#include "MidiBankTreeWidgetItem.h"
#include "BankEditorDialog.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/MidiDevice.h"
#include "gui/widgets/LineEdit.h"
#include "gui/general/IconLoader.h"
#include "gui/dialogs/SelectBankDialog.h"
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/CommandHistory.h"

#include <QCheckBox>
#include <QCursor>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QIcon>
#include <QPoint>
#include <QPushButton>
#include <QMenu>
#include <QSpinBox>
#include <QString>
#include <QToolButton>

#include <algorithm>  // std::sort
#include <string>


namespace Rosegarden
{


// Load once for performance.
static const QIcon &getNoKeyMapIcon()
{
    // Use a white icon to indicate there is no keymap for this program.
    static const QIcon noKeyMapIcon(IconLoader::loadPixmap("key-white"));

    return noKeyMapIcon;
}

// Load once for performance.
static const QIcon &getKeyMapIcon()
{
    // Use a green icon to indicate there *is* a keymap for this program.
    static const QIcon keyMapIcon(IconLoader::loadPixmap("key-green"));

    return keyMapIcon;
}

static const QString defaultTitle{QObject::tr("Bank and Program details")};


MidiProgramsEditor::MidiProgramsEditor(BankEditorDialog *bankEditor,
                                       QWidget *parent) :
    NameSetEditor(bankEditor,
                  defaultTitle,  // title
                  parent,
                  true)  // showKeyMapButtons
{
    QWidget *topWidget = new QWidget(m_topFrame);
    topWidget->setContentsMargins(0, 0, 0, 0);
    m_topLayout->addWidget(topWidget, 0, 0, 3, 3);

    QGridLayout *gridLayout = new QGridLayout(topWidget);
    gridLayout->setSpacing(0);

    int row{0};

    // Percussion
    gridLayout->addWidget(new QLabel(tr("Percussion"), topWidget), row, 0);
    m_percussion = new QLabel(topWidget);
    m_percussion->setAlignment(Qt::AlignHCenter);
    gridLayout->addWidget(m_percussion, row, 1);

    ++row;

    // MSB Value
    gridLayout->addWidget(new QLabel(tr("MSB Value"), topWidget), row, 0);
    m_msb = new QLabel(topWidget);
    m_msb->setAlignment(Qt::AlignHCenter);
    gridLayout->addWidget(m_msb, row, 1);

    ++row;

    // LSB Value
    gridLayout->addWidget(new QLabel(tr("LSB Value"), topWidget), row, 0);
    m_lsb = new QLabel(topWidget);
    m_lsb->setAlignment(Qt::AlignHCenter);
    gridLayout->addWidget(m_lsb, row, 1);

    ++row;

    // Spacer
    gridLayout->setRowMinimumHeight(row, 10);

    ++row;

    // Edit button
    QPushButton *editBank = new QPushButton(tr("Edit"), topWidget);
    connect(editBank, &QPushButton::clicked,
            this, &MidiProgramsEditor::slotBankEditClicked);
    gridLayout->addWidget(editBank, row, 0, 1, 2);
}

void MidiProgramsEditor::changeBank(ProgramList &programList,
                                    const MidiBank &oldBank,
                                    const MidiBank &newBank)
{
    // For each program in programList...
    for (MidiProgram &program : programList) {
        // If this one is in the old bank, update it to the new.
        if (program.getBank().compareKey(oldBank)) {
            program = MidiProgram(newBank,
                                  program.getProgram(),
                                  program.getName());
        }
    }
}

void
MidiProgramsEditor::clearAll()
{
    RG_DEBUG << "clearAll";

    NameSetEditor::clearAll();

    setTitle(defaultTitle);

    m_percussion->setText(tr("no"));
    m_msb->setText("0");
    m_lsb->setText("0");

    m_currentBank = MidiBank();

    setEnabled(false);
}

void
MidiProgramsEditor::populate(const MidiBankTreeWidgetItem *bankItem)
{
    RG_DEBUG << "populate()";
    RG_DEBUG << "  bankItem->getBank = " << bankItem->getBank();

    MidiDevice *device = bankItem->getDevice();
    if (!device)
        return;

    m_device = device;

    const BankList &bankList = device->getBanks();

    m_currentBank = bankList[bankItem->getBank()];

    setEnabled(true);

    setTitle(bankItem->text(0));

    // Percussion
    m_percussion->setText(m_currentBank.isPercussion() ? tr("yes") : tr("no"));

    // MSB Value
    m_msb->setText(QString::number(m_currentBank.getMSB()));

    // LSB Value
    m_lsb->setText(QString::number(m_currentBank.getLSB()));

    // Provided By
    m_librarian->setText(strtoqstr(m_device->getLibrarianName()));
    m_librarianEmail->setText(strtoqstr(m_device->getLibrarianEmail()));

    // Program List

    const bool haveKeyMappings = (m_device->getKeyMappings().size() > 0);
    const ProgramList &programList = device->getPrograms();

    // For each name field (LineEdit) on the UI...
    // programIndex is also the program change number.
    for (size_t programIndex = 0;
         programIndex < m_names.size();
         ++programIndex) {

        QToolButton *keyMapButton = getKeyMapButton(programIndex);
        keyMapButton->setMaximumHeight(12);
        // No sense enabling the button if there is nothing to select.
        keyMapButton->setEnabled(haveKeyMappings);

        bool found = false;
        MidiProgram foundProgram;

        // Find the program
        for (const MidiProgram &midiProgram : programList) {
            // Not it?  Try the next.
            if (!m_currentBank.compareKey(midiProgram.getBank()))
                continue;

            // Found?  We're done.
            if (midiProgram.getProgram() == programIndex) {
                found = true;
                foundProgram = midiProgram;
                break;
            }
        }

        // If not found, clear and continue.
        if (!found) {
            m_names[programIndex]->clear();
            keyMapButton->setIcon(getNoKeyMapIcon());
            keyMapButton->setToolTip("");
            continue;
        }

        // Found it.

        // Name

        QString programName = strtoqstr(foundProgram.getName());
        m_names[programIndex]->setText(programName);
        // Show start of label.
        m_names[programIndex]->setCursorPosition(0);

        // Key map icon and ToolTip

        const MidiKeyMapping *midiKeyMapping =
                m_device->getKeyMappingForProgram(foundProgram);
        if (midiKeyMapping) {
            // Indicate that this program has a keymap.
            keyMapButton->setIcon(getKeyMapIcon());
            // Put the name in the tool tip.
            keyMapButton->setToolTip(tr("Key Mapping: %1").arg(
                    strtoqstr(midiKeyMapping->getName())));
        } else {  // No key mapping.
            // Indicate that this program has no keymap.
            keyMapButton->setIcon(getNoKeyMapIcon());
            keyMapButton->setToolTip("");
        }
    }
}

void MidiProgramsEditor::slotEditingFinished()
{
    RG_DEBUG << "slotEditingFinished()";

    const LineEdit *lineEdit = dynamic_cast<const LineEdit *>(sender());

    if (!lineEdit) {
        RG_WARNING << "slotEditingFinished(): WARNING: Signal sender is not a LineEdit.";
        return;
    }

    const QString programName = lineEdit->text();

    // Zero-based program number.  This is the same as the MIDI program change.
    const unsigned programNumber = lineEdit->property("index").toUInt();

    RG_DEBUG << "slotEditingFinished(): " << programName << programNumber;

    // Make a copy so we can change the name.
    ProgramList newProgramList = m_device->getPrograms();

    ProgramList::iterator programIter =
            findProgramIter(newProgramList, m_currentBank, programNumber);

    // If the MidiProgram doesn't exist, add it.
    if (programIter == newProgramList.end()) {
        // If the program name is empty, do nothing.
        if (programName.isEmpty())
            return;

        // Create a new MidiProgram and add it to newProgramList.
        MidiProgram newProgram(m_currentBank, programNumber);
        newProgram.setName(qstrtostr(programName));
        newProgramList.push_back(newProgram);

        // Sort newProgramList.
        // We need to sort this for the MIPP.  It just needs the PCs in
        // order.  That's how it displays them.
        // If we do not sort this, the .rg file is also mixed up.  But
        // that's not a serious issue since no one looks at that.
        // ??? Is there a better place to sort this?  What if we take
        //     the first steps toward moving to std::set?  Make it a
        //     std::set, but continue using it like a vector.  That
        //     way it is always sorted.
        std::sort(newProgramList.begin(),
                  newProgramList.end(),
                  [](const MidiProgram &lhs, const MidiProgram &rhs){ return lhs.lessKey(rhs); });

        // Get the new MidiProgram
        programIter = findProgramIter(newProgramList,
                                      m_currentBank,
                                      programNumber);

    } else {  // The MidiProgram already exists in newProgramList.

        // If the name hasn't changed, bail.
        if (qstrtostr(programName) == programIter->getName())
            return;

        // If the label is now empty...
        if (programName.isEmpty()) {
            //RG_DEBUG << "slotEditingFinished(): deleting empty program (" << programNumber << ")";
            // Delete the program.
            newProgramList.erase(programIter);
        } else {  // Rename the program.
            programIter->setName(qstrtostr(programName));
        }
    }

    // Modify the Device.

    ModifyDeviceCommand *command =
        m_bankEditor->makeCommand(tr("program changed"));
    command->setProgramList(newProgramList);
    CommandHistory::getInstance()->addCommand(command);

}

void
MidiProgramsEditor::slotKeyMapButtonPressed()
{
    if (!m_device)
        return;

    QToolButton *button = dynamic_cast<QToolButton *>(sender());
    if (!button) {
        RG_WARNING << "slotKeyMapButtonPressed(): WARNING: Sender is not a QPushButton.";
        return;
    }

    const unsigned programChange = button->property("index").toUInt();

    // Save the program number we are editing for slotKeyMapMenuItemSelected().
    m_keyMapProgramNumber = programChange;

    // Create a pop-up menu filled with the available key mappings.

    QMenu *menu = new QMenu(button);

    // Add the initial "<no key mapping>".
    QAction *action = menu->addAction(tr("<no key mapping>"));
    action->setObjectName("-1");

    const MidiProgram *program = findProgram(
            m_device->getPrograms(), m_currentBank, programChange);
    if (!program)
        return;

    const KeyMappingList &keyMappingList = m_device->getKeyMappings();
    if (keyMappingList.empty())
        return;

    // To track down current key map index for menu positioning.
    const std::string &currentKeyMapName = program->getKeyMapping();
    // Keep track of the current key map selection for
    // popup menu positioning.
    int currentKeyMapIndex = -1;

    // For each key mapping...
    for (size_t keyMapIndex = 0;
         keyMapIndex < keyMappingList.size();
         ++keyMapIndex) {
        // Add the mapping to the menu.
        action = menu->addAction(strtoqstr(keyMappingList[keyMapIndex].getName()));
        action->setObjectName(QString("%1").arg(keyMapIndex));

        // If the current keymap for this program is found, keep track of it.
        if (keyMappingList[keyMapIndex].getName() == currentKeyMapName)
            currentKeyMapIndex = static_cast<int>(keyMapIndex);
    }

    connect(menu, &QMenu::triggered,
            this, &MidiProgramsEditor::slotKeyMapMenuItemSelected);

    // Compute the position for the pop-up menu.

    // Make sure the menu will be positioned such that the mouse pointer
    // is over the currently selected item.

    QRect actionRect =
            menu->actionGeometry(menu->actions().value(currentKeyMapIndex + 1));
    QPoint menuPos = QCursor::pos();
    // Adjust position so that the mouse will end up on top of
    // the current selection.
    menuPos.rx() -= 10;
    menuPos.ry() -= actionRect.top() + actionRect.height() / 2;

    // Display the menu.
    menu->popup(menuPos);

    // slotKeyMapMenuItemSelected() is the next step in this process.
}

void
MidiProgramsEditor::slotKeyMapMenuItemSelected(QAction *action)
{
    // The user has selected an item from the menu presented
    // by slotKeyMapButtonPressed().

    if (!m_device)
        return;

    const KeyMappingList &keyMappingList = m_device->getKeyMappings();
    if (keyMappingList.empty())
        return;

    // Make a copy so we can modify it.
    ProgramList newProgramList = m_device->getPrograms();

    MidiProgram *program =
            findProgram(newProgramList, m_currentBank, m_keyMapProgramNumber);
    if (!program)
        return;

    // Extract the key map number from the object name.
    const int keyMapIndex = action->objectName().toInt();

    std::string newMapping;

    // No key mapping?
    if (keyMapIndex <= -1) {
        newMapping = "";
    } else {
        if (keyMapIndex < static_cast<int>(keyMappingList.size()))
            newMapping = keyMappingList[keyMapIndex].getName();
    }

    program->setKeyMapping(newMapping);

    // Modify the Device.

    ModifyDeviceCommand *command =
        m_bankEditor->makeCommand(tr("change key mapping"));
    command->setProgramList(newProgramList);
    CommandHistory::getInstance()->addCommand(command);
}

const MidiProgram *
MidiProgramsEditor::findProgram(const ProgramList &programList,
                                const MidiBank &bank,
                                int programNo)
{
    for (const MidiProgram &midiProgram : programList) {
        // Match?
        if (midiProgram.getBank().compareKey(bank)  &&
            midiProgram.getProgram() == programNo) {
            return &midiProgram;
        }
    }

    return nullptr;
}

MidiProgram *
MidiProgramsEditor::findProgram(ProgramList &programList,
                                const MidiBank &bank,
                                int programNo)
{
    for (MidiProgram &midiProgram : programList) {
        // Match?
        if (midiProgram.getBank().compareKey(bank)  &&
            midiProgram.getProgram() == programNo) {
            return &midiProgram;
        }
    }

    return nullptr;
}

ProgramList::iterator
MidiProgramsEditor::findProgramIter(ProgramList &programList,
                                   const MidiBank &bank,
                                   int programNo)
{
    // For each program in the programList...
    for (ProgramList::iterator programIter = programList.begin();
         programIter != programList.end();
         ++programIter) {
        // Match?
        if (programIter->getBank().compareKey(bank) &&
            programIter->getProgram() == programNo)
            return programIter;
    }

    return programList.end();
}

void MidiProgramsEditor::slotBankEditClicked(bool /*checked*/)
{
    SelectBankDialog selectBankDialog(
            this,
            m_device->getBanks(),
            m_currentBank,
            true);  // allowOriginal
    if (selectBankDialog.exec() == QDialog::Rejected)
        return;

    const MidiBank newBank = selectBankDialog.getBank();
    // No change?  Bail.
    if (newBank.compareKey(m_currentBank))
        return;

    // Create a new program list with the new LSB.

    ProgramList newProgramList = m_device->getPrograms();
    changeBank(newProgramList, m_currentBank, newBank);

    // Create a new bank list with the new LSB.

    const BankList &oldBankList = m_device->getBanks();
    BankList newBankList;
    for (const MidiBank &oldMidiBank : oldBankList) {
        // If this is the one we want to change, add the new bank.
        if (oldMidiBank == m_currentBank)
            newBankList.push_back(newBank);
        else  // Go with the old bank.
            newBankList.push_back(oldMidiBank);
    }

    // Modify the Device.

    ModifyDeviceCommand *command =
            m_bankEditor->makeCommand(tr("change bank"));
    command->setBankList(newBankList);
    command->setProgramList(newProgramList);
    CommandHistory::getInstance()->addCommand(command);

    // Update the current bank.
    m_currentBank = newBank;
}


}
