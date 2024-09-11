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
#include "commands/studio/ModifyDeviceCommand.h"
#include "document/CommandHistory.h"

#include <QCheckBox>
#include <QCursor>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QIcon>
#include <QPoint>
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


MidiProgramsEditor::MidiProgramsEditor(BankEditorDialog *bankEditor,
                                       QWidget *parent) :
    NameSetEditor(bankEditor,
                  tr("Bank and Program details"),  // title
                  parent,
                  true)  // showKeyMapButtons
{
    QWidget *topWidget = new QWidget(m_topFrame);
    topWidget->setContentsMargins(0, 0, 0, 0);

    QGridLayout *gridLayout = new QGridLayout(topWidget);
    gridLayout->setSpacing(0);

    // Percussion
    gridLayout->addWidget(new QLabel(tr("Percussion"), topWidget),
                          0, 0, Qt::AlignLeft);
    m_percussion = new QCheckBox(topWidget);
    connect(m_percussion, &QAbstractButton::clicked,
            this, &MidiProgramsEditor::slotPercussionClicked);
    gridLayout->addWidget(m_percussion, 0, 1, Qt::AlignLeft);

    // MSB Value
    gridLayout->addWidget(new QLabel(tr("MSB Value"), topWidget),
                          1, 0, Qt::AlignLeft);
    m_msb = new QSpinBox(topWidget);
    m_msb->setToolTip(tr("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));
    m_msb->setMinimum(0);
    m_msb->setMaximum(127);
    connect(m_msb,
                //QOverload<int>::of(&QSpinBox::valueChanged),  // Qt5.7+
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MidiProgramsEditor::slotNewMSB);
    gridLayout->addWidget(m_msb, 1, 1, Qt::AlignLeft);

    // LSB Value
    gridLayout->addWidget(new QLabel(tr("LSB Value"), topWidget),
                          2, 0, Qt::AlignLeft);
    m_lsb = new QSpinBox(topWidget);
    m_lsb->setToolTip(tr("Selects a LSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));
    m_lsb->setMinimum(0);
    m_lsb->setMaximum(127);
    connect(m_lsb,
                //QOverload<int>::of(&QSpinBox::valueChanged),  // Qt5.7+
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MidiProgramsEditor::slotNewLSB);
    gridLayout->addWidget(m_lsb, 2, 1, Qt::AlignLeft);

    m_topLayout->addWidget(topWidget, 0, 0, 3, 3);
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

    // ??? This string is duplicated in the ctor.  Why?
    setTitle(tr("Bank and Program details"));

    m_percussion->setChecked(false);

    // setValue() emits valueChanged().
    m_msb->blockSignals(true);
    m_msb->setValue(0);
    m_msb->blockSignals(false);

    // setValue() emits valueChanged().
    m_lsb->blockSignals(true);
    m_lsb->setValue(0);
    m_lsb->blockSignals(false);

    m_currentBank = MidiBank();

    m_librarian->clear();
    m_librarianEmail->clear();

    // Clear all name fields.
    // ??? Seems like the baseclass (NameSetEditor) should offer this.
    //     clearNames().  Or maybe an override-able clearAll()?
    for (size_t i = 0; i < m_names.size(); ++i)
        m_names[i]->clear();

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
    m_percussion->setChecked(m_currentBank.isPercussion());

    // MSB Value
    // setValue() emits valueChanged().
    m_msb->blockSignals(true);
    m_msb->setValue(m_currentBank.getMSB());
    m_msb->blockSignals(false);

    // LSB Value
    // setValue() emits valueChanged().
    m_lsb->blockSignals(true);
    m_lsb->setValue(m_currentBank.getLSB());
    m_lsb->blockSignals(false);

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

void
MidiProgramsEditor::slotPercussionClicked()
{
    RG_DEBUG << "slotPercussionClicked()";

    // Create a new bank.

    bool isPercussion = m_percussion->isChecked();
    MidiByte msb = m_currentBank.getMSB();
    MidiByte lsb = m_currentBank.getLSB();

    makeUnique(isPercussion, msb, lsb);

    MidiBank newBank(isPercussion,
                     msb,
                     lsb,
                     m_currentBank.getName());

    // Create a new program list with the new percussion setting.

    ProgramList newProgramList = m_device->getPrograms();
    changeBank(newProgramList, m_currentBank, newBank);

    // Create a new bank list with the new percussion setting.

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
        m_bankEditor->makeCommand(tr("toggle bank percussion"));
    command->setBankList(newBankList);
    command->setProgramList(newProgramList);
    CommandHistory::getInstance()->addCommand(command);

    // and update the current bank
    m_currentBank = newBank;
}

void
MidiProgramsEditor::slotNewMSB(int value)
{
    RG_DEBUG << "slotNewMSB(" << value << ")";

    // Create a new bank.

    bool isPercussion = m_currentBank.isPercussion();
    MidiByte msb = value;
    MidiByte lsb = m_currentBank.getLSB();

    // ??? Changing the LSB is not what the user is expecting.
    //     They are changing the MSB.
    makeUnique(isPercussion, msb, lsb,
               true);  // preferLSBChange

    MidiBank newBank(isPercussion,
                     msb,
                     lsb,
                     m_currentBank.getName());

    // Create a new program list with the new MSB.

    ProgramList newProgramList = m_device->getPrograms();
    changeBank(newProgramList, m_currentBank, newBank);

    // Create a new bank list with the new MSB.

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
        m_bankEditor->makeCommand(tr("bank set msb"));
    command->setBankList(newBankList);
    command->setProgramList(newProgramList);
    CommandHistory::getInstance()->addCommand(command);

    // and update the current bank
    m_currentBank = newBank;
}

void
MidiProgramsEditor::slotNewLSB(int value)
{
    RG_DEBUG << "slotNewLSB(" << value << ")";

    // Create a new bank.

    bool isPercussion = m_currentBank.isPercussion();
    MidiByte msb = m_currentBank.getMSB();
    MidiByte lsb = value;

    // ??? Changing the MSB is not what the user is expecting.
    //     They are changing the LSB.
    makeUnique(isPercussion, msb, lsb,
               false);  // preferLSBChange

    MidiBank newBank(isPercussion,
                     msb,
                     lsb,
                     m_currentBank.getName());

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
        m_bankEditor->makeCommand(tr("bank set lsb"));
    command->setBankList(newBankList);
    command->setProgramList(newProgramList);
    CommandHistory::getInstance()->addCommand(command);

    // and update the current bank
    m_currentBank = newBank;
}

void MidiProgramsEditor::slotEditingFinished()
{
    RG_DEBUG << "slotEditingFinished";

    const LineEdit *lineEdit = dynamic_cast<const LineEdit *>(sender());

    if (!lineEdit) {
        RG_WARNING << "slotEditingFinished(): WARNING: Signal sender is not a LineEdit.";
        return;
    }

    QString programName = lineEdit->text();

    // Zero-based program number.  This is the same as the MIDI program change.
    const unsigned programNumber = lineEdit->property("index").toUInt();

    RG_DEBUG << "slotEditingFinished" << programName << programNumber;

    // Get the MidiProgram that needs to be changed.
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

        // If the label is now empty...
        if (programName.isEmpty()) {
            //RG_DEBUG << "slotEditingFinished(): deleting empty program (" << programNumber << ")";
            newProgramList.erase(programIter);
        }
    }

    // Has the name actually changed
    if (qstrtostr(programName) == programIter->getName()) return;

    programIter->setName(qstrtostr(programName));

    // now make the change
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

    const unsigned id = button->property("index").toUInt();
    const ProgramList& programList = m_device->getPrograms();

    const MidiProgram *program = findProgram(programList, m_currentBank, id);
    if (!program)
        return;

    const KeyMappingList &keyMappingList = m_device->getKeyMappings();
    if (keyMappingList.empty())
        return;

    // Save the program number we are editing for slotKeyMapMenuItemSelected().
    m_keyMapProgramNumber = id;

    // Create a pop-up menu filled with the available key mappings.

    QMenu *menu = new QMenu(button);

    const MidiKeyMapping *currentMapping =
            m_device->getKeyMappingForProgram(*program);

    // Keep track of the current key map selection for
    // popup menu positioning.
    int currentKeyMap = 0;

    // Add the initial "<no key mapping>".
    QAction *action = menu->addAction(tr("<no key mapping>"));
    action->setObjectName("0");

    // For each key mapping...
    for (size_t i = 0; i < keyMappingList.size(); ++i) {
        // Add the mapping to the menu.
        action = menu->addAction(strtoqstr(keyMappingList[i].getName()));
        action->setObjectName(QString("%1").arg(i+1));

        // If the current keymap for this program is found, keep track of it.
        if (currentMapping  &&  (keyMappingList[i] == *currentMapping))
            currentKeyMap = static_cast<int>(i + 1);
    }

    connect(menu, &QMenu::triggered,
            this, &MidiProgramsEditor::slotKeyMapMenuItemSelected);

    // Compute the position for the pop-up menu.

    // Make sure the menu will be positioned such that the mouse pointer
    // is over the currently selected item.

    QRect actionRect =
            menu->actionGeometry(menu->actions().value(currentKeyMap));
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

    const ProgramList& programList = m_device->getPrograms();
    const MidiProgram *program =
        findProgram(programList, m_currentBank, m_keyMapProgramNumber);
    if (!program)
        return;

    // Extract the key map number from the object name.
    // Subtract one to convert from 1-based key map number to 0-based.
    // Simplifies keyMappingList[] vector access.
    const int keyMapNumber = action->objectName().toInt() - 1;

    std::string newMapping;

    // No key mapping?
    if (keyMapNumber <= -1) {
        newMapping = "";
    } else {
        if (keyMapNumber < static_cast<int>(keyMappingList.size()))
            newMapping = keyMappingList[keyMapNumber].getName();
    }

    // Set the key mapping.
    // ??? EDIT!  Where's the command so we can undo?
    // ??? Only the name is used?  Then we need to disallow key mappings
    //     with empty names.  BankEditorDialog currently allows this.
    m_device->setKeyMappingForProgram(*program, newMapping);

    // Update the key mapping icon.

    bool haveKeyMappings = (m_device->getKeyMappings().size() > 0);
    QToolButton *keyMapButton = getKeyMapButton(m_keyMapProgramNumber);

    // <no key mapping> selected?
    if (keyMapNumber == -1) {
        keyMapButton->setIcon(getNoKeyMapIcon());
        keyMapButton->setToolTip("");
    } else {
        keyMapButton->setIcon(getKeyMapIcon());
        keyMapButton->setToolTip(tr("Key Mapping: %1").arg(strtoqstr(newMapping)));
    }
    keyMapButton->setEnabled(haveKeyMappings);
}

const MidiProgram *
MidiProgramsEditor::findProgram(const ProgramList& programList,
                               const MidiBank &bank,
                               int programNo)
{
    for (const MidiProgram &midiProgram : programList) {
        // Match?
        if (midiProgram.getBank().compareKey(bank) &&
            midiProgram.getProgram() == programNo) {
            return &midiProgram;
        }
    }

    return nullptr;
}

ProgramList::iterator
MidiProgramsEditor::findProgramIter(ProgramList& programList,
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

void MidiProgramsEditor::makeUnique(
        bool &isPercussion, MidiByte &msb, MidiByte &lsb, bool preferLSBChange)
{
    RG_DEBUG << "makeUnique" << isPercussion << msb << lsb;
    // The combination of the three variables must be unique. This
    // routine should only be called when a change is made to one of
    // the three variables so it does no harm to compare with the
    // actual bank

    const BankList& banks = m_device->getBanks();
    // first check if the variables are already unique
    bool unique = true;
    for (size_t i = 0; i < banks.size(); ++i) {
        if (banks[i].isPercussion() == isPercussion &&
            banks[i].getMSB() == msb &&
            banks[i].getLSB() == lsb) {
            unique = false;
            break;
        }
    }
    if (unique) return;
    // try all lsbs
    if (preferLSBChange) {
        for (MidiByte newLsb = MidiMinValue; newLsb<MidiMaxValue; newLsb++) {
            bool unique = true;
            for (size_t i = 0; i < banks.size(); ++i) {
                if (banks[i].isPercussion() == isPercussion &&
                    banks[i].getMSB() == msb &&
                    banks[i].getLSB() == newLsb) {
                    unique = false;
                    break;
                }
            }
            if (unique) {
                lsb = newLsb;
                RG_DEBUG << "makeUniqe changing to" <<
                    isPercussion << msb << lsb;
                return;
            }
        }
    }
    // try all msbs
    for (MidiByte newMsb = MidiMinValue; newMsb<MidiMaxValue; newMsb++) {
        bool unique = true;
        for (size_t i = 0; i < banks.size(); ++i) {
            if (banks[i].isPercussion() == isPercussion &&
                banks[i].getMSB() == newMsb &&
                banks[i].getLSB() == lsb) {
            unique = false;
            break;
            }
        }
        if (unique) {
            msb = newMsb;
            RG_DEBUG << "makeUniqe changing to" <<
                isPercussion << msb << lsb;
            return;
        }
    }
    if (preferLSBChange) return;
    for (MidiByte newLsb = MidiMinValue; newLsb<MidiMaxValue; newLsb++) {
        bool unique = true;
        for (size_t i = 0; i < banks.size(); ++i) {
            if (banks[i].isPercussion() == isPercussion &&
                banks[i].getMSB() == msb &&
                banks[i].getLSB() == newLsb) {
                unique = false;
                break;
            }
        }
        if (unique) {
            lsb = newLsb;
            RG_DEBUG << "makeUniqe changing to" <<
                isPercussion << msb << lsb;
            return;
        }
    }
    RG_DEBUG << "makeUnique giving up";
}

}
