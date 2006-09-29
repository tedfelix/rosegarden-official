/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
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


#include "MIDIInstrumentParameterPanel.h"
#include <qlayout.h>

#include "sound/Midi.h"
#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/Colour.h"
#include "base/Composition.h"
#include "base/ControlParameter.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/Rotary.h"
#include "InstrumentParameterPanel.h"
#include "sound/MappedEvent.h"
#include "sound/MappedInstrument.h"
#include <kcombobox.h>
#include <ksqueezedtextlabel.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qfontmetrics.h>
#include <qframe.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qregexp.h>
#include <qsignalmapper.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

MIDIInstrumentParameterPanel::MIDIInstrumentParameterPanel(RosegardenGUIDoc *doc, QWidget* parent):
        InstrumentParameterPanel(doc, parent),
        m_rotaryFrame(0),
        m_rotaryMapper(new QSignalMapper(this))
{
    m_mainGrid = new QGridLayout(this, 10, 4, 2, 1);

    m_connectionLabel = new KSqueezedTextLabel(this);
    m_bankValue = new KComboBox(this);
    m_channelValue = new KComboBox(this);
    m_programValue = new KComboBox(this);
    m_variationValue = new KComboBox(this);
    m_bankCheckBox = new QCheckBox(this);
    m_programCheckBox = new QCheckBox(this);
    m_variationCheckBox = new QCheckBox(this);
    m_percussionCheckBox = new QCheckBox(this);

    m_bankValue->setSizeLimit(20);
    m_programValue->setSizeLimit(20);
    m_variationValue->setSizeLimit(20);

    m_bankLabel = new QLabel(i18n("Bank"), this);
    m_variationLabel = new QLabel(i18n("Variation"), this);
    m_programLabel = new QLabel(i18n("Program"), this);
    QLabel *channelLabel = new QLabel(i18n("Channel out"), this);
    QLabel *percussionLabel = new QLabel(i18n("Percussion"), this);

    // Ensure a reasonable amount of space in the program dropdowns even
    // if no instrument initially selected
    QFontMetrics metrics(m_programValue->font());
    int width22 = metrics.width("1234567890123456789012");
    int width25 = metrics.width("1234567890123456789012345");

    m_bankValue->setFixedWidth(width22);
    m_programValue->setFixedWidth(width22);
    m_variationValue->setFixedWidth(width22);

    m_connectionLabel->setFixedWidth(width25);
    m_connectionLabel->setAlignment(Qt::AlignCenter);

    // Configure the empty final row to accomodate any extra vertical space.

    m_mainGrid->setRowStretch(m_mainGrid->numRows() - 1, 1);

    // Configure the empty final column to accomodate any extra horizontal
    // space.

    m_mainGrid->setColStretch(m_mainGrid->numCols() - 1, 1);

    m_mainGrid->addMultiCellWidget(m_instrumentLabel, 0, 0, 0, 2, AlignCenter);
    m_mainGrid->addMultiCellWidget(m_connectionLabel, 1, 1, 0, 2, AlignCenter);

    m_mainGrid->addMultiCellWidget(channelLabel, 2, 2, 0, 1, AlignLeft);
    m_mainGrid->addWidget(m_channelValue, 2, 2, AlignRight);

    m_mainGrid->addMultiCellWidget(percussionLabel, 3, 3, 0, 1, AlignLeft);
    m_mainGrid->addWidget(m_percussionCheckBox, 3, 2, AlignRight);

    m_mainGrid->addWidget(m_bankLabel, 4, 0, AlignLeft);
    m_mainGrid->addWidget(m_bankCheckBox, 4, 1);
    m_mainGrid->addWidget(m_bankValue, 4, 2, AlignRight);

    m_mainGrid->addWidget(m_programLabel, 5, 0);
    m_mainGrid->addWidget(m_programCheckBox, 5, 1);
    m_mainGrid->addWidget(m_programValue, 5, 2, AlignRight);

    m_mainGrid->addWidget(m_variationLabel, 6, 0);
    m_mainGrid->addWidget(m_variationCheckBox, 6, 1);
    m_mainGrid->addWidget(m_variationValue, 6, 2, AlignRight);

    // Populate channel lists
    //
    for (int i = 0; i < 16; i++) {
        m_channelValue->insertItem(QString("%1").arg(i + 1));
    }

    m_channelValue->setSizeLimit(16);

    // Disable these by default - they are activate by their
    // checkboxes
    //
    m_programValue->setDisabled(true);
    m_bankValue->setDisabled(true);
    m_variationValue->setDisabled(true);

    // Only active if we have an Instrument selected
    //
    m_percussionCheckBox->setDisabled(true);
    m_programCheckBox->setDisabled(true);
    m_bankCheckBox->setDisabled(true);
    m_variationCheckBox->setDisabled(true);

    // Connect up the toggle boxes
    //
    connect(m_percussionCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotTogglePercussion(bool)));

    connect(m_programCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotToggleProgramChange(bool)));

    connect(m_bankCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotToggleBank(bool)));

    connect(m_variationCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(slotToggleVariation(bool)));


    // Connect activations
    //
    connect(m_bankValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_variationValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectVariation(int)));

    connect(m_programValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectProgram(int)));

    connect(m_channelValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectChannel(int)));

    // don't select any of the options in any dropdown
    m_programValue->setCurrentItem( -1);
    m_bankValue->setCurrentItem( -1);
    m_channelValue->setCurrentItem( -1);
    m_variationValue->setCurrentItem( -1);

    connect(m_rotaryMapper, SIGNAL(mapped(int)),
            this, SLOT(slotControllerChanged(int)));
}

void
MIDIInstrumentParameterPanel::setupForInstrument(Instrument *instrument)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::setupForInstrument" << endl;
    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (instrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::setupForInstrument:"
        << " No MidiDevice for Instrument "
        << instrument->getId() << endl;
        return ;
    }

    m_selectedInstrument = instrument;

    // Set instrument name
    //
    m_instrumentLabel->setText(strtoqstr(instrument->getPresentationName()));

    // Set Studio Device name
    //
    QString connection(strtoqstr(md->getConnection()));
    if (connection == "") {
        m_connectionLabel->setText(i18n("[ %1 ]").arg(i18n("No connection")));
    } else {

        // remove trailing "(duplex)", "(read only)", "(write only)" etc
        connection.replace(QRegExp("\\s*\\([^)0-9]+\\)\\s*$"), "");

        QString text = i18n("[ %1 ]").arg(connection);
        /*QString origText(text);

        QFontMetrics metrics(m_connectionLabel->fontMetrics());
        int maxwidth = metrics.width
            ("Program: [X]   Acoustic Grand Piano 123");// kind of arbitrary!

        int hlen = text.length() / 2;
        while (metrics.width(text) > maxwidth && text.length() > 10) {
            --hlen;
            text = origText.left(hlen) + "..." + origText.right(hlen);
        }

        if (text.length() > origText.length() - 7) text = origText;*/
        m_connectionLabel->setText(text);
    }

    // Enable all check boxes
    //
    m_percussionCheckBox->setDisabled(false);
    m_programCheckBox->setDisabled(false);
    m_bankCheckBox->setDisabled(false);
    m_variationCheckBox->setDisabled(false);

    // Activate all checkboxes
    //
    m_percussionCheckBox->setChecked(instrument->isPercussion());
    m_programCheckBox->setChecked(instrument->sendsProgramChange());
    m_bankCheckBox->setChecked(instrument->sendsBankSelect());
    m_variationCheckBox->setChecked(instrument->sendsBankSelect());

    // Basic parameters
    //
    m_channelValue->setCurrentItem((int)instrument->getMidiChannel());

    // Check for program change
    //
    populateBankList();
    populateProgramList();
    populateVariationList();

    // Setup the ControlParameters
    //
    setupControllers(md);

    // Set all the positions by controller number
    //
    for (RotaryMap::iterator it = m_rotaries.begin() ;
            it != m_rotaries.end(); ++it) {
        MidiByte value = 0;

        // Special cases
        //
        if (it->first == MIDI_CONTROLLER_PAN)
            value = int(instrument->getPan());
        else if (it->first == MIDI_CONTROLLER_VOLUME)
            value = int(instrument->getVolume());
        else {
            try {
                value = instrument->getControllerValue(
                            MidiByte(it->first));
            } catch (...) {
                continue;
            }
        }

        setRotaryToValue(it->first, int(value));
    }
}

void
MIDIInstrumentParameterPanel::setupControllers(MidiDevice *md)
{
    if (!m_rotaryFrame) {
        m_rotaryFrame = new QFrame(this);
        m_mainGrid->addMultiCellWidget(m_rotaryFrame, 8, 8, 0, 2, Qt::AlignHCenter);
        m_rotaryGrid = new QGridLayout(m_rotaryFrame, 10, 3, 8, 1);
        m_rotaryGrid->addItem(new QSpacerItem(10, 4), 0, 1);
    }

    // To cut down on flicker, we avoid destroying and recreating
    // widgets as far as possible here.  If a label already exists,
    // we just set its text; if a rotary exists, we only replace it
    // if we actually need a different one.

    Composition &comp = m_doc->getComposition();
    ControlList list = md->getControlParameters();

    // sort by IPB position
    //
    std::sort(list.begin(), list.end(),
              ControlParameter::ControlPositionCmp());

    int count = 0;
    RotaryMap::iterator rmi = m_rotaries.begin();

    for (ControlList::iterator it = list.begin();
            it != list.end(); ++it) {
        if (it->getIPBPosition() == -1)
            continue;

        // Get the knob colour - only if the colour is non-default (>0)
        //
        QColor knobColour = Qt::black; // special case for Rotary
        if (it->getColourIndex() > 0) {
            Colour c =
                comp.getGeneralColourMap().getColourByIndex
                (it->getColourIndex());
            knobColour = QColor(c.getRed(), c.getGreen(), c.getBlue());
        }

        Rotary *rotary = 0;

        if (rmi != m_rotaries.end()) {

            // Update the controller number that is associated with the
            // existing rotary widget.

            rmi->first = it->getControllerValue();

            // Update the properties of the existing rotary widget.

            rotary = rmi->second.first;
            int redraw = 0; // 1 -> position, 2 -> all

            if (rotary->getMinValue() != it->getMin()) {
                rotary->setMinValue(it->getMin());
                redraw = 1;
            }
            if (rotary->getMaxValue() != it->getMax()) {
                rotary->setMaxValue(it->getMax());
                redraw = 1;
            }
            if (rotary->getKnobColour() != knobColour) {
                rotary->setKnobColour(knobColour);
                redraw = 2;
            }
            if (redraw == 1 || rotary->getPosition() != it->getDefault()) {
                rotary->setPosition(it->getDefault());
                if (redraw == 1)
                    redraw = 0;
            }
            if (redraw == 2) {
                rotary->repaint();
            }

            // Update the controller name that is associated with
            // with the existing rotary widget.

            QLabel *label = rmi->second.second;
            label->setText(strtoqstr(it->getName()));

            ++rmi;

        } else {

            QHBox *hbox = new QHBox(m_rotaryFrame);
            hbox->setSpacing(8);

            float smallStep = 1.0;

            float bigStep = 5.0;
            if (it->getMax() - it->getMin() < 10)
                bigStep = 1.0;
            else if (it->getMax() - it->getMin() < 20)
                bigStep = 2.0;

            rotary = new Rotary
                     (hbox,
                      it->getMin(),
                      it->getMax(),
                      smallStep,
                      bigStep,
                      it->getDefault(),
                      20,
                      Rotary::NoTicks,
                      false,
                      it->getDefault() == 64); //!!! hacky

            rotary->setKnobColour(knobColour);

            // Add a label
            QLabel *label = new KSqueezedTextLabel(strtoqstr(it->getName()), hbox);

            RG_DEBUG << "Adding new widget at " << (count / 2) << "," << (count % 2) << endl;

            // Add the compound widget
            //
            m_rotaryGrid->addWidget(hbox, count / 2, (count % 2) * 2, AlignLeft);
            hbox->show();

            // Add to list
            //
            m_rotaries.push_back(std::pair<int, RotaryPair>
                                 (it->getControllerValue(),
                                  RotaryPair(rotary, label)));

            // Connect
            //
            connect(rotary, SIGNAL(valueChanged(float)),
                    m_rotaryMapper, SLOT(map()));

            rmi = m_rotaries.end();
        }

        // Add signal mapping
        //
        m_rotaryMapper->setMapping(rotary,
                                   int(it->getControllerValue()));

        count++;
    }

    if (rmi != m_rotaries.end()) {
        for (RotaryMap::iterator rmj = rmi; rmj != m_rotaries.end(); ++rmj) {
            delete rmj->second.first;
            delete rmj->second.second;
        }
        m_rotaries = std::vector<std::pair<int, RotaryPair> >
                     (m_rotaries.begin(), rmi);
    }

    m_rotaryFrame->show();
}

void
MIDIInstrumentParameterPanel::setRotaryToValue(int controller, int value)
{
    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::setRotaryToValue - "
             << "controller = " << controller
             << ", value = " << value << std::endl;
             */

    for (RotaryMap::iterator it = m_rotaries.begin() ; it != m_rotaries.end(); ++it) {
        if (it->first == controller) {
            it->second.first->setPosition(float(value));
            return ;
        }
    }
}

void
MIDIInstrumentParameterPanel::slotSelectChannel(int index)
{
    if (m_selectedInstrument == 0)
        return ;

    m_selectedInstrument->setMidiChannel(index);

    // don't use the emit - use this method instead
    StudioControl::sendMappedInstrument(
        MappedInstrument(m_selectedInstrument));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::populateBankList()
{
    if (m_selectedInstrument == 0)
        return ;

    m_bankValue->clear();
    m_banks.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::populateBankList:"
        << " No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    int currentBank = -1;
    BankList banks;

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::populateBankList: "
             << "variation type is " << md->getVariationType() << endl;
             */

    if (md->getVariationType() == MidiDevice::NoVariations) {

        banks = md->getBanks(m_selectedInstrument->isPercussion());

        if (!banks.empty()) {
            if (m_bankLabel->isHidden()) {
                m_bankLabel->show();
                m_bankCheckBox->show();
                m_bankValue->show();
            }
        } else {
            m_bankLabel->hide();
            m_bankCheckBox->hide();
            m_bankValue->hide();
        }

        for (unsigned int i = 0; i < banks.size(); ++i) {
            if (m_selectedInstrument->getProgram().getBank() == banks[i]) {
                currentBank = i;
            }
        }

    } else {

        MidiByteList bytes;
        bool useMSB = (md->getVariationType() == MidiDevice::VariationFromLSB);

        if (useMSB) {
            bytes = md->getDistinctMSBs(m_selectedInstrument->isPercussion());
        } else {
            bytes = md->getDistinctLSBs(m_selectedInstrument->isPercussion());
        }

        if (bytes.size() < 2) {
            if (!m_bankLabel->isHidden()) {
                m_bankLabel->hide();
                m_bankCheckBox->hide();
                m_bankValue->hide();
            }
        } else {
            if (m_bankLabel->isHidden()) {
                m_bankLabel->show();
                m_bankCheckBox->show();
                m_bankValue->show();
            }
        }

        if (useMSB) {
            for (unsigned int i = 0; i < bytes.size(); ++i) {
                BankList bl = md->getBanksByMSB
                              (m_selectedInstrument->isPercussion(), bytes[i]);
                RG_DEBUG << "MIDIInstrumentParameterPanel::populateBankList: have " << bl.size() << " variations for msb " << bytes[i] << endl;

                if (bl.size() == 0)
                    continue;
                if (m_selectedInstrument->getMSB() == bytes[i]) {
                    currentBank = banks.size();
                }
                banks.push_back(bl[0]);
            }
        } else {
            for (unsigned int i = 0; i < bytes.size(); ++i) {
                BankList bl = md->getBanksByLSB
                              (m_selectedInstrument->isPercussion(), bytes[i]);
                RG_DEBUG << "MIDIInstrumentParameterPanel::populateBankList: have " << bl.size() << " variations for lsb " << bytes[i] << endl;
                if (bl.size() == 0)
                    continue;
                if (m_selectedInstrument->getLSB() == bytes[i]) {
                    currentBank = banks.size();
                }
                banks.push_back(bl[0]);
            }
        }
    }

    for (BankList::const_iterator i = banks.begin();
            i != banks.end(); ++i) {
        m_banks.push_back(*i);
        m_bankValue->insertItem(strtoqstr(i->getName()));
    }

    m_bankValue->setEnabled(m_selectedInstrument->sendsBankSelect());

    if (currentBank < 0 && !banks.empty()) {
        m_bankValue->setCurrentItem(0);
        slotSelectBank(0);
    } else {
        m_bankValue->setCurrentItem(currentBank);
    }
}

void
MIDIInstrumentParameterPanel::populateProgramList()
{
    if (m_selectedInstrument == 0)
        return ;

    m_programValue->clear();
    m_programs.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::populateProgramList: No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::populateProgramList:"
             << " variation type is " << md->getVariationType() << endl;
    */

    MidiBank bank( m_selectedInstrument->isPercussion(),
                   m_selectedInstrument->getMSB(),
                   m_selectedInstrument->getLSB());

    if (m_selectedInstrument->sendsBankSelect()) {
        bank = m_selectedInstrument->getProgram().getBank();
    }

    int currentProgram = -1;

    ProgramList programs = md->getPrograms(bank);

    if (!programs.empty()) {
        if (m_programLabel->isHidden()) {
            m_programLabel->show();
            m_programCheckBox->show();
            m_programValue->show();
        }
    } else {
        m_programLabel->hide();
        m_programCheckBox->hide();
        m_programValue->hide();
    }

    for (unsigned int i = 0; i < programs.size(); ++i) {
        std::string programName = programs[i].getName();
        if (programName != "") {
            m_programValue->insertItem(QString("%1. %2")
                                       .arg(programs[i].getProgram() + 1)
                                       .arg(strtoqstr(programName)));
            if (m_selectedInstrument->getProgram() == programs[i]) {
                currentProgram = m_programs.size();
            }
            m_programs.push_back(programs[i]);
        }
    }

    m_programValue->setEnabled(m_selectedInstrument->sendsProgramChange());

    if (currentProgram < 0 && !m_programs.empty()) {
        m_programValue->setCurrentItem(0);
        slotSelectProgram(0);
    } else {
        m_programValue->setCurrentItem(currentProgram);

        // Ensure that stored program change value is same as the one
        // we're now showing (BUG 937371)
        //
        if (!m_programs.empty()) {
            m_selectedInstrument->setProgramChange
            ((m_programs[m_programValue->currentItem()]).getProgram());
        }
    }
}

void
MIDIInstrumentParameterPanel::populateVariationList()
{
    if (m_selectedInstrument == 0)
        return ;

    m_variationValue->clear();
    m_variations.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::populateVariationList: No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::populateVariationList:"
             << " variation type is " << md->getVariationType() << endl;
    */

    if (md->getVariationType() == MidiDevice::NoVariations) {
        if (!m_variationLabel->isHidden()) {
            m_variationLabel->hide();
            m_variationCheckBox->hide();
            m_variationValue->hide();
        }
        return ;
    }

    bool useMSB = (md->getVariationType() == MidiDevice::VariationFromMSB);
    MidiByteList variations;

    if (useMSB) {
        MidiByte lsb = m_selectedInstrument->getLSB();
        variations = md->getDistinctMSBs(m_selectedInstrument->isPercussion(),
                                         lsb);
        RG_DEBUG << "MIDIInstrumentParameterPanel::populateVariationList: have " << variations.size() << " variations for lsb " << lsb << endl;

    } else {
        MidiByte msb = m_selectedInstrument->getMSB();
        variations = md->getDistinctLSBs(m_selectedInstrument->isPercussion(),
                                         msb);
        RG_DEBUG << "MIDIInstrumentParameterPanel::populateVariationList: have " << variations.size() << " variations for msb " << msb << endl;
    }

    m_variationValue->setCurrentItem( -1);

    MidiProgram defaultProgram;

    if (useMSB) {
        defaultProgram = MidiProgram
                         (MidiBank(m_selectedInstrument->isPercussion(),
                                   0,
                                   m_selectedInstrument->getLSB()),
                          m_selectedInstrument->getProgramChange());
    } else {
        defaultProgram = MidiProgram
                         (MidiBank(m_selectedInstrument->isPercussion(),
                                   m_selectedInstrument->getMSB(),
                                   0),
                          m_selectedInstrument->getProgramChange());
    }
    std::string defaultProgramName = md->getProgramName(defaultProgram);

    int currentVariation = -1;

    for (unsigned int i = 0; i < variations.size(); ++i) {

        MidiProgram program;

        if (useMSB) {
            program = MidiProgram
                      (MidiBank(m_selectedInstrument->isPercussion(),
                                variations[i],
                                m_selectedInstrument->getLSB()),
                       m_selectedInstrument->getProgramChange());
        } else {
            program = MidiProgram
                      (MidiBank(m_selectedInstrument->isPercussion(),
                                m_selectedInstrument->getMSB(),
                                variations[i]),
                       m_selectedInstrument->getProgramChange());
        }

        std::string programName = md->getProgramName(program);

        if (programName != "") { // yes, that is how you know whether it exists
            /*
            	    m_variationValue->insertItem(programName == defaultProgramName ?
            					 i18n("(default)") :
            					 strtoqstr(programName));
            */
            m_variationValue->insertItem(QString("%1. %2")
                                         .arg(variations[i] + 1)
                                         .arg(strtoqstr(programName)));
            if (m_selectedInstrument->getProgram() == program) {
                currentVariation = m_variations.size();
            }
            m_variations.push_back(variations[i]);
        }
    }

    if (currentVariation < 0 && !m_variations.empty()) {
        m_variationValue->setCurrentItem(0);
        slotSelectVariation(0);
    } else {
        m_variationValue->setCurrentItem(currentVariation);
    }

    if (m_variations.size() < 2) {
        if (!m_variationLabel->isHidden()) {
            m_variationLabel->hide();
            m_variationCheckBox->hide();
            m_variationValue->hide();
        }

    } else {
        //!!! seem to have problems here -- the grid layout doesn't
        //like us adding stuff in the middle so if we go from 1
        //visible row (say program) to 2 (program + variation) the
        //second one overlaps the control knobs

        if (m_variationLabel->isHidden()) {
            m_variationLabel->show();
            m_variationCheckBox->show();
            m_variationValue->show();
        }

        if (m_programValue->width() > m_variationValue->width()) {
            m_variationValue->setMinimumWidth(m_programValue->width());
        } else {
            m_programValue->setMinimumWidth(m_variationValue->width());
        }
    }

    m_variationValue->setEnabled(m_selectedInstrument->sendsBankSelect());
}

void
MIDIInstrumentParameterPanel::slotTogglePercussion(bool value)
{
    if (m_selectedInstrument == 0) {
        m_percussionCheckBox->setChecked(false);
        emit updateAllBoxes();
        return ;
    }

    m_selectedInstrument->setPercussion(value);

    populateBankList();
    populateProgramList();
    populateVariationList();

    sendBankAndProgram();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
                               strtoqstr(m_selectedInstrument->
                                         getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotToggleBank(bool value)
{
    if (m_selectedInstrument == 0) {
        m_bankCheckBox->setChecked(false);
        emit updateAllBoxes();
        return ;
    }

    m_variationCheckBox->setChecked(value);
    m_selectedInstrument->setSendBankSelect(value);

    m_bankValue->setDisabled(!value);
    populateBankList();
    populateProgramList();
    populateVariationList();

    sendBankAndProgram();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
                               strtoqstr(m_selectedInstrument->
                                         getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotToggleProgramChange(bool value)
{
    if (m_selectedInstrument == 0) {
        m_programCheckBox->setChecked(false);
        emit updateAllBoxes();
        return ;
    }

    m_selectedInstrument->setSendProgramChange(value);

    m_programValue->setDisabled(!value);
    populateProgramList();
    populateVariationList();

    if (value)
        sendBankAndProgram();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
                               strtoqstr(m_selectedInstrument->
                                         getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotToggleVariation(bool value)
{
    if (m_selectedInstrument == 0) {
        m_variationCheckBox->setChecked(false);
        emit updateAllBoxes();
        return ;
    }

    m_bankCheckBox->setChecked(value);
    m_selectedInstrument->setSendBankSelect(value);

    m_variationValue->setDisabled(!value);
    populateVariationList();

    sendBankAndProgram();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
                               strtoqstr(m_selectedInstrument->
                                         getProgramName()));
    emit updateAllBoxes();
}

void
MIDIInstrumentParameterPanel::slotSelectBank(int index)
{
    if (m_selectedInstrument == 0)
        return ;

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::slotSelectBank: No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    const MidiBank *bank = &m_banks[index];

    bool change = false;

    if (md->getVariationType() != MidiDevice::VariationFromLSB) {
        if (m_selectedInstrument->getLSB() != bank->getLSB()) {
            m_selectedInstrument->setLSB(bank->getLSB());
            change = true;
        }
    }
    if (md->getVariationType() != MidiDevice::VariationFromMSB) {
        if (m_selectedInstrument->getMSB() != bank->getMSB()) {
            m_selectedInstrument->setMSB(bank->getMSB());
            change = true;
        }
    }

    populateProgramList();

    if (change) {
        sendBankAndProgram();
        emit updateAllBoxes();
    }
}

void
MIDIInstrumentParameterPanel::slotSelectProgram(int index)
{
    const MidiProgram *prg = &m_programs[index];
    if (prg == 0) {
        RG_DEBUG << "program change not found in bank" << endl;
        return ;
    }

    bool change = false;
    if (m_selectedInstrument->getProgramChange() != prg->getProgram()) {
        m_selectedInstrument->setProgramChange(prg->getProgram());
        change = true;
    }

    populateVariationList();

    if (change) {
        sendBankAndProgram();
        emit changeInstrumentLabel(m_selectedInstrument->getId(),
                                   strtoqstr(m_selectedInstrument->
                                             getProgramName()));
        emit updateAllBoxes();
    }
}

void
MIDIInstrumentParameterPanel::slotSelectVariation(int index)
{
    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::slotSelectVariation: No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    if (index < 0 || index > int(m_variations.size())) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::slotSelectVariation: index " << index << " out of range" << endl;
        return ;
    }

    MidiByte v = m_variations[index];

    bool change = false;

    if (md->getVariationType() == MidiDevice::VariationFromLSB) {
        if (m_selectedInstrument->getLSB() != v) {
            m_selectedInstrument->setLSB(v);
            change = true;
        }
    } else if (md->getVariationType() == MidiDevice::VariationFromMSB) {
        if (m_selectedInstrument->getMSB() != v) {
            m_selectedInstrument->setMSB(v);
            change = true;
        }
    }

    if (change) {
        sendBankAndProgram();
    }
}

void
MIDIInstrumentParameterPanel::sendBankAndProgram()
{
    if (m_selectedInstrument == 0)
        return ;

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::sendBankAndProgram: No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    if (m_selectedInstrument->sendsBankSelect()) {

        // Send the bank select message before any PC message
        //
        MappedEvent mEMSB(m_selectedInstrument->getId(),
                          MappedEvent::MidiController,
                          MIDI_CONTROLLER_BANK_MSB,
                          m_selectedInstrument->getMSB());

        RG_DEBUG << "MIDIInstrumentParameterPanel::sendBankAndProgram - "
        << "sending MSB = "
        << int(m_selectedInstrument->getMSB())
        << endl;

        StudioControl::sendMappedEvent(mEMSB);

        MappedEvent mELSB(m_selectedInstrument->getId(),
                          MappedEvent::MidiController,
                          MIDI_CONTROLLER_BANK_LSB,
                          m_selectedInstrument->getLSB());

        RG_DEBUG << "MIDIInstrumentParameterPanel::sendBankAndProgram - "
        << "sending LSB = "
        << int(m_selectedInstrument->getLSB())
        << endl;

        StudioControl::sendMappedEvent(mELSB);
    }

    MappedEvent mE(m_selectedInstrument->getId(),
                   MappedEvent::MidiProgramChange,
                   m_selectedInstrument->getProgramChange(),
                   (MidiByte)0);

    RG_DEBUG << "MIDIInstrumentParameterPanel::sendBankAndProgram - "
    << "sending program change = "
    << int(m_selectedInstrument->getProgramChange())
    << endl;


    // Send the controller change
    //
    StudioControl::sendMappedEvent(mE);
}

void
MIDIInstrumentParameterPanel::slotControllerChanged(int controllerNumber)
{

    RG_DEBUG << "MIDIInstrumentParameterPanel::slotControllerChanged - "
    << "controller = " << controllerNumber << "\n";


    if (m_selectedInstrument == 0)
        return ;

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md)
        return ;

    /*
    ControlParameter *controller = 
    md->getControlParameter(MidiByte(controllerNumber));
        */

    int value = getValueFromRotary(controllerNumber);

    if (value == -1) {
        RG_DEBUG << "MIDIInstrumentParameterPanel::slotControllerChanged - "
        << "couldn't get value of rotary for controller "
        << controllerNumber << endl;
        return ;
    }


    // two special cases
    if (controllerNumber == int(MIDI_CONTROLLER_PAN)) {
        float adjValue = value;
        if (m_selectedInstrument->getType() == Instrument::Audio ||
                m_selectedInstrument->getType() == Instrument::SoftSynth)
            value += 100;

        m_selectedInstrument->setPan(MidiByte(adjValue));
    } else if (controllerNumber == int(MIDI_CONTROLLER_VOLUME)) {
        m_selectedInstrument->setVolume(MidiByte(value));
    } else // just set the controller (this will create it on the instrument if
        // it doesn't exist)
    {
        m_selectedInstrument->setControllerValue(MidiByte(controllerNumber),
                MidiByte(value));

        RG_DEBUG << "SET CONTROLLER VALUE (" << controllerNumber << ") = " << value << endl;
    }
    /*
    else
    {
        RG_DEBUG << "MIDIInstrumentParameterPanel::slotControllerChanged - "
                 << "no controller retrieved\n";
        return;
    }
    */

    MappedEvent mE(m_selectedInstrument->getId(),
                   MappedEvent::MidiController,
                   (MidiByte)controllerNumber,
                   (MidiByte)value);
    StudioControl::sendMappedEvent(mE);

    emit updateAllBoxes();
    emit instrumentParametersChanged(m_selectedInstrument->getId());

}

int
MIDIInstrumentParameterPanel::getValueFromRotary(int rotary)
{
    for (RotaryMap::iterator it = m_rotaries.begin(); it != m_rotaries.end(); ++it) {
        if (it->first == rotary)
            return int(it->second.first->getPosition());
    }

    return -1;
}

void
MIDIInstrumentParameterPanel::showAdditionalControls(bool showThem)
{
    m_instrumentLabel->setShown(showThem);
    int index = 0;
    for (RotaryMap::iterator it = m_rotaries.begin(); it != m_rotaries.end(); ++it) {
        it->second.first->parentWidget()->setShown(showThem || (index < 8));
        //it->second.first->setShown(showThem || (index < 8));
        //it->second.second->setShown(showThem || (index < 8));
        index++;
    }
}

}
#include "MIDIInstrumentParameterPanel.moc"
