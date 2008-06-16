/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ControlParameterEditDialog.h"
#include <qlayout.h>

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/ControlParameter.h"
#include "base/Event.h"
#include "base/MidiTypes.h"
#include "document/RosegardenGUIDoc.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qcolor.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpixmap.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

const QString notShowing(i18n("<not showing>"));

ControlParameterEditDialog::ControlParameterEditDialog(
    QWidget *parent,
    ControlParameter *control,
    RosegardenGUIDoc *doc):
        KDialogBase(parent, 0, true,
                    i18n("Edit Control Parameter"), Ok | Cancel),
        m_doc(doc),
        m_control(control)
{
    m_dialogControl = *control; // copy in the ControlParameter

    QVBox *vbox = makeVBoxMainWidget();

    QGroupBox *groupBox = new QGroupBox
                          (1, Horizontal, i18n("Control Event Properties"), vbox);

    QFrame *frame = new QFrame(groupBox);

    QGridLayout *layout = new QGridLayout(frame, 4, 3, 10, 5);

    layout->addWidget(new QLabel(i18n("Name:"), frame), 0, 0);
    m_nameEdit = new QLineEdit(frame);
    layout->addWidget(m_nameEdit, 0, 1);

    layout->addWidget(new QLabel(i18n("Type:"), frame), 1, 0);
    m_typeCombo = new KComboBox(frame);
    layout->addMultiCellWidget(m_typeCombo, 1, 1, 1, 2);

    layout->addWidget(new QLabel(i18n("Description:"), frame), 2, 0);
    m_description = new QLineEdit(frame);
    layout->addMultiCellWidget(m_description, 2, 2, 1, 2);

    // hex value alongside decimal value
    m_hexValue = new QLabel(frame);
    layout->addWidget(m_hexValue, 3, 1);

    layout->addWidget(new QLabel(i18n("Control Event value:"), frame), 3, 0);
    m_controllerBox = new QSpinBox(frame);
    layout->addWidget(m_controllerBox, 3, 2);

    layout->addWidget(new QLabel(i18n("Minimum value:"), frame), 4, 0);
    m_minBox = new QSpinBox(frame);
    layout->addMultiCellWidget(m_minBox, 4, 4, 1, 2);

    layout->addWidget(new QLabel(i18n("Maximum value:"), frame), 5, 0);
    m_maxBox = new QSpinBox(frame);
    layout->addMultiCellWidget(m_maxBox, 5, 5, 1, 2);

    layout->addWidget(new QLabel(i18n("Default value:"), frame), 6, 0);
    m_defaultBox = new QSpinBox(frame);
    layout->addMultiCellWidget(m_defaultBox, 6, 6, 1, 2);

    layout->addWidget(new QLabel(i18n("Color:"), frame), 7, 0);
    m_colourCombo = new KComboBox(frame);
    layout->addMultiCellWidget(m_colourCombo, 7, 7, 1, 2);

    layout->addWidget(new QLabel(i18n("Instrument Parameter Box position:"), frame), 8, 0);
    m_ipbPosition = new KComboBox(frame);
    layout->addMultiCellWidget(m_ipbPosition, 8, 8, 1, 2);

    connect(m_nameEdit, SIGNAL(textChanged(const QString&)),
            SLOT(slotNameChanged(const QString&)));

    connect(m_typeCombo, SIGNAL(activated(int)),
            SLOT(slotTypeChanged(int)));

    connect(m_description, SIGNAL(textChanged(const QString&)),
            SLOT(slotDescriptionChanged(const QString &)));

    connect(m_controllerBox, SIGNAL(valueChanged(int)),
            SLOT(slotControllerChanged(int)));

    connect(m_minBox, SIGNAL(valueChanged(int)),
            SLOT(slotMinChanged(int)));

    connect(m_maxBox, SIGNAL(valueChanged(int)),
            SLOT(slotMaxChanged(int)));

    connect(m_defaultBox, SIGNAL(valueChanged(int)),
            SLOT(slotDefaultChanged(int)));

    connect(m_colourCombo, SIGNAL(activated(int)),
            SLOT(slotColourChanged(int)));

    connect(m_ipbPosition, SIGNAL(activated(int)),
            SLOT(slotIPBPositionChanged(int)));

    //m_nameEdit->selectAll();
    //m_description->selectAll();

    // set limits
    m_controllerBox->setMinValue(0);
    m_controllerBox->setMaxValue(127);

    m_minBox->setMinValue(INT_MIN);
    m_minBox->setMaxValue(INT_MAX);

    m_maxBox->setMinValue(INT_MIN);
    m_maxBox->setMaxValue(INT_MAX);

    m_defaultBox->setMinValue(INT_MIN);
    m_defaultBox->setMaxValue(INT_MAX);

    // populate combos
    m_typeCombo->insertItem(strtoqstr(Controller::EventType));
    m_typeCombo->insertItem(strtoqstr(PitchBend::EventType));
    /*
    m_typeCombo->insertItem(strtoqstr(KeyPressure::EventType));
    m_typeCombo->insertItem(strtoqstr(ChannelPressure::EventType));
    */

    // Populate colour combo
    //
    //
    ColourMap &colourMap = m_doc->getComposition().getGeneralColourMap();
    RCMap::const_iterator it;
    QPixmap colourPixmap(16, 16);

    for (it = colourMap.begin(); it != colourMap.end(); ++it) {
        Colour c = it->second.first;
        colourPixmap.fill(QColor(c.getRed(), c.getGreen(), c.getBlue()));
        m_colourCombo->insertItem(colourPixmap, strtoqstr(it->second.second));
    }

    // Populate IPB position combo
    //
    m_ipbPosition->insertItem(notShowing);
    for (unsigned int i = 0; i < 32; i++)
        m_ipbPosition->insertItem(QString("%1").arg(i));

    if (m_control->getType() == Controller::EventType)
        m_typeCombo->setCurrentItem(0);
    else if (m_control->getType() == PitchBend::EventType)
        m_typeCombo->setCurrentItem(1);
    /*
    else if (m_control->getType() == KeyPressure::EventType)
        m_typeCombo->setCurrentItem(2);
    else if (m_control->getType() == ChannelPressure::EventType)
        m_typeCombo->setCurrentItem(3);
        */

    populate();
}

void
ControlParameterEditDialog::populate()
{
    m_nameEdit->setText(strtoqstr(m_control->getName()));

    m_description->setText(strtoqstr(m_control->getDescription()));
    m_controllerBox->setValue(int(m_control->getControllerValue()));

    QString hexValue;
    hexValue.sprintf("(0x%x)", m_control->getControllerValue());
    m_hexValue->setText(hexValue);

    m_minBox->setValue(m_control->getMin());
    m_maxBox->setValue(m_control->getMax());
    m_defaultBox->setValue(m_control->getDefault());

    int pos = 0, setItem = 0;
    ColourMap &colourMap = m_doc->getComposition().getGeneralColourMap();
    RCMap::const_iterator it;
    for (it = colourMap.begin(); it != colourMap.end(); ++it)
        if (m_control->getColourIndex() == it->first)
            setItem = pos++;

    m_colourCombo->setCurrentItem(setItem);

    // set combo position
    m_ipbPosition->setCurrentItem(m_control->getIPBPosition() + 1);

    // If the type has changed and there are no defaults then we have to
    // supply some.
    //
    if (qstrtostr(m_typeCombo->currentText()) == PitchBend::EventType ||
            qstrtostr(m_typeCombo->currentText()) == KeyPressure::EventType ||
            qstrtostr(m_typeCombo->currentText()) == ChannelPressure::EventType) {
        m_controllerBox->setEnabled(false);
        m_ipbPosition->setEnabled(false);
        m_colourCombo->setEnabled(false);
        m_hexValue->setEnabled(false);
        m_minBox->setEnabled(false);
        m_maxBox->setEnabled(false);
        m_defaultBox->setEnabled(false);
    } else if (qstrtostr(m_typeCombo->currentText()) == Controller::EventType) {
        m_controllerBox->setEnabled(true);
        m_ipbPosition->setEnabled(true);
        m_colourCombo->setEnabled(true);
        m_hexValue->setEnabled(true);
        m_minBox->setEnabled(true);
        m_maxBox->setEnabled(true);
        m_defaultBox->setEnabled(true);
    }

}

void
ControlParameterEditDialog::slotNameChanged(const QString &str)
{
    RG_DEBUG << "ControlParameterEditDialog::slotNameChanged" << endl;
    m_dialogControl.setName(qstrtostr(str));
}

void
ControlParameterEditDialog::slotTypeChanged(int value)
{
    RG_DEBUG << "ControlParameterEditDialog::slotTypeChanged" << endl;
    m_dialogControl.setType(qstrtostr(m_typeCombo->text(value)));

    populate();
}

void
ControlParameterEditDialog::slotDescriptionChanged(const QString &str)
{
    RG_DEBUG << "ControlParameterEditDialog::slotDescriptionChanged" << endl;
    m_dialogControl.setDescription(qstrtostr(str));
}

void
ControlParameterEditDialog::slotControllerChanged(int value)
{
    RG_DEBUG << "ControlParameterEditDialog::slotControllerChanged" << endl;
    m_dialogControl.setControllerValue(value);

    // set hex value
    QString hexValue;
    hexValue.sprintf("(0x%x)", value);
    m_hexValue->setText(hexValue);
}

void
ControlParameterEditDialog::slotMinChanged(int value)
{
    RG_DEBUG << "ControlParameterEditDialog::slotMinChanged" << endl;
    m_dialogControl.setMin(value);
}

void
ControlParameterEditDialog::slotMaxChanged(int value)
{
    RG_DEBUG << "ControlParameterEditDialog::slotMaxChanged" << endl;
    m_dialogControl.setMax(value);
}

void
ControlParameterEditDialog::slotDefaultChanged(int value)
{
    RG_DEBUG << "ControlParameterEditDialog::slotDefaultChanged" << endl;
    m_dialogControl.setDefault(value);
}

void
ControlParameterEditDialog::slotColourChanged(int value)
{
    RG_DEBUG << "ControlParameterEditDialog::slotColourChanged" << endl;
    m_dialogControl.setColourIndex(value);
}

void
ControlParameterEditDialog::slotIPBPositionChanged(int value)
{
    RG_DEBUG << "ControlParameterEditDialog::slotIPBPositionChanged" << endl;
    m_dialogControl.setIPBPosition(value - 1);
}

}
#include "ControlParameterEditDialog.moc"
