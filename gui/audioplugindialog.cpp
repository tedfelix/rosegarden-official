/*
    Rosegarden-4 v0.2
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <math.h>

#include <klocale.h>

#include <qlabel.h>
#include <qdial.h>
#include <qfont.h>
#include <qpushbutton.h>

#include "audioplugindialog.h"
#include "audiopluginmanager.h"
#include "widgets.h"
#include "rosestrings.h"

namespace Rosegarden
{

AudioPluginDialog::AudioPluginDialog(QWidget *parent,
                                     AudioPluginManager *aPM,
                                     Instrument *instrument,
                                     int index):
    KDialogBase(parent, "", false, i18n("Rosegarden Audio Plugin"), Close),
    m_pluginManager(aPM),
    m_instrument(instrument),
    m_index(index),
    m_generating(true)
{
    QVBox *v = makeVBoxMainWidget();

    // Copied the fonts
    QFont plainFont;
    plainFont.setPointSize(plainFont.pointSize() * 9 / 10);

    QFont boldFont;
    boldFont.setPointSize(int(boldFont.pointSize() * 9.5 / 10.0 + 0.5));
    boldFont.setBold(true);

    setFont(boldFont);

    QHBox *h = new QHBox(v);

    // top line
    m_bypassButton = new QPushButton(i18n("Bypass"), h);
    m_bypassButton->setToggleButton(true);

    // spacing
    /*QLabel *idLabel =*/ new QLabel("", h);

    m_pluginId = new QLabel(i18n("<no id>"), h);
    m_pluginId->setFont(plainFont);
    m_pluginId->setAlignment(AlignRight);

    //new QLabel(i18n("Audio Plugin"), v);
    m_pluginList = new RosegardenComboBox(true, v);
    m_pluginList->insertItem(i18n("<no plugin>"));

    connect(m_bypassButton, SIGNAL(toggled(bool)),
            this, SLOT(slotBypassed(bool)));

    // Store the height so we can resize the whole dialog later
    // if required
    //
    m_headHeight = m_pluginList->height();

    connect(m_pluginList, SIGNAL(activated(int)),
            this, SLOT(slotPluginSelected(int)));

    connect(m_pluginList, SIGNAL(propagate(int)),
            this, SLOT(slotPluginSelected(int)));

    std::vector<QString> names = m_pluginManager->getPluginNames();
    std::vector<QString>::iterator it = names.begin();

    for (; it != names.end(); it++)
    {
        m_pluginList->insertItem(*it);
    }

    // Check for plugin and setup as required
    AudioPluginInstance *inst = instrument->getPlugin(index);
    if (inst)
    {
        if (inst->isAssigned())
        {
            if (inst->isBypassed())
                m_bypassButton->setDown(true);

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
AudioPluginDialog::slotPluginSelected(int number)
{
    QString caption = strtoqstr(m_instrument->getName()) + QString(" - ");

    // tell the sequencer
    emit pluginSelected(m_index, number - 1);

    if (number == 0)
    {
        setCaption(caption + i18n("<no plugin>"));
        m_pluginId->setText(i18n("<no id>"));
    }

    AudioPlugin *plugin = m_pluginManager->getPlugin(number - 1);

    /*
    // Destroy all the old widgets
    //
    ControlIterator it = m_pluginWidgets.begin();
    for (; it != m_pluginWidgets.end(); it++)
        delete (*it);
    m_pluginWidgets.erase(m_pluginWidgets.begin(), m_pluginWidgets.end());
    */

    // Destroy old lines
    //
    ControlLineIterator cIt = m_controlLines.begin();
    for (; cIt != m_controlLines.end(); cIt++)
        delete (*cIt);
    m_controlLines.erase(m_controlLines.begin(), m_controlLines.end());

    int height = 0;

    if (plugin)
    {
        setCaption(caption + plugin->getName());
        m_pluginId->setText(QString("%1").arg(plugin->getUniqueId()));

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

        QHBox *controlLine;

        for (; it != plugin->end(); it++)
        {
            if (((float(count))/2.0) == (float(count/2)))
            {
                controlLine = new QHBox(mainWidget());
                controlLine->show();

                // store so we can remove it later
                m_controlLines.push_back(controlLine);
            }
            else
            {
                // spacer
                QLabel *label = new QLabel(QString("   "), controlLine);
                label->show();
            }


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
                    new PluginControl(controlLine,
                                      PluginControl::Rotary,
                                      *it,
                                      m_pluginManager,
                                      count,
                                      inst->getPort(count)->value);


                connect(control, SIGNAL(valueChanged(int, float)),
                        this, SLOT(slotPluginPortChanged(int, float)));
                control->show();

                height += control->height();

                /*
                cout << "NAME = " << (*it)->getName() <<
                        ", TYPE = " << (*it)->getType() << endl;
                        */
                /*
                cout << "CONTROL PORT = \"" << (*it)->getName() << endl;
                cout << "UPPER BOUND  = " << (*it)->getUpperBound() << endl;
                cout << "LOWER BOUND  = " << (*it)->getLowerBound() << endl;
                cout << endl;
                */

                m_pluginWidgets.push_back(control);


            }
            count++;
        }
    }

    // rather severe for the moment
    this->setFixedHeight(m_headHeight + height);

}

void
AudioPluginDialog::slotPluginPortChanged(int portIndex, float value)
{
    // store the new value
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);
    inst->getPort(portIndex)->value = value;

    emit pluginPortChanged(m_index, portIndex, value);
}

void
AudioPluginDialog::slotBypassed(bool bypassed)
{
    AudioPluginInstance *inst = m_instrument->getPlugin(m_index);

    if (inst)
        inst->setBypass(bypassed);
}


// --------------------- PluginControl -------------------------
//
PluginControl::PluginControl(QWidget *parent,
                             ControlType type,
                             PluginPort *port,
                             AudioPluginManager *aPM,
                             int index,
                             float initialValue):
    QHBox(parent),
    m_type(type),
    m_port(port),
    m_multiplier(1.0),
    m_pluginManager(aPM),
    m_index(index)
{
    QFont plainFont;
    plainFont.setPointSize((plainFont.pointSize() * 9 )/ 10);
    setFont(plainFont);

    QLabel *title = new QLabel(port->getName(), this);
    setStretchFactor(title, 20);

    QLabel *value = new QLabel(this);
    setStretchFactor(value, 1);

    if (type == Rotary)
    {
        // defaults
        float lowerBound = 0.0;
        float upperBound = 1.0;

        if (m_port->getRange() & PluginPort::Below)
            lowerBound = float(port->getLowerBound());

        if (m_port->getRange() & PluginPort::Above)
            upperBound = float(port->getUpperBound());

        if (m_port->getRange() & PluginPort::SampleRate)
        {
            lowerBound *= m_pluginManager->getSampleRate();
            upperBound *= m_pluginManager->getSampleRate();
        }

        //if (upperBound < 0.001) upperBound = 0.0;
        //if (lowerBound < 0.001) lowerBound = 0.0;

        QLabel *low = new QLabel(QString("%1").arg(lowerBound), this);
        low->setIndent(10);
        low->setAlignment(AlignRight|AlignBottom);
        setStretchFactor(low, 1);

        m_dial = new QDial(this);
        setStretchFactor(m_dial, 8);

        // Ensure that we always have at least fifty steps
        //
        while ((fabs(upperBound - lowerBound)) * m_multiplier < 50.0)
            m_multiplier *= 10.0;

        //cout << "LOWER = " << lowerBound * m_multiplier << endl;
        //cout << "UPPER = " << upperBound * m_multiplier << endl;

        m_dial->setMinValue(int(lowerBound * m_multiplier));
        m_dial->setMaxValue(int(upperBound * m_multiplier));

        // Set the initial value
        //
        int value = int(initialValue * m_multiplier);
        if (value < int(lowerBound * m_multiplier))
            value = int(lowerBound * m_multiplier);
        else if (value > int(upperBound * m_multiplier))
            value = int(upperBound * m_multiplier);

        m_dial->setValue(value);
        slotValueChanged(value);

        //cout << "INITIAL VALUE = " << value << endl;

        connect(m_dial, SIGNAL(valueChanged(int)),
                this, SLOT(slotValueChanged(int)));

        QLabel *upp = new QLabel(QString("%1").arg(upperBound), this);
        upp->setIndent(10);
        upp->setAlignment(AlignLeft|AlignBottom);
        setStretchFactor(upp, 1);
    }
}

void
PluginControl::slotValueChanged(int value)
{
    emit valueChanged(m_index, (float(value))/m_multiplier);
}

void
PluginControl::setValue(float value)
{
    m_dial->setValue(int(value * m_multiplier));
    // set the label too..
}




}

