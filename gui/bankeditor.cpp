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
#include <qpopupmenu.h>
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
#include "SoftSynthDevice.h"
#include "MidiProgram.h"

using Rosegarden::MidiBank;
using Rosegarden::MidiProgram;


MidiDeviceListViewItem::MidiDeviceListViewItem(Rosegarden::DeviceId deviceId,
                                               QListView* parent, QString name)
    : KListViewItem(parent, name),
      m_deviceId(deviceId)
{
}

MidiDeviceListViewItem::MidiDeviceListViewItem(Rosegarden::DeviceId deviceId,
                                               QListViewItem* parent, QString name,
					       bool percussion,
                                               int msb, int lsb)
    : KListViewItem(parent, name,
		    QString(percussion ? i18n("Percussion Bank") : i18n("Bank")),
		    QString().setNum(msb), QString().setNum(lsb)),
      m_deviceId(deviceId)
{
}

MidiDeviceListViewItem::MidiDeviceListViewItem(Rosegarden::DeviceId deviceId,
                                               QListViewItem* parent, QString name)
    : KListViewItem(parent, name, i18n("Key Mapping"), "", ""),
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
      m_percussion(percussion),
      m_bankNb(bankNb)
{
}

void MidiBankListViewItem::setPercussion(bool percussion)
{
    m_percussion = percussion;
    setText(1, QString(percussion ? i18n("Percussion Bank") : i18n("Bank")));
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

    if (!bankItem) {
	MidiKeyMapListViewItem *keyItem = dynamic_cast<MidiKeyMapListViewItem *>(i);
	if (keyItem) return -1; // banks before key maps
    }

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

MidiKeyMapListViewItem::MidiKeyMapListViewItem(Rosegarden::DeviceId deviceId,
					       QListViewItem* parent,
					       QString name)
    : MidiDeviceListViewItem(deviceId, parent, name),
      m_name(name)
{
    setText(1, i18n("Key Mapping"));
}

int MidiKeyMapListViewItem::compare(QListViewItem *i, int col, bool ascending) const
{
    if (dynamic_cast<MidiBankListViewItem *>(i)) {
	return 1; // banks before key maps
    }

    return MidiDeviceListViewItem::compare(i, col, ascending);
}

//--------------------------------------------------

NameSetEditor::NameSetEditor(BankEditorDialog* bankEditor,
			     QString title,
			     QWidget* parent,
			     const char* name,
			     QString headingPrefix,
			     bool showEntryButtons)
    : QVGroupBox(title, parent, name),
      m_bankEditor(bankEditor),
      m_mainFrame(new QFrame(this))
{
    m_mainLayout = new QGridLayout(m_mainFrame,
				   4,  // rows
				   6,  // cols
				   2); // margin
 
    // Librarian
    //
    QGroupBox *groupBox = new QGroupBox(2,
                                        Qt::Horizontal,
                                        i18n("Librarian"),
                                        m_mainFrame);
    m_mainLayout->addMultiCellWidget(groupBox, 0, 2, 3, 5);

    new QLabel(i18n("Name"), groupBox);
    m_librarian = new QLabel(groupBox);

    new QLabel(i18n("Email"), groupBox);
    m_librarianEmail = new QLabel(groupBox);

    QToolTip::add(groupBox,
                  i18n("The librarian maintains the Rosegarden device data for this device.\nIf you've made modifications to suit your own device, it might be worth\nliaising with the librarian in order to publish your information for the benefit\nof others."));

    QTabWidget* tabw = new QTabWidget(this);

    tabw->setMargin(10);

    QHBox *h;
    QVBox *v;
    QHBox *numBox;
    
    unsigned int tabs = 4;
    unsigned int cols = 2;
    unsigned int labelId = 0;

    for (unsigned int tab = 0; tab < tabs; ++tab)
    {
	h = new QHBox(tabw);

	for (unsigned int col = 0; col < cols; ++col)
	{
	    v = new QVBox(h);

	    for (unsigned int row = 0; row < 128/(tabs*cols); ++row)
	    {
		numBox = new QHBox(v);
		QString numberText = QString("%1").arg(labelId + 1);

		QLabel *label = new QLabel(numberText, numBox);
		label->setFixedWidth(40);
		label->setAlignment(AlignCenter);

		if (showEntryButtons) {
		    QPushButton *button = new QPushButton("", numBox, numberText);
		    button->setMaximumWidth(40);
		    button->setMaximumHeight(20);
		    button->setFlat(true);
		    connect(button, SIGNAL(clicked()),
			    this, SLOT(slotEntryButtonPressed()));
		    m_entryButtons.push_back(button);
		}

		KLineEdit* lineEdit = new KLineEdit(numBox, numberText);
		lineEdit->setMinimumWidth(110);
		lineEdit->setCompletionMode(KGlobalSettings::CompletionAuto);
		lineEdit->setCompletionObject(&m_completion);
		m_names.push_back(lineEdit);
		
		connect(m_names[labelId],
			SIGNAL(textChanged(const QString&)),
			this,
			SLOT(slotNameChanged(const QString&)));

		++labelId;
	    }
	}
	
	tabw->addTab(h,
		     (tab == 0 ? headingPrefix + QString(" %1 - %2") :
		      QString("%1 - %2")).
		     arg(tab * (128/tabs) + 1).
		     arg((tab + 1) * (128 / tabs)));
    }
}

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
					      3,  // rows
                                              2,  // cols
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

    QToolTip::add(m_msb,
            i18n("Selects a MSB controller Bank number (MSB/LSB pairs are always unique for any Device)"));

    QToolTip::add(m_lsb,
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

    for(unsigned int i = 0; i < m_names.size(); ++i)
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
        return;
    }
    
    Rosegarden::DeviceId deviceId = bankItem->getDeviceId();
    m_device = m_bankEditor->getMidiDevice(deviceId);
    if (!m_device) return;
    
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

    Rosegarden::ProgramList programSubset = getBankSubset(*m_currentBank);
    Rosegarden::ProgramList::iterator it;

    QPixmap noKeyPixmap, keyPixmap;
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QString file = pixmapDir + "/toolbar/key-white.png";
    if (QFile(file).exists()) noKeyPixmap = QPixmap(file);
    file = pixmapDir + "/toolbar/key-green.png";
    if (QFile(file).exists()) keyPixmap = QPixmap(file);

    bool haveKeyMappings = m_currentBank->isPercussion()
			   && (m_device->getKeyMappings().size() > 0);

    for (unsigned int i = 0; i < m_names.size(); i++) {
	m_names[i]->clear();
	getEntryButton(i)->setEnabled(haveKeyMappings);
	getEntryButton(i)->setPixmap(noKeyPixmap);
	QToolTip::remove( getEntryButton(i) );
	
        for (it = programSubset.begin(); it != programSubset.end(); it++) {
            if (it->getProgram() == i) {

                QString programName = strtoqstr(it->getName());
                m_completion.addItem(programName);
                m_names[i]->setText(programName);

		if (m_device->getKeyMappingForProgram(*it)) {
		    getEntryButton(i)->setPixmap(keyPixmap);
		    QToolTip::add( getEntryButton(i), 
			i18n("Key Mapping: %1").arg(        
			m_device->getKeyMappingForProgram(*it)->getName() ) );
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
MidiProgramsEditor::slotNameChanged(const QString& programName)
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

    RG_DEBUG << "MidiProgramsEditor::slotNameChanged("
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

class BlahPopupMenu2 : public QPopupMenu
{
    // just to make itemHeight public
public:
    BlahPopupMenu2(QWidget *parent) : QPopupMenu(parent) { }
    using QPopupMenu::itemHeight;
};

void
MidiProgramsEditor::slotEntryButtonPressed()
{
    QPushButton* button = dynamic_cast<QPushButton*>(const_cast<QObject *>(sender()));
    if (!button) {
        RG_DEBUG << "MidiProgramsEditor::slotEntryButtonPressed() : %%% ERROR - signal sender is not a QPushButton\n";
        return;
    }

    QString senderName = button->name();

    if (!m_device) return;

    const Rosegarden::KeyMappingList &kml = m_device->getKeyMappings();
    if (kml.empty()) return;

    // Adjust value back to zero rated
    //
    unsigned int id = senderName.toUInt() - 1;
    Rosegarden::MidiProgram *program = getProgram(*getCurrentBank(), id);
    if (!program) return;
    m_currentMenuProgram = id;

    BlahPopupMenu2 *menu = new BlahPopupMenu2(button);

    const Rosegarden::MidiKeyMapping *currentMapping =
	m_device->getKeyMappingForProgram(*program);
    int currentEntry = 0;

    menu->insertItem(i18n("<no key mapping>"), this,
		     SLOT(slotEntryMenuItemSelected(int)), 0, 0);
    menu->setItemParameter(0, 0);

    for (int i = 0; i < kml.size(); ++i) {
	menu->insertItem(strtoqstr(kml[i].getName()),
			 this, SLOT(slotEntryMenuItemSelected(int)),
			 0, i+1);
	menu->setItemParameter(i+1, i+1);
	if (currentMapping && (kml[i] == *currentMapping)) currentEntry = i+1;
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
    if (!m_device) return;

    const Rosegarden::KeyMappingList &kml = m_device->getKeyMappings();
    if (kml.empty()) return;

    Rosegarden::MidiProgram *program = getProgram(*getCurrentBank(), m_currentMenuProgram);
    if (!program) return;
    
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
	QToolTip::remove(btn);
    } else {
	QString file = pixmapDir + "/toolbar/key-green.png";
	if (QFile(file).exists()) {
	    btn->setPixmap(QPixmap(file));
	}
	QToolTip::add(btn, i18n("Key Mapping: %1").arg(newMapping));
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


MidiKeyMappingEditor::MidiKeyMappingEditor(BankEditorDialog* bankEditor,
					   QWidget* parent,
					   const char* name)
    : NameSetEditor(bankEditor,
		    i18n("Key Mapping details"),
		    parent, name, i18n("Pitches"), false),
      m_device(0)
{
    QWidget *additionalWidget = makeAdditionalWidget(m_mainFrame);
    if (additionalWidget) {
	m_mainLayout->addMultiCellWidget(additionalWidget, 0, 2, 0, 2);
    }
}

QWidget *
MidiKeyMappingEditor::makeAdditionalWidget(QWidget *parent)
{
    return 0;
}

void
MidiKeyMappingEditor::clearAll()
{
    blockAllSignals(true);

    for (unsigned int i = 0; i < m_names.size(); ++i)
        m_names[i]->clear();

    setTitle(i18n("Key Mapping details"));

    setEnabled(false);

    blockAllSignals(false);
}

void
MidiKeyMappingEditor::populate(QListViewItem* item)
{
    RG_DEBUG << "MidiKeyMappingEditor::populate\n";

    MidiKeyMapListViewItem *keyItem =
	dynamic_cast<MidiKeyMapListViewItem *>(item);
    if (!keyItem) {
        RG_DEBUG << "MidiKeyMappingEditor::populate : not a key item - returning\n";
        return;
    }
    
    Rosegarden::MidiDevice* device = m_bankEditor->getCurrentMidiDevice();
    if (!device) return;

    m_device = device;
    m_mappingName = qstrtostr(keyItem->getName());

    setEnabled(true);

    reset();
}

void
MidiKeyMappingEditor::reset()
{
    if (!m_device) return;

    setTitle(m_mappingName);

    const Rosegarden::MidiKeyMapping *m = m_device->getKeyMappingByName(m_mappingName);

    if (!m) {
	RG_DEBUG << "WARNING: MidiKeyMappingEditor::reset: No such mapping as " << m_mappingName << endl;
    }

    m_mapping = *m;

    blockAllSignals(true);

    // Librarian details
    //
    m_librarian->setText(strtoqstr(m_device->getLibrarianName()));
    m_librarianEmail->setText(strtoqstr(m_device->getLibrarianEmail()));

    for (Rosegarden::MidiKeyMapping::KeyNameMap::const_iterator it =
	     m_mapping.getMap().begin();
	 it != m_mapping.getMap().end(); ++it) {

	int i = it->first;
	if (i < 0 || i > 127) {
	    RG_DEBUG << "WARNING: MidiKeyMappingEditor::reset: Key " << i
		     << " out of range in mapping " << m_mapping.getName()
		     << endl;
	    continue;
	}

	QString name = strtoqstr(it->second);
	m_completion.addItem(name);
	m_names[i]->setText(name);
        m_names[i]->setCursorPosition(0);
    }

    blockAllSignals(false);
}

void
MidiKeyMappingEditor::slotNameChanged(const QString& name)
{
    const KLineEdit* lineEdit = dynamic_cast<const KLineEdit*>(sender());
    if (!lineEdit) {
        RG_DEBUG << "MidiKeyMappingEditor::slotNameChanged() : %%% ERROR - signal sender is not a KLineEdit\n";
        return;
    }

    QString senderName = sender()->name();

    // Adjust value back to zero rated
    //
    unsigned int pitch = senderName.toUInt() - 1;

    RG_DEBUG << "MidiKeyMappingEditor::slotNameChanged("
             << name << ") : pitch = " << pitch << endl;

    if (qstrtostr(name) != m_mapping.getMap()[pitch]) {
	m_mapping.getMap()[pitch] = qstrtostr(name);
	m_bankEditor->setModified(true);
    }
}

void
MidiKeyMappingEditor::slotEntryButtonPressed()
{
}

void MidiKeyMappingEditor::blockAllSignals(bool block)
{
    const QObjectList* allChildren = queryList("KLineEdit", "[0-9]+");
    QObjectListIt it(*allChildren);
    QObject *obj;

    while ( (obj = it.current()) != 0 ) {
        obj->blockSignals(block);
        ++it;
    }
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
    m_deleteAllReally(false),
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
    m_listView->addColumn(i18n("Type"));
    m_listView->addColumn(i18n("MSB"));
    m_listView->addColumn(i18n("LSB"));
    m_listView->setRootIsDecorated(true);
    m_listView->setShowSortIndicator(true);
    m_listView->setItemsRenameable(true);
    m_listView->restoreLayout(kapp->config(), BankEditorConfigGroup);

    QFrame *bankBox = new QFrame(leftPart);
    QGridLayout *gridLayout = new QGridLayout(bankBox, 4, 2, 6, 6);

    m_addBank        = new QPushButton(i18n("Add Bank"), bankBox);
    m_addKeyMapping  = new QPushButton(i18n("Add Key Mapping"), bankBox);
    m_delete         = new QPushButton(i18n("Delete"), bankBox);
    m_deleteAll      = new QPushButton(i18n("Delete All"), bankBox);
    gridLayout->addWidget(m_addBank, 0, 0);
    gridLayout->addWidget(m_addKeyMapping, 0, 1);
    gridLayout->addWidget(m_delete, 1, 0);
    gridLayout->addWidget(m_deleteAll, 1, 1);

    // Tips
    //
    QToolTip::add(m_addBank,
                  i18n("Add a Bank to the current device"));

    QToolTip::add(m_addKeyMapping,
                  i18n("Add a Percussion Key Mapping to the current device"));

    QToolTip::add(m_delete,
                  i18n("Delete the current Bank or Key Mapping"));

    QToolTip::add(m_deleteAll,
                  i18n("Delete all Banks and Key Mappings from the current Device"));

    m_importBanks = new QPushButton(i18n("Import..."), bankBox);
    m_exportBanks = new QPushButton(i18n("Export..."), bankBox);
    gridLayout->addWidget(m_importBanks, 2, 0);
    gridLayout->addWidget(m_exportBanks, 2, 1);

    // Tips
    //
    QToolTip::add(m_importBanks,
            i18n("Import Bank and Program data from a Rosegarden file to the current Device"));
    QToolTip::add(m_exportBanks,
            i18n("Export all Device and Bank information to a Rosegarden format  interchange file"));

    m_copyPrograms = new QPushButton(i18n("Copy"), bankBox);
    m_pastePrograms = new QPushButton(i18n("Paste"), bankBox);
    gridLayout->addWidget(m_copyPrograms, 3, 0);
    gridLayout->addWidget(m_pastePrograms, 3, 1);

    // Tips
    //
    QToolTip::add(m_copyPrograms,
            i18n("Copy all Program names from current Bank to clipboard"));

    QToolTip::add(m_pastePrograms,
            i18n("Paste Program names from clipboard to current Bank"));

    connect(m_listView, SIGNAL(currentChanged(QListViewItem*)),
            this,       SLOT(slotPopulateDevice(QListViewItem*)));

    QFrame *vbox = new QFrame(splitter);
    QVBoxLayout *vboxLayout = new QVBoxLayout(vbox, 8, 6);

    m_programEditor = new MidiProgramsEditor(this, vbox);
    vboxLayout->addWidget(m_programEditor);

    m_keyMappingEditor = new MidiKeyMappingEditor(this, vbox);
    vboxLayout->addWidget(m_keyMappingEditor);
    m_keyMappingEditor->hide();

    m_programEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
    m_keyMappingEditor->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));

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

    connect(m_addKeyMapping, SIGNAL(clicked()),
            this, SLOT(slotAddKeyMapping()));

    connect(m_delete, SIGNAL(clicked()),
            this, SLOT(slotDelete()));

    connect(m_deleteAll, SIGNAL(clicked()),
            this, SLOT(slotDeleteAll()));

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
	m_keyMappingEditor->setDisabled(true);
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
			    bool percussion = bankItem->isPercussion();
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

    // delete items whose corresponding devices are no longer present,
    // and update the other ones
    //
    std::vector<MidiDeviceListViewItem*> itemsToDelete;

    MidiDeviceListViewItem* sibling = dynamic_cast<MidiDeviceListViewItem*>
	(m_listView->firstChild());
    
    while (sibling) {

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

    const Rosegarden::KeyMappingList &mappings = midiDevice->getKeyMappings();
    for (unsigned int i = 0; i < mappings.size(); ++i) {
        RG_DEBUG << "BankEditorDialog::populateDeviceItem - adding key mapping "
                 << itemName << " - " << strtoqstr(mappings[i].getName())
                 << endl;
        new MidiKeyMapListViewItem(midiDevice->getId(), deviceItem,
				   strtoqstr(mappings[i].getName()));
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
    Rosegarden::KeyMappingList keymaps = midiDevice->getKeyMappings();

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
            
    for (unsigned int i = 0; i < keymaps.size(); ++i) {

	QListViewItem *child = deviceItem->firstChild();
	bool have = false;
    
	while (child) {
	    MidiKeyMapListViewItem *keyItem =
		dynamic_cast<MidiKeyMapListViewItem*>(child);
	    if (keyItem) {
		if (keyItem->getName() == strtoqstr(keymaps[i].getName())) {
		    have = true;
		}
	    }
	    child = child->nextSibling();
	}

	if (have) continue;

        RG_DEBUG << "BankEditorDialog::updateDeviceItem - adding "
                 << itemName << " - " << strtoqstr(keymaps[i].getName())
                 << endl;
        new MidiKeyMapListViewItem(midiDevice->getId(), deviceItem,
				   strtoqstr(keymaps[i].getName()));
    }
            
    // delete banks which are no longer present
    //
    std::vector<QListViewItem*> childrenToDelete;

    QListViewItem* child = deviceItem->firstChild();
    
    while (child) {

	MidiBankListViewItem *bankItem =
	    dynamic_cast<MidiBankListViewItem *>(child);
	if (bankItem) {
	    if (bankItem->getBank() >= int(banks.size()))
		childrenToDelete.push_back(child);
	    else { // update the banks MSB/LSB which might have changed
		bankItem->setPercussion(banks[bankItem->getBank()].isPercussion());
		bankItem->setMSB(banks[bankItem->getBank()].getMSB());
		bankItem->setLSB(banks[bankItem->getBank()].getLSB());
	    }
        }

	MidiKeyMapListViewItem *keyItem =
	    dynamic_cast<MidiKeyMapListViewItem *>(child);
	if (keyItem) {
	    if (!midiDevice->getKeyMappingByName(qstrtostr(keyItem->getName()))) {
		childrenToDelete.push_back(child);
	    }
        }
        
        child = child->nextSibling();
    }

    for (unsigned int i = 0; i < childrenToDelete.size(); ++i) 
	delete childrenToDelete[i];
}

bool
BankEditorDialog::deviceItemHasBank(MidiDeviceListViewItem* deviceItem, int bankNb)
{
    QListViewItem *child = deviceItem->firstChild();
    
    while (child) {
	MidiBankListViewItem *bankItem =
	    dynamic_cast<MidiBankListViewItem*>(child);
	if (bankItem) {
	    if (bankItem->getBank() == bankNb) return true;
	}
	child = child->nextSibling();
    }
    
    return false;
}


void
BankEditorDialog::clearItemChildren(QListViewItem* item)
{
    QListViewItem* child = 0;
    
    while ((child = item->firstChild())) delete child;
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

    ModifyDeviceCommand *command = 0;
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
                                          device->getLibrarianEmail()); // rename

        command->clearBankAndProgramList();

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
                                          device->getLibrarianEmail());

        command->setVariation(variation);
        command->setBankList(m_bankList);
        command->setProgramList(m_programList);
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

    MidiKeyMapListViewItem *keyItem = dynamic_cast<MidiKeyMapListViewItem *>(item);

    if (keyItem) {

	stateChanged("on_key_item");
	stateChanged("on_bank_item", KXMLGUIClient::StateReverse);
    
	m_delete->setEnabled(true);

	Rosegarden::MidiDevice *device = getMidiDevice(keyItem->getDeviceId());
	if (!device) return;

	setProgramList(device);

	m_keyMappingEditor->populate(item);

	m_programEditor->hide();
	m_keyMappingEditor->show();
	
	m_lastDevice = keyItem->getDeviceId();

	return;
    }

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(item);

    if (bankItem) {

	stateChanged("on_bank_item");
	stateChanged("on_key_item", KXMLGUIClient::StateReverse);
    
	m_delete->setEnabled(true);
	m_copyPrograms->setEnabled(true);

	if (m_copyBank.first != Rosegarden::Device::NO_DEVICE)
	    m_pastePrograms->setEnabled(true);

	Rosegarden::MidiDevice *device = getMidiDevice(bankItem->getDeviceId());
	if (!device) return;

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

	m_programEditor->populate(item);

	m_keyMappingEditor->hide();
	m_programEditor->show();
	
	m_lastDevice = bankItem->getDeviceId();

	return;
    }

    // Device, not bank or key mapping
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
    m_delete->setEnabled(false);
    m_copyPrograms->setEnabled(false);
    m_pastePrograms->setEnabled(false);
    
    m_variationToggle->setChecked(device->getVariationType() !=
				  Rosegarden::MidiDevice::NoVariations);
    m_variationCombo->setEnabled(m_variationToggle->isChecked());
    m_variationCombo->setCurrentItem
	(device->getVariationType() ==
	 Rosegarden::MidiDevice::VariationFromLSB ? 0 : 1);
    
    stateChanged("on_bank_item", KXMLGUIClient::StateReverse);
    stateChanged("on_key_item", KXMLGUIClient::StateReverse);
    m_programEditor->clearAll();
    m_keyMappingEditor->clearAll();
}

void
BankEditorDialog::slotApply()
{
    RG_DEBUG << "BankEditorDialog::slotApply()\n";

    ModifyDeviceCommand *command = 0;
    
    Rosegarden::MidiDevice *device = getMidiDevice(m_lastDevice);

    // Make sure that we don't delete all the banks and programs
    // if we've not populated them here yet.
    //
    if (m_bankList.size() == 0 && m_programList.size() == 0 &&
            m_deleteAllReally == false)
    {
        RG_DEBUG << "BankEditorDialog::slotApply() : m_bankList size = 0\n";

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail());

        command->clearBankAndProgramList();
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

        RG_DEBUG << "BankEditorDialog::slotApply() : m_bankList size = "
                 << m_bankList.size() << endl;

        command = new ModifyDeviceCommand(m_studio,
                                          m_lastDevice,
                                          m_deviceNameMap[m_lastDevice],
                                          device->getLibrarianName(),
                                          device->getLibrarianEmail());

	MidiKeyMapListViewItem *keyItem = dynamic_cast<MidiKeyMapListViewItem*>
	    (m_listView->currentItem());
	if (keyItem) {
	    Rosegarden::KeyMappingList kml(device->getKeyMappings());
	    for (int i = 0; i < kml.size(); ++i) {
		if (kml[i].getName() == qstrtostr(keyItem->getName())) {
		    kml[i] = m_keyMappingEditor->getMapping();
		    break;
		}
	    }
	    command->setKeyMappingList(kml);
	}

        command->setVariation(variation);
        command->setBankList(m_bankList);
        command->setProgramList(m_programList);
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

    m_programEditor->reset();
    m_programEditor->populate(m_listView->currentItem());
    m_keyMappingEditor->reset();
    m_keyMappingEditor->populate(m_listView->currentItem());

    MidiDeviceListViewItem* deviceItem = getParentDeviceItem
	(m_listView->currentItem());

    if (deviceItem) {
	Rosegarden::MidiDevice *device = getMidiDevice(deviceItem);
	m_variationToggle->setChecked(device->getVariationType() !=
				      Rosegarden::MidiDevice::NoVariations);
	m_variationCombo->setEnabled(m_variationToggle->isChecked());
	m_variationCombo->setCurrentItem
	    (device->getVariationType() ==
	     Rosegarden::MidiDevice::VariationFromLSB ? 0 : 1);
    }

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

    if (dynamic_cast<MidiBankListViewItem*>(item))
        // go up to the parent device item
        item = item->parent();

    if (dynamic_cast<MidiKeyMapListViewItem*>(item))
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
BankEditorDialog::slotAddKeyMapping()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiDeviceListViewItem* deviceItem = getParentDeviceItem(currentItem);
    Rosegarden::MidiDevice *device = getMidiDevice(currentItem);
   
    if (device) {

	QString name = "";
	int n = 0;
	while (name == "" || device->getKeyMappingByName(qstrtostr(name)) != 0) {
	    ++n;
	    if (n == 1) name = i18n("<new mapping>");
	    else name = i18n("<new mapping %1>").arg(n);
	}

	Rosegarden::MidiKeyMapping newKeyMapping(qstrtostr(name));

	ModifyDeviceCommand *command = new ModifyDeviceCommand
	    (m_studio,
	     device->getId(),
	     device->getName(),
	     device->getLibrarianName(),
	     device->getLibrarianEmail());

	Rosegarden::KeyMappingList kml;
	kml.push_back(newKeyMapping);
	command->setKeyMappingList(kml);
	command->setOverwrite(false);
	command->setRename(false);

	addCommandToHistory(command);

	updateDialog();
        selectDeviceItem(device);
    }
}

void
BankEditorDialog::slotDelete()
{
    if (!m_listView->currentItem()) return;

    QListViewItem* currentItem = m_listView->currentItem();

    MidiBankListViewItem* bankItem = dynamic_cast<MidiBankListViewItem*>(currentItem);

    Rosegarden::MidiDevice *device = getMidiDevice(currentItem);

    if (device && bankItem)
    {
        int currentBank = bankItem->getBank();

        int reply =
            KMessageBox::warningYesNo(this, i18n("Really delete this bank?"));

        if (reply == KMessageBox::Yes)
        {
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
            m_listView->blockSignals(true);
            delete currentItem;
            m_listView->blockSignals(false);

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

	return;
    }

    MidiKeyMapListViewItem* keyItem = dynamic_cast<MidiKeyMapListViewItem*>(currentItem);

    if (keyItem && device) {
	
        int reply =
            KMessageBox::warningYesNo(this, i18n("Really delete this key mapping?"));

        if (reply == KMessageBox::Yes) {

	    std::string keyMappingName = qstrtostr(keyItem->getName());

	    ModifyDeviceCommand *command = new ModifyDeviceCommand
		(m_studio,
		 device->getId(),
		 device->getName(),
		 device->getLibrarianName(),
		 device->getLibrarianEmail());

	    Rosegarden::KeyMappingList kml = device->getKeyMappings();

	    for (Rosegarden::KeyMappingList::iterator i = kml.begin();
		 i != kml.end(); ++i) {
		if (i->getName() == keyMappingName) {
		    RG_DEBUG << "erasing " << keyMappingName << endl;
		    kml.erase(i);
		    break;
		}
	    }

	    RG_DEBUG <<" setting " << kml.size() << " key mappings to device " << endl;

	    command->setKeyMappingList(kml);
	    command->setOverwrite(true);

	    addCommandToHistory(command);

	    RG_DEBUG <<" device has " << device->getKeyMappings().size() << " key mappings now " << endl;

	    updateDialog();
	}

	return;
    }
}

void
BankEditorDialog::slotDeleteAll()
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
        m_deleteAllReally = true;
        slotApply();
        m_deleteAllReally = false;

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
    //!!! percussion? this is actually only called in the expectation
    // that percussion==false at the moment

    for (int msb = 0; msb < 128; ++msb) {
	for (int lsb = 0; lsb < 128; ++lsb) {
	    Rosegarden::BankList::iterator i = m_bankList.begin();
	    for ( ; i != m_bankList.end(); ++i) {
		if (i->getLSB() == lsb && i->getMSB() == msb) {
		    break;
		}
	    }
	    if (i == m_bankList.end()) return std::pair<int, int>(msb, lsb);
	}
    }

    return std::pair<int, int>(0, 0);
}

void
BankEditorDialog::slotModifyDeviceOrBankName(QListViewItem* item, const QString &label, int)
{
    RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName" << endl;

    MidiDeviceListViewItem* deviceItem =
        dynamic_cast<MidiDeviceListViewItem*>(item);
    MidiBankListViewItem* bankItem =
	dynamic_cast<MidiBankListViewItem*>(item);
    MidiKeyMapListViewItem *keyItem =
	dynamic_cast<MidiKeyMapListViewItem*>(item);
    
    if (bankItem) {

        // renaming a bank item

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
                 << "modify bank name to " << label << endl;

        if (m_bankList[bankItem->getBank()].getName() != qstrtostr(label)) {
            m_bankList[bankItem->getBank()].setName(qstrtostr(label));
            setModified(true);
        }
        
    } else if (keyItem) {

        RG_DEBUG << "BankEditorDialog::slotModifyDeviceOrBankName - "
                 << "modify key mapping name to " << label << endl;

	QString oldName = keyItem->getName();
	
	QListViewItem* currentItem = m_listView->currentItem();
	Rosegarden::MidiDevice *device = getMidiDevice(currentItem);

	if (device) {
	    
	    ModifyDeviceCommand *command = new ModifyDeviceCommand
		(m_studio,
		 device->getId(),
		 device->getName(),
		 device->getLibrarianName(),
		 device->getLibrarianEmail());

	    Rosegarden::KeyMappingList kml = device->getKeyMappings();

	    for (Rosegarden::KeyMappingList::iterator i = kml.begin();
		 i != kml.end(); ++i) {
		if (i->getName() == oldName) {
		    i->setName(label);
		    break;
		}
	    }

	    command->setKeyMappingList(kml);
	    command->setOverwrite(true);

	    addCommandToHistory(command);

	    updateDialog();
	}

    } else if (deviceItem) { // must be last, as the others are subclasses
	
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
        (deviceDir,
         "audio/x-rosegarden-device audio/x-rosegarden audio/x-soundfont",
         this, i18n("Import Banks from Device in File"));

    if (url.isEmpty()) return;

    ImportDeviceDialog *dialog = new ImportDeviceDialog(this, url);
    if (dialog->doImport() && dialog->exec() == QDialog::Accepted) {

	MidiDeviceListViewItem* deviceItem =
	    dynamic_cast<MidiDeviceListViewItem*>
	    (m_listView->selectedItem());

	if (!deviceItem) {
	    KMessageBox::error(this, "Some internal error: cannot locate selected device");
	    return;
	}

	ModifyDeviceCommand *command = 0;

	Rosegarden::BankList banks(dialog->getBanks());
	Rosegarden::ProgramList programs(dialog->getPrograms());
	Rosegarden::ControlList controls(dialog->getControllers());
	Rosegarden::KeyMappingList keyMappings(dialog->getKeyMappings());
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

	command = new ModifyDeviceCommand(m_studio,
                                          deviceItem->getDeviceId(),
                                          dialog->getDeviceName(),
                                          librarianName,
                                          librarianEmail);
        
        if (dialog->shouldOverwriteBanks()) {
            command->setVariation(variation);
	}
        if (dialog->shouldImportBanks()) {
            command->setBankList(banks);
            command->setProgramList(programs);
        }
        if (dialog->shouldImportControllers()) {
            command->setControlList(controls);
	}
	if (dialog->shouldImportKeyMappings()) {
	    command->setKeyMappingList(keyMappings);
	}
        command->setOverwrite(dialog->shouldOverwriteBanks());
        command->setRename(dialog->shouldRename());

	addCommandToHistory(command);

	// No need to redraw the dialog, this is done by
	// slotUpdate, signalled by the MultiViewCommandHistory
	Rosegarden::MidiDevice *device = getMidiDevice(deviceItem);
	if (device)
	    selectDeviceItem(device);
    }

    delete dialog;
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

    MidiDeviceListViewItem* deviceItem =
	dynamic_cast<MidiDeviceListViewItem*>
	(m_listView->selectedItem());
    
    std::vector<Rosegarden::DeviceId> devices;
    Rosegarden::MidiDevice *md = getMidiDevice(deviceItem);

    if (md) {
	ExportDeviceDialog *ed = new ExportDeviceDialog
	    (this, strtoqstr(md->getName()));
	if (ed->exec() != QDialog::Accepted) return;
	if (ed->getExportType() == ExportDeviceDialog::ExportOne) {
	    devices.push_back(md->getId());
	}
    }
    
    m_doc->exportStudio(name, devices);
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
    if (m_modified) {
        
        int res = KMessageBox::warningYesNoCancel(this, 
                                                  i18n("There are unsaved changes.\n"
                                                       "Do you want to apply the changes before exiting "
                                                       "the Bank Editor or discard the changes ?"),
                                                  i18n("Unsaved Changes"),
                                                  i18n("&Apply"),
                                                  i18n("&Discard"));
        if (res == KMessageBox::Yes) {

            slotApply();

        } else if (res == KMessageBox::Cancel)
            return;
    }
    
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

	    if (md) {
		if (md->getDirection() == Rosegarden::MidiDevice::Play) {
		    m_devices.push_back(*it);
		    m_fromCombo->insertItem(strtoqstr((*it)->getName()));
		    m_toCombo->insertItem(strtoqstr((*it)->getName()));
		}
	    } else {
		Rosegarden::SoftSynthDevice *sd = 
		    dynamic_cast<Rosegarden::SoftSynthDevice *>(*it);
		if (sd) {
		    m_devices.push_back(*it);
		    m_fromCombo->insertItem(strtoqstr((*it)->getName()));
		    m_toCombo->insertItem(strtoqstr((*it)->getName()));
		}
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

#include "bankeditor.moc"
