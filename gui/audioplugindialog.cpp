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

#include "audioplugindialog.h"
#include "audiopluginmanager.h"
#include "widgets.h"
#include "rosestrings.h"

namespace Rosegarden
{

AudioPluginDialog::AudioPluginDialog(QWidget *parent,
                                     AudioPluginManager *aPM,
                                     Instrument *instrument):
    KDialogBase(parent, "", false, i18n("Rosegarden Audio Plugin"), Close),
    m_pluginManager(aPM),
    m_instrument(instrument)
{
    QVBox *v = makeVBoxMainWidget();

    //new QLabel(i18n("Audio Plugin"), v);
    m_pluginList = new RosegardenComboBox(true, v);
    m_pluginList->insertItem(i18n("<no plugin>"));

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

    slotPluginSelected(m_pluginList->currentItem());
}

void
AudioPluginDialog::slotPluginSelected(int number)
{
    QString caption = strtoqstr(m_instrument->getName()) + QString(" - ");

    if (number == 0)
        setCaption(caption + i18n("<no plugin>"));

    AudioPlugin *plugin = m_pluginManager->getPlugin(number - 1);

    // Destroy all the old widgets
    //
    ControlIterator it = m_pluginWidgets.begin();
    for (; it != m_pluginWidgets.end(); it++)
        delete (*it);
    m_pluginWidgets.erase(m_pluginWidgets.begin(), m_pluginWidgets.end());

    int height = 0;

    if (plugin)
    {
        setCaption(caption + plugin->getName());

        PortIterator it = plugin->begin();
        int count = 0;

        for (; it != plugin->end(); it++)
        {
            // Weed out non-control ports and those which have erroneous
            // looking bounds - uninitialised values?
            //
            if ((*it)->getType() & PluginPort::Control &&
                (fabs((*it)->getUpperBound() - (*it)->getLowerBound()) > 0.001))
            {
                PluginControl *control =
                    new PluginControl(mainWidget(),
                                      PluginControl::Rotary,
                                      *it);
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

// --------------------- PluginControl -------------------------
//
PluginControl::PluginControl(QWidget *parent,
                             ControlType type,
                             PluginPort *port):
    QHBox(parent),
    m_type(type),
    m_port(port)
{
    QFont plainFont;
    plainFont.setPointSize((plainFont.pointSize() * 9 )/ 10);
    setFont(plainFont);

    QLabel *title = new QLabel(port->getName(), this);
    setStretchFactor(title, 20);


    if (type == Rotary)
    {
        float upperBound = float(port->getUpperBound());
        float lowerBound = float(port->getLowerBound());

        if (upperBound < 0.001) upperBound = 0.0;
        if (lowerBound < 0.001) lowerBound = 0.0;

        QLabel *low = new QLabel(QString("%1").arg(lowerBound), this);
        low->setIndent(10);
        low->setAlignment(AlignRight|AlignBottom);
        setStretchFactor(low, 1);

        m_dial = new QDial(this);
        setStretchFactor(m_dial, 8);

        QLabel *upp = new QLabel(QString("%1").arg(upperBound), this);
        upp->setIndent(10);
        upp->setAlignment(AlignLeft|AlignBottom);
        setStretchFactor(upp, 1);

        /*
            float diff = upperBound - lowerBound;
            if (diff < 1.0)
            {
            }
        */
    }
}



}

