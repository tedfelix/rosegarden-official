// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <algorithm>

#include <qobjectlist.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <qdir.h>

#include <kapp.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <kseparator.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kio/netaccess.h>

#include "bankeditor.h"
#include "constants.h"
#include "widgets.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "studiocommands.h"
#include "rosedebug.h"
#include "importdevicedialog.h"

#include "Studio.h"
#include "MidiDevice.h"
#include "MidiProgram.h"

using Rosegarden::MidiBank;
using Rosegarden::MidiProgram;


MidiDeviceListViewItem::MidiDeviceListViewItem(Rosegarden::DeviceId deviceId,
                                               QListView* parent, QString name)
    : QListViewItem(parent, name),
      m_deviceId(deviceId)
{
}

MidiDeviceListViewItem::MidiDeviceListViewItem(Rosegarden::DeviceId deviceId,
                                               QListViewItem* parent, QString name,
					       bool percussion,
                                               int msb, int lsb)
    : QListViewItem(parent, name,
		    QString(percussion ? i18n("Yes") : i18n("No")),
		    QString().setNum(msb), QString().setNum(lsb)),
      m_deviceId(deviceId)
{
}

int MidiDeviceListViewItem::compare(QListViewItem *i, int col, bool ascending) const
{
    MidiDeviceListViewItem* item = dynamic_cast<MidiDeviceListViewItem*>(i);
    if (!item) return QListViewItem::compare(i, col, ascending);
    if (col == 0) return 
	getDeviceId() >  item->getDeviceId() ? 1 :
	getDeviceId() == item->getDeviceId() ? 0 :
	-1;
    
    int thisVal = text(col).toInt(),
        otherVal = item->text(col).toInt();

    if (thisVal == otherVal) {
        if (col == 2) { // if sorting on MSB, suborder with LSB
            return compare(i, 3, ascending);
	} else {
            return 0;
	}
    }

    // 'ascending' should be ignored according to Qt docs
    //
    return (thisVal > otherVal) ? 1 : -1;

    //!!! how to use percussion here?
}

//--------------------------------------------------

MidiBankListViewItem::MidiBankListViewItem(Rosegarden::DeviceId deviceId,
                                           int bankNb,
                                           QListViewItem* parent,
                                           QString name,
					   bool percussion, int msb, int lsb)
    : MidiDeviceListViewItem(deviceId, parent, name, percussion, msb, lsb),
      m_bankNb(bankNb)
{
}

void MidiBankListViewItem::setPercussion(bool percussion)
{
    setText(1, QString(percussion ? i18n("Yes") : i18n("No")));
}

void MidiBankListViewItem::setMSB(int msb)
{
    setText(2, QString().setNum(msb));
}

void MidiBankListViewItem::setLSB(int lsb)
{
    setText(3, QString().setNum(lsb));
}

int MidiBankListViewItem::compare(QListViewItem *i, int col, bool ascending) const
{
    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(i);
    if (!bankItem || (col != 2 && col != 3)) {
	return MidiDeviceListViewItem::compare(i, col, ascending);
    }

    int thisVal = text(col).toInt(),
        otherVal = bankItem->text(col).toInt();

    if (thisVal == otherVal) {
        if (col == 2) { // if sorting on MSB, suborder with LSB
            return compare(i, 3, ascending);
	} else {
            return 0;
	}
    }

    // 'ascending' should be ignored according to Qt docs
    //
    return 
	thisVal >  otherVal ? 1 :
	thisVal == otherVal ? 0	:
	-1;
    
}

//--------------------------------------------------

MidiProgramsEditor::MidiProgramsEditor(BankEditorDialog* bankEditor,
                                       QWidget* parent,
                                       const char* name)
    : QVGroupBox(i18n("Bank and Program details"),
                 parent, name),
      m_bankEditor(bankEditor),
      m_mainFrame(new QFrame(this)),
      m_bankName(new QLabel(m_mainFrame)),
      m_percussion(new QCheckBox(m_mainFrame)),
      m_msb(new QSpinBox(m_mainFrame)),
      m_lsb(new QSpinBox(m_mainFrame)),
      m_bankList(bankEditor->getBankList()),
      m_programList(bankEditor->getProgramList()),
      m_oldBank(false, 0, 0)
{

    QGridLayout *gridLayout = new QGridLayout(m_mainFrame,
                                              4,  // rows
                                              6,  // cols
                                              2); // margin
 


    /*
    gridLayout->addWidget(new QLabel(i18n("Bank Name"), m_mainFrame),
                          0, 0, AlignLeft);
                          */
    QFont boldFont = m_bankName->font();
    boldFont.setBold(true);
    m_bankName->setFont(boldFont);

    gridLayout->addMultiCellWidget(m_bankName, 0, 0, 0, 1, AlignLeft);

    gridLayout->addWidget(new QLabel(i18n("Percussion"), m_mainFrame),
                          1, 0, AlignLeft);
    gridLayout->addWidget(m_percussion, 1, 1, AlignLeft);
    connect(m_percussion, SIGNAL(clicked()),
	    this, SLOT(slotNewPercussion()));

    gridLayout->addWidget(new QLabel(i18n("MSB Value"), m_mainFrame),
                          2, 0, AlignLeft);
    m_msb->setMinValue(0);
    m_msb->setMaxValue(127);
    gridLayout->addWidget(m_msb, 2, 1, AlignLeft);

    QToolTip::add(m_msb,
            i18n("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    QToolTip::add(m_lsb,
            i18n("Selects a LSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    connect(m_msb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewMSB(int)));

    gridLayout->addWidget(new QLabel(i18n("LSB Value"), m_mainFrame),
                          3, 0, AlignLeft);
    m_lsb->setMinValue(0);
    m_lsb->setMaxValue(127);
    gridLayout->addWidget(m_lsb, 3, 1, AlignLeft);

    connect(m_lsb, SIGNAL(valueChanged(int)),
            this, SLOT(slotNewLSB(int)));

    // Librarian
    //
    QGroupBox *groupBox = new QGroupBox(2,
                                        Qt::Horizontal,
                                        i18n("Librarian"),
                                        m_mainFrame);
    gridLayout->addMultiCellWidget(groupBox, 0, 3, 3, 5);


    new QLabel(i18n("Name"), groupBox);
    m_librarian = new QLabel(groupBox);

    new QLabel(i18n("Email"), groupBox);
    m_librarianEmail = new QLabel(groupBox);

    QToolTip::add(groupBox,
                  i18n("The librarian maintains the generic Bank and Program information for this device.\nIf you've made modifications to a Bank to suit your own device it might be worth\nliaising with the librarian in order to publish your Bank information for the benefit\nof others."));

    QTabWidget* tabw = new QTabWidget(this);

    tabw->setMargin(10);

    QHBox *progHBox;
    QVBox *progVBox;
    QHBox *numBox;
    QLabel *label;
    
    unsigned int tabs = 4;
    unsigned int cols = 2;
    unsigned int labelId = 0;

    for (unsigned int tab = 0; tab < tabs; ++tab)
    {
	progHBox = new QHBox(tabw);

	for (unsigned int col = 0; col < cols; ++col)
	{
	    progVBox = new QVBox(progHBox);

	    for (unsigned int row = 0; row < 128/(tabs*cols); ++row)
	    {
		numBox = new QHBox(progVBox);
		label = new QLabel(QString("%1").arg(labelId + 1), numBox);
		label->setFixedWidth(40);
		label->setAlignment(AlignHCenter);

		KLineEdit* lineEdit = new KLineEdit(numBox, label->text().data());
		lineEdit->setMinimumWidth(110);
		lineEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
		lineEdit->setCompletionObject(&m_completion);
		m_programNames.push_back(lineEdit);
		
		connect(m_programNames[labelId],
			SIGNAL(textChanged(const QString&)),
			this,
			SLOT(slotProgramChanged(const QString&)));

		++labelId;
	    }
	}
	
	tabw->addTab(progHBox,
		     (tab == 0 ? i18n("Programs %1 - %2") : QString("%1 - %2")).
		     arg(tab * (128/tabs) + 1).
		     arg((tab + 1) * (128 / tabs)));
    }
}

Rosegarden::ProgramList
MidiProgramsEditor::getBankSubset(const MidiBank &bank)
{
    Rosegarden::ProgramList program;
    Rosegarden::ProgramList::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
    {
        if (it->getBank() == bank)
            program.push_back(*it);
    }

    return program;
}

Rosegarden::MidiBank*
MidiProgramsEditor::getCurrentBank()
{
    return m_currentBank;
}

void
MidiProgramsEditor::modifyCurrentPrograms(const MidiBank &oldBank,
					  const MidiBank &newBank)
{
    Rosegarden::ProgramList::iterator it;

    for (it = m_programList.begin(); it != m_programList.end(); it++)
    {
        if (it->getBank() == oldBank)
        {
	    *it = MidiProgram(newBank, it->getProgram(), it->getName());
        }
    }
}

void
MidiProgramsEditor::clearAll()
{
    blockAllSignals(true);

    for(unsigned int i = 0; i < m_programNames.size(); ++i)
        m_programNames[i]->clear();
    
    m_bankName->clear();
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
MidiProgramsEditor::populateBank(QListViewItem* item)
{
    RG_DEBUG << "MidiProgramsEditor::populateBank\n";

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);
    if (!bankItem) {
        RG_DEBUG << "MidiProgramsEditor::populateBank : not a bank item - returning\n";
        return;
    }
    
    Rosegarden::DeviceId deviceId = bankItem->getDeviceId();
    Rosegarden::MidiDevice* device = m_bankEditor->getMidiDevice(deviceId);
    if (!device) return;
    
    setEnabled(true);

    setBankName(item->text(0));

    RG_DEBUG << "MidiProgramsEditor::populateBank : bankItem->getBank = "
             << bankItem->getBank() << endl;

    m_currentBank = &(m_bankList[bankItem->getBank()]); // device->getBankByIndex(bankItem->getBank());

    blockAllSignals(true);

    // set the bank values
    m_percussion->setChecked(m_currentBank->isPercussion());
    m_msb->setValue(m_currentBank->getMSB());
    m_lsb->setValue(m_currentBank->getLSB());

    m_oldBank = *m_currentBank;

    // Librarian details
    //
    m_librarian->setText(strtoqstr(device->getLibrarianName()));
    m_librarianEmail->setText(strtoqstr(device->getLibrarianEmail()));

    Rosegarden::ProgramList programSubset = getBankSubset(*m_currentBank);
    Rosegarden::ProgramList::iterator it;

    for (unsigned int i = 0; i < m_programNames.size(); i++) {
        m_programNames[i]->clear();

        for (it = programSubset.begin(); it != programSubset.end(); it++) {
            if (it->getProgram() == i) {
                QString programName = strtoqstr(it->getName());
                m_completion.addItem(programName);
                m_programNames[i]->setText(programName);
                break;
            }
        }

        // show start of label
        m_programNames[i]->setCursorPosition(0);
    }

    blockAllSignals(false);
}

void
MidiProgramsEditor::resetMSBLSB()
{
    m_percussion->blockSignals(true);
    m_msb->blockSignals(true);
    m_lsb->blockSignals(true);

    m_percussion->setChecked(m_oldBank.isPercussion());
    m_msb->setValue(m_oldBank.getMSB());
    m_lsb->setValue(m_oldBank.getLSB());

    if (m_currentBank)
    {
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

    try
        {
            msb = ensureUniqueMSB(value, value > getCurrentBank()->getMSB());
        }
    catch(bool)
        {
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

    try
        {
            lsb = ensureUniqueLSB(value, value > getCurrentBank()->getLSB());
        }
    catch(bool)
        {
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
MidiProgramsEditor::slotProgramChanged(const QString& programName)
{
    const KLineEdit* lineEdit = dynamic_cast<const KLineEdit*>(sender());
    if (!lineEdit) {
        RG_DEBUG << "MidiProgramsEditor::slotProgramChanged() : %%% ERROR - signal sender is not a KLineEdit\n";
        return;
    }

    QString senderName = sender()->name();

    // Adjust value back to zero rated
    //
    unsigned int id = senderName.toUInt() - 1;

    RG_DEBUG << "BankEditorDialog::slotProgramChanged("
             << programName << ") : id = " << id << endl;

    Rosegarden::MidiProgram *program = getProgram(*getCurrentBank(), id);

    if (program == 0)
    {
        // Do nothing if program name is empty
        if (programName.isEmpty()) return;

        program = new Rosegarden::MidiProgram(*getCurrentBank(), id);
        m_programList.push_back(*program);

        // Sort the program list by id
        std::sort(m_programList.begin(), m_programList.end(), ProgramCmp());

        // Now, get with the program
        //
        program = getProgram(*getCurrentBank(), id);
    }
    else
    {
        // If we've found a program and the label is now empty
        // then remove it from the program list.
        //
        if (programName.isEmpty())
        {
            Rosegarden::ProgramList::iterator it = m_programList.begin();
            Rosegarden::ProgramList tmpProg;

            for (; it != m_programList.end(); it++)
            {
                if (((unsigned int)it->getProgram()) == id)
                {
                    m_programList.erase(it);
                    m_bankEditor->setModified(true);
                    RG_DEBUG << "deleting empty program (" << id << ")" << endl;
                    return;
                }
            }
        }
    }

    if (qstrtostr(programName) != program->getName())
    {
        program->setName(qstrtostr(programName));
        m_bankEditor->setModified(true);
    }
}

int
MidiProgramsEditor::ensureUniqueMSB(int msb, bool ascending)
{
    int newMSB = msb;
    while (banklistContains(MidiBank(m_percussion->isChecked(),
				     newMSB, m_lsb->value()))
           && newMSB < 128
           && newMSB > -1)
        if (ascending) newMSB++;
        else newMSB--;

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
        if (ascending) newLSB++;
        else newLSB--;

   if (newLSB == -1 || newLSB == 128)
       throw false;

    return newLSB;
}

bool
MidiProgramsEditor::banklistContains(const MidiBank &bank)
{
    Rosegarden::BankList::iterator it;

    for (it = m_bankList.begin(); it != m_bankList.end(); it++)
        if (*it == bank)
            return true;

    return false;
}

Rosegarden::MidiProgram*
MidiProgramsEditor::getProgram(const MidiBank &bank, int programNo)
{
    Rosegarden::ProgramList::iterator it = m_programList.begin();

    for (; it != m_programList.end(); it++)
    {
        if (it->getBank() == bank && it->getProgram() == programNo)
            return &(*it);
    }

    return 0;

}

void
MidiProgramsEditor::setBankName(const QString& s)
{
    m_bankName->setText(s);
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

//
//--------------------------------------------------
//

BankEditorDialog::BankEditorDialog(QWidget *parent,
                                   RosegardenGUIDoc *doc,
				   Rosegarden::DeviceId defaultDevice):
    KMainWindow(parent, "bankeditordialog"),
    m_studio(&doc->getStudio()),
    m_doc(doc),
    m_copyBank(Rosegarden::Device::NO_DEVICE, -1),
    m_modified(false),
    m_keepBankList(false),
    m_deleteAll(false),
    m_lastDevice(Rosegarden::Device::NO_DEVICE),
    m_updateDeviceList(false)
{
    QVBox* mainFrame = new QVBox(this);
    setCentralWidget(mainFrame);

    setCaption(i18n("Manage MIDI Banks and Programs"));

    QSplitter* splitter = new QSplitter(mainFrame);

    QFrame* btnBox = new QFrame(mainFrame);

    btnBox->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    QHBoxLayout* layout = new QHBoxLayout(btnBox, 4, 10);

    m_closeButton = new QPushButton(btnBox);
    m_applyButton = new QPushButton(i18n("Apply"), btnBox);
    m_resetButton = new QPushButton(i18n("Reset"), btnBox);

    layout->addStretch(10);
    layout->addWidget(m_applyButton);
    layout->addWidget(m_resetButton);
    layout->addSpacing(15);
    layout->addWidget(m_closeButton);
    layout->addSpacing(5);

    connect(m_applyButton, SIGNAL(clicked()),
            this, SLOT(slotApply()));
    connect(m_resetButton, SIGNAL(clicked()),
            this, SLOT(slotReset()));

    //
    // Left-side list view
    //
    QVBox* leftPart = new QVBox(splitter);
    m_listView = new KListView(leftPart);
    m_listView->addColumn(i18n("MIDI Device"));
    m_listView->addColumn(i18n("Percussion"));
    m_listView->addColumn(i18n("MSB"));
    m_listView->addColumn(i18n("LSB"));
    m_listView->setRootIsDecorated(true);
    m_listView->setShowSortIndicator(true);
    m_listView->setItemsRenameable(true);
    m_listView->restoreLayout(kapp->config(), "Bank Editor");

    QFrame *bankBox = new QFrame(leftPart);
    QGridLayout *gridLayout = new QGridLayout(bankBox, 3, 3, 6, 6);

    m_addBank        = new QPushButton(i18n("Add"), bankBox);
    m_deleteBank     = new QPushButton(i18n("Delete"), bankBox);
    m_deleteAllBanks = new QPushButton(i18n("Delete All"), bankBox);
    gridLayout->addWidget(m_addBank, 0, 0);
    gridLayout->addWidget(m_deleteBank, 0, 1);
    gridLayout->addWidget(m_deleteAllBanks, 0, 2);

    // Tips
    //
    QToolTip::add(m_addBank,
                  i18n("Add a Bank to the current device"));

    QToolTip::add(m_deleteBank,
                  i18n("Delete the current Bank"));

    QToolTip::add(m_deleteAllBanks,
                  i18n("Delete all Banks from the current Device"));

    m_importBanks = new QPushButton(i18n("Import"), bankBox);
    m_exportBanks = new QPushButton(i18n("Export"), bankBox);
    gridLayout->addWidget(m_importBanks, 1, 0);
    gridLayout->addWidget(m_exportBanks, 1, 1);
//    bankBox->addSpace(10); // spacer

    // Tips
    //
    QToolTip::add(m_importBanks,
            i18n("Import Bank and Program data from a Rosegarden file to the current Device"));
    QToolTip::add(m_exportBanks,
            i18n("Export all Device and Bank information to a Rosegarden format  interchange file"));

    m_copyPrograms = new QPushButton(i18n("Copy"), bankBox);
    m_pastePrograms = new QPushButton(i18n("Paste"), bankBox);
    gridLayout->addWidget(m_copyPrograms, 2, 0);
    gridLayout->addWidget(m_pastePrograms, 2, 1);

    // Tips
    //
    QToolTip::add(m_copyPrograms,
            i18n("Copy all Program names from current Bank to clipboard"));

    QToolTip::add(m_pastePrograms,
            i18n("Paste Program names from clipboard to current Bank"));

    connect(m_listView, SIGNAL(currentChanged(QListViewItem*)),
            this,       SLOT(slotPopulateDevice(QListViewItem*)));

    QFrame *vbox = new QFrame(splitter);
    QVBoxLayout *vboxLayout = new QVBoxLayout(vbox, 10, 10);

    m_programEditor = new MidiProgramsEditor(this, vbox);
    vboxLayout->addWidget(m_programEditor);

    m_programEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));

    m_optionBox  = new QVGroupBox(i18n("Options"), vbox);
    vboxLayout->addWidget(m_optionBox);

    QHBox *variationBox = new QHBox(m_optionBox);
    m_variationToggle = new QCheckBox(i18n("Show Variation list based on "), variationBox);
    m_variationCombo = new KComboBox(variationBox);
    m_variationCombo->insertItem(i18n("LSB"));
    m_variationCombo->insertItem(i18n("MSB"));

    // device/bank modification
    connect(m_listView, SIGNAL(itemRenamed             (QListViewItem*,const QString&,int)),
            this,       SLOT(slotModifyDeviceOrBankName(QListViewItem*,const QString&,int)));

    connect(m_addBank, SIGNAL(clicked()),
            this, SLOT(slotAddBank()));

    connect(m_deleteBank, SIGNAL(clicked()),
            this, SLOT(slotDeleteBank()));

    connect(m_deleteAllBanks, SIGNAL(clicked()),
            this, SLOT(slotDeleteAllBanks()));

    connect(m_importBanks, SIGNAL(clicked()),
            this, SLOT(slotImport()));

    connect(m_exportBanks, SIGNAL(clicked()),
            this, SLOT(slotExport()));

    connect(m_copyPrograms, SIGNAL(clicked()),
            this, SLOT(slotEditCopy()));

    connect(m_pastePrograms, SIGNAL(clicked()),
            this, SLOT(slotEditPaste()));

    connect(m_variationToggle, SIGNAL(clicked()),
	    this, SLOT(slotVariationToggled()));
    
    connect(m_variationCombo, SIGNAL(activated(int)),
	    this, SLOT(slotVariationChanged(int)));

    setupActions();

    m_doc->getCommandHistory()->attachView(actionCollection());
    connect(m_doc->getCommandHistory(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdate()));

    // Initialise the dialog
    //
    initDialog();
    setModified(false);

    // Check for no Midi devices and disable everything
    //
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;
    bool haveMidiPlayDevice = false;
    for (it = devices->begin(); it != devices->end(); ++it) {
        Rosegarden::MidiDevice *md =
            dynamic_cast<Rosegarden::MidiDevice *>(*it);
        if (md && md->getDirection() == Rosegarden::MidiDevice::Play) {
            haveMidiPlayDevice = true;
            break;
        }
    }
    if (!haveMidiPlayDevice) {
        leftPart->setDisabled(true);
        m_programEditor->setDisabled(true);
	m_optionBox->setDisabled(true);
    }

    if (defaultDevice != Rosegarden::Device::NO_DEVICE) {
	setCurrentDevice(defaultDevice);
    }

    setAutoSaveSettings(BankEditorConfigGroup, true);
}

BankEditorDialog::~BankEditorDialog()
{
    RG_DEBUG << "~BankEditorDialog()\n";

    m_listView->saveLayout(kapp->config(), BankEditorConfigGroup);

    if (m_doc) // see slotFileClose() for an explanation on why we need to test m_doc
        m_doc->getCommandHistory()->detachView(actionCollection());
}


void
BankEditorDialog::setupActions()
{
    KAction* close = KStdAction::close (this, SLOT(slotFileClose()),         actionCollection());

    m_closeButton->setText(close->text());
    connect(m_closeButton, SIGNAL(clicked()),
            this, SLOT(slotFileClose()));

    KStdAction::copy     (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste    (this, SLOT(slotEditPaste()),      actionCollection());

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

    createGUI("bankeditor.rc");
}


void
BankEditorDialog::initDialog()
{
    // Clear down
    //
    m_deviceNameMap.clear();
    m_listView->clear();

    // Fill list view
    //
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;

    for (it = devices->begin(); it != devices->end(); ++it)
    {
        if ((*it)->getType() == Rosegarden::Device::Midi)
        {
            Rosegarden::MidiDevice* midiDevice =
                dynamic_cast<Rosegarden::MidiDevice*>(*it);
            if (!midiDevice) continue;
            
            // skip read-only devices
            if (midiDevice->getDirection() == Rosegarden::MidiDevice::Record)
                continue;

            m_deviceNameMap[midiDevice->getId()] = midiDevice->getName();
            QString itemName = strtoqstr(midiDevice->getName());

            RG_DEBUG << "BankEditorDialog::initDialog - adding "
                     << itemName << endl;

            QListViewItem* deviceItem = new MidiDeviceListViewItem
                (midiDevice->getId(), m_listView, itemName);
            deviceItem->setOpen(true);

            populateDeviceItem(deviceItem, midiDevice);
        }
    }

    // Select the first Device
    //
    populateDevice(m_listView->firstChild());
    m_listView->setSelected(m_listView->firstChild(), true);

}

void
BankEditorDialog::updateDialog()
{
    // Update list view
    //
    Rosegarden::DeviceList *devices = m_studio->getDevices();
    Rosegarden::DeviceListIterator it;
    bool deviceLabelUpdate = false;

    for (it = devices->begin(); it != devices->end(); ++it) {

        if ((*it)->getType() != Rosegarden::Device::Midi) continue;
        
        Rosegarden::MidiDevice* midiDevice =
            dynamic_cast<Rosegarden::MidiDevice*>(*it);
        if (!midiDevice) continue;
            
        // skip read-only devices
        if (midiDevice->getDirection() == Rosegarden::MidiDevice::Record)
            continue;

        if (m_deviceNameMap.find(midiDevice->getId()) != m_deviceNameMap.end()) 
        {
            // Device already displayed but make sure the label is up to date
            //
            //
            QListViewItem* currentItem = m_listView->currentItem();

            if (currentItem)
            {
                MidiDeviceListViewItem* deviceItem = 
                    getParentDeviceItem(currentItem);

                if (deviceItem &&
                    deviceItem->getDeviceId() == midiDevice->getId())
                {
                    if (deviceItem->text(0) != strtoqstr(midiDevice->getName()))
                    {
                        deviceItem->setText(0, 
                                strtoqstr(midiDevice->getName()));
                        m_deviceNameMap[midiDevice->getId()] = 
                                midiDevice->getName();

                        /*
                       cout << "NEW TEXT FOR DEVICE " << midiDevice->getId()
                            << " IS " << midiDevice->getName() << endl;
                       cout << "LIST ITEM ID = "
                            << deviceItem->getDeviceId() << endl;
                            */

                        deviceLabelUpdate = true;
                    }

		    QListViewItem *child = deviceItem->firstChild();

		    while (child) {

			MidiBankListViewItem *bankItem =
			    dynamic_cast<MidiBankListViewItem *>(child);

			if (bankItem) {
			    bool percussion = (bankItem->text(1).lower() == "yes");
			    int msb = bankItem->text(2).toInt();
			    int lsb = bankItem->text(3).toInt();
			    std::string bankName =
				midiDevice->getBankName
				(Rosegarden::MidiBank(percussion, msb, lsb));
			    if (bankName != "" &&
				bankItem->text(0) != strtoqstr(bankName)) {
				bankItem->setText(0, strtoqstr(bankName));
			    }
			}

			child = child->nextSibling();
		    }
                }
            }

            continue;
        }

        m_deviceNameMap[midiDevice->getId()] = midiDevice->getName();
        QString itemName = strtoqstr(midiDevice->getName());

        RG_DEBUG << "BankEditorDialog::updateDialog - adding "
                 << itemName << endl;

        QListViewItem* deviceItem = new MidiDeviceListViewItem
            (midiDevice->getId(), m_listView, itemName);
        deviceItem->setOpen(true);

        populateDeviceItem(deviceItem, midiDevice);

    }


    // delete items which corresponding devices are no longer present,
    // and update the other ones
    //
    std::vector<MidiDeviceListViewItem*> itemsToDelete;

    MidiDeviceListViewItem* sibling = dynamic_cast<MidiDeviceListViewItem*>(m_listView->firstChild());
    
    while(sibling) {

        if (m_deviceNameMap.find(sibling->getDeviceId()) == m_deviceNameMap.end())
            itemsToDelete.push_back(sibling);
        else
            updateDeviceItem(sibling);

        sibling = dynamic_cast<MidiDeviceListViewItem*>(sibling->nextSibling());
    }

    for(unsigned int i = 0; i < itemsToDelete.size(); ++i) delete itemsToDelete[i];

    m_listView->sort();

    if (deviceLabelUpdate) emit deviceNamesChanged();
}

void
BankEditorDialog::setCurrentDevice(Rosegarden::DeviceId device)
{
    for (QListViewItem *item = m_listView->firstChild(); item;
	 item = item->nextSibling()) {
	MidiDeviceListViewItem *deviceItem =
	    dynamic_cast<MidiDeviceListViewItem *>(item);
	if (deviceItem && deviceItem->getDeviceId() == device) {
	    m_listView->setSelected(item, true);
	    break;
	}
    }
}

void
BankEditorDialog::populateDeviceItem(QListViewItem* deviceItem, Rosegarden::MidiDevice* midiDevice)
{
    clearItemChildren(deviceItem);

    QString itemName = strtoqstr(midiDevice->getName());

    Rosegarden::BankList banks = midiDevice->getBanks();
    // add banks for this device
    for (unsigned int i = 0; i < banks.size(); ++i) {
        RG_DEBUG << "BankEditorDialog::populateDeviceItem - adding "
                 << itemName << " - " << strtoqstr(banks[i].getName())
                 << endl;
        new MidiBankListViewItem(midiDevice->getId(), i, deviceItem,
                                 strtoqstr(banks[i].getName()),
				 banks[i].isPercussion(),
                                 banks[i].getMSB(), banks[i].getLSB());
    }
            
    
}

void
BankEditorDialog::updateDeviceItem(MidiDeviceListViewItem* deviceItem)
{
    Rosegarden::MidiDevice* midiDevice = getMidiDevice(deviceItem->getDeviceId());
    if (!midiDevice) {
        RG_DEBUG << "BankEditorDialog::updateDeviceItem : WARNING no midi device for this item\n";
        return;
    }

    QString itemName = strtoqstr(midiDevice->getName());

    Rosegarden::BankList banks = midiDevice->getBanks();
    // add missing banks for this device
    //
    for (unsigned int i = 0; i < banks.size(); ++i) {
        if (deviceItemHasBank(deviceItem, i)) continue;
        
        RG_DEBUG << "BankEditorDialog::updateDeviceItem - adding "
                 << itemName << " - " << strtoqstr(banks[i].getName())
                 << endl;
        new MidiBankListViewItem(midiDevice->getId(), i, deviceItem,
                                 strtoqstr(banks[i].getName()),
				 banks[i].isPercussion(),
                                 banks[i].getMSB(), banks[i].getLSB());
    }
            

    // delete banks which are no longer present
    //
    std::vector<MidiBankListViewItem*> childrenToDelete;

    MidiBankListViewItem* child = dynamic_cast<MidiBankListViewItem*>(deviceItem->firstChild());
    
    while(child) {
        if (child->getBank() >= int(banks.size()))
            childrenToDelete.push_back(child);
        else { // update the banks MSB/LSB which might have changed
	    child->setPercussion(banks[child->getBank()].isPercussion());
            child->setMSB(banks[child->getBank()].getMSB());
            child->setLSB(banks[child->getBank()].getLSB());
        }
        
        child = dynamic_cast<MidiBankListViewItem*>(child->nextSibling());
    }

    for(unsigned int i = 0; i < childrenToDelete.size(); ++i) delete childrenToDelete[i];
}

bool
BankEditorDialog::deviceItemHasBank(MidiDeviceListViewItem* deviceItem, int bankNb)
{
    MidiBankListViewItem *child = dynamic_cast<MidiBankListViewItem*>(deviceItem->firstChild());
    
    while(child) {
        if (child->getBank() == bankNb) return true;
        child = dynamic_cast<MidiBankListViewItem*>(child->nextSibling());
    }
    
    return false;
}


void
BankEditorDialog::clearItemChildren(QListViewItem* item)
{
    QListViewItem* child = 0;
    
    while((child = item->firstChild())) delete child;
}


Rosegarden::MidiDevice*
BankEditorDialog::getCurrentMidiDevice()
{
    return getMidiDevice(m_listView->currentItem());
}

void
BankEditorDialog::checkModified()
{
    if (!m_modified) return;

    setModified(false);

    //     // then ask if we want to apply the changes

    //     int reply = KMessageBox::questionYesNo(this,
    //                                            i18n("Apply pending changes?"));

    ModifyDeviceCommand *command;
    Rosegarden::MidiDevice *device = getMidiDevice(m_lastDevice);
    if (!device) {
	RG_DEBUG << "%%% WARNING : BankEditorDialog::checkModified() - NO MIDI DEVICE for device "
		 << m_lastDevice << endl;
	return;
    }

    if (m_bankList.size() == 0 && m_programList.size() == 0) {

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail(),
					  0,
                                          0,
                                          0,
					  0,
                                          true, // overwrite
					  true); // rename

    } else {

	Rosegarden::MidiDevice::VariationType variation =
	    Rosegarden::MidiDevice::NoVariations;
	if (m_variationToggle->isChecked()) {
	    if (m_variationCombo->currentItem() == 0) {
		variation = Rosegarden::MidiDevice::VariationFromLSB;
	    } else {
		variation = Rosegarden::MidiDevice::VariationFromMSB;
	    }
	}

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail(),
					  &variation,
                                          &m_bankList,
                                          &m_programList,
					  0,
                                          true,
					  true);
    }

    addCommandToHistory(command);

    setModified(false);
}

void
BankEditorDialog::slotPopulateDevice(QListViewItem* item)
{
    RG_DEBUG << "BankEditorDialog::slotPopulateDevice" << endl;

    if (!item) return;

    checkModified();

    populateDevice(item);
}

void
BankEditorDialog::populateDevice(QListViewItem* item)
{
    RG_DEBUG << "BankEditorDialog::populateDevice\n";

    if (!item) return;

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);

    if (!bankItem) {

        // Ensure we fill these lists for the new device
        //
        MidiDeviceListViewItem* deviceItem = getParentDeviceItem(item);

        m_lastDevice = deviceItem->getDeviceId();

        Rosegarden::MidiDevice *device = getMidiDevice(deviceItem);
        if (!device) {
            RG_DEBUG << "BankEditorDialog::populateDevice - no device for this item\n";
            return;
        }

        m_bankList = device->getBanks();
        setProgramList(device);

        RG_DEBUG << "BankEditorDialog::populateDevice : not a bank item - disabling" << endl;
        m_deleteBank->setEnabled(false);
        m_copyPrograms->setEnabled(false);
        m_pastePrograms->setEnabled(false);

	m_variationToggle->setChecked(device->getVariationType() !=
				      Rosegarden::MidiDevice::NoVariations);
	m_variationCombo->setEnabled(m_variationToggle->isChecked());
	m_variationCombo->setCurrentItem
	    (device->getVariationType() ==
	     Rosegarden::MidiDevice::VariationFromLSB ? 0 : 1);

        stateChanged("on_bank_item", KXMLGUIClient::StateReverse);
        m_programEditor->clearAll();

        return;
    }

    stateChanged("on_bank_item");
    
    m_deleteBank->setEnabled(true);
    m_copyPrograms->setEnabled(true);

    if (m_copyBank.first != Rosegarden::Device::NO_DEVICE)
        m_pastePrograms->setEnabled(true);

    Rosegarden::MidiDevice *device = getMidiDevice(bankItem->getDeviceId());

    if (!m_keepBankList || m_bankList.size() == 0)
        m_bankList    = device->getBanks();
    else
        m_keepBankList = false;

    setProgramList(device);

    m_variationToggle->setChecked(device->getVariationType() !=
				  Rosegarden::MidiDevice::NoVariations);
    m_variationCombo->setEnabled(m_variationToggle->isChecked());
    m_variationCombo->setCurrentItem
	(device->getVariationType() ==
	 Rosegarden::MidiDevice::VariationFromLSB ? 0 : 1);

    m_lastBank = m_bankList[bankItem->getBank()];

    m_programEditor->populateBank(item);
	
    m_lastDevice = bankItem->getDeviceId();
}

void
BankEditorDialog::slotApply()
{
    RG_DEBUG << "BankEditorDialog::slotApply()\n";

    ModifyDeviceCommand *command;
    
    Rosegarden::MidiDevice *device = getMidiDevice(m_lastDevice);

    // Make sure that we don't delete all the banks and programs
    // if we've not populated them here yet.
    //
    if (m_bankList.size() == 0 && m_programList.size() == 0 &&
            m_deleteAll == false)
    {
        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail(),
					  0,
                                          0,
                                          0,
					  0,
                                          true,
					  true);
    }
    else
    {
	Rosegarden::MidiDevice::VariationType variation =
	    Rosegarden::MidiDevice::NoVariations;
	if (m_variationToggle->isChecked()) {
	    if (m_variationCombo->currentItem() == 0) {
		variation = Rosegarden::MidiDevice::VariationFromLSB;
	    } else {
		variation = Rosegarden::MidiDevice::VariationFromMSB;
	    }
	}

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail(),
					  &variation,
                                          &m_bankList,
                                          &m_programList,
					  0,
                                          true,
					  true);

    }
    addCommandToHistory(command);

    // Our freaky fudge to update instrument/device names externally
    //
    if (m_updateDeviceList)
    {
        emit deviceNamesChanged();
        m_updateDeviceList = false;
    }

    setModified(false);
}

void
BankEditorDialog::slotReset()
{
    resetProgramList();
    m_programEditor->resetMSBLSB();
    m_programEditor->populateBank(m_listView->currentItem());
    updateDialog();

    setModified(false);
}

void
BankEditorDialog::resetProgramList()
{
    m_programList = m_oldProgramList;
}

void
BankEditorDialog::setProgramList(Rosegarden::MidiDevice *device)
{
    m_programList = device->getPrograms();
    m_oldProgramList = m_programList;
}

void
BankEditorDialog::slotUpdate()
{
    updateDialog();
}


MidiDeviceListViewItem*
BankEditorDialog::getParentDeviceItem(QListViewItem* item)
{
    if (!item) return 0;

    if (dynamic_cast<MidiBankListViewItem*>(item)) // this is a bank item,
        // go up to the parent device item
        item = item->parent();

    if (!item) {
        RG_DEBUG << "BankEditorDialog::getParentDeviceItem : missing parent device item for bank item - this SHOULD NOT HAPPEN" << endl;
        return 0;
    }

    return dynamic_cast<MidiDeviceListViewItem*>(item);
}


void
BankEditorDialog::slotAddBank()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);
    Rosegarden::MidiDevice *device = getMidiDevice(currentItem);
   
    if (device)
    {
        // If the bank and program lists are empty then try to
        // populate them.
        //
        if (m_bankList.size() == 0 && m_programList.size() == 0)
        {
            m_bankList = device->getBanks();
            setProgramList(device);
        }

        std::pair<int, int> bank = getFirstFreeBank(m_listView->currentItem());

        Rosegarden::MidiBank newBank(false,
				     bank.first, bank.second,
				     qstrtostr(i18n("<new bank>")));
        m_bankList.push_back(newBank);

        QListViewItem* newBankItem =
            new MidiBankListViewItem(deviceItem->getDeviceId(),
                                     m_bankList.size() - 1,
                                     deviceItem,
                                     strtoqstr(newBank.getName()),
				     newBank.isPercussion(),
                                     newBank.getMSB(), newBank.getLSB());
        keepBankListForNextPopulate();
        m_listView->setCurrentItem(newBankItem);

        slotApply();
        selectDeviceItem(device);
    }
}

void
BankEditorDialog::slotDeleteBank()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(currentItem);

    Rosegarden::MidiDevice *device = getMidiDevice(currentItem);

    if (device)
    {
        int currentBank = bankItem->getBank();

        int reply =
            KMessageBox::warningYesNo(this, i18n("Really delete this bank?"));

        if (reply == KMessageBox::Yes)
        {
            int newBank = currentBank - 1;
        
            if (newBank < 0) newBank = 0;

	    MidiBank bank = m_bankList[currentBank];

            // Copy across all programs that aren't in the doomed bank
            //
            Rosegarden::ProgramList::iterator it;
            Rosegarden::ProgramList tempList;
            for (it = m_programList.begin(); it != m_programList.end(); it++)
                if (!(it->getBank() == bank))
                    tempList.push_back(*it);

            // Erase the bank and repopulate
            //
            Rosegarden::BankList::iterator er =
                m_bankList.begin();
            er += currentBank;
            m_bankList.erase(er);
            m_programList = tempList;
            keepBankListForNextPopulate();

            // the listview automatically selects a new current item
            delete currentItem;

            // Don't allow pasting from this defunct device
            //
            if (m_copyBank.first == bankItem->getDeviceId() &&
                m_copyBank.second == bankItem->getBank())
            {
                m_pastePrograms->setEnabled(false);
                m_copyBank = std::pair<Rosegarden::DeviceId, int>
                    (Rosegarden::Device::NO_DEVICE, -1);
            }

            slotApply();
            selectDeviceItem(device);

        }
    }
}

void
BankEditorDialog::slotDeleteAllBanks()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();
    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);
    Rosegarden::MidiDevice *device = getMidiDevice(deviceItem);

    QString question = i18n("Really delete all banks for ") +
                       strtoqstr(device->getName()) + QString(" ?");

    int reply = KMessageBox::warningYesNo(this, question);

    if (reply == KMessageBox::Yes)
    {

        // erase all bank items
        QListViewItem* child = 0;
        while((child = deviceItem->firstChild())) delete child;

        m_bankList.clear();
        m_programList.clear();

        // Don't allow pasting from this defunct device
        //
        if (m_copyBank.first == deviceItem->getDeviceId())
        {
            m_pastePrograms->setEnabled(false);
            m_copyBank = std::pair<Rosegarden::DeviceId, int>
                (Rosegarden::Device::NO_DEVICE, -1);
        }

        // Urgh, we have this horrible flag that we're using to frig this.
        // (we might not need this anymore but I'm too scared to remove it
        // now).
        //
        m_deleteAll = true;
        slotApply();
        m_deleteAll = false;

        selectDeviceItem(device);

    }
}

Rosegarden::MidiDevice*
BankEditorDialog::getMidiDevice(Rosegarden::DeviceId id)
{
    Rosegarden::Device *device = m_studio->getDevice(id);
    Rosegarden::MidiDevice *midiDevice =
	dynamic_cast<Rosegarden::MidiDevice *>(device);

    /*
    if (device) {
	if (!midiDevice) {
	    std::cerr << "ERROR: BankEditorDialog::getMidiDevice: device "
		      << id << " is not a MIDI device" << std::endl;
	}
    } else {
	std::cerr
	    << "ERROR: BankEditorDialog::getMidiDevice: no such device as "
	    << id << std::endl;
    }
    */

    return midiDevice;
}

Rosegarden::MidiDevice*
BankEditorDialog::getMidiDevice(QListViewItem* item)
{
    MidiDeviceListViewItem* deviceItem =
        dynamic_cast<MidiDeviceListViewItem*>(item);
    if (!deviceItem) return 0;

    return getMidiDevice(deviceItem->getDeviceId());
}

// Try to find a unique MSB/LSB pair for a new bank
//
std::pair<int, int>
BankEditorDialog::getFirstFreeBank(QListViewItem* item)
{
    int msb = 0;
    int lsb = 0;

    Rosegarden::MidiDevice *device = getMidiDevice(item);

    //!!! percussion? this is actually only called in the expectation
    // that percussion==false at the moment

    if (device)
    {
        do
        {
            lsb = 0;
            while(m_programEditor->banklistContains(MidiBank(false, msb, lsb)) && lsb < 128)
                lsb++;
        }
        while (lsb == 128 && msb++);
    }

    return std::pair<int, int>(msb, lsb);
}

void
BankEditorDialog::slotModifyDeviceOrBankName(QListViewItem* item, const QString &label, int)
{
    RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName" << endl;

    MidiDeviceListViewItem* deviceItem =
        dynamic_cast<MidiDeviceListViewItem*>(item);
    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);
    
    if (bankItem) {

        // renaming a bank item

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
                 << "modify bank name to " << label << endl;

        if (m_bankList[bankItem->getBank()].getName() != qstrtostr(label)) {
            m_bankList[bankItem->getBank()].setName(qstrtostr(label));
            setModified(true);
        }
        
    } else if (deviceItem) {

        // renaming a device item

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
                 << "modify device name to " << label << endl;

        if (m_deviceNameMap[deviceItem->getDeviceId()] != qstrtostr(label)) {
            m_deviceNameMap[deviceItem->getDeviceId()] = qstrtostr(label);
            setModified(true);

            m_updateDeviceList = true;
        }
    }
    
}

void
BankEditorDialog::selectDeviceItem(Rosegarden::MidiDevice *device)
{
    QListViewItem *child = m_listView->firstChild();
    MidiDeviceListViewItem *midiDeviceItem;
    Rosegarden::MidiDevice *midiDevice;

    do
    {
        midiDeviceItem = dynamic_cast<MidiDeviceListViewItem*>(child);

        if (midiDeviceItem)
        {
            midiDevice = getMidiDevice(midiDeviceItem);

            if (midiDevice == device)
            {
                m_listView->setSelected(child, true);
                return;
            }
        }

    }
    while ((child = child->nextSibling()));
}
void
BankEditorDialog::selectDeviceBankItem(Rosegarden::DeviceId deviceId,
                                       int bank)
{
    QListViewItem *deviceChild = m_listView->firstChild();
    QListViewItem *bankChild;
    int deviceCount = 0, bankCount = 0;

    do
    {
        bankChild = deviceChild->firstChild();

        MidiDeviceListViewItem *midiDeviceItem =
            dynamic_cast<MidiDeviceListViewItem*>(deviceChild);

        if (midiDeviceItem && bankChild)
        {
            do
            {
                if (deviceId == midiDeviceItem->getDeviceId() &
                    bank == bankCount)
                {
                    m_listView->setSelected(bankChild, true);
                    return;
                }
                bankCount++;

            } while ((bankChild = bankChild->nextSibling()));
        }

        deviceCount++;
        bankCount = 0;
    }
    while ((deviceChild = deviceChild->nextSibling()));
}

void
BankEditorDialog::slotVariationToggled()
{
    setModified(true);
    m_variationCombo->setEnabled(m_variationToggle->isChecked());
}

void
BankEditorDialog::slotVariationChanged(int)
{
    setModified(true);
}

void
BankEditorDialog::setModified(bool modified)
{
    RG_DEBUG << "BankEditorDialog::setModified("
             << modified << ")" << endl;

    if (modified) {

        m_applyButton->setEnabled(true);
        m_resetButton->setEnabled(true);
        m_closeButton->setEnabled(false);
        m_listView->setEnabled(false);

    } else {

        m_applyButton->setEnabled(false);
        m_resetButton->setEnabled(false);
        m_closeButton->setEnabled(true);
        m_listView->setEnabled(true);

    }

    m_modified = modified;
}

void
BankEditorDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
    setModified(false);
}


MultiViewCommandHistory*
BankEditorDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

void
BankEditorDialog::slotImport()
{
    QString deviceDir = KGlobal::dirs()->findResource("appdata", "library/");
    QDir dir(deviceDir);
    if (!dir.exists()) {
        deviceDir = ":ROSEGARDENDEVICE";
    } else {
        deviceDir = "file://" + deviceDir;
    }

    KURL url = KFileDialog::getOpenURL
        (
#if KDE_VERSION >= 306
         deviceDir,
#else
         QString::null,
#endif
         "*.rgd *.rg *.sf2|Rosegarden and Soundfont files\n*.rgd|Rosegarden device file\n*.rg|Rosegarden file\n*.sf2|SoundFont file\n*|All files",
         this, i18n("Import Banks from Device in File"));

    if (url.isEmpty()) return;

    ImportDeviceDialog *dialog = new ImportDeviceDialog(this, url);
    if (dialog->exec() == QDialog::Accepted) {

	MidiDeviceListViewItem* deviceItem =
	    dynamic_cast<MidiDeviceListViewItem*>
	    (m_listView->selectedItem());

	if (!deviceItem) {
	    KMessageBox::error(this, "Some internal error: cannot locate selected device");
	    return;
	}

	KCommand *command = 0;

	Rosegarden::BankList banks(dialog->getBanks());
	Rosegarden::ProgramList programs(dialog->getPrograms());
	Rosegarden::ControlList controls(dialog->getControllers());
	Rosegarden::MidiDevice::VariationType variation(dialog->getVariationType());
	std::string librarianName(dialog->getLibrarianName());
	std::string librarianEmail(dialog->getLibrarianEmail());

	// don't record the librarian when
	// merging banks -- it's misleading.
	// (also don't use variation type)
	if (!dialog->shouldOverwriteBanks()) {
	    librarianName = "";
	    librarianEmail = "";
	}

	command =
	    new ModifyDeviceCommand(
		m_studio,
		deviceItem->getDeviceId(),
		dialog->getDeviceName(),
		librarianName,
		librarianEmail,
		dialog->shouldOverwriteBanks() ? &variation : 0,
		dialog->shouldImportBanks() ? &banks : 0,
		dialog->shouldImportBanks() ? &programs : 0,
		dialog->shouldImportControllers() ? &controls : 0,
		dialog->shouldOverwriteBanks(),
		dialog->shouldRename());

	addCommandToHistory(command);

	// No need to redraw the dialog, this is done by
	// slotUpdate, signalled by the MultiViewCommandHistory
	Rosegarden::MidiDevice *device = getMidiDevice(deviceItem);
	if (device)
	    selectDeviceItem(device);
    }

    updateDialog();
}

// Store the current bank for copy
//
void
BankEditorDialog::slotEditCopy()
{
    MidiBankListViewItem* bankItem
        = dynamic_cast<MidiBankListViewItem*>(m_listView->currentItem());

    if (bankItem)
    {
        m_copyBank = std::pair<Rosegarden::DeviceId, int>(bankItem->getDeviceId(),
                                                          bankItem->getBank());
        m_pastePrograms->setEnabled(true);
    }
}

void
BankEditorDialog::slotEditPaste()
{
    MidiBankListViewItem* bankItem
        = dynamic_cast<MidiBankListViewItem*>(m_listView->currentItem());

    if (bankItem)
    {
        // Get the full program and bank list for the source device
        //
        Rosegarden::MidiDevice *device = getMidiDevice(m_copyBank.first);
        std::vector<Rosegarden::MidiBank> tempBank = device->getBanks();

        Rosegarden::ProgramList::iterator it;
        std::vector<Rosegarden::MidiProgram> tempProg;

        // Remove programs that will be overwritten
        //
        for (it = m_programList.begin(); it != m_programList.end(); it++)
        {
	    if (!(it->getBank() == m_lastBank))
                tempProg.push_back(*it);
        }
        m_programList = tempProg;

        // Now get source list and msb/lsb
        //
        tempProg = device->getPrograms();
	MidiBank sourceBank = tempBank[m_copyBank.second];

        // Add the new programs
        //
        for (it = tempProg.begin(); it != tempProg.end(); it++)
        {
            if (it->getBank() == sourceBank)
            {
                // Insert with new MSB and LSB
                //
                Rosegarden::MidiProgram copyProgram(m_lastBank,
						    it->getProgram(),
						    it->getName());

                m_programList.push_back(copyProgram);
            }
        }

        // Save these for post-apply 
        //
	Rosegarden::DeviceId devPos = bankItem->getDeviceId();
        int bankPos = bankItem->getBank();

        slotApply();

        // Select same bank
        //
        selectDeviceBankItem(devPos, bankPos);
    }
}

void
BankEditorDialog::slotExport()
{
    QString extension = "rgd";

    QString name =
        KFileDialog::getSaveFileName(":ROSEGARDEN",
               (extension.isEmpty() ? QString("*") : ("*." + extension)),
               this,
               i18n("Export Device as..."));

    // Check for the existence of the name
    if (name.isEmpty()) return;

    // Append extension if we don't have one
    //
    if (!extension.isEmpty())
    {
        if (!name.endsWith("." + extension))
        {
            name += "." + extension;
        }
    }

    QFileInfo info(name);

    if (info.isDir())
    {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return;
    }

    if (info.exists())
    {
        int overwrite = KMessageBox::questionYesNo
            (this, i18n("The specified file exists.  Overwrite?"));

        if (overwrite != KMessageBox::Yes) return;

    }


    //std::cout << "GOT FILENAME = " << name << std::endl;
    m_doc->exportStudio(name);

}

void
BankEditorDialog::slotFileClose()
{
    RG_DEBUG << "BankEditorDialog::slotFileClose()\n";

    // We need to do this because we might be here due to a
    // documentAboutToChange signal, in which case the document won't
    // be valid by the time we reach the dtor, since it will be
    // triggered when the closeEvent is actually processed.
    //
    m_doc->getCommandHistory()->detachView(actionCollection());
    m_doc = 0;
    close();
}

void
BankEditorDialog::closeEvent(QCloseEvent *e)
{
    emit closing();
    KMainWindow::closeEvent(e);
}

const char* const BankEditorDialog::BankEditorConfigGroup = "Bank Editor";

// ------------------------ RemapInstrumentDialog -----------------------
//
RemapInstrumentDialog::RemapInstrumentDialog(QWidget *parent,
                                             RosegardenGUIDoc *doc):
    KDialogBase(parent, "", true, i18n("Remap Instrument assigments..."),
                Ok | Apply | Cancel),
    m_doc(doc)
{
    QVBox *vBox = makeVBoxMainWidget();

    m_buttonGroup = new QButtonGroup(1, Qt::Horizontal,
                                     i18n("Device or Instrument"),
                                     vBox);

    new QLabel(i18n("Remap Tracks by all Instruments on a Device or by single Instrument"), m_buttonGroup);
    m_deviceButton = new QRadioButton(i18n("Device"), m_buttonGroup);
    m_instrumentButton = new QRadioButton(i18n("Instrument"), m_buttonGroup);


    connect(m_buttonGroup, SIGNAL(released(int)),
            this, SLOT(slotRemapReleased(int)));

    QGroupBox *groupBox = new QGroupBox(2, Qt::Horizontal,
                                        i18n("Choose Source and Destination"),
                                        vBox);

    new QLabel(i18n("From"), groupBox);
    new QLabel(i18n("To"), groupBox);
    m_fromCombo = new KComboBox(groupBox);
    m_toCombo = new KComboBox(groupBox);

    m_buttonGroup->setButton(0);
    populateCombo(0);
}

void
RemapInstrumentDialog::populateCombo(int id)
{
    m_fromCombo->clear();
    m_toCombo->clear();
    Rosegarden::Studio *studio = &m_doc->getStudio();

    if (id == 0)
    {
        Rosegarden::DeviceList *devices = studio->getDevices();
        Rosegarden::DeviceListIterator it;
	m_devices.clear();

        for (it = devices->begin(); it != devices->end(); it++)
        {
	    Rosegarden::MidiDevice *md =
		dynamic_cast<Rosegarden::MidiDevice *>(*it);

	    if (md && md->getDirection() == Rosegarden::MidiDevice::Play)
	    {
		m_devices.push_back(*it);
		m_fromCombo->insertItem(strtoqstr((*it)->getName()));
		m_toCombo->insertItem(strtoqstr((*it)->getName()));
	    }
        }

        if (m_devices.size() == 0)
        {
            m_fromCombo->insertItem(i18n("<no devices>"));
            m_toCombo->insertItem(i18n("<no devices>"));
        }
    }
    else 
    {
        m_instruments = studio->getPresentationInstruments();
        Rosegarden::InstrumentList::iterator it = m_instruments.begin();

        for (; it != m_instruments.end(); it++)
        {
            m_fromCombo->insertItem(strtoqstr((*it)->getPresentationName()));
            m_toCombo->insertItem(strtoqstr((*it)->getPresentationName()));
        }
    }
}


void
RemapInstrumentDialog::slotRemapReleased(int id)
{
    populateCombo(id);
}

void
RemapInstrumentDialog::slotOk()
{
    slotApply();
    accept();
}

void
RemapInstrumentDialog::slotApply()
{
    if (m_buttonGroup->id(m_buttonGroup->selected()) == 0) // devices
    {
        ModifyDeviceMappingCommand *command =
            new ModifyDeviceMappingCommand
	    (m_doc,
	     m_devices[m_fromCombo->currentItem()]->getId(),
	     m_devices[m_toCombo->currentItem()]->getId());
        addCommandToHistory(command);
    }
    else // instruments
    {
        ModifyInstrumentMappingCommand *command =
            new ModifyInstrumentMappingCommand
	    (m_doc,
	     m_instruments[m_fromCombo->currentItem()]->getId(),
	     m_instruments[m_toCombo->currentItem()]->getId());
        addCommandToHistory(command);
    }

    emit applyClicked();
}

void
RemapInstrumentDialog::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
}


MultiViewCommandHistory*
RemapInstrumentDialog::getCommandHistory()
{
    return m_doc->getCommandHistory();
}

