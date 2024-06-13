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


MidiProgramsEditor::MidiProgramsEditor(BankEditorDialog *bankEditor,
                                       QWidget *parent) :
    NameSetEditor(bankEditor,
                  tr("Bank and Program details"),  // title
                  parent,
                  true),  // showKeyMapButtons
    m_bankList(bankEditor->getBankList()),
    m_programList(bankEditor->getProgramList())
{
    QFrame *frame = initWidgets(m_topFrame);
    m_topLayout->addWidget(frame, 0, 0, 3, 3);
}

QFrame *
MidiProgramsEditor::initWidgets(QWidget *parent)
{
    // ??? Inline this into the ctor like every other dialog.

    QFrame *frame = new QFrame(parent);
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

    return frame;
}

ProgramList
MidiProgramsEditor::getBankSubset(const MidiBank &bank)
{
    ProgramList programList;

    // For each program, copy the ones for the requested bank to programList.
    for (const MidiProgram &program : m_programList) {
        if (program.getBank().partialCompare(bank))
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
        if (program.getBank().partialCompare(oldBank))
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

    // Need this because NameSetEditor connects to each name
    // field's textChanged signal instead of textEdited.
    // ??? Fix NameSetEditor!
    blockAllSignals(true);
    for (size_t i = 0; i < m_names.size(); ++i)
        m_names[i]->clear();
    blockAllSignals(false);

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

    // Use a white icon to indicate there is no keymap for this program.
    static const QIcon noKeymapIcon(IconLoader::loadPixmap("key-white"));
    // Use a green icon to indicate there *is* a keymap for this program.
    static const QIcon keymapIcon(IconLoader::loadPixmap("key-green"));

    const bool haveKeyMappings = (m_device->getKeyMappings().size() > 0);

    // Need this because NameSetEditor connects to each name
    // field's textChanged signal instead of textEdited.
    // ??? Fix NameSetEditor!
    blockAllSignals(true);

    // For each name in the program list...
    // programIndex is also the program change number.
    for (size_t programIndex = 0; programIndex < m_names.size(); ++programIndex) {

        QToolButton *keyMapButton = getKeyMapButton(programIndex);
        // ??? Seems like something NameSetEditor should take care of?
        keyMapButton->setMaximumHeight(12);
        keyMapButton->setEnabled(haveKeyMappings);

        // ??? Restructure this as:
        //       - find program in programSubset
        //       - if not found, clear and continue.
        //       - Set everything up.

        // Assume not found and clear everything.
        m_names[programIndex]->clear();
        keyMapButton->setIcon(noKeymapIcon);
        keyMapButton->setToolTip("");

        // Find the program in programSubset...
        for (const MidiProgram &midiProgram : programSubset) {
            // Not it?  Try the next.
            if (midiProgram.getProgram() != programIndex)
                continue;

            // Found it.

            QString programName = strtoqstr(midiProgram.getName());
            m_completions << programName;
            m_names[programIndex]->setText(programName);
            // show start of label
            m_names[programIndex]->setCursorPosition(0);

            const MidiKeyMapping *midiKeyMapping =
                    m_device->getKeyMappingForProgram(midiProgram);
            if (midiKeyMapping) {
                // Indicate that this program has a keymap.
                keyMapButton->setIcon(keymapIcon);
                // Put the name in the tool tip.
                keyMapButton->setToolTip(tr("Key Mapping: %1").arg(
                        strtoqstr(midiKeyMapping->getName())));
            } else {  // No key mapping.
                // Indicate that this program has no keymap.
                keyMapButton->setIcon(noKeymapIcon);
                keyMapButton->setToolTip("");
            }

            break;
        }
    }
    blockAllSignals(false);
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

    m_bankEditor->slotApply();
}

void
MidiProgramsEditor::slotNewLSB(int value)
{
    RG_DEBUG << "slotNewLSB(" << value << ")";

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

    m_bankEditor->slotApply();
}

struct ProgramCmp
{
    bool operator()(const Rosegarden::MidiProgram &p1,
                    const Rosegarden::MidiProgram &p2) const
    {
        if (p1.getProgram() == p2.getProgram()) {
            const Rosegarden::MidiBank &b1(p1.getBank());
            const Rosegarden::MidiBank &b2(p2.getBank());
            if (b1.getMSB() == b2.getMSB())
                if (b1.getLSB() == b2.getLSB())
                    return ((b1.isPercussion() ? 1 : 0) < (b2.isPercussion() ? 1 : 0));
                else return (b1.getLSB() < b2.getLSB());
            else return (b1.getMSB() < b2.getMSB());
        } else return (p1.getProgram() < p2.getProgram());
    }
};

void
MidiProgramsEditor::slotNameChanged(const QString& programName)
{
    const LineEdit *lineEdit = dynamic_cast<const LineEdit *>(sender());

    if (!lineEdit) {
        RG_WARNING << "slotNameChanged(): WARNING: Signal sender is not a LineEdit.";
        return;
    }

    const unsigned id = lineEdit->property("index").toUInt();

    //RG_DEBUG << "slotNameChanged(" << programName << ") : id = " << id;

    MidiBank *currBank = m_currentBank;

    if (!currBank) {
        RG_WARNING << "slotNameChanged(): WARNING: currBank is nullptr.";
        return;
    }

    //RG_DEBUG << "slotNameChanged(): currBank: " << currBank;

    //RG_DEBUG << "slotNameChanged(): current bank name: " << currBank->getName();

    MidiProgram *program = getProgram(*currBank, id);

    // If the MidiProgram doesn't exist
    if (!program) {
        // Do nothing if program name is empty
        if (programName.isEmpty())
            return;

        // Create a new MidiProgram and add it to m_programList.
        MidiProgram newProgram(*m_currentBank, id);
        m_programList.push_back(newProgram);

        // Sort by program number.
        std::sort(m_programList.begin(), m_programList.end(), ProgramCmp());

        // Now, get the MidiProgram from the m_programList.
        program = getProgram(*m_currentBank, id);

    } else {
        // If we've found a program and the label is now empty,
        // remove it from the program list.
        if (programName.isEmpty()) {
            for (ProgramList::iterator it = m_programList.begin();
                 it != m_programList.end();
                 ++it) {
                if (static_cast<unsigned>(it->getProgram()) == id) {
                    m_programList.erase(it);
                    m_bankEditor->slotApply();

                    //RG_DEBUG << "slotNameChanged(): deleting empty program (" << id << ")";

                    return;
                }
            }
        }
    }

    if (!program) {
        RG_WARNING << "slotNameChanged(): WARNING: program is nullptr.";
        return;
    }

    //RG_DEBUG << "slotNameChanged(): program: " << program;

    // If the name has actually changed
    if (qstrtostr(programName) != program->getName()) {
        program->setName(qstrtostr(programName));
        m_bankEditor->slotApply();
    }
}

void
MidiProgramsEditor::slotKeyMapButtonPressed()
{
    QToolButton *button = dynamic_cast<QToolButton *>(sender());

    if (!button) {
        RG_WARNING << "slotKeyMapButtonPressed() : WARNING: Sender is not a QToolButton.";
        return;
    }

    if (!m_device)
        return;

    const KeyMappingList &kml = m_device->getKeyMappings();
    if (kml.empty())
        return;

    const unsigned id = button->property("index").toUInt();

    MidiProgram *program = getProgram(*m_currentBank, id);
    if (!program)
        return;

    m_currentMenuProgram = id;

    // Create a new popup menu.
    QMenu *menu = new QMenu(button);

    const MidiKeyMapping *currentMapping =
        m_device->getKeyMappingForProgram(*program);

    int currentKeyMap = 0;

    QAction *a = menu->addAction(tr("<no key mapping>"));
    a->setObjectName("0");

    for (size_t i = 0; i < kml.size(); ++i) {
        a = menu->addAction(strtoqstr(kml[i].getName()));
        a->setObjectName(QString("%1").arg(i+1));

        if (currentMapping  &&  (kml[i] == *currentMapping))
            currentKeyMap = static_cast<int>(i + 1);
    }

    connect(menu, SIGNAL(triggered(QAction *)),
            this, SLOT(slotKeyMapMenuItemSelected(QAction *)));

    // Compute the position for the pop-up menu.

    // QMenu::popup() can do this for us, but it doesn't place the
    // cursor over top of the current selection.

    // Get the QRect for the current entry.
    QRect actionRect =
            menu->actionGeometry(menu->actions().value(currentKeyMap));

    QPoint pos = QCursor::pos();
    pos.rx() -= 10;
    pos.ry() -= actionRect.top() + actionRect.height() / 2;

    // Display the menu.
    menu->popup(pos);
}

void
MidiProgramsEditor::slotKeyMapMenuItemSelected(QAction *a)
{
    slotKeyMapMenuItemSelected(a->objectName().toInt());
}

void
MidiProgramsEditor::slotKeyMapMenuItemSelected(int i)
{
    if (!m_device)
        return ;

    const KeyMappingList &kml = m_device->getKeyMappings();
    if (kml.empty())
        return ;

    MidiProgram *program = getProgram(*m_currentBank, m_currentMenuProgram);
    if (!program)
        return ;

    std::string newMapping;

    if (i == 0) { // no key mapping
        newMapping = "";
    } else {
        --i;
        if (i < (int)kml.size()) {
            newMapping = kml[i].getName();
        }
    }

    m_device->setKeyMappingForProgram(*program, newMapping);
//     QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIcon icon;

    bool haveKeyMappings = (m_device->getKeyMappings().size() > 0);  //@@@ JAS restored from before port/
    QToolButton *btn = getKeyMapButton(m_currentMenuProgram);

    if (newMapping.empty()) {
        icon = IconLoader::load( "key-white" );
        if( ! icon.isNull() ) {
            btn->setIcon( icon );
        }
        // QToolTip::remove(btn);
        btn->setToolTip(QString(""));       //@@@ Usefull ?
    } else {
        icon = IconLoader::load( "key-green" );
        if( ! icon.isNull() ){
            btn->setIcon( icon );
        }
        btn->setToolTip(tr("Key Mapping: %1").arg(strtoqstr(newMapping)));
    }
    btn->setEnabled(haveKeyMappings);
}

int
MidiProgramsEditor::ensureUniqueMSB(int msb, bool ascending)
{
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
    for (BankList::iterator it = m_bankList.begin();
         it != m_bankList.end();
         ++it)
    {
        // Just compare the MSB/LSB.
        if (it->getMSB() == bank.getMSB()  &&  it->getLSB() == bank.getLSB())
            return true;
    }

    return false;
}

MidiProgram*
MidiProgramsEditor::getProgram(const MidiBank &bank, int programNo)
{
    ProgramList::iterator it = m_programList.begin();

    for (; it != m_programList.end(); ++it) {
        if (it->getBank().partialCompare(bank)  &&
            it->getProgram() == programNo) {

            // Only show hits to avoid overflow of console.
            RG_DEBUG << "it->getBank() " << "== bank";
            return &(*it);
        }
    }

    return nullptr;
}

void
MidiProgramsEditor::setBankName(const QString& s)
{
    setTitle(s);
}

void MidiProgramsEditor::blockAllSignals(bool block)
{
    // Blocks all LineEdit signals.

    QList<LineEdit *> allChildren =
        findChildren<LineEdit*>((QRegularExpression)"[0-9]+");
    QList<LineEdit *>::iterator it;

    for (it = allChildren.begin(); it != allChildren.end(); ++it) {
        (*it)->blockSignals(block);
    }
}


}
