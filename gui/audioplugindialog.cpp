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

#include <cmath>
#include <set>

#include <qlabel.h>
#include <qdial.h>
#include <qfont.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qaccel.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kcombobox.h>

#include "audioplugindialog.h"
#include "audiopluginmanager.h"
#include "widgets.h"
#include "rosestrings.h"
#include "colours.h"

#include "rosedebug.h"

namespace Rosegarden
{

AudioPluginDialog::AudioPluginDialog(QWidget *parent,
                                     AudioPluginManager *aPM,
                                     Instrument *instrument,
                                     int index):
    KDialogBase(parent, "", false, i18n("Audio Plugin"), Close),
    m_pluginManager(aPM),
    m_instrument(instrument),
    m_index(index),
    m_generating(true)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
                              QSizePolicy::Fixed));

    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *pluginSelectionBox = new QGroupBox
	(1, Horizontal, i18n("Plugin"), vbox);

    makePluginParamsBox(vbox);

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

    m_pluginId = new QLabel(i18n("<id>"), h);
    m_pluginId->setAlignment(AlignRight);
    QToolTip::add(m_pluginId, i18n("Unique ID of plugin."));

    connect(m_pluginList, SIGNAL(activated(int)),
            this, SLOT(slotPluginSelected(int)));

    connect(m_pluginCategoryList, SIGNAL(activated(int)),
            this, SLOT(slotCategorySelected(int)));

    populatePluginCategoryList();
    populatePluginList();

    m_generating = false;

    m_accelerators = new QAccel(this);
}

void
AudioPluginDialog::populatePluginCategoryList()
{
    std::set<QString> categories;
    for (PluginIterator i = m_pluginManager->begin();
	 i != m_pluginManager->end(); ++i) {
	if ((*i)->getCategory() != "") categories.insert((*i)->getCategory());
    }
    if (categories.empty()) {
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
	    category = "";
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

	if (needCategory) {
	    if ((*i)->getCategory() != category) continue;
	}
	
	m_pluginList->insertItem((*i)->getName());
	m_pluginsInList.push_back(count);

	if (inst && inst->isAssigned()) {
	    if ((*i)->getUniqueId() == inst->getId()) {
		m_pluginList->setCurrentItem(m_pluginList->count()-1);
	    }
	}
    }

    slotPluginSelected(m_pluginList->currentItem());
}

void
AudioPluginDialog::makePluginParamsBox(QWidget *parent)
{
    m_pluginParamsBox = new QFrame(parent);

    m_gridLayout = new QGridLayout(m_pluginParamsBox,
                                   1,   // rows (will expand)
                                   10,  // columns
                                   5); // margin

}

void
AudioPluginDialog::slotCategorySelected(int)
{
    populatePluginList();
}

void
AudioPluginDialog::slotPluginSelected(int i)
{
    int number = m_pluginsInList[i];

    RG_DEBUG << "AudioPluginDialog::::slotPluginSelected - "
             << "setting up plugin from position " << number << " at menu item " << i << endl;

    QString caption =
	strtoqstr(m_instrument->getName()) +
	QString(" [ %1 ] - ").arg(m_index + 1);

    // tell the sequencer
    emit pluginSelected(m_index, number - 1);

    if (number == 0)
    {
        setCaption(caption + i18n("<no plugin>"));
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

    makePluginParamsBox(parent);
    
    if (plugin)
    {
        setCaption(caption + plugin->getName());
        m_pluginId->setText(QString("Id: %1").arg(plugin->getUniqueId()));

        QString pluginInfo = plugin->getAuthor() + QString("\n") + 
                             plugin->getCopyright();

        QToolTip::hide();
        QToolTip::remove(m_pluginList);

        QToolTip::add(m_pluginList, pluginInfo);

        // Set the unique id on our own instance - clear the ports down
        //
        AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
        if (inst)
        {
            inst->setId(plugin->getUniqueId());

            // Only clear ports if this method is accessed by user
            // action (after the constructor)
            //
            if (m_generating == false)
                inst->clearPorts();
        }

        PortIterator it = plugin->begin();
        int count = 0;

        // if we've got more than 10 control ports then opt for a slider
        // model so they fit on the screen

        for (; it != plugin->end(); ++it)
        {
            // Weed out non-control ports and those which have erroneous
            // looking bounds - uninitialised values?
            //
            if ((*it)->getType() & PluginPort::Control)
            {
                // Check for port existence and create with default value
                // if it doesn't exist.  Modification occurs through the
                // slotPluginPortChanged signal.
                //
                if (inst->getPort(count) == 0)
                    inst->addPort(count, 0.0);

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
            }

            count++;
        }

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

    emit pluginPortChanged(m_index, control->getIndex(), value);
}

void
AudioPluginDialog::slotBypassChanged(bool bp)
{
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);

    if (inst)
        inst->setBypass(bp);

    emit bypassed(m_index, bp);
}

void
AudioPluginDialog::closeEvent(QCloseEvent *e)
{
    e->accept();
    emit destroyed(m_index);
}

void
AudioPluginDialog::slotClose()
{
    emit destroyed(m_index);
    reject();
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
    

    QLabel *controlTitle = new QLabel(port->getName(), parent);
    controlTitle->setFont(plainFont);

    QLabel *controlValue = new QLabel(parent);
    controlValue->setFont(plainFont);

    if (type == Rotary)
    {
        // defaults
        float lowerBound = 0.0;
        float upperBound = 1.0;

        // Default value appears to be more or less ignored in most
        // plugins so let's ignore it ourselves.
        //
	//float defaultValue = (float)(port->getDefaultValue());
        //cout << "DEFAULT VALUE = " << defaultValue << endl;

        if (m_port->getRange() & PluginPort::Below)
            lowerBound = float(port->getLowerBound());

        if (m_port->getRange() & PluginPort::Above)
            upperBound = float(port->getUpperBound());

        if (m_port->getRange() & PluginPort::SampleRate)
        {
            lowerBound *= m_pluginManager->getSampleRate();
            upperBound *= m_pluginManager->getSampleRate();
        }

        if (lowerBound > upperBound)
        {
            float swap = upperBound;
            upperBound = lowerBound;
            lowerBound = swap;
        }

        QLabel *low = new QLabel(QString("%1").arg(lowerBound), parent);
        low->setIndent(10);
        low->setAlignment(AlignRight|AlignBottom);

        float step = (upperBound - lowerBound) / 100.0;

        m_dial = new RosegardenRotary(parent,
                                      lowerBound,   // min
                                      upperBound,   // max
                                      step,         // step
                                      step * 10.0,
                                      initialValue, // initial
                                      30);          // size

        m_dial->setKnobColour(RosegardenGUIColours::RotaryPlugin);

        //m_dial->setPosition(defaultValue);
        //emit valueChanged(defaultValue);

        connect(m_dial, SIGNAL(valueChanged(float)),
                this, SLOT(slotValueChanged(float)));

        QLabel *upp = new QLabel(QString("%1").arg(upperBound), parent);
        upp->setIndent(10);
        upp->setAlignment(AlignLeft|AlignBottom);
        upp->setFont(plainFont);

        controlTitle->show();
        controlValue->show();
        low->show();
        m_dial->show();
        upp->show();

        m_layout->addItem(new QWidgetItem(controlTitle));
        m_layout->addItem(new QWidgetItem(controlValue));
        m_layout->addItem(new QWidgetItem(low));
        m_layout->addItem(new QWidgetItem(m_dial));
        m_layout->addItem(new QWidgetItem(upp));

        RG_DEBUG << "setting port value = " << initialValue << endl;
    }
}

void
PluginControl::setValue(float value)
{
    m_dial->setPosition(value);
}

void
PluginControl::slotValueChanged(float value)
{
    emit valueChanged(value);
}

}
