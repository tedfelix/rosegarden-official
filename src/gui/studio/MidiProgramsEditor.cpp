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
#include "NameSetEditor.h"
#include "BankEditorDialog.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "gui/widgets/LineEdit.h"
#include "gui/general/IconLoader.h"

#include <QCheckBox>
#include <QCursor>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QIcon>
#include <QPoint>
#include <QMenu>
#include <QSpinBox>
#include <QString>
#include <QToolButton>
#include <QTreeWidgetItem>

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
                  true),  // showKeyMapButtons
    m_bankList(bankEditor->getBankList()),
    m_programList(bankEditor->getProgramList())
{
    QFrame *frame = new QFrame(m_topFrame);
    frame->setContentsMargins(0, 0, 0, 0);

    QGridLayout *gridLayout = new QGridLayout(frame);
    gridLayout->setSpacing(0);

    // Percussion
    gridLayout->addWidget(new QLabel(tr("Percussion"), frame),
                          0, 0, Qt::AlignLeft);
    m_percussion = new QCheckBox(frame);
    connect(m_percussion, &QAbstractButton::clicked,
            this, &MidiProgramsEditor::slotPercussionClicked);
    gridLayout->addWidget(m_percussion, 0, 1, Qt::AlignLeft);

    // MSB Value
    gridLayout->addWidget(new QLabel(tr("MSB Value"), frame),
                          1, 0, Qt::AlignLeft);
    m_msb = new QSpinBox(frame);
    m_msb->setToolTip(tr("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));
    m_msb->setMinimum(0);
    m_msb->setMaximum(127);
    connect(m_msb,
                //QOverload<int>::of(&QSpinBox::valueChanged),  // Qt5.7+
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MidiProgramsEditor::slotNewMSB);
    gridLayout->addWidget(m_msb, 1, 1, Qt::AlignLeft);

    // LSB Value
    gridLayout->addWidget(new QLabel(tr("LSB Value"), frame),
                          2, 0, Qt::AlignLeft);
    m_lsb = new QSpinBox(frame);
    m_lsb->setToolTip(tr("Selects a LSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));
    m_lsb->setMinimum(0);
    m_lsb->setMaximum(127);
    connect(m_lsb,
                //QOverload<int>::of(&QSpinBox::valueChanged),  // Qt5.7+
                static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MidiProgramsEditor::slotNewLSB);
    gridLayout->addWidget(m_lsb, 2, 1, Qt::AlignLeft);

    m_topLayout->addWidget(frame, 0, 0, 3, 3);
}

ProgramList
MidiProgramsEditor::getBankSubset(const MidiBank &bank)
{
    ProgramList programList;

    // For each program, copy the ones for the requested bank to programList.
    for (const MidiProgram &program : m_programList) {
        if (program.getBank().compareKey(bank))
            programList.push_back(program);
    }

    return programList;
}

void
MidiProgramsEditor::modifyCurrentPrograms(
        const MidiBank &oldBank, const MidiBank &newBank)
{
    // For each program in m_programList...
    for (MidiProgram &program : m_programList) {
        // If this one is in the old bank, update it to the new.
        if (program.getBank().compareKey(oldBank))
            program = MidiProgram(
                    newBank, program.getProgram(), program.getName());
    }
}

void
MidiProgramsEditor::clearAll()
{
    setTitle(tr("Bank and Program details"));

    m_percussion->setChecked(false);
    m_msb->setValue(0);
    m_lsb->setValue(0);
    m_currentBank = nullptr;

    m_librarian->clear();
    m_librarianEmail->clear();

    for (size_t i = 0; i < m_names.size(); ++i)
        m_names[i]->clear();

    setEnabled(false);
}

void
MidiProgramsEditor::populate(QTreeWidgetItem *item)
{
    RG_DEBUG << "populate()";

    MidiBankTreeWidgetItem *bankItem =
            dynamic_cast<MidiBankTreeWidgetItem*>(item);
    if (!bankItem) {
        RG_DEBUG << "populate(): not a bank item - returning";
        return;
    }

    RG_DEBUG << "populate() : bankItem->getBank = " << bankItem->getBank();

    //m_currentBank = m_device->getBankByIndex(bankItem->getBank());
    m_currentBank = &(m_bankList[bankItem->getBank()]);
    m_oldBank = *m_currentBank;

    DeviceId deviceId = bankItem->getDeviceId();
    m_device = m_bankEditor->getMidiDevice(deviceId);
    if (!m_device)
        return;

    setEnabled(true);

    setTitle(bankItem->text(0));

    // Percussion
    m_percussion->setChecked(m_currentBank->isPercussion());

    // MSB Value
    m_msb->setValue(m_currentBank->getMSB());

    // LSB Value
    m_lsb->setValue(m_currentBank->getLSB());

    // Provided By
    m_librarian->setText(strtoqstr(m_device->getLibrarianName()));
    m_librarianEmail->setText(strtoqstr(m_device->getLibrarianEmail()));

    // Program List

    // Get the programs for the current bank.
    ProgramList programSubset = getBankSubset(*m_currentBank);

    const bool haveKeyMappings = (m_device->getKeyMappings().size() > 0);

    // For each name field (LineEdit) on the UI...
    // programIndex is also the program change number.
    for (size_t programIndex = 0; programIndex < m_names.size(); ++programIndex) {

        QToolButton *keyMapButton = getKeyMapButton(programIndex);
        keyMapButton->setMaximumHeight(12);
        keyMapButton->setEnabled(haveKeyMappings);

        bool found = false;
        MidiProgram foundProgram;

        // Find the program in programSubset.
        for (const MidiProgram &midiProgram : programSubset) {
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

        // Icon and ToolTip

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
MidiProgramsEditor::reset()
{
    // Go back to m_oldBank's MSB/LSB and percussion setting.

    m_percussion->setChecked(m_oldBank.isPercussion());
    m_msb->setValue(m_oldBank.getMSB());
    m_lsb->setValue(m_oldBank.getLSB());

    // Make sure all the programs in m_programList are set back to the m_oldBank
    // MSB/LSB.
    if (m_currentBank) {
        modifyCurrentPrograms(*m_currentBank, m_oldBank);
        *m_currentBank = m_oldBank;
    }
}

void
MidiProgramsEditor::slotPercussionClicked()
{
    RG_DEBUG << "slotPercussionClicked()";

    MidiBank newBank(
            m_percussion->isChecked(),
            m_msb->value(),
            m_lsb->value(),
            m_currentBank->getName());

    // Make sure the programs in m_programList have the new percussion setting.
    modifyCurrentPrograms(*m_currentBank, newBank);

    // Update the current bank.
    *m_currentBank = newBank;

    // Refresh the tree so that it shows "Percussion" or "Bank" as appropriate.
    m_bankEditor->slotApply();
}

void
MidiProgramsEditor::slotNewMSB(int value)
{
    RG_DEBUG << "slotNewMSB(" << value << ")";

    // ??? Not sure we should clean this up since we are getting rid of it.

    int msb;

    try {
        msb = ensureUniqueMSB(value, value > m_currentBank->getMSB());
    } catch (bool) {
        msb = m_currentBank->getMSB();
    }

    MidiBank newBank(m_percussion->isChecked(),
                     msb,
                     m_lsb->value(),
                     m_currentBank->getName());

    modifyCurrentPrograms(*m_currentBank, newBank);

    m_msb->setValue(msb);

    *m_currentBank = newBank;

    // Refresh the tree so that it shows the new MSB.
    m_bankEditor->slotApply();
}

void
MidiProgramsEditor::slotNewLSB(int value)
{
    RG_DEBUG << "slotNewLSB(" << value << ")";

    // ??? Not sure we should clean this up since we are getting rid of it.

    int lsb;

    try {
        lsb = ensureUniqueLSB(value, value > m_currentBank->getLSB());
    } catch (bool) {
        lsb = m_currentBank->getLSB();
    }

    MidiBank newBank(m_percussion->isChecked(),
                     m_msb->value(),
                     lsb,
                     m_currentBank->getName());

    modifyCurrentPrograms(*m_currentBank, newBank);

    m_lsb->setValue(lsb);

    *m_currentBank = newBank;

    // Refresh the tree so that it shows the new MSB.
    m_bankEditor->slotApply();
}

void
MidiProgramsEditor::slotNameChanged(const QString &programName)
{
    //RG_DEBUG << "slotNameChanged(" << programName << ")";

    // This is called for every single change to the edit box.  E.g.
    // If the user types "hi", this slot is called twice.  Once with
    // "h" and again with "hi".

    // ??? Can we be more efficient?  E.g. only make the change when
    //     the cursor leaves the edit box, or "Ok" is clicked?

    if (!m_currentBank) {
        RG_WARNING << "slotNameChanged(): WARNING: m_currentBank is nullptr.";
        return;
    }

    const LineEdit *lineEdit = dynamic_cast<const LineEdit *>(sender());

    if (!lineEdit) {
        RG_WARNING << "slotNameChanged(): WARNING: Signal sender is not a LineEdit.";
        return;
    }

    // Zero-based program number.  This is the same as the MIDI program change.
    const unsigned programNumber = lineEdit->property("index").toUInt();

    //RG_DEBUG << "slotNameChanged(" << programName << ") : id = " << programNumber;

    // Get the MidiProgram that needs to be changed from m_programList.
    ProgramList::iterator programIter =
            getProgramIter(*m_currentBank, programNumber);

    // If the MidiProgram doesn't exist in m_programList, add it.
    if (programIter == m_programList.end()) {
        // If the program name is empty, do nothing.
        if (programName.isEmpty())
            return;

        // Create a new MidiProgram and add it to m_programList.
        MidiProgram newProgram(*m_currentBank, programNumber);
        m_programList.push_back(newProgram);

        // Sort m_programList.
        // We need to sort this for the MIPP.  It just needs the PCs in
        // order.  That's how it displays them.
        // If we do not sort this, the .rg file is also mixed up.  But
        // that's not a serious issue since no one looks at that.
        // ??? MidiProgram's op< is backwards.  It is
        //     comparing program and then bank.  But bank is more important.
        // ??? Is there a better place to sort this?  What if we take
        //     the first steps toward moving to std::set?  Make it a
        //     std::set, but continue using it like a vector.  That
        //     way it is always sorted.
        std::sort(m_programList.begin(), m_programList.end());

        // Get the new MidiProgram from m_programList.
        programIter = getProgramIter(*m_currentBank, programNumber);

    } else {  // The MidiProgram already exists in m_programList.

        // If the label is now empty...
        if (programName.isEmpty()) {
            //RG_DEBUG << "slotNameChanged(): deleting empty program (" << programNumber << ")";
            m_programList.erase(programIter);
            // ??? Why?
            m_bankEditor->slotApply();

            return;
        }
    }

    if (programIter == m_programList.end()) {
        RG_WARNING << "slotNameChanged(): WARNING: programIter is end().";
        return;
    }

    //RG_DEBUG << "slotNameChanged(): program: " << program;

    // If the name has actually changed
    if (qstrtostr(programName) != programIter->getName()) {
        programIter->setName(qstrtostr(programName));
        // ??? Why?
        m_bankEditor->slotApply();
    }
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

    MidiProgram *program = getProgram(*m_currentBank, id);
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

    MidiProgram *program = getProgram(*m_currentBank, m_keyMapProgramNumber);
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
    // ??? Only the name is used?  Then we need to disallow key mappings
    //     with empty names.  The UI currently allows this.
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

int
MidiProgramsEditor::ensureUniqueMSB(int msb, bool ascending)
{
    // ??? Not sure we should clean this up since we are getting rid of it.

    bool percussion = false; // Doesn't matter
    int newMSB = msb;
    while (banklistContains(MidiBank(percussion,
                                     newMSB, m_lsb->value()))
            && newMSB < 128
            && newMSB > -1)
        if (ascending)
            newMSB++;
        else
            newMSB--;

    if (newMSB == -1 || newMSB == 128)
        throw false;

    return newMSB;
}

int
MidiProgramsEditor::ensureUniqueLSB(int lsb, bool ascending)
{
    // ??? Not sure we should clean this up since we are getting rid of it.

    bool percussion = false; // Doesn't matter
    int newLSB = lsb;
    while (banklistContains(MidiBank(percussion,
                                     m_msb->value(), newLSB))
            && newLSB < 128
            && newLSB > -1)
        if (ascending)
            newLSB++;
        else
            newLSB--;

    if (newLSB == -1 || newLSB == 128)
        throw false;

    return newLSB;
}

bool
MidiProgramsEditor::banklistContains(const MidiBank &bank)
{
    // For each bank
    for (const MidiBank &currentBank : m_bankList)
    {
        // Just compare the MSB/LSB.
        if (currentBank.getMSB() == bank.getMSB()  &&
            currentBank.getLSB() == bank.getLSB())
            return true;
    }

    return false;
}

MidiProgram *
MidiProgramsEditor::getProgram(const MidiBank &bank, int programNo)
{
    for (MidiProgram &midiProgram : m_programList) {
        // Match?
        if (midiProgram.getBank().getMSB() == bank.getMSB()  &&
            midiProgram.getBank().getLSB() == bank.getLSB()  &&
            midiProgram.getProgram() == programNo) {
            return &midiProgram;
        }
    }

    return nullptr;
}

ProgramList::iterator
MidiProgramsEditor::getProgramIter(const MidiBank &bank, int programNo)
{
    // For each program in m_programList...
    for (ProgramList::iterator programIter = m_programList.begin();
         programIter != m_programList.end();
         ++programIter) {
        // Match?
        if (programIter->getBank().getMSB() == bank.getMSB()  &&
            programIter->getBank().getLSB() == bank.getLSB()  &&
            programIter->getProgram() == programNo)
            return programIter;
    }

    return m_programList.end();
}


}
