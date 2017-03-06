/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#define RG_MODULE_STRING "[MidiProgramsEditor]"

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
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QVBoxLayout>
#include <QObjectList>
#include <QPixmap>
#include <QIcon>
#include <QPoint>
#include <QMenu>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QFile>

#include <algorithm>


namespace Rosegarden
{

MidiProgramsEditor::MidiProgramsEditor(BankEditorDialog* bankEditor,
                                       QWidget* parent,
                                       const char* name)
        : NameSetEditor(bankEditor,
                        tr("Bank and Program details"),
                        parent, name, tr("Programs"), true),
        m_device(0),
        m_bankList(bankEditor->getBankList()),
        m_programList(bankEditor->getProgramList()),
        m_oldBank(false, 0, 0)
{
    QWidget *additionalWidget = makeAdditionalWidget(m_mainFrame);
    if (additionalWidget) {
        m_mainLayout->addWidget(additionalWidget, 0, 0, 3, 3);
    }
}

QWidget *
MidiProgramsEditor::makeAdditionalWidget(QWidget *parent)
{
    QFrame *frame = new QFrame(parent);

    m_percussion = new QCheckBox(frame);
    m_msb = new QSpinBox(frame);
    m_lsb = new QSpinBox(frame);

    frame->setContentsMargins(0, 0, 0, 0);
    QGridLayout *gridLayout = new QGridLayout(frame);
    gridLayout->setSpacing(0);

    gridLayout->addWidget(new QLabel(tr("Percussion"), frame),
                          0, 0, Qt::AlignLeft);
    gridLayout->addWidget(m_percussion, 0, 1, Qt::AlignLeft);
    connect(m_percussion, SIGNAL(clicked()),
            this, SLOT(slotNewPercussion()));

    gridLayout->addWidget(new QLabel(tr("MSB Value"), frame),
                          1, 0, Qt::AlignLeft);
    m_msb->setMinimum(0);
    m_msb->setMaximum(127);
    gridLayout->addWidget(m_msb, 1, 1, Qt::AlignLeft);

    m_msb->setToolTip(tr("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    m_lsb->setToolTip(tr("Selects a LSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    connect(m_msb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewMSB(int)));

    gridLayout->addWidget(new QLabel(tr("LSB Value"), frame),
                          2, 0, Qt::AlignLeft);
    m_lsb->setMinimum(0);
    m_lsb->setMaximum(127);
    gridLayout->addWidget(m_lsb, 2, 1, Qt::AlignLeft);

    connect(m_lsb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewLSB(int)));

    return frame;
}

ProgramList
MidiProgramsEditor::getBankSubset(const MidiBank &bank)
{
    ProgramList program;
    ProgramList::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); ++it) {
        if (it->getBank().partialCompare(bank))
            program.push_back(*it);
    }

    return program;
}

MidiBank*
MidiProgramsEditor::getCurrentBank()
{
    return m_currentBank;
}

void
MidiProgramsEditor::modifyCurrentPrograms(const MidiBank &oldBank,
        const MidiBank &newBank)
{
    ProgramList::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); ++it) {
        if (it->getBank().partialCompare(oldBank)) {
            *it = MidiProgram(newBank, it->getProgram(), it->getName());
        }
    }
}

void
MidiProgramsEditor::clearAll()
{
    blockAllSignals(true);

    for (size_t i = 0; i < m_names.size(); ++i)
        m_names[i]->clear();

    setTitle(tr("Bank and Program details"));

    m_percussion->setChecked(false);
    m_msb->setValue(0);
    m_lsb->setValue(0);
    m_librarian->clear();
    m_librarianEmail->clear();
    m_currentBank = 0;
    setEnabled(false);

    blockAllSignals(false);
}

void
MidiProgramsEditor::populate(QTreeWidgetItem* item)
{
    RG_DEBUG << "MidiProgramsEditor::populate\n";

    MidiBankTreeWidgetItem* bankItem = dynamic_cast<MidiBankTreeWidgetItem*>(item);
    if (!bankItem) {
        RG_DEBUG << "MidiProgramsEditor::populate : not a bank item - returning\n";
        return ;
    }

    DeviceId deviceId = bankItem->getDeviceId();
    m_device = m_bankEditor->getMidiDevice(deviceId);
    if (!m_device)
        return ;

    setEnabled(true);

    setBankName(item->text(0));

    RG_DEBUG << "MidiProgramsEditor::populate : bankItem->getBank = "
    << bankItem->getBank() << endl;

    m_currentBank = &(m_bankList[bankItem->getBank()]); // m_device->getBankByIndex(bankItem->getBank());

    blockAllSignals(true);

    // set the bank values
    m_percussion->setChecked(m_currentBank->isPercussion());
    m_msb->setValue(m_currentBank->getMSB());
    m_lsb->setValue(m_currentBank->getLSB());

    m_oldBank = *m_currentBank;

    // Librarian details
    //
    m_librarian->setText(strtoqstr(m_device->getLibrarianName()));
    m_librarianEmail->setText(strtoqstr(m_device->getLibrarianEmail()));

    ProgramList programSubset = getBankSubset(*m_currentBank);
    ProgramList::iterator it;

    QPixmap noKeyPixmap, keyPixmap;

    IconLoader il;
    noKeyPixmap = il.loadPixmap("key-white");
    keyPixmap = il.loadPixmap("key-green");

    bool haveKeyMappings = m_device->getKeyMappings().size() > 0;

    for (unsigned int i = 0; i < (unsigned int)m_names.size(); i++) {

        m_names[i]->clear();
        getKeyMapButton(i)->setEnabled(haveKeyMappings);
        getKeyMapButton(i)->setIcon(QIcon(noKeyPixmap));
        // QToolTip::remove
        //    ( getKeyMapButton(i) );
        getKeyMapButton(i)->setToolTip(QString(""));  //@@@ Usefull ?
        getKeyMapButton(i)->setMaximumHeight( 12 );

        for (it = programSubset.begin(); it != programSubset.end(); ++it) {
            if (it->getProgram() == i) {

                // zero in on "Harpsichord" vs. "Coupled Harpsichord to cut down
                // on noise (0-based)
//                if (i == 6) std::cout << "it->getName(): " << it->getName() << std::endl;
                QString programName = strtoqstr(it->getName());
                m_completions << programName;
                m_names[i]->setText(programName);

                if (m_device->getKeyMappingForProgram(*it)) {
                    getKeyMapButton(i)->setIcon(QIcon(keyPixmap));
                    getKeyMapButton(i)->setToolTip
                        (tr("Key Mapping: %1") 
                              .arg(strtoqstr(m_device->getKeyMappingForProgram(*it)->getName())));
                }

                break;
            }
        }

        // show start of label
        m_names[i]->setCursorPosition(0);
    }

    blockAllSignals(false);
}

void
MidiProgramsEditor::reset()
{
    m_percussion->blockSignals(true);
    m_msb->blockSignals(true);
    m_lsb->blockSignals(true);

    m_percussion->setChecked(m_oldBank.isPercussion());
    m_msb->setValue(m_oldBank.getMSB());
    m_lsb->setValue(m_oldBank.getLSB());

    if (m_currentBank) {
        modifyCurrentPrograms(*m_currentBank, m_oldBank);
        *m_currentBank = m_oldBank;
    }

    m_percussion->blockSignals(false);
    m_msb->blockSignals(false);
    m_lsb->blockSignals(false);
}

void
MidiProgramsEditor::slotNewPercussion()
{
    RG_DEBUG << "MidiProgramsEditor::slotNewPercussion";
    bool percussion = false; // Doesn't matter
    MidiBank *newBank;
    if (banklistContains(MidiBank(percussion, m_msb->value(), m_lsb->value()))) {
        RG_DEBUG << "MidiProgramsEditor::slotNewPercussion: calling setChecked(" << !percussion << ")";
        newBank = new MidiBank(m_percussion->isChecked(),
                         m_msb->value(),
                         m_lsb->value(), getCurrentBank()->getName());
    } else {
        newBank = new MidiBank(true,
                         m_msb->value(),
                         m_lsb->value());
    }
    modifyCurrentPrograms(*getCurrentBank(), *newBank);
    *getCurrentBank() = *newBank;
    m_bankEditor->slotApply();
    
    // Hack to force the percussion icons to switch state if needed.
    // code stole from populate.
    if (m_device) {
        bool haveKeyMappings = m_device->getKeyMappings().size() > 0;

        for (unsigned int i = 0; i < (unsigned int)m_names.size(); i++) {
            getKeyMapButton(i)->setEnabled(haveKeyMappings);
        }
    }
}

void
MidiProgramsEditor::slotNewMSB(int value)
{
    RG_DEBUG << "MidiProgramsEditor::slotNewMSB(" << value << ")\n";

    m_msb->blockSignals(true);

    int msb;

    try {
        msb = ensureUniqueMSB(value, value > getCurrentBank()->getMSB());
    } catch (bool) {
        msb = getCurrentBank()->getMSB();
    }

    MidiBank newBank(m_percussion->isChecked(),
                     msb,
                     m_lsb->value(), getCurrentBank()->getName());

    modifyCurrentPrograms(*getCurrentBank(), newBank);

    m_msb->setValue(msb);
    *getCurrentBank() = newBank;

    m_msb->blockSignals(false);

    m_bankEditor->slotApply();
}

void
MidiProgramsEditor::slotNewLSB(int value)
{
    RG_DEBUG << "MidiProgramsEditor::slotNewLSB(" << value << ")\n";

    m_lsb->blockSignals(true);

    int lsb;

    try {
        lsb = ensureUniqueLSB(value, value > getCurrentBank()->getLSB());
    } catch (bool) {
        lsb = getCurrentBank()->getLSB();
    }

    MidiBank newBank(m_percussion->isChecked(),
                     m_msb->value(),
                     lsb, getCurrentBank()->getName());

    modifyCurrentPrograms(*getCurrentBank(), newBank);

    m_lsb->setValue(lsb);
    *getCurrentBank() = newBank;

    m_lsb->blockSignals(false);

    m_bankEditor->slotApply();
}

struct ProgramCmp
{
    bool operator()(const Rosegarden::MidiProgram &p1,
                    const Rosegarden::MidiProgram &p2)
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
    const LineEdit* lineEdit = dynamic_cast<const LineEdit*>(sender());
    if (!lineEdit) {
        RG_DEBUG << "MidiProgramsEditor::slotNameChanged() : %%% ERROR - signal sender is not a Rosegarden::LineEdit\n";
        return ;
    }

    QString senderName = sender()->objectName();
    // "Harpsichord" in default GM bank 1:0, "Coupled Harpsichord" in bank 8:0
//    if (senderName == "7") std::cout << "senderName is: " << senderName.toStdString()
//                                     << " programName is: " << programName.toStdString() << std::endl;

    // Adjust value back to zero rated
    //
    unsigned int id = senderName.toUInt() - 1;
//    std::cout << "id is: " << id << std::endl;

    RG_DEBUG << "MidiProgramsEditor::slotNameChanged(" << programName << ") : id = " << id;
    
    MidiBank* currBank;
    currBank = getCurrentBank();
    if (!currBank) {
        RG_DEBUG << "Error: currBank is NULL in MidiProgramsEditor::slotNameChanged() ";
        return;
    } else {
        RG_DEBUG << "currBank: " << currBank;
    }

    RG_DEBUG << "current bank name: " << currBank->getName();
    MidiProgram *program = getProgram(*currBank, id);
//     MidiProgram *program = getProgram(*currBank, id);

    if (program == 0) {
        // Do nothing if program name is empty
        if (programName.isEmpty())
            return ;

        program = new MidiProgram(*getCurrentBank(), id);
        m_programList.push_back(*program);

        // Sort the program list by id
        std::sort(m_programList.begin(), m_programList.end(), ProgramCmp());

        // Now, get with the program
        //
        program = getProgram(*getCurrentBank(), id);
    } else {
        // If we've found a program and the label is now empty
        // then remove it from the program list.
        //
        if (programName.isEmpty()) {
            ProgramList::iterator it = m_programList.begin();
            ProgramList tmpProg;

            for (; it != m_programList.end(); ++it) {
                if (((unsigned int)it->getProgram()) == id) {
                    m_programList.erase(it);
                    m_bankEditor->slotApply();
                    RG_DEBUG << "deleting empty program (" << id << ")";
                    return ;
                }
            }
        }
    }

    if (!program) {
        RG_DEBUG << "Error: program is NULL in MidiProgramsEditor::slotNameChanged() ";
        return;
    } else {
        RG_DEBUG << "program: " << program;
    }
    
    if (qstrtostr(programName) != program->getName()) {
        program->setName(qstrtostr(programName));
        m_bankEditor->slotApply();
    }
}

void
MidiProgramsEditor::slotKeyMapButtonPressed()
{
    QToolButton* button = dynamic_cast<QToolButton*>(const_cast<QObject *>(sender()));
    if (!button) {
        RG_DEBUG << "MidiProgramsEditor::slotKeyMapButtonPressed() : %%% ERROR - signal sender is not a QPushButton\n";
        return ;
    }

//    std::cout << "editor button name" << button->objectName().toStdString() << std::endl;

    QString senderName = button->objectName();

    if (!m_device)
        return ;

    const KeyMappingList &kml = m_device->getKeyMappings();
    if (kml.empty())
        return ;

    // Adjust value back to zero rated
    //
    unsigned int id = senderName.toUInt() - 1;
    MidiProgram *program = getProgram(*getCurrentBank(), id);
    if (!program)
        return ;
    m_currentMenuProgram = id;

    QMenu *menu = new QMenu(button);

    const MidiKeyMapping *currentMapping =
        m_device->getKeyMappingForProgram(*program);

    int currentKeyMap = 0;

    QAction *a = menu->addAction(tr("<no key mapping>"));
    a->setObjectName("0");

    for (size_t i = 0; i < kml.size(); ++i) {
        a = menu->addAction(strtoqstr(kml[i].getName()));
        a->setObjectName(QString("%1").arg(i+1));
        if (currentMapping && (kml[i] == *currentMapping)) currentKeyMap = int(i + 1);
    }

    connect(menu, SIGNAL(triggered(QAction *)),
            this, SLOT(slotKeyMapMenuItemSelected(QAction *)));

    int itemHeight = menu->actionGeometry(actions().value(0)).height() + 2;
    QPoint pos = QCursor::pos();

    pos.rx() -= 10;
    pos.ry() -= (itemHeight / 2 + currentKeyMap * itemHeight);

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

    MidiProgram *program = getProgram(*getCurrentBank(), m_currentMenuProgram);
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
    IconLoader il;
    QIcon icon;

    bool haveKeyMappings = (m_device->getKeyMappings().size() > 0);  //@@@ JAS restored from before port/
    QToolButton *btn = getKeyMapButton(m_currentMenuProgram);

    if (newMapping.empty()) {
        icon = il.load( "key-white" );
        if( ! icon.isNull() ) {
            btn->setIcon( icon );
        }
        // QToolTip::remove(btn);
        btn->setToolTip(QString(""));       //@@@ Usefull ?
    } else {
        icon = il.load( "key-green" );
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

            //Only show hits to avoid overflow of console.
            RG_DEBUG << "it->getBank() " << "== bank";
            return &(*it);
        }
    }

    return 0;
}

void
MidiProgramsEditor::setBankName(const QString& s)
{
    setTitle(s);
}

void MidiProgramsEditor::blockAllSignals(bool block)
{
    QList<LineEdit *> allChildren = findChildren<LineEdit*>((QRegExp)"[0-9]+");
    QList<LineEdit *>::iterator it;

    for (it = allChildren.begin(); it != allChildren.end(); ++it) {
        (*it)->blockSignals(block);
    }

    m_msb->blockSignals(block);
    m_lsb->blockSignals(block);
}

}
