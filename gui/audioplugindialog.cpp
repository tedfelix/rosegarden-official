// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <cmath>
#include <set>

#include <qlabel.h>
#include <qdial.h>
#include <qtable.h>
#include <qfont.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qaccel.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kcombobox.h>
#include <kstdaction.h>
#include <kaction.h>

#include "audioplugindialog.h"
#include "audiopluginmanager.h"
#include "widgets.h"
#include "rosestrings.h"
#include "colours.h"
#include "rosegardenguidoc.h"
#include "studiocontrol.h"

#include "rosedebug.h"

#include "MappedStudio.h"

#ifdef HAVE_LIBLO
#include "audiopluginoscgui.h"
#endif

namespace Rosegarden
{

AudioPluginClipboard _pluginClipboard;

AudioPluginDialog::AudioPluginDialog(QWidget *parent,
                                     AudioPluginManager *aPM,
#ifdef HAVE_LIBLO
				     AudioPluginOSCGUIManager *aGM,
#endif
                                     Instrument *instrument,
                                     int index):
    KDialogBase(parent, "", false, i18n("Audio Plugin"),
#ifdef HAVE_LIBLO
		Close | Details),
#else
		Close),
#endif
    m_pluginManager(aPM),
#ifdef HAVE_LIBLO
    m_pluginGUIManager(aGM),
#endif
    m_instrument(instrument),
    m_programLabel(0),
    m_index(index),
    m_generating(true),
    m_guiShown(false)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::Fixed));

#ifdef HAVE_LIBLO
    setButtonText(Details, i18n("Editor"));
#endif

    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *pluginSelectionBox = new QGroupBox
	(1, Horizontal, i18n("Plugin"), vbox);

    makePluginParamsBox(vbox, 0);

    m_pluginCategoryBox = new QHBox(pluginSelectionBox);
    new QLabel(i18n("Category:"), m_pluginCategoryBox);
    m_pluginCategoryList = new KComboBox(m_pluginCategoryBox);

    QHBox *hbox = new QHBox(pluginSelectionBox);
    m_pluginLabel = new QLabel(i18n("Plugin:"), hbox);
    m_pluginList = new KComboBox(hbox);
    QToolTip::add(m_pluginList, i18n("Select a plugin from this list."));

    QHBox *h = new QHBox(pluginSelectionBox);

    // top line
    m_bypass = new QCheckBox(i18n("Bypass"), h);
    QToolTip::add(m_bypass, i18n("Bypass this plugin."));

    connect(m_bypass, SIGNAL(toggled(bool)),
            this, SLOT(slotBypassChanged(bool)));


    m_insOuts = new QLabel(i18n("<ports>"), h);
    m_insOuts->setAlignment(AlignRight);
    QToolTip::add(m_insOuts, i18n("Input and output port counts."));

    m_pluginId = new QLabel(i18n("<id>"), h);
    m_pluginId->setAlignment(AlignRight);
    QToolTip::add(m_pluginId, i18n("Unique ID of plugin."));

    connect(m_pluginList, SIGNAL(activated(int)),
            this, SLOT(slotPluginSelected(int)));

    connect(m_pluginCategoryList, SIGNAL(activated(int)),
            this, SLOT(slotCategorySelected(int)));

    // new line
    h = new QHBox(pluginSelectionBox);
    m_copyButton = new QPushButton(i18n("Copy"), h);
    connect(m_copyButton, SIGNAL(clicked()),
            this, SLOT(slotCopy()));
    QToolTip::add(m_copyButton, i18n("Copy plugin parameters"));

    m_pasteButton = new QPushButton(i18n("Paste"), h);
    connect(m_pasteButton, SIGNAL(clicked()),
            this, SLOT(slotPaste()));
    QToolTip::add(m_pasteButton, i18n("Paste plugin parameters"));

    m_defaultButton = new QPushButton(i18n("Default"), h);
    connect(m_defaultButton, SIGNAL(clicked()),
            this, SLOT(slotDefault()));
    QToolTip::add(m_defaultButton, i18n("Set to defaults"));

    populatePluginCategoryList();
    populatePluginList();

    m_generating = false;

    m_accelerators = new QAccel(this);
}

#ifdef HAVE_LIBLO
void
AudioPluginDialog::slotDetails()
{
    slotShowGUI();
}
#endif

void
AudioPluginDialog::slotShowGUI()
{
    if (!m_guiShown) {
	emit showPluginGUI(m_instrument->getId(), m_index);
	m_guiShown = true;
    }
}

void
AudioPluginDialog::populatePluginCategoryList()
{
    std::set<QString> categories;
    for (PluginIterator i = m_pluginManager->begin();
	 i != m_pluginManager->end(); ++i) {
	if ((*i)->getCategory() != "") categories.insert((*i)->getCategory());
    }
    if (isSynth() || categories.empty()) {
	m_pluginCategoryBox->hide();
	m_pluginLabel->hide();
    }

    m_pluginCategoryList->clear();
    m_pluginCategoryList->insertItem(i18n("(any)"));
    m_pluginCategoryList->insertItem(i18n("(unclassified)"));

    for (std::set<QString>::iterator i = categories.begin();
	 i != categories.end(); ++i) {
	m_pluginCategoryList->insertItem(*i);
    }
}

void
AudioPluginDialog::populatePluginList()
{
    m_pluginList->clear();
    m_pluginsInList.clear();

    m_pluginList->insertItem(i18n("(none)"));
    m_pluginsInList.push_back(0);

    QString category;
    bool needCategory = false;

    if (m_pluginCategoryList->isVisible() &&
	m_pluginCategoryList->currentItem() > 0) {
	needCategory = true;
	if (m_pluginCategoryList->currentItem() == 1) {
	    category = QString();
	} else {
	    category = m_pluginCategoryList->currentText();
	}
    }

    // Check for plugin and setup as required
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
    if (inst) m_bypass->setChecked(inst->isBypassed());

    int count = 0;

    for (PluginIterator i = m_pluginManager->begin();
	 i != m_pluginManager->end(); ++i) {

	++count;

	if ((*i)->isSynth() != isSynth()) continue;

	if (needCategory) {
	    if ((*i)->getCategory() != category) continue;
	}
	
	m_pluginList->insertItem((*i)->getName());
	m_pluginsInList.push_back(count);

	if (inst && inst->isAssigned()) {
	    if ((*i)->getIdentifier() == inst->getIdentifier().c_str()) {
		m_pluginList->setCurrentItem(m_pluginList->count()-1);
	    }
	}
    }

    slotPluginSelected(m_pluginList->currentItem());
}

void
AudioPluginDialog::makePluginParamsBox(QWidget *parent, int portCount)
{
    m_pluginParamsBox = new QFrame(parent);

    int columns = 2;
    if (portCount > 24) {
	columns = (portCount-1) / 12 + 1;
    }

    m_gridLayout = new QGridLayout(m_pluginParamsBox,
                                   1,  // rows (will expand)
                                   columns * 4,
                                   5); // margin

    m_gridLayout->setColStretch(3, 2);
    m_gridLayout->setColStretch(7, 2);
}

void
AudioPluginDialog::slotCategorySelected(int)
{
    populatePluginList();
}

void
AudioPluginDialog::slotPluginSelected(int i)
{
    bool guiWasShown = m_guiShown;

    if (m_guiShown) {
	emit stopPluginGUI(m_instrument->getId(), m_index);
	m_guiShown = false;
    }

    int number = m_pluginsInList[i];

    RG_DEBUG << "AudioPluginDialog::::slotPluginSelected - "
             << "setting up plugin from position " << number << " at menu item " << i << endl;

    QString caption =
	strtoqstr(m_instrument->getName()) +
	QString(" [ %1 ] - ").arg(m_index + 1);

    if (number == 0)
    {
        setCaption(caption + i18n("<no plugin>"));
	m_insOuts->setText(i18n("<ports>"));
        m_pluginId->setText(i18n("<id>"));

        QToolTip::hide();
        QToolTip::remove(m_pluginList);

        QToolTip::add(m_pluginList, i18n("Select a plugin from this list."));
    }

    AudioPlugin *plugin = m_pluginManager->getPlugin(number - 1);

    // Destroy old param widgets
    //
    QWidget* parent = dynamic_cast<QWidget*>(m_pluginParamsBox->parent());

    delete m_pluginParamsBox;
    m_pluginWidgets.clear(); // The widgets are deleted with the parameter box
    m_programCombo = 0;

    int portCount = 0;
    if (plugin) {
        for (PortIterator it = plugin->begin(); it != plugin->end(); ++it) {
            if (((*it)->getType() & PluginPort::Control) &&
		((*it)->getType() & PluginPort::Input)) ++portCount;
	}
    }

    makePluginParamsBox(parent, portCount);

    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
    if (!inst) return;

    QLabel *programLabel = 0;

    if (plugin && plugin->isSynth()) {

	m_programLabel = new QLabel(i18n("Program:  "), m_pluginParamsBox);
	m_programCombo = new KComboBox(m_pluginParamsBox);
	m_gridLayout->addMultiCellWidget(m_programLabel,
					 0, 0, 0, 0, Qt::AlignRight);
	m_gridLayout->addMultiCellWidget(m_programCombo,
					 0, 0, 1, m_gridLayout->numCols()-1,
					 Qt::AlignLeft);
	connect(m_programCombo, SIGNAL(activated(const QString &)),
		this, SLOT(slotPluginProgramChanged(const QString &)));
    }

    if (plugin)
    {
	setCaption(caption + plugin->getName());
	m_pluginId->setText(i18n("Id: %1").arg(plugin->getUniqueId()));

        QString pluginInfo = plugin->getAuthor() + QString("\n") + 
                             plugin->getCopyright();

        QToolTip::hide();
        QToolTip::remove(m_pluginList);
        QToolTip::add(m_pluginList, pluginInfo);

	inst->setIdentifier(plugin->getIdentifier().data());

	// Only clear ports if this method is accessed by user
	// action (after the constructor)
	//
	if (m_generating == false) inst->clearPorts();

        PortIterator it = plugin->begin();
        int count = 0;
	int ins = 0, outs = 0;

        for (; it != plugin->end(); ++it)
        {
            if (((*it)->getType() & PluginPort::Control) &&
		((*it)->getType() & PluginPort::Input))
            {
                // Check for port existence and create with default value
                // if it doesn't exist.  Modification occurs through the
                // slotPluginPortChanged signal.
                //
                if (inst->getPort(count) == 0) {
                    inst->addPort(count, (float)(*it)->getDefaultValue());
		    std::cerr << "Plugin port name " << (*it)->getName() << ", default: " << (*it)->getDefaultValue() << std::endl;
		}

                PluginControl *control =
                    new PluginControl(m_pluginParamsBox,
				      m_gridLayout,
                                      PluginControl::Rotary,
                                      *it,
                                      m_pluginManager,
                                      count,
                                      inst->getPort(count)->value);

                connect(control, SIGNAL(valueChanged(float)),
                        this, SLOT(slotPluginPortChanged(float)));

                m_pluginWidgets.push_back(control);

            } else if ((*it)->getType() & PluginPort::Audio) {
		if ((*it)->getType() & PluginPort::Input) ++ins;
		else if ((*it)->getType() & PluginPort::Output) ++outs;
	    }

            ++count;
        }

	if (ins == 1 && outs == 1) m_insOuts->setText(i18n("mono"));
	else if (ins == 2 && outs == 2) m_insOuts->setText(i18n("stereo"));
	else m_insOuts->setText(i18n("%1 in, %2 out").arg(ins).arg(outs));

	QString shortName(plugin->getName());
	int parenIdx = shortName.find(" (");
	if (parenIdx > 0) {
	    shortName = shortName.left(parenIdx);
	    if (shortName == "Null") shortName = "Plugin";
	}
	//m_pluginParamsBox->setTitle(i18n("Parameters for %1").arg(shortName));
	m_pluginParamsBox->show();

    } else {
	//m_pluginParamsBox->hide();
    }

    adjustSize();
    setFixedSize(minimumSizeHint());

    // tell the sequencer
    emit pluginSelected(m_instrument->getId(), m_index, number - 1);

    if (plugin && plugin->isSynth()) {
	int current = 0;
	QStringList programs = getProgramsForInstance(inst, current);
    
	if (programs.count() > 0) {
	    m_programCombo->insertStringList(programs);
	    m_programCombo->setCurrentItem(current);
	} else {
	    m_programLabel->hide();
	    m_programCombo->hide();
	}
    }

    if (guiWasShown) {
	emit showPluginGUI(m_instrument->getId(), m_index);
	m_guiShown = true;
    }

#ifdef HAVE_LIBLO
    bool gui = m_pluginGUIManager->hasGUI(m_instrument->getId(), m_index);
    actionButton(Details)->setEnabled(gui);
#endif
    
}

QStringList
AudioPluginDialog::getProgramsForInstance(AudioPluginInstance *inst, int &current)
{
    QStringList list;
    int mappedId = inst->getMappedId();
    QString currentProgram = strtoqstr(inst->getProgram());

    MappedObjectPropertyList propertyList = StudioControl::getStudioObjectProperty
	(mappedId, MappedPluginSlot::Programs);

    for (MappedObjectPropertyList::iterator i = propertyList.begin();
	 i != propertyList.end(); ++i) {
	if (*i == currentProgram) current = list.count();
	list.append(*i);
    }

    return list;
}

void
AudioPluginDialog::slotPluginPortChanged(float value)
{
    const QObject* object = sender();

    const PluginControl* control = dynamic_cast<const PluginControl*>(object);

    if (!control) return;

    // store the new value
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
    inst->getPort(control->getIndex())->value = value;

    emit pluginPortChanged(m_instrument->getId(),
			   m_index, control->getIndex(), value);
}

void
AudioPluginDialog::slotPluginProgramChanged(const QString &value)
{
    // store the new value
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
    inst->setProgram(qstrtostr(value));

    emit pluginProgramChanged(m_instrument->getId(), m_index, value);
}

void
AudioPluginDialog::updatePluginPortControl(int port)
{        
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
    if (inst) {
	PluginPortInstance *pti = inst->getPort(port);
	if (pti) {
	    for (std::vector<PluginControl *>::iterator i = m_pluginWidgets.begin();
		 i != m_pluginWidgets.end(); ++i) {
		if ((*i)->getIndex() == port) {
		    (*i)->setValue(pti->value, false);
		    return;
		}
	    }
	}
    }
}

void
AudioPluginDialog::updatePluginProgramControl()
{
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
    if (inst) {
	std::string program = inst->getProgram();
	if (m_programCombo) {
	    m_programCombo->blockSignals(true);
	    m_programCombo->setCurrentText(strtoqstr(program));
	    m_programCombo->blockSignals(false);
	}
    }
}

void
AudioPluginDialog::updatePluginProgramList()
{
    if (!m_programLabel) return;

    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
    if (!inst) return;

    m_programCombo->blockSignals(true);

    while (m_programCombo->count() > 0) {
	m_programCombo->removeItem(0);
    }

    int current = 0;
    QStringList programs = getProgramsForInstance(inst, current);
    
    if (programs.count() > 0) {
	m_programCombo->show();
	m_programLabel->show();
	m_programCombo->insertStringList(programs);
	m_programCombo->setCurrentItem(current);
    } else {
	m_programLabel->hide();
	m_programCombo->hide();
    }

    m_programCombo->blockSignals(false);
}

void
AudioPluginDialog::slotBypassChanged(bool bp)
{
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);

    if (inst)
        inst->setBypass(bp);

    emit bypassed(m_instrument->getId(), m_index, bp);
}

void
AudioPluginDialog::closeEvent(QCloseEvent *e)
{
    e->accept();
    emit destroyed(m_instrument->getId(), m_index);
}

void
AudioPluginDialog::slotClose()
{
    emit destroyed(m_instrument->getId(), m_index);
    reject();
}

void
AudioPluginDialog::slotCopy()
{
    int item = m_pluginList->currentItem();
    int number = m_pluginsInList[item];

    if (number > 0)
    {
        Rosegarden::AudioPluginClipboard *clipboard = 
            m_pluginManager->getPluginClipboard();

        clipboard->m_pluginNumber = number;
        clipboard->m_controlValues.clear();

        std::cout << "AudioPluginDialog::slotCopy - plugin number = " << number
                  << std::endl;


        std::vector<PluginControl*>::iterator it;
        for (it = m_pluginWidgets.begin(); it != m_pluginWidgets.end(); ++it)
        {
            std::cout << "AudioPluginDialog::slotCopy - "
                      << "value = " << (*it)->getValue() << std::endl;

            clipboard->m_controlValues.push_back((*it)->getValue());
        }
    }

}

void
AudioPluginDialog::slotPaste()
{
    Rosegarden::AudioPluginClipboard *clipboard = m_pluginManager->getPluginClipboard();

    std::cout << "AudioPluginDialog::slotPaste - paste plugin id "
              << clipboard->m_pluginNumber << std::endl;

    if (clipboard->m_pluginNumber != -1)
    {
        int count = 0;
        for (std::vector<int>::iterator it = m_pluginsInList.begin();
                it != m_pluginsInList.end(); ++it)
        {
            if ((*it) == clipboard->m_pluginNumber)
                break;
            count++;
        }

        if (count >= int(m_pluginsInList.size())) return;

        // now select the plugin
        //
        slotPluginSelected(count);
    }

}

void
AudioPluginDialog::slotDefault()
{
}




// --------------------- PluginControl -------------------------
//
PluginControl::PluginControl(QWidget *parent,
			     QGridLayout *layout,
                             ControlType type,
                             PluginPort *port,
                             AudioPluginManager *aPM,
                             int index,
                             float initialValue):
    QObject(parent),
    m_layout(layout),
    m_type(type),
    m_port(port),
    m_pluginManager(aPM),
    m_index(index)
{
    QFont plainFont;
    plainFont.setPointSize((plainFont.pointSize() * 9 )/ 10);

    QLabel *controlTitle =
	new QLabel(QString("%1    ").arg(strtoqstr(port->getName())), parent);
    controlTitle->setFont(plainFont);

    if (type == Rotary)
    {
	float lowerBound = port->getLowerBound();
	float upperBound = port->getUpperBound();
	// Default value was already handled when calling this constructor

        if (lowerBound > upperBound)
        {
            float swap = upperBound;
            upperBound = lowerBound;
            lowerBound = swap;
        }

        QLabel *low;
	if (port->getDisplayHint() & Rosegarden::PluginPort::Integer) {
	    low = new QLabel(QString("%1").arg(int(lowerBound)), parent);
	} else {
	    low = new QLabel(QString("%1").arg(lowerBound), parent);
	}
        low->setFont(plainFont);

        float step = (upperBound - lowerBound) / 100.0;
	float pageStep = step * 10.0;
	RosegardenRotary::TickMode ticks = RosegardenRotary::PageStepTicks;
	bool snapToTicks = false;

	if (port->getDisplayHint() & Rosegarden::PluginPort::Integer) {
	    step = 1.0;
	    ticks = RosegardenRotary::StepTicks;
	    if (upperBound - lowerBound > 30.0) pageStep = 10.0;
	    snapToTicks = true;
	}
	if (port->getDisplayHint() & Rosegarden::PluginPort::Toggled) {
	    lowerBound = -0.0001;
	    upperBound = 1.0001;
	    step = 1.0;
	    pageStep = 1.0;
	    ticks = RosegardenRotary::StepTicks;
	    snapToTicks = true;
	}

        m_dial = new RosegardenRotary(parent,
                                      lowerBound,   // min
                                      upperBound,   // max
                                      step,         // step
                                      pageStep,     // page step
                                      initialValue, // initial
                                      30,           // size
				      ticks,
				      snapToTicks);

        m_dial->setKnobColour(RosegardenGUIColours::RotaryPlugin);

        //m_dial->setPosition(defaultValue);
        //emit valueChanged(defaultValue);

        connect(m_dial, SIGNAL(valueChanged(float)),
                this, SLOT(slotValueChanged(float)));

        QLabel *upp;
	if (port->getDisplayHint() & Rosegarden::PluginPort::Integer) {
	    upp = new QLabel(QString("%1").arg(int(upperBound)), parent);
	} else {
	    upp = new QLabel(QString("%1").arg(upperBound), parent);
	}
        upp->setFont(plainFont);

        controlTitle->show();
        low->show();
        m_dial->show();
        upp->show();

	QWidgetItem *item = new QWidgetItem(controlTitle);
	item->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	m_layout->addItem(item);

	item = new QWidgetItem(low);
	item->setAlignment(Qt::AlignRight | Qt::AlignBottom);
	m_layout->addItem(item);
	
	item = new QWidgetItem(m_dial);
	item->setAlignment(Qt::AlignCenter);
	m_layout->addItem(item);
	
	item = new QWidgetItem(upp);
	item->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	m_layout->addItem(item);

        RG_DEBUG << "setting port value = " << initialValue << endl;
    }
}

void
PluginControl::setValue(float value, bool emitSignals)
{
    if (!emitSignals) m_dial->blockSignals(true);
    m_dial->setPosition(value);
    if (!emitSignals) m_dial->blockSignals(false);
}

float
PluginControl::getValue() const
{
    return m_dial == 0 ? 0 : m_dial->getPosition();
}


void
PluginControl::slotValueChanged(float value)
{
    emit valueChanged(value);
}

}
