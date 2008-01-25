/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MidiProgramsEditor.h"
#include "MidiBankListViewItem.h"
#include "NameSetEditor.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "BankEditorDialog.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "gui/widgets/RosegardenPopupMenu.h"
#include <kcompletion.h>
#include <kglobal.h>
#include <klineedit.h>
#include <klocale.h>
#include <kstddirs.h>
#include <qcheckbox.h>
#include <qcursor.h>
#include <qfile.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qobjectlist.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qtooltip.h>
#include <qvgroupbox.h>
#include <qwidget.h>
#include <algorithm>

namespace Rosegarden
{

MidiProgramsEditor::MidiProgramsEditor(BankEditorDialog* bankEditor,
                                       QWidget* parent,
                                       const char* name)
        : NameSetEditor(bankEditor,
                        i18n("Bank and Program details"),
                        parent, name, i18n("Programs"), true),
        m_device(0),
        m_bankList(bankEditor->getBankList()),
        m_programList(bankEditor->getProgramList()),
        m_oldBank(false, 0, 0)
{
    QWidget *additionalWidget = makeAdditionalWidget(m_mainFrame);
    if (additionalWidget) {
        m_mainLayout->addMultiCellWidget(additionalWidget, 0, 2, 0, 2);
    }
}

QWidget *
MidiProgramsEditor::makeAdditionalWidget(QWidget *parent)
{
    QFrame *frame = new QFrame(parent);

    m_percussion = new QCheckBox(frame);
    m_msb = new QSpinBox(frame);
    m_lsb = new QSpinBox(frame);

    QGridLayout *gridLayout = new QGridLayout(frame,
                              3,   // rows
                              2,   // cols
                              2); // margin

    gridLayout->addWidget(new QLabel(i18n("Percussion"), frame),
                          0, 0, AlignLeft);
    gridLayout->addWidget(m_percussion, 0, 1, AlignLeft);
    connect(m_percussion, SIGNAL(clicked()),
            this, SLOT(slotNewPercussion()));

    gridLayout->addWidget(new QLabel(i18n("MSB Value"), frame),
                          1, 0, AlignLeft);
    m_msb->setMinValue(0);
    m_msb->setMaxValue(127);
    gridLayout->addWidget(m_msb, 1, 1, AlignLeft);

    QToolTip::add
        (m_msb,
                i18n("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    QToolTip::add
        (m_lsb,
                i18n("Selects a LSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    connect(m_msb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewMSB(int)));

    gridLayout->addWidget(new QLabel(i18n("LSB Value"), frame),
                          2, 0, AlignLeft);
    m_lsb->setMinValue(0);
    m_lsb->setMaxValue(127);
    gridLayout->addWidget(m_lsb, 2, 1, AlignLeft);

    connect(m_lsb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewLSB(int)));

    return frame;
}

ProgramList
MidiProgramsEditor::getBankSubset(const MidiBank &bank)
{
    ProgramList program;
    ProgramList::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++) {
        if (it->getBank() == bank)
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

    for (it = m_programList.begin(); it != m_programList.end(); it++) {
        if (it->getBank() == oldBank) {
            *it = MidiProgram(newBank, it->getProgram(), it->getName());
        }
    }
}

void
MidiProgramsEditor::clearAll()
{
    blockAllSignals(true);

    for (unsigned int i = 0; i < m_names.size(); ++i)
        m_names[i]->clear();

    setTitle(i18n("Bank and Program details"));

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
MidiProgramsEditor::populate(QListViewItem* item)
{
    RG_DEBUG << "MidiProgramsEditor::populate\n";

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);
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
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QString file = pixmapDir + "/toolbar/key-white.png";
    if (QFile(file).exists())
        noKeyPixmap = QPixmap(file);
    file = pixmapDir + "/toolbar/key-green.png";
    if (QFile(file).exists())
        keyPixmap = QPixmap(file);

    bool haveKeyMappings = m_currentBank->isPercussion()
                           && (m_device->getKeyMappings().size() > 0);

    for (unsigned int i = 0; i < m_names.size(); i++) {
        m_names[i]->clear();
        getEntryButton(i)->setEnabled(haveKeyMappings);
        getEntryButton(i)->setPixmap(noKeyPixmap);
        QToolTip::remove
            ( getEntryButton(i) );

        for (it = programSubset.begin(); it != programSubset.end(); it++) {
            if (it->getProgram() == i) {

                QString programName = strtoqstr(it->getName());
                m_completion.addItem(programName);
                m_names[i]->setText(programName);

                if (m_device->getKeyMappingForProgram(*it)) {
                    getEntryButton(i)->setPixmap(keyPixmap);
                    QToolTip::add
                        (getEntryButton(i),
                                i18n("Key Mapping: %1").arg(
                                    strtoqstr(m_device->getKeyMappingForProgram(*it)->getName())));
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
    RG_DEBUG << "MidiProgramsEditor::slotNewPercussion" << endl;
    bool percussion = m_percussion->isChecked();
    m_percussion->blockSignals(true);
    if (banklistContains(MidiBank(percussion, m_msb->value(), m_lsb->value()))) {
        RG_DEBUG << "MidiProgramsEditor::slotNewPercussion: calling setChecked(" << !percussion << ")" << endl;
        m_percussion->setChecked(!percussion);
    } else {
        MidiBank newBank(percussion,
                         m_msb->value(),
                         m_lsb->value());
        modifyCurrentPrograms(*getCurrentBank(), newBank);
        *getCurrentBank() = newBank;
    }
    m_percussion->blockSignals(false);
    m_bankEditor->setModified(true);
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
                     m_lsb->value());

    modifyCurrentPrograms(*getCurrentBank(), newBank);

    m_msb->setValue(msb);
    *getCurrentBank() = newBank;

    m_msb->blockSignals(false);

    m_bankEditor->setModified(true);
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
                     lsb);

    modifyCurrentPrograms(*getCurrentBank(), newBank);

    m_lsb->setValue(lsb);
    *getCurrentBank() = newBank;

    m_lsb->blockSignals(false);

    m_bankEditor->setModified(true);
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
    const KLineEdit* lineEdit = dynamic_cast<const KLineEdit*>(sender());
    if (!lineEdit) {
        RG_DEBUG << "MidiProgramsEditor::slotProgramChanged() : %%% ERROR - signal sender is not a KLineEdit\n";
        return ;
    }

    QString senderName = sender()->name();

    // Adjust value back to zero rated
    //
    unsigned int id = senderName.toUInt() - 1;

    RG_DEBUG << "MidiProgramsEditor::slotNameChanged("
    << programName << ") : id = " << id << endl;

    MidiProgram *program = getProgram(*getCurrentBank(), id);

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

            for (; it != m_programList.end(); it++) {
                if (((unsigned int)it->getProgram()) == id) {
                    m_programList.erase(it);
                    m_bankEditor->setModified(true);
                    RG_DEBUG << "deleting empty program (" << id << ")" << endl;
                    return ;
                }
            }
        }
    }

    if (qstrtostr(programName) != program->getName()) {
        program->setName(qstrtostr(programName));
        m_bankEditor->setModified(true);
    }
}

void
MidiProgramsEditor::slotEntryButtonPressed()
{
    QPushButton* button = dynamic_cast<QPushButton*>(const_cast<QObject *>(sender()));
    if (!button) {
        RG_DEBUG << "MidiProgramsEditor::slotEntryButtonPressed() : %%% ERROR - signal sender is not a QPushButton\n";
        return ;
    }

    QString senderName = button->name();

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

    RosegardenPopupMenu *menu = new RosegardenPopupMenu(button);

    const MidiKeyMapping *currentMapping =
        m_device->getKeyMappingForProgram(*program);
    int currentEntry = 0;

    menu->insertItem(i18n("<no key mapping>"), this,
                     SLOT(slotEntryMenuItemSelected(int)), 0, 0);
    menu->setItemParameter(0, 0);

    for (int i = 0; i < kml.size(); ++i) {
        menu->insertItem(strtoqstr(kml[i].getName()),
                         this, SLOT(slotEntryMenuItemSelected(int)),
                         0, i + 1);
        menu->setItemParameter(i + 1, i + 1);
        if (currentMapping && (kml[i] == *currentMapping))
            currentEntry = i + 1;
    }

    int itemHeight = menu->itemHeight(0) + 2;
    QPoint pos = QCursor::pos();

    pos.rx() -= 10;
    pos.ry() -= (itemHeight / 2 + currentEntry * itemHeight);

    menu->popup(pos);
}

void
MidiProgramsEditor::slotEntryMenuItemSelected(int i)
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
        if (i < kml.size()) {
            newMapping = kml[i].getName();
        }
    }

    m_device->setKeyMappingForProgram(*program, newMapping);
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    bool haveKeyMappings = (m_device->getKeyMappings().size() > 0);
    QPushButton *btn = getEntryButton(m_currentMenuProgram);

    if (newMapping.empty()) {
        QString file = pixmapDir + "/toolbar/key-white.png";
        if (QFile(file).exists()) {
            btn->setPixmap(QPixmap(file));
        }
        QToolTip::remove
            (btn);
    } else {
        QString file = pixmapDir + "/toolbar/key-green.png";
        if (QFile(file).exists()) {
            btn->setPixmap(QPixmap(file));
        }
        QToolTip::add
            (btn, i18n("Key Mapping: %1").arg(strtoqstr(newMapping)));
    }
    btn->setEnabled(haveKeyMappings);
}

int
MidiProgramsEditor::ensureUniqueMSB(int msb, bool ascending)
{
    int newMSB = msb;
    while (banklistContains(MidiBank(m_percussion->isChecked(),
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
    int newLSB = lsb;
    while (banklistContains(MidiBank(m_percussion->isChecked(),
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
    BankList::iterator it;

    for (it = m_bankList.begin(); it != m_bankList.end(); it++)
        if (*it == bank)
            return true;

    return false;
}

MidiProgram*
MidiProgramsEditor::getProgram(const MidiBank &bank, int programNo)
{
    ProgramList::iterator it = m_programList.begin();

    for (; it != m_programList.end(); it++) {
        if (it->getBank() == bank && it->getProgram() == programNo)
            return &(*it);
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
    const QObjectList* allChildren = queryList("KLineEdit", "[0-9]+");
    QObjectListIt it(*allChildren);
    QObject *obj;

    while ( (obj = it.current()) != 0 ) {
        obj->blockSignals(block);
        ++it;
    }

    m_msb->blockSignals(block);
    m_lsb->blockSignals(block);
}

}
#include "MidiProgramsEditor.moc"
