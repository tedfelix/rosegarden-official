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

// Disable RG_DEBUG output.  Must be defined prior to including Debug.h.
// Downside to this is that we also lose all the warnings.
// We need an RG_WARNING.  Instead, I've gone with std::cerr.
#define RG_NO_DEBUG_PRINT

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
#include <iostream>

namespace Rosegarden
{

MIDIInstrumentParameterPanel::MIDIInstrumentParameterPanel(
        RosegardenDocument *doc, QWidget *parent) :
    InstrumentParameterPanel(doc, parent),
    m_rotaryFrame(0),
    m_rotaryMapper(new QSignalMapper(this))
{
    RG_DEBUG << "MIDIInstrumentParameterPanel ctor";

    setObjectName("MIDI Instrument Parameter Panel");

    // Grid
    setContentsMargins(2, 2, 2, 2);
    m_mainGrid = new QGridLayout(this);
    m_mainGrid->setMargin(0);
    m_mainGrid->setSpacing(1);
    m_mainGrid->setColumnStretch(2, 1);
    setLayout(m_mainGrid);

    // Font
    QFont f;
    f.setPointSize(f.pointSize() * 90 / 100);
    f.setBold(false);
    QFontMetrics metrics(f);

    // Instrument Label
    m_instrumentLabel->setFont(f);
    const int width25 = metrics.width("1234567890123456789012345");
    m_instrumentLabel->setFixedWidth(width25);
    m_instrumentLabel->setAlignment(Qt::AlignCenter);
    m_mainGrid->addWidget(m_instrumentLabel, 0, 0, 1, 4, Qt::AlignCenter);

    // Connection Label
    m_connectionLabel = new SqueezedLabel(this);
    m_connectionLabel->setFont(f);
    // we still have to use the QFontMetrics here, or a SqueezedLabel will
    // squeeze itself down to 0.
    const int width30 = metrics.width("123456789012345678901234567890");
    m_connectionLabel->setFixedWidth(width30);
    m_connectionLabel->setAlignment(Qt::AlignCenter);
    m_mainGrid->addWidget(m_connectionLabel, 1, 0, 1, 4, Qt::AlignCenter);

    // Percussion Label
    QLabel *percussionLabel = new QLabel(tr("Percussion"), this);
    percussionLabel->setFont(f);
    m_mainGrid->addWidget(percussionLabel, 3, 0, 1, 2, Qt::AlignLeft);

    // Percussion CheckBox
    m_percussionCheckBox = new QCheckBox(this);
    m_percussionCheckBox->setFont(f);
    m_percussionCheckBox->setToolTip(tr("<qt><p>Check this to tell Rosegarden that this is a percussion instrument.  This allows you access to any percussion key maps and drum kits you may have configured in the studio</p></qt>"));
    connect(m_percussionCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotPercussionClicked(bool)));
    m_mainGrid->addWidget(m_percussionCheckBox, 3, 3, Qt::AlignLeft);

    // Bank Label
    m_bankLabel = new QLabel(tr("Bank"), this);
    m_bankLabel->setFont(f);
    m_mainGrid->addWidget(m_bankLabel, 4, 0, Qt::AlignLeft);

    // Bank CheckBox
    m_bankCheckBox = new QCheckBox(this);
    m_bankCheckBox->setFont(f);
    m_bankCheckBox->setToolTip(tr("<qt>Send bank select</qt>"));
    connect(m_bankCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotBankClicked(bool)));
    m_mainGrid->addWidget(m_bankCheckBox, 4, 1, Qt::AlignRight);

    // Ensure a reasonable amount of space in the dropdowns even
    // if no instrument initially selected.
    // setMinimumWidth() using QFontMetrics wasn't cutting it at all, so let's
    // try what I used in the plugin manager dialog, with
    // setMinimumContentsLength() instead:
    const int comboWidth = 25;

    // Bank ComboBox
    m_bankComboBox = new QComboBox(this);
    m_bankComboBox->setFont(f);
    m_bankComboBox->setToolTip(tr("<qt>Set the MIDI bank from which to select programs</qt>"));
    m_bankComboBox->setMaxVisibleItems(20);
    m_bankComboBox->setMinimumContentsLength(comboWidth);
    // Activated by m_bankCheckBox
    //m_bankComboBox->setDisabled(true);
    connect(m_bankComboBox, SIGNAL(activated(int)),
            this, SLOT(slotSelectBank(int)));
    //m_bankComboBox->setCurrentIndex(-1);
    m_mainGrid->addWidget(m_bankComboBox, 4, 2, 1, 2, Qt::AlignRight);

    // Program Label
    m_programLabel = new QLabel(tr("Program"), this);
    m_programLabel->setFont(f);
    m_mainGrid->addWidget(m_programLabel, 5, 0, Qt::AlignLeft);

    // Program CheckBox
    m_programCheckBox = new QCheckBox(this);
    m_programCheckBox->setFont(f);
    m_programCheckBox->setToolTip(tr("<qt>Send program change</qt>"));
    connect(m_programCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotProgramClicked(bool)));
    m_mainGrid->addWidget(m_programCheckBox, 5, 1, Qt::AlignRight);

    // Program ComboBox
    m_programComboBox = new QComboBox(this);
    m_programComboBox->setFont(f);
    m_programComboBox->setToolTip(tr("<qt>Set the MIDI program or &quot;patch&quot;</p></qt>"));
    m_programComboBox->setMaxVisibleItems(20);
    m_programComboBox->setMinimumContentsLength(comboWidth);
    // Activated by m_programCheckBox
    //m_programComboBox->setDisabled(true);
    connect(m_programComboBox, SIGNAL(activated(int)),
            this, SLOT(slotSelectProgram(int)));
    //m_programComboBox->setCurrentIndex(-1);
    m_mainGrid->addWidget(m_programComboBox, 5, 2, 1, 2, Qt::AlignRight);

    // Variation Label
    m_variationLabel = new QLabel(tr("Variation"), this);
    m_variationLabel->setFont(f);
    m_mainGrid->addWidget(m_variationLabel, 6, 0);

    // Variation CheckBox
    m_variationCheckBox = new QCheckBox(this);
    m_variationCheckBox->setFont(f);
    m_variationCheckBox->setToolTip(tr("<qt>Send bank select for variation</qt>"));
    connect(m_variationCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotVariationClicked(bool)));
    m_mainGrid->addWidget(m_variationCheckBox, 6, 1);

    // Variation ComboBox
    m_variationComboBox = new QComboBox(this);
    m_variationComboBox->setFont(f);
    m_variationComboBox->setToolTip(tr("<qt>Set variations on the program above, if available in the studio</qt>"));
    m_variationComboBox->setMaxVisibleItems(20);
    m_variationComboBox->setMinimumContentsLength(comboWidth);
    // Activated by m_variationCheckBox
    //m_variationComboBox->setDisabled(true);
    connect(m_variationComboBox, SIGNAL(activated(int)),
            this, SLOT(slotSelectVariation(int)));
    //m_variationComboBox->setCurrentIndex(-1);
    m_mainGrid->addWidget(m_variationComboBox, 6, 2, 1, 2, Qt::AlignRight);

    // Channel Label
    QLabel *channelLabel = new QLabel(tr("Channel"), this);
    channelLabel->setFont(f);
    QString channelTip(tr("<qt><p><i>Auto</i>, allocate channel automatically; <i>Fixed</i>, fix channel to instrument number</p></qt>"));
    channelLabel->setToolTip(channelTip);
    m_mainGrid->addWidget(channelLabel, 7, 0, Qt::AlignLeft);

    // Channel ComboBox
    m_channelValue = new QComboBox(this);
    m_channelValue->setFont(f);
    m_channelValue->setToolTip(channelTip);
    m_channelValue->setMaxVisibleItems(2);
    // Everything else sets up elsewhere, but these don't vary per instrument:
    m_channelValue->addItem(tr("Auto"));
    m_channelValue->addItem(tr("Fixed"));
    m_channelValue->setMinimumContentsLength(comboWidth);
    connect(m_channelValue, SIGNAL(activated(int)),
            this, SLOT(slotSelectChannel(int)));
    //m_channelValue->setCurrentIndex(-1);
    m_mainGrid->addWidget(m_channelValue, 7, 2, 1, 2, Qt::AlignRight);

    // Receive External Label
    m_receiveExternalLabel = new QLabel(tr("Receive external"), this);
    m_receiveExternalLabel->setFont(f);
    QString receiveExternalTip = tr("<qt>Use program changes from an external source to manipulate these controls (only valid for the currently-active track) [Shift + P]</qt>");
    m_receiveExternalLabel->setToolTip(receiveExternalTip);
    m_mainGrid->addWidget(m_receiveExternalLabel, 8, 0, 1, 3, Qt::AlignLeft);
    
    // Receive External CheckBox
    m_receiveExternalCheckBox = new QCheckBox(this);
    m_receiveExternalCheckBox->setFont(f);
    m_receiveExternalCheckBox->setToolTip(receiveExternalTip);
    m_receiveExternalCheckBox->setShortcut((QKeySequence)"Shift+P");
    m_receiveExternalCheckBox->setChecked(false);
    m_mainGrid->addWidget(m_receiveExternalCheckBox, 8, 3, Qt::AlignLeft);

    // Rotary Mapper
    connect(m_rotaryMapper, SIGNAL(mapped(int)),
            this, SLOT(slotControllerChanged(int)));
}

void
MIDIInstrumentParameterPanel::clearReceiveExternal()
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::clearReceiveExternal()";

    m_receiveExternalCheckBox->setChecked(false);
}

void
MIDIInstrumentParameterPanel::setupForInstrument(Instrument *instrument)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::setupForInstrument() begin";

    // In some cases setupForInstrument gets called several times.
    // This shortcuts this activity since only one setup is needed.
    // ??? Problem is that this prevents legitimate changes to the
    //     instrument from getting to the UI.  Removing this fixes an
    //     update bug when importing a device file.  I think it also
    //     fixes an update bug related to "Receive external".
    // ??? However, this might be preventing endless recursion as this routine
    //     might indirectly fire off calls to itself.  Initial digging seems
    //     to indicate that this routine will not fire off signals.  It tries
    //     hard not to, anyway.
#if 1
    if (m_selectedInstrument == instrument) {
        RG_DEBUG << "setupForInstrument(): Early exit.  Instrument didn't change.";
        RG_DEBUG << "MIDIInstrumentParameterPanel::setupForInstrument() end";
        return;
    }
#endif

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (instrument->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::setupForInstrument(): No MidiDevice for Instrument " << instrument->getId() << '\n';
        RG_DEBUG << "MIDIInstrumentParameterPanel::setupForInstrument() end";
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

    // Update CheckBoxes
    
    m_percussionCheckBox->setChecked(instrument->isPercussion());
    m_programCheckBox->setChecked(instrument->sendsProgramChange());
    m_bankCheckBox->setChecked(instrument->sendsBankSelect());
    m_variationCheckBox->setChecked(instrument->sendsBankSelect());

    // Update ComboBoxes

    m_channelValue->setCurrentIndex(
            m_selectedInstrument->hasFixedChannel() ? 1 : 0);

    // ??? Do these fire off the combobox handlers?  Maybe.  Although each of
    //     these culminates in a setCurrentIndex() which we've verified does
    //     not fire an activated(int), some of these do directly call the
    //     handlers which in turn will fire multiple signals.  This needs
    //     to be investigated.
    updateBankComboBox();
    updateProgramComboBox();
    updateVariationComboBox();

    // Update Rotary Widgets

    // Setup the Rotaries
    // ??? Does this fire off the rotary handlers?  Probably not.
    setupControllers(md);

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
        // ??? Does this fire off the rotary handlers?  Testing indicates no.
        setRotaryToValue(it->controller, int(value));
    }

    RG_DEBUG << "MIDIInstrumentParameterPanel::setupForInstrument() end";
}

void
MIDIInstrumentParameterPanel::setupControllers(MidiDevice *md)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::setupControllers()";

    if (!m_rotaryFrame) {
        m_rotaryFrame = new QFrame(this);
        m_mainGrid->addWidget(m_rotaryFrame, 10, 0, 1, 3, Qt::AlignHCenter);
        // Put some space between the rotaries and the widgets above.
        m_mainGrid->setRowStretch(9, 20);
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
        const Colour c = comp.getGeneralColourMap().getColourByIndex(
                it->getColourIndex());
        const QColor knobColour = QColor(c.getRed(), c.getGreen(), c.getBlue());

        Rotary *rotary = 0;

        // If the Rotary widgets have already been created, update them.
        if (rmi != m_rotaries.end()) {

            // Update the controller number that is associated with the
            // existing rotary widget.

            rmi->controller = it->getControllerValue();

            // Update the properties of the existing rotary widget.

            rotary = rmi->rotary;

            rotary->setMinimum(it->getMin());
            rotary->setMaximum(it->getMax());
            rotary->setCentered((it->getDefault() == 64));
            rotary->setPosition(it->getDefault());
            rotary->setKnobColour(knobColour);

            // Update the controller name.
            rmi->label->setText(QObject::tr(it->getName().c_str()));

            // Next Rotary widget
            ++rmi;

        } else {  // Need to create the Rotary widget.

            // Create a horizontal box for the Rotary/Label pair.
            QWidget *hbox = new QWidget(m_rotaryFrame);
            QHBoxLayout *hboxLayout = new QHBoxLayout;
            hboxLayout->setSpacing(8);
            hboxLayout->setMargin(0);
            hbox->setLayout(hboxLayout);

            // Add a Rotary

            float pageStep = 5.0;
            if (it->getMax() - it->getMin() < 10)
                pageStep = 1.0;
            else if (it->getMax() - it->getMin() < 20)
                pageStep = 2.0;

            rotary = new Rotary(hbox,          // parent
                                it->getMin(),  // minimum
                                it->getMax(),  // maximum
                                1.0,           // step
                                pageStep,      // pageStep
                                it->getDefault(),  // initialPosition
                                20,                // size
                                Rotary::NoTicks,   // ticks
                                false,             // snapToTicks
                                (it->getDefault() == 64));  // centred
            rotary->setKnobColour(knobColour);
            hboxLayout->addWidget(rotary);

            // Add a label

            SqueezedLabel *label = new SqueezedLabel(QObject::tr(it->getName().c_str()), hbox);
            label->setFont(font());
            hboxLayout->addWidget(label);

            RG_DEBUG << "setupControllers(): Adding new widget at " << (count / 2) << "," << (count % 2);

            // Add the compound (Rotary and Label) widget to the grid.
            m_rotaryGrid->addWidget(hbox, count / 2, (count % 2) * 2, Qt::AlignLeft);
            hbox->show();

            // Add to the Rotary info list
            RotaryInfo ri;
            ri.rotary = rotary;
            ri.label = label;
            ri.controller = it->getControllerValue();
            m_rotaries.push_back(ri);

            // Connect for changes to the Rotary by the user.
            connect(rotary, SIGNAL(valueChanged(float)),
                    m_rotaryMapper, SLOT(map()));

            rmi = m_rotaries.end();
        }

        // Add signal mapping
        //
        m_rotaryMapper->setMapping(rotary,
                                   int(it->getControllerValue()));

        ++count;
    }

    // If there are more rotary widgets than this instrument needs,
    // delete them.
    if (rmi != m_rotaries.end()) {
        for (RotaryInfoVector::iterator rmj = rmi; rmj != m_rotaries.end(); ++rmj) {
            delete rmj->rotary;
            delete rmj->label;
        }
        m_rotaries = RotaryInfoVector(m_rotaries.begin(), rmi);
    }
}

void
MIDIInstrumentParameterPanel::setRotaryToValue(int controller, int value)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::setRotaryToValue(controller => " << controller << ", value => " << value << ")";

    for (RotaryInfoVector::iterator it = m_rotaries.begin(); it != m_rotaries.end(); ++it) {
        if (it->controller == controller) {
            it->rotary->setPosition(float(value));
            return ;
        }
    }
}

int
MIDIInstrumentParameterPanel::getValueFromRotary(int controller)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::getValueFromRotary(" << controller << ")";

    for (RotaryInfoVector::iterator it = m_rotaries.begin(); it != m_rotaries.end(); ++it) {
        if (it->controller == controller)
            return int(it->rotary->getPosition());
    }

    return -1;
}

void
MIDIInstrumentParameterPanel::updateBankComboBox()
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::updateBankComboBox()";

    if (m_selectedInstrument == 0) return;

    m_bankComboBox->clear();
    m_banks.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::updateBankComboBox(): No MidiDevice for Instrument " << m_selectedInstrument->getId() << '\n';
        return ;
    }

    int currentBank = -1;
    BankList banks;

    RG_DEBUG << "updateBankComboBox(): Variation type is " << md->getVariationType();

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
                RG_DEBUG << "updateBankComboBox(): Have " << bl.size() << " variations for MSB " << bytes[i];

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

                RG_DEBUG << "updateBankComboBox(): Have " << bl.size() << " variations for LSB " << bytes[i];

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
    RG_DEBUG << "MIDIInstrumentParameterPanel::updateProgramComboBox()";

    if (m_selectedInstrument == 0)
        return ;

    m_programComboBox->clear();
    m_programs.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::updateProgramComboBox(): No MidiDevice for Instrument " << m_selectedInstrument->getId() << '\n';
        return ;
    }

    RG_DEBUG << "updateProgramComboBox(): variation type is " << md->getVariationType();

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
    RG_DEBUG << "MIDIInstrumentParameterPanel::updateVariationComboBox()";

    if (m_selectedInstrument == 0)
        return ;

    m_variationComboBox->clear();
    m_variations.clear();

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::updateVariationComboBox(): No MidiDevice for Instrument " << m_selectedInstrument->getId() << '\n';
        return ;
    }

    RG_DEBUG << "updateVariationComboBox(): Variation type is " << md->getVariationType();

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
        RG_DEBUG << "updateVariationComboBox(): Have " << variations.size() << " variations for LSB " << lsb;

    } else {
        MidiByte msb = m_selectedInstrument->getMSB();
        variations = md->getDistinctLSBs(m_selectedInstrument->isPercussion(),
                                         msb);

        RG_DEBUG << "updateVariationComboBox(): Have " << variations.size() << " variations for MSB " << msb;
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
MIDIInstrumentParameterPanel::slotPercussionClicked(bool checked)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotPercussionClicked(" << checked << ")";

    if (m_selectedInstrument == 0) {
        m_percussionCheckBox->setChecked(false);
        emit updateAllBoxes();
        return ;
    }

    m_selectedInstrument->setPercussion(checked);

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
MIDIInstrumentParameterPanel::slotBankClicked(bool checked)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotBankClicked()";

    if (m_selectedInstrument == 0) {
        m_bankCheckBox->setChecked(false);
        emit updateAllBoxes();
        return ;
    }

    m_variationCheckBox->setChecked(checked);
    m_selectedInstrument->setSendBankSelect(checked);

    // Keep bank value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_bankComboBox->setDisabled(false);
    } else {
        m_bankComboBox->setDisabled(!checked);
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
MIDIInstrumentParameterPanel::slotProgramClicked(bool checked)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotProgramClicked()";

    if (m_selectedInstrument == 0) {
        m_programCheckBox->setChecked(false);
        emit updateAllBoxes();
        return ;
    }

    m_selectedInstrument->setSendProgramChange(checked);

    // Keep program value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_bankComboBox->setDisabled(false);
    } else {
        m_programComboBox->setDisabled(!checked);
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
MIDIInstrumentParameterPanel::slotVariationClicked(bool checked)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotVariationClicked()";

    if (m_selectedInstrument == 0) {
        m_variationCheckBox->setChecked(false);
        emit updateAllBoxes();
        return ;
    }

    m_bankCheckBox->setChecked(checked);
    m_selectedInstrument->setSendBankSelect(checked);

    // Keep variation value enabled if percussion map is in use
    if  (m_percussionCheckBox->isChecked()) {
        m_bankComboBox->setDisabled(false);
    } else {
        m_variationComboBox->setDisabled(!checked);
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
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotSelectBank()";

    if (m_selectedInstrument == 0)
        return ;

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::slotSelectBank(): No MidiDevice for Instrument " << m_selectedInstrument->getId() << '\n';
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
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotExternalProgramChange()";

    // If we aren't set to "Receive External", bail.
    if (!m_receiveExternalCheckBox->isChecked())
        return;

    if (!m_selectedInstrument)
        return ;

    MidiDevice *md =
            dynamic_cast<MidiDevice *>(m_selectedInstrument->getDevice());

    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::slotExternalProgramChange(): No MidiDevice for Instrument " << m_selectedInstrument->getId() << '\n';
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
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotSelectProgram()";

    const MidiProgram *prg = &m_programs[index];
    if (prg == 0) {
        std::cerr << "MIDIInstrumentParameterPanel::slotSelectProgram(): Program change not found in bank.\n";
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
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotSelectVariation()";

    MidiDevice *md = dynamic_cast<MidiDevice*>
                     (m_selectedInstrument->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::slotSelectVariation(): No MidiDevice for Instrument " << m_selectedInstrument->getId() << '\n';
        return ;
    }

    if (index < 0 || index > int(m_variations.size())) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::slotSelectVariation(): index " << index << " out of range\n";
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
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotControllerChanged(" << controllerNumber << ")";

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
        std::cerr << "MIDIInstrumentParameterPanel::slotControllerChanged(): Couldn't get value of rotary for controller " << controllerNumber << '\n';
        return ;
    }

    m_selectedInstrument->setControllerValue(MidiByte(controllerNumber),
            MidiByte(value));

    emit updateAllBoxes();
    emit instrumentParametersChanged(m_selectedInstrument->getId());

}

void
MIDIInstrumentParameterPanel::
slotSelectChannel(int index)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel::slotSelectChannel(" << index << ")";

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
