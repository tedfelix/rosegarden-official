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

#include <qlabel.h>
#include <qdial.h>
#include <qfont.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlayout.h>

#include <klocale.h>
#include <kcombobox.h>

#include "audioplugindialog.h"
#include "audiopluginmanager.h"
#include "widgets.h"
#include "rosestrings.h"

#include "rosedebug.h"

namespace Rosegarden
{

AudioPluginDialog::AudioPluginDialog(QWidget *parent,
                                     AudioPluginManager *aPM,
                                     Instrument *instrument,
                                     int index):
    //!!! should be an ok/cancel dialog, but that's trickier to do
    //    
    // .. well just make sure that we still send realtime parameters
    // when changing rotaries etc.
    //
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

    m_pluginList = new KComboBox(pluginSelectionBox);
    m_pluginList->insertItem(i18n("(none)"));

    QHBox *h = new QHBox(pluginSelectionBox);

    // top line
    m_bypass = new QCheckBox(i18n("Bypass"), h);

    connect(m_bypass, SIGNAL(toggled(bool)),
            this, SLOT(slotBypassChanged(bool)));

    m_pluginId = new QLabel(i18n("<no id>"), h);
    m_pluginId->setAlignment(AlignRight);

    connect(m_pluginList, SIGNAL(activated(int)),
            this, SLOT(slotPluginSelected(int)));
/*!!!
    connect(m_pluginList, SIGNAL(propagate(int)),
            this, SLOT(slotPluginSelected(int)));
*/
    std::vector<QString> names = m_pluginManager->getPluginNames();
    std::vector<QString>::iterator it = names.begin();

    for (; it != names.end(); ++it)
    {
        m_pluginList->insertItem(*it);
    }

    // Check for plugin and setup as required
    AudioPluginInstance *inst = instrument->getPlugin(index);
    if (inst)
    {
	m_bypass->setChecked(inst->isBypassed());

        if (inst->isAssigned())
        {
            // Get the position from the unique id (add one for the first
            // null entry).
            //
            int position = aPM->getPositionByUniqueId(inst->getId()) + 1;
            m_pluginList->setCurrentItem(position);
            slotPluginSelected(position);
        }
        else
            slotPluginSelected(m_pluginList->currentItem());
    }
    else
        slotPluginSelected(m_pluginList->currentItem());

    m_generating = false;
}

void
AudioPluginDialog::makePluginParamsBox(QWidget *parent)
{
    m_pluginParamsBox = new QFrame(parent);
    //     m_pluginParamsBox = new QGroupBox
    //         (1, Horizontal, i18n("Plugin Parameters"), vbox);
    //m_pluginParamsBox->hide();

    m_gridLayout = new QGridLayout(m_pluginParamsBox,
                                   1,   // rows (will expand)
                                   10,  // columns
                                   5); // margin

}

void
AudioPluginDialog::slotPluginSelected(int number)
{

    RG_DEBUG << "AudioPluginDialog::::slotPluginSelected - "
             << "setting up plugin in slot " << number << endl;

    QString caption =
	strtoqstr(m_instrument->getName()) +
	QString(" [ %1 ] - ").arg(m_index + 1);

    // tell the sequencer
    emit pluginSelected(m_index, number - 1);

    if (number == 0)
    {
        setCaption(caption + i18n("<no plugin>"));
        m_pluginId->setText(i18n("<no id>"));
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
//             if (((float(count))/2.0) == (float(count/2)))
//             {
// 		controlLine = new QHBox(m_pluginParamsBox);
//                 controlLine->show();

//                 // store so we can remove it later
//                 m_controlLines.push_back(controlLine);
//             }
//             else
//             {
//                 // spacer
//                 QLabel *label = new QLabel(QString("   "), controlLine);
//                 label->show();
//             }


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

//     RG_DEBUG << "AudioPluginDialog::slotPluginPortChanged("
//              << value << ") from control "
//              << control->getIndex()
//              << endl;

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
	float defaultValue = (float)(port->getDefaultValue());

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
