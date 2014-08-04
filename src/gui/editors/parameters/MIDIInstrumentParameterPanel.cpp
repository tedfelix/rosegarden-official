/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2014 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MIDIInstrumentParameterPanel]"

#include "MIDIInstrumentParameterPanel.h"

#include "InstrumentParameterPanel.h"
#include "document/RosegardenDocument.h"
#include "gui/widgets/SqueezedLabel.h"
#include "gui/widgets/Rotary.h"
#include "sequencer/RosegardenSequencer.h"
#include "misc/Debug.h"
#include "base/Colour.h"
#include "base/Composition.h"
#include "base/ControlParameter.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"

#include <QComboBox>
#include <QCheckBox>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QRegExp>
#include <QSignalMapper>
#include <QString>
#include <QWidget>

#include <algorithm>  // std::sort
#include <string>


namespace Rosegarden
{

MIDIInstrumentParameterPanel::MIDIInstrumentParameterPanel(RosegardenDocument *doc, QWidget* parent):
        InstrumentParameterPanel(doc, parent),
        m_rotaryFrame(0),
        m_rotaryMapper(new QSignalMapper(this))
{
    setObjectName("MIDI Instrument Parameter Panel");

    QFont f;
    f.setPointSize(f.pointSize() * 90 / 100);
    f.setBold(false);

    QFontMetrics metrics(f);
    int width25 = metrics.width("1234567890123456789012345");

    m_instrumentLabel->setFont(f);
    m_instrumentLabel->setFixedWidth(width25);
    m_instrumentLabel->setAlignment(Qt::AlignCenter);

    setContentsMargins(2, 2, 2, 2);
    m_mainGrid = new QGridLayout(this);
    m_mainGrid->setMargin(0);
    m_mainGrid->setSpacing(1);
    setLayout(m_mainGrid);

    m_connectionLabel = new SqueezedLabel(this);
    m_bankComboBox = new QComboBox(this);
    m_programComboBox = new QComboBox(this);
    m_variationComboBox = new QComboBox(this);
    m_bankCheckBox = new QCheckBox(this);
    m_programCheckBox = new QCheckBox(this);
    m_variationCheckBox = new QCheckBox(this);
    m_percussionCheckBox = new QCheckBox(this);
    m_channelValue = new QComboBox(this);

    // Everything else sets up elsewhere, but these don't vary per instrument:
    m_channelValue->addItem(tr("auto"));
    m_channelValue->addItem(tr("fixed"));

    m_connectionLabel->setFont(f);
    m_bankComboBox->setFont(f);
    m_programComboBox->setFont(f);
    m_variationComboBox->setFont(f);
    m_bankCheckBox->setFont(f);
    m_programCheckBox->setFont(f);
    m_variationCheckBox->setFont(f);
    m_percussionCheckBox->setFont(f);
    m_channelValue->setFont(f);

    m_bankComboBox->setToolTip(tr("<qt>Set the MIDI bank from which to select programs</qt>"));
    m_programComboBox->setToolTip(tr("<qt>Set the MIDI program or &quot;patch&quot;</p></qt>"));
    m_variationComboBox->setToolTip(tr("<qt>Set variations on the program above, if available in the studio</qt>"));
    m_percussionCheckBox->setToolTip(tr("<qt><p>Check this to tell Rosegarden that this is a percussion instrument.  This allows you access to any percussion key maps and drum kits you may have configured in the studio</p></qt>"));
    m_channelValue->setToolTip(tr("<qt><p><i>Auto</i>, allocate channel automatically; <i>Fixed</i>, fix channel to instrument number</p></qt>"));

    m_bankComboBox->setMaxVisibleItems(20);
    m_programComboBox->setMaxVisibleItems(20);
    m_variationComboBox->setMaxVisibleItems(20);
    m_channelValue->setMaxVisibleItems(2);
    
    m_bankLabel = new QLabel(tr("Bank"), this);
    m_variationLabel = new QLabel(tr("Variation"), this);
    m_programLabel = new QLabel(tr("Program"), this);
    QLabel *percussionLabel = new QLabel(tr("Percussion"), this);
    QLabel *channelLabel = new QLabel(tr("Channel"), this);
    
    m_bankLabel->setFont(f);
    m_variationLabel->setFont(f);
    m_programLabel->setFont(f);
    percussionLabel->setFont(f);
    channelLabel->setFont(f);
    
    // Ensure a reasonable amount of space in the program dropdowns even
    // if no instrument initially selected

    // setMinimumWidth() using QFontMetrics wasn't cutting it at all, so let's
    // try what I used in the plugin manager dialog, with
    // setMinimumContentsLength() instead:
    QString metric("Acoustic Grand Piano #42B");
    int width22 = metric.size();
    
    m_bankComboBox->setMinimumContentsLength(width22);
    m_programComboBox->setMinimumContentsLength(width22);
    m_variationComboBox->setMinimumContentsLength(width22);
    m_channelValue->setMinimumContentsLength(width22);

    // we still have to use the QFontMetrics here, or a SqueezedLabel will
    // squeeze itself down to 0.
    int width30 = metrics.width("123456789012345678901234567890");
    m_connectionLabel->setFixedWidth(width30);
    m_connectionLabel->setAlignment(Qt::AlignCenter);
    
    
    QString programTip = tr("<qt>Use program changes from an external source to manipulate these controls (only valid for the currently-active track) [Shift + P]</qt>");
    m_receiveExternalCheckBox = new QCheckBox(this);
    m_receiveExternalCheckBox->setFont(f);
    m_receiveExternalLabel = new QLabel(tr("Receive external"), this);
    m_receiveExternalLabel->setFont(f);
    m_receiveExternalLabel->setToolTip(programTip);
    
    m_receiveExternalCheckBox->setDisabled(false);
    m_receiveExternalCheckBox->setChecked(false);
    m_receiveExternalCheckBox->setToolTip(programTip);
    m_receiveExternalCheckBox->setShortcut((QKeySequence)"Shift+P");



    m_mainGrid->setColumnStretch(2, 1);

    m_mainGrid->addWidget(m_instrumentLabel, 0, 0, 1, 4, Qt::AlignCenter);
    m_mainGrid->addWidget(m_connectionLabel, 1, 0, 1, 4, Qt::AlignCenter);

    m_mainGrid->addWidget(percussionLabel, 3, 0, 1, 2, Qt::AlignLeft);
    m_mainGrid->addWidget(m_percussionCheckBox, 3, 3, Qt::AlignLeft);

    m_mainGrid->addWidget(m_bankLabel, 4, 0, Qt::AlignLeft);
    m_mainGrid->addWidget(m_bankCheckBox, 4, 1, Qt::AlignRight);
    m_mainGrid->addWidget(m_bankComboBox, 4, 2, 1, 2, Qt::AlignRight);

    m_mainGrid->addWidget(m_programLabel, 5, 0, Qt::AlignLeft);
    m_mainGrid->addWidget(m_programCheckBox, 5, 1, Qt::AlignRight);
    m_mainGrid->addWidget(m_programComboBox, 5, 2, 1, 2, Qt::AlignRight);

    m_mainGrid->addWidget(m_variationLabel, 6, 0);
    m_mainGrid->addWidget(m_variationCheckBox, 6, 1);
    m_mainGrid->addWidget(m_variationComboBox, 6, 2, 1, 2, Qt::AlignRight);
      
    m_mainGrid->addWidget(channelLabel, 7, 0, Qt::AlignLeft);
    m_mainGrid->addWidget(m_channelValue, 7, 2, 1, 2, Qt::AlignRight);

    m_mainGrid->addWidget(m_receiveExternalLabel, 8, 0, 1, 3, Qt::AlignLeft);
    m_mainGrid->addWidget(m_receiveExternalCheckBox, 8, 3, Qt::AlignLeft);

    // Disable these by default - they are activated by their checkboxes
    //
    m_programComboBox->setDisabled(true);
    m_bankComboBox->setDisabled(true);
    m_variationComboBox->setDisabled(true);

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
    connect(m_bankComboBox, SIGNAL(activated(int)),
            this, SLOT(slotSelectBank(int)));

    connect(m_variationComboBox, SIGNAL(activated(int)),
            this, SLOT(slotSelectVariation(int)));

    connect(m_programComboBox, SIGNAL(activated(int)),
            this, SLOT(slotSelectProgram(int)));
    
    // don't select any of the options in any dropdown
    m_programComboBox->setCurrentIndex( -1);
    m_bankComboBox->setCurrentIndex( -1);
    m_variationComboBox->setCurrentIndex( -1);
    m_channelValue->setCurrentIndex(-1);

    connect(m_rotaryMapper, SIGNAL(mapped(int)),
            this, SLOT(slotControllerChanged(int)));
    connect(m_channelValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectChannel(int)));
}

void
MIDIInstrumentParameterPanel::clearReceiveExternal()
{
    m_receiveExternalCheckBox->setChecked(false);
}

void
MIDIInstrumentParameterPanel::setupForInstrument(Instrument *instrument)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::setupForInstrument" << endl;

    // In some cases setupForInstrument gets called several times.
    // This shortcuts this activity since only one setup is needed.
    // ??? Problem is that this prevents legitimate changes to the
    //     instrument from getting to the UI.
    if (m_selectedInstrument == instrument) {
        RG_DEBUG << "MIDIInstrumentParameterPanel::setupForInstrument "
                 << "-- early exit.  instrument didn't change." << endl;
        return;
    }

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (instrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::setupForInstrument:"
        << " No MidiDevice for Instrument "
        << instrument->getId() << endl;
        return ;
    }

    setSelectedInstrument(instrument,
                          instrument->getLocalizedPresentationName());

    // Set Studio Device name
    //
    QString connection(RosegardenSequencer::getInstance()->getConnection(md->getId()));

    if (connection == "") {
        m_connectionLabel->setText(tr("[ %1 ]").arg(tr("No connection")));
    } else {

        // remove trailing "(duplex)", "(read only)", "(write only)" etc
        connection.replace(QRegExp("\\s*\\([^)0-9]+\\)\\s*$"), "");

        QString text = QObject::tr("[ %1 ]").arg(connection);
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
        m_connectionLabel->setText(QObject::tr(text.toStdString().c_str()));
    }

    // Enable all check boxes
    //
    m_percussionCheckBox->setDisabled(false);
    m_programCheckBox->setDisabled(false);
    m_bankCheckBox->setDisabled(false);
    m_variationCheckBox->setDisabled(false);

    // Activate all checkboxes
    //
    
    // Block signals
    m_percussionCheckBox-> blockSignals(true);
    m_programCheckBox->    blockSignals(true);
    m_bankCheckBox->       blockSignals(true);
    m_variationCheckBox->  blockSignals(true);
    m_channelValue->       blockSignals(true);
    
    // Change state
    m_percussionCheckBox->setChecked(instrument->isPercussion());
    m_programCheckBox->setChecked(instrument->sendsProgramChange());
    m_bankCheckBox->setChecked(instrument->sendsBankSelect());
    m_variationCheckBox->setChecked(instrument->sendsBankSelect());
    m_channelValue->setCurrentIndex(
            m_selectedInstrument->hasFixedChannel() ? 1 : 0);

    // Unblock signals
    m_percussionCheckBox-> blockSignals(false);
    m_programCheckBox->    blockSignals(false);
    m_bankCheckBox->       blockSignals(false);
    m_variationCheckBox->  blockSignals(false);
    m_channelValue->       blockSignals(false);

    // Basic parameters
    //
    //
    // Check for program change
    //
    updateBankComboBox();
    updateProgramComboBox();
    updateVariationComboBox();
    
    // Setup the ControlParameters
    //
    setupControllers(md);

    m_mainGrid->setRowStretch(9, 20);

    // Set all the positions by controller number
    //
    for (RotaryInfoVector::iterator it = m_rotaries.begin() ;
            it != m_rotaries.end(); ++it) {
        MidiByte value = 0;

        try {
            value = instrument->getControllerValue(
                        MidiByte(it->controller));
        } catch (...) {
            continue;
        }
        setRotaryToValue(it->controller, int(value));
    }

}

void
MIDIInstrumentParameterPanel::setupControllers(MidiDevice *md)
{
    QFont f(font());

    if (!m_rotaryFrame) {
        m_rotaryFrame = new QFrame(this);
        m_mainGrid->addWidget(m_rotaryFrame, 10, 0, 1, 3, Qt::AlignHCenter);
        m_rotaryFrame->setContentsMargins(8, 8, 8, 8);
        m_rotaryGrid = new QGridLayout(m_rotaryFrame);
        m_rotaryGrid->setSpacing(1);
        m_rotaryGrid->setMargin(0);
        m_rotaryGrid->addItem(new QSpacerItem(10, 4), 0, 1);
        m_rotaryFrame->setLayout(m_rotaryGrid);
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
    RotaryInfoVector::iterator rmi = m_rotaries.begin();

    for (ControlList::iterator it = list.begin();
            it != list.end(); ++it) {
        if (it->getIPBPosition() == -1)
            continue;

        // Get the knob colour (even if it's default, because otherwise it turns
        // black instead of the default color from the map!  it was here the
        // whole time, this simple!)
        //
        QColor knobColour = it->getColourIndex();
        Colour c =
            comp.getGeneralColourMap().getColourByIndex
            (it->getColourIndex());
        knobColour = QColor(c.getRed(), c.getGreen(), c.getBlue());

        Rotary *rotary = 0;

        if (rmi != m_rotaries.end()) {

            // Update the controller number that is associated with the
            // existing rotary widget.

            rmi->controller = it->getControllerValue();

            // Update the properties of the existing rotary widget.

            rotary = rmi->rotary;
            int redraw = 0; // 1 -> position, 2 -> all

            if (rotary->getMinValue() != it->getMin()) {
                rotary->setMinimum(it->getMin());
                redraw = 1;
            }
            if (rotary->getMaxValue() != it->getMax()) {
                rotary->setMaximum(it->getMax());
                redraw = 1;
            }
            
            bool isCentered = it->getDefault() == 64;
            if (rotary->getCentered() != isCentered) {
                rotary->setCentered(isCentered);
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

            QLabel *label = rmi->label;
            label->setText(QObject::tr(it->getName().c_str()));

            ++rmi;

        } else {

            QWidget *hbox = new QWidget(m_rotaryFrame);
            QHBoxLayout *hboxLayout = new QHBoxLayout;
            hboxLayout->setSpacing(8);
            hboxLayout->setMargin(0);

            float smallStep = 1.0;

            float bigStep = 5.0;
            if (it->getMax() - it->getMin() < 10)
                bigStep = 1.0;
            else if (it->getMax() - it->getMin() < 20)
                bigStep = 2.0;

            rotary = new Rotary(hbox,
                                it->getMin(),
                                it->getMax(),
                                smallStep,
                                bigStep,
                                it->getDefault(),
                                20,
                                Rotary::NoTicks,
                                false,
                                it->getDefault() == 64); //!!! hacky

            hboxLayout->addWidget(rotary);
            hbox->setLayout(hboxLayout);

            rotary->setKnobColour(knobColour);

            // Add a label
            QLabel *label = new SqueezedLabel(QObject::tr(it->getName().c_str()), hbox);
            label->setFont(f);
            hboxLayout->addWidget(label);

            RG_DEBUG << "Adding new widget at " << (count / 2) << "," << (count % 2) << endl;

            // Add the compound widget
            //
            m_rotaryGrid->addWidget(hbox, count / 2, (count % 2) * 2, Qt::AlignLeft);
            hbox->show();

            // Add to list
            //
            RotaryInfo ri;
            ri.rotary = rotary;
            ri.label = label;
            ri.controller = it->getControllerValue();
            m_rotaries.push_back(ri);

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
        for (RotaryInfoVector::iterator rmj = rmi; rmj != m_rotaries.end(); ++rmj) {
            delete rmj->rotary;
            delete rmj->label;
        }
        m_rotaries = RotaryInfoVector(m_rotaries.begin(), rmi);
    }

//    m_rotaryFrame->show();
}

void
MIDIInstrumentParameterPanel::setRotaryToValue(int controller, int value)
{
    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::setRotaryToValue - "
             << "controller = " << controller
             << ", value = " << value << std::endl;
             */

    for (RotaryInfoVector::iterator it = m_rotaries.begin() ; it != m_rotaries.end(); ++it) {
        if (it->controller == controller) {
            it->rotary->setPosition(float(value));
            return ;
        }
    }
}

void
MIDIInstrumentParameterPanel::updateBankComboBox()
{
    if (m_selectedInstrument == 0) return;

    m_bankComboBox->clear();
    m_banks.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::updateBankComboBox:"
        << " No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    int currentBank = -1;
    BankList banks;

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::updateBankComboBox: "
             << "variation type is " << md->getVariationType() << endl;
             */

    if (md->getVariationType() == MidiDevice::NoVariations) {

        banks = md->getBanks(m_selectedInstrument->isPercussion());

        if (!banks.empty()) {
            if (m_bankLabel->isHidden()) {
                m_bankLabel->show();
                m_bankCheckBox->show();
                m_bankComboBox->show();
            }
        } else {
            m_bankLabel->hide();
            m_bankCheckBox->hide();
            m_bankComboBox->hide();
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
                m_bankComboBox->hide();
            }
        } else {
            if (m_bankLabel->isHidden()) {
                m_bankLabel->show();
                m_bankCheckBox->show();
                m_bankComboBox->show();
            }
        }

        if (useMSB) {
            for (unsigned int i = 0; i < bytes.size(); ++i) {
                BankList bl = md->getBanksByMSB
                              (m_selectedInstrument->isPercussion(), bytes[i]);
                RG_DEBUG << "MIDIInstrumentParameterPanel::updateBankComboBox: have " << bl.size() << " variations for msb " << bytes[i] << endl;

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
                RG_DEBUG << "MIDIInstrumentParameterPanel::updateBankComboBox: have " << bl.size() << " variations for lsb " << bytes[i] << endl;
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
        m_bankComboBox->addItem(QObject::tr(i->getName().c_str()));
    }

    // Keep bank value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_bankComboBox->setDisabled(false);
    } else {
        m_bankComboBox->setEnabled(m_selectedInstrument->sendsBankSelect());
    }    

    if (currentBank < 0 && !banks.empty()) {
        m_bankComboBox->setCurrentIndex(0);
        slotSelectBank(0);
    } else {
        m_bankComboBox->setCurrentIndex(currentBank);
    }
}

void
MIDIInstrumentParameterPanel::updateProgramComboBox()
{
    if (m_selectedInstrument == 0)
        return ;

    m_programComboBox->clear();
    m_programs.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::updateProgramComboBox: No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::updateProgramComboBox:"
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
            m_programComboBox->show();
            m_receiveExternalCheckBox->show();
            m_receiveExternalLabel->show();
        }
    } else {
        m_programLabel->hide();
        m_programCheckBox->hide();
        m_programComboBox->hide();
        m_receiveExternalCheckBox->hide();
        m_receiveExternalLabel->hide();
    }

    for (unsigned int i = 0; i < programs.size(); ++i) {
        std::string programName = programs[i].getName();
        if (programName != "") {
            m_programComboBox->addItem(QObject::tr("%1. %2")
                                       .arg(programs[i].getProgram() + 1)
                                       .arg(QObject::tr(programName.c_str())));
            if (m_selectedInstrument->getProgram() == programs[i]) {
                currentProgram = m_programs.size();
            }
            m_programs.push_back(programs[i]);
        }
    }

    // Keep program value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_programComboBox->setDisabled(false);
    } else {
        m_programComboBox->setEnabled(m_selectedInstrument->sendsProgramChange());
    }    

    if (currentProgram < 0 && !m_programs.empty()) {
        m_programComboBox->setCurrentIndex(0);
        slotSelectProgram(0);
    } else {
        m_programComboBox->setCurrentIndex(currentProgram);

        // Ensure that stored program change value is same as the one
        // we're now showing (BUG 937371)
        //
        if (!m_programs.empty()) {
            m_selectedInstrument->setProgramChange
            ((m_programs[m_programComboBox->currentIndex()]).getProgram());
        }
    }
}

void
MIDIInstrumentParameterPanel::updateVariationComboBox()
{
    if (m_selectedInstrument == 0)
        return ;

    m_variationComboBox->clear();
    m_variations.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::updateVariationComboBox: No MidiDevice for Instrument "
        << m_selectedInstrument->getId() << endl;
        return ;
    }

    /*
    RG_DEBUG << "MIDIInstrumentParameterPanel::updateVariationComboBox:"
             << " variation type is " << md->getVariationType() << endl;
    */

    if (md->getVariationType() == MidiDevice::NoVariations) {
        if (!m_variationLabel->isHidden()) {
            m_variationLabel->hide();
            m_variationCheckBox->hide();
            m_variationComboBox->hide();
        }
        return ;
    }

    bool useMSB = (md->getVariationType() == MidiDevice::VariationFromMSB);
    MidiByteList variations;

    if (useMSB) {
        MidiByte lsb = m_selectedInstrument->getLSB();
        variations = md->getDistinctMSBs(m_selectedInstrument->isPercussion(),
                                         lsb);
        RG_DEBUG << "MIDIInstrumentParameterPanel::updateVariationComboBox: have " << variations.size() << " variations for lsb " << lsb << endl;

    } else {
        MidiByte msb = m_selectedInstrument->getMSB();
        variations = md->getDistinctLSBs(m_selectedInstrument->isPercussion(),
                                         msb);
        RG_DEBUG << "MIDIInstrumentParameterPanel::updateVariationComboBox: have " << variations.size() << " variations for msb " << msb << endl;
    }

    m_variationComboBox->setCurrentIndex( -1);

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
                    m_variationComboBox->addItem(programName == defaultProgramName ?
                                 tr("(default)") :
                                 strtoqstr(programName));
            */
            m_variationComboBox->addItem(QObject::tr("%1. %2")
                                         .arg(variations[i] + 1)
                                         .arg(QObject::tr(programName.c_str())));
            if (m_selectedInstrument->getProgram() == program) {
                currentVariation = m_variations.size();
            }
            m_variations.push_back(variations[i]);
        }
    }

    if (currentVariation < 0 && !m_variations.empty()) {
        m_variationComboBox->setCurrentIndex(0);
        slotSelectVariation(0);
    } else {
        m_variationComboBox->setCurrentIndex(currentVariation);
    }

    if (m_variations.size() < 2) {
        if (!m_variationLabel->isHidden()) {
            m_variationLabel->hide();
            m_variationCheckBox->hide();
            m_variationComboBox->hide();
        }

    } else {
        //!!! seem to have problems here -- the grid layout doesn't
        //like us adding stuff in the middle so if we go from 1
        //visible row (say program) to 2 (program + variation) the
        //second one overlaps the control knobs

        if (m_variationLabel->isHidden()) {
            m_variationLabel->show();
            m_variationCheckBox->show();
            m_variationComboBox->show();
        }

        if (m_programComboBox->width() > m_variationComboBox->width()) {
            m_variationComboBox->setMinimumWidth(m_programComboBox->width());
        } else {
            m_programComboBox->setMinimumWidth(m_variationComboBox->width());
        }
    }

    // Keep variation value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_variationComboBox->setDisabled(false);
    } else {
        m_variationComboBox->setEnabled(m_selectedInstrument->sendsBankSelect());
    }    
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

    updateBankComboBox();
    updateProgramComboBox();
    updateVariationComboBox();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
                               m_selectedInstrument->
                                         getProgramName().c_str());
    emit updateAllBoxes();

    emit instrumentParametersChanged(m_selectedInstrument->getId());
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

    // Keep bank value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_bankComboBox->setDisabled(false);
    } else {
        m_bankComboBox->setDisabled(!value);
    }

    updateBankComboBox();
    updateProgramComboBox();
    updateVariationComboBox();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
                               m_selectedInstrument->
                                         getProgramName().c_str());
    emit updateAllBoxes();

    emit instrumentParametersChanged(m_selectedInstrument->getId());
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

    // Keep program value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_bankComboBox->setDisabled(false);
    } else {
        m_programComboBox->setDisabled(!value);
    }

    updateProgramComboBox();
    updateVariationComboBox();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
                               m_selectedInstrument->
                                         getProgramName().c_str());
    emit updateAllBoxes();

    emit instrumentParametersChanged(m_selectedInstrument->getId());
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

    // Keep variation value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_bankComboBox->setDisabled(false);
    } else {
        m_variationComboBox->setDisabled(!value);
    }

    updateVariationComboBox();

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
                               m_selectedInstrument->
                                         getProgramName().c_str());
    emit updateAllBoxes();

    emit instrumentParametersChanged(m_selectedInstrument->getId());
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

    updateProgramComboBox();

    if (change) {
        emit updateAllBoxes();
    }

    emit changeInstrumentLabel(m_selectedInstrument->getId(),
            m_selectedInstrument->getProgramName().c_str());

    emit instrumentParametersChanged(m_selectedInstrument->getId());
}

void
MIDIInstrumentParameterPanel::slotExternalProgramChange(int prog, int bankLSB, int bankMSB )
{
    // If we aren't set to "Receive External", bail.
    if (!m_receiveExternalCheckBox->isChecked())
        return;

    if (!m_selectedInstrument)
        return ;

    MidiDevice *md =
            dynamic_cast<MidiDevice *>(m_selectedInstrument->getDevice());

    if (!md) {
        RG_DEBUG << "WARNING: MIDIInstrumentParameterPanel::slotExternalProgramChange(): No MidiDevice for Instrument "
                 << m_selectedInstrument->getId() << endl;
        return ;
    }

    bool changedBank = false;

    // MSB Bank Select
    if (bankMSB >= 0) {  // &&  md->getVariationType() != MidiDevice::VariationFromMSB ) {
        // If the MSB is changing
        if (m_selectedInstrument->getMSB() != bankMSB) {
            m_selectedInstrument->setMSB(bankMSB);
            changedBank = true;
        }
    }

    // LSB Bank Select
    if (bankLSB >= 0) { // &&  md->getVariationType() != MidiDevice::VariationFromLSB) {
        // If the LSB is changing
        if (m_selectedInstrument->getLSB() != bankLSB) {
            m_selectedInstrument->setLSB(bankLSB);
            changedBank = true;
        }
    }

    bool change = false;

    // ??? Can prog be -1?

    // If the Program Change is changing
    if (m_selectedInstrument->getProgramChange() !=
                static_cast<MidiByte>(prog)) {
        m_selectedInstrument->setProgramChange(static_cast<MidiByte>(prog));
        change = true;
    }

    //updateVariationComboBox();

    // If anything changed, update the UI.
    if (change  ||  changedBank) {
        //emit changeInstrumentLabel(m_selectedInstrument->getId(),
        //                           m_selectedInstrument->
        //                                     getProgramName().c_str());

        emit updateAllBoxes();
        
        emit instrumentParametersChanged(m_selectedInstrument->getId());
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

    updateVariationComboBox();

    if (change) {
        emit changeInstrumentLabel(m_selectedInstrument->getId(),
                                   m_selectedInstrument->
                                             getProgramName().c_str());
        emit updateAllBoxes();
    }

    emit instrumentParametersChanged(m_selectedInstrument->getId());
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

    if (md->getVariationType() == MidiDevice::VariationFromLSB) {
        if (m_selectedInstrument->getLSB() != v) {
            m_selectedInstrument->setLSB(v);
        }
    } else if (md->getVariationType() == MidiDevice::VariationFromMSB) {
        if (m_selectedInstrument->getMSB() != v) {
            m_selectedInstrument->setMSB(v);
        }
    }

    emit instrumentParametersChanged(m_selectedInstrument->getId());
}

// In place of the old sendBankAndProgram, instruments themselves now
// signal the affected channel managers when changed.

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

    m_selectedInstrument->setControllerValue(MidiByte(controllerNumber),
            MidiByte(value));

    emit updateAllBoxes();
    emit instrumentParametersChanged(m_selectedInstrument->getId());

}

int
MIDIInstrumentParameterPanel::getValueFromRotary(int controller)
{
    for (RotaryInfoVector::iterator it = m_rotaries.begin(); it != m_rotaries.end(); ++it) {
        if (it->controller == controller)
            return int(it->rotary->getPosition());
    }

    return -1;
}

void
MIDIInstrumentParameterPanel::
slotSelectChannel(int index)
{
    if (m_selectedInstrument == 0)
        { return; }
    if (index == 1) {
        m_selectedInstrument->setFixedChannel();
    } else {
        m_selectedInstrument->releaseFixedChannel();
    }
}

}
#include "MIDIInstrumentParameterPanel.moc"
