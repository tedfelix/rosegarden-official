/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
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
// Warnings are currently done with std::cerr to make sure they appear
// even in a release build.
#define RG_NO_DEBUG_PRINT

#include "MIDIInstrumentParameterPanel.h"

#include "InstrumentParameterPanel.h"
#include "document/RosegardenDocument.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/widgets/SqueezedLabel.h"
#include "gui/widgets/Rotary.h"
#include "sequencer/RosegardenSequencer.h"
#include "misc/Debug.h"
#include "base/Colour.h"
#include "base/Composition.h"
#include "base/ControlParameter.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
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

#include <algorithm>  // std::sort(), std::remove_if()
#include <string>
#include <iostream>
#include <cmath>

namespace Rosegarden
{

MIDIInstrumentParameterPanel::MIDIInstrumentParameterPanel(QWidget *parent) :
    InstrumentParameterPanel(parent),
    m_rotaryFrame(NULL),
    m_rotaryGrid(NULL)
{
    RG_DEBUG << "MIDIInstrumentParameterPanel ctor";

    setObjectName("MIDI Instrument Parameter Panel");

    // Font
    QFont f;
    f.setPointSize(f.pointSize() * 90 / 100);
    f.setBold(false);
    QFontMetrics metrics(f);
    // Compute a width for the labels that will prevent them from becoming
    // so large that they make a mess out of the layout.
    const int labelWidth = metrics.width("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

    // Widgets

    // Instrument Label
    m_instrumentLabel->setFont(f);
    // Set a fixed width to prevent the label from growing too large.
    m_instrumentLabel->setFixedWidth(labelWidth);
    m_instrumentLabel->setAlignment(Qt::AlignCenter);

    // Connection Label
    m_connectionLabel = new SqueezedLabel;
    m_connectionLabel->setFont(f);
    // Set a fixed width to prevent the label from growing too large.
    m_connectionLabel->setFixedWidth(labelWidth);
    m_connectionLabel->setAlignment(Qt::AlignCenter);

    // Percussion Label
    QLabel *percussionLabel = new QLabel(tr("Percussion"), this);
    percussionLabel->setFont(f);

    // Percussion CheckBox
    m_percussionCheckBox = new QCheckBox;
    m_percussionCheckBox->setFont(f);
    m_percussionCheckBox->setToolTip(tr("<qt><p>Check this to tell Rosegarden that this is a percussion instrument.  This allows you access to any percussion key maps and drum kits you may have configured in the studio</p></qt>"));
    connect(m_percussionCheckBox, SIGNAL(clicked(bool)),
            SLOT(slotPercussionClicked(bool)));

    // Bank Label
    m_bankLabel = new QLabel;
    m_bankLabel->setText(tr("Bank"));
    m_bankLabel->setFont(f);

    // Bank CheckBox
    m_bankCheckBox = new QCheckBox;
    m_bankCheckBox->setFont(f);
    m_bankCheckBox->setToolTip(tr("<qt>Send bank select</qt>"));
    connect(m_bankCheckBox, SIGNAL(clicked(bool)),
            SLOT(slotBankClicked(bool)));

    // Since these ComboBoxes may have a very large number of items,
    // expand the maximum dropdown size (normally 10) to show more of
    // them at a time.
    const int maxVisibleItems = 20;
    // Ensure the comboboxes are all at least this wide (in characters).
    const int minimumContentsLength = 25;

    // Bank ComboBox
    m_bankComboBox = new QComboBox;
    m_bankComboBox->setFont(f);
    m_bankComboBox->setToolTip(tr("<qt>Set the MIDI bank from which to select programs</qt>"));
    m_bankComboBox->setMaxVisibleItems(maxVisibleItems);
    m_bankComboBox->setMinimumContentsLength(minimumContentsLength);
    connect(m_bankComboBox, SIGNAL(activated(int)),
            SLOT(slotSelectBank(int)));

    // Program Label
    m_programLabel = new QLabel;
    m_programLabel->setText(tr("Program"));
    m_programLabel->setFont(f);

    // Program CheckBox
    m_programCheckBox = new QCheckBox;
    m_programCheckBox->setFont(f);
    m_programCheckBox->setToolTip(tr("<qt>Send program change</qt>"));
    connect(m_programCheckBox, SIGNAL(clicked(bool)),
            SLOT(slotProgramClicked(bool)));

    // Program ComboBox
    m_programComboBox = new QComboBox;
    m_programComboBox->setFont(f);
    m_programComboBox->setToolTip(tr("<qt>Set the MIDI program or &quot;patch&quot;</p></qt>"));
    m_programComboBox->setMaxVisibleItems(maxVisibleItems);
    m_programComboBox->setMinimumContentsLength(minimumContentsLength);
    connect(m_programComboBox, SIGNAL(activated(int)),
            SLOT(slotSelectProgram(int)));

    // Variation Label
    m_variationLabel = new QLabel;
    m_variationLabel->setText(tr("Variation"));
    m_variationLabel->setFont(f);

    // Variation CheckBox
    m_variationCheckBox = new QCheckBox;
    m_variationCheckBox->setFont(f);
    m_variationCheckBox->setToolTip(tr("<qt>Send bank select for variation</qt>"));
    connect(m_variationCheckBox, SIGNAL(clicked(bool)),
            SLOT(slotVariationClicked(bool)));

    // Variation ComboBox
    m_variationComboBox = new QComboBox;
    m_variationComboBox->setFont(f);
    m_variationComboBox->setToolTip(tr("<qt>Set variations on the program above, if available in the studio</qt>"));
    m_variationComboBox->setMaxVisibleItems(maxVisibleItems);
    m_variationComboBox->setMinimumContentsLength(minimumContentsLength);
    connect(m_variationComboBox, SIGNAL(activated(int)),
            SLOT(slotSelectVariation(int)));

    // Channel Label
    QLabel *channelLabel = new QLabel(tr("Channel"), this);
    channelLabel->setFont(f);
    QString channelTip(tr("<qt><p><i>Auto</i>, allocate channel automatically; <i>Fixed</i>, fix channel to instrument number</p></qt>"));
    channelLabel->setToolTip(channelTip);

    // Channel ComboBox
    m_channelValue = new QComboBox;
    m_channelValue->setFont(f);
    m_channelValue->setToolTip(channelTip);
    m_channelValue->setMaxVisibleItems(2);
    // Everything else sets up elsewhere, but these don't vary per instrument:
    m_channelValue->addItem(tr("Auto"));
    m_channelValue->addItem(tr("Fixed"));
    m_channelValue->setMinimumContentsLength(minimumContentsLength);
    connect(m_channelValue, SIGNAL(activated(int)),
            SLOT(slotSelectChannel(int)));

    // Receive External Label
    QLabel *receiveExternalLabel = new QLabel(tr("Receive external"), this);
    receiveExternalLabel->setFont(f);
    QString receiveExternalTip = tr("<qt>Use program changes from an external source to manipulate these controls (only valid for the currently-active track) [Shift + P]</qt>");
    receiveExternalLabel->setToolTip(receiveExternalTip);
    
    // Receive External CheckBox
    m_receiveExternalCheckBox = new QCheckBox;
    m_receiveExternalCheckBox->setFont(f);
    m_receiveExternalCheckBox->setToolTip(receiveExternalTip);
    m_receiveExternalCheckBox->setShortcut((QKeySequence)"Shift+P");
    m_receiveExternalCheckBox->setChecked(false);

    // Rotary Frame and Grid
    m_rotaryFrame = new QFrame(this);
    m_rotaryFrame->setContentsMargins(8, 8, 8, 8);
    m_rotaryGrid = new QGridLayout(m_rotaryFrame);
    m_rotaryGrid->setSpacing(1);
    m_rotaryGrid->setMargin(0);
    m_rotaryGrid->addItem(new QSpacerItem(10, 4), 0, 1);
    m_rotaryFrame->setLayout(m_rotaryGrid);

    // Rotary Mapper
    m_rotaryMapper = new QSignalMapper(this);
    connect(m_rotaryMapper, SIGNAL(mapped(int)),
            SLOT(slotControllerChanged(int)));

    // Layout

    QGridLayout *mainGrid = new QGridLayout(this);
    mainGrid->setMargin(0);
    mainGrid->setSpacing(3);
    mainGrid->setColumnStretch(2, 1);

    mainGrid->addWidget(m_instrumentLabel, 0, 0, 1, 4, Qt::AlignCenter);

    mainGrid->addWidget(m_connectionLabel, 1, 0, 1, 4, Qt::AlignCenter);

    mainGrid->addItem(new QSpacerItem(1, 5), 2, 0, 1, 4);

    mainGrid->addWidget(percussionLabel, 3, 0, 1, 2, Qt::AlignLeft);
    mainGrid->addWidget(m_percussionCheckBox, 3, 3, Qt::AlignLeft);

    mainGrid->addWidget(m_bankLabel, 4, 0, Qt::AlignLeft);
    mainGrid->addWidget(m_bankCheckBox, 4, 1, Qt::AlignRight);
    mainGrid->addWidget(m_bankComboBox, 4, 2, 1, 2, Qt::AlignRight);

    mainGrid->addWidget(m_programLabel, 5, 0, Qt::AlignLeft);
    mainGrid->addWidget(m_programCheckBox, 5, 1, Qt::AlignRight);
    mainGrid->addWidget(m_programComboBox, 5, 2, 1, 2, Qt::AlignRight);

    mainGrid->addWidget(m_variationLabel, 6, 0);
    mainGrid->addWidget(m_variationCheckBox, 6, 1);
    mainGrid->addWidget(m_variationComboBox, 6, 2, 1, 2, Qt::AlignRight);

    mainGrid->addWidget(channelLabel, 7, 0, Qt::AlignLeft);
    mainGrid->addWidget(m_channelValue, 7, 2, 1, 2, Qt::AlignRight);

    mainGrid->addWidget(m_receiveExternalCheckBox, 8, 3, Qt::AlignLeft);
    mainGrid->addWidget(receiveExternalLabel, 8, 0, 1, 3, Qt::AlignLeft);

    mainGrid->addItem(new QSpacerItem(1, 5), 9, 0, 1, 4);

    // Add the rotary frame to the main grid layout.
    mainGrid->addWidget(m_rotaryFrame, 10, 0, 1, 4, Qt::AlignHCenter);

    mainGrid->addItem(new QSpacerItem(1, 1), 11, 0, 1, 4);
    // Let the last row take up the rest of the space.  This keeps
    // the widgets above compact vertically.
    mainGrid->setRowStretch(11, 1);

    setLayout(mainGrid);

    setContentsMargins(2, 7, 2, 2);

    // Connections

    connect(RosegardenMainWindow::self(),
                SIGNAL(documentChanged(RosegardenDocument *)),
            SLOT(slotNewDocument(RosegardenDocument *)));

    connect(Instrument::getStaticSignals().data(),
            SIGNAL(changed(Instrument *)),
            SLOT(slotInstrumentChanged(Instrument *)));

    connect(Instrument::getStaticSignals().data(),
                SIGNAL(controlChange(Instrument *, int)),
            SLOT(slotControlChange(Instrument *, int)));

    connect(RosegardenMainWindow::self()->getSequenceManager(),
                SIGNAL(signalSelectProgramNoSend(int,int,int)),
            SLOT(slotExternalProgramChange(int,int,int)));
}

void
MIDIInstrumentParameterPanel::updateWidgets()
{
    RG_DEBUG << "updateWidgets() begin";

    if (!getSelectedInstrument())
        return;

    MidiDevice *md = dynamic_cast<MidiDevice *>(getSelectedInstrument()->getDevice());
    if (!md) {
        RG_WARNING << "updateWidgets(): WARNING: No MidiDevice for Instrument " << getSelectedInstrument()->getId();
        RG_DEBUG << "setupForInstrument() end";
        return;
    }

    // Instrument name

    m_instrumentLabel->setText(
            getSelectedInstrument()->getLocalizedPresentationName());

    // Studio Device (connection) name

    QString connection(RosegardenSequencer::getInstance()->getConnection(md->getId()));

    if (connection == "") {
        connection = tr("No connection");
    } else {
        // remove trailing "(duplex)", "(read only)", "(write only)" etc
        connection.replace(QRegExp("\\s*\\([^)0-9]+\\)\\s*$"), "");
    }

    m_connectionLabel->setText("[ " + connection + " ]");

    // Percussion
    m_percussionCheckBox->setChecked(getSelectedInstrument()->isPercussion());
    
    // Bank
    m_bankCheckBox->setChecked(getSelectedInstrument()->sendsBankSelect());
    updateBankComboBox();

    // Program
    m_programCheckBox->setChecked(getSelectedInstrument()->sendsProgramChange());
    updateProgramComboBox();

    // Variation
    m_variationCheckBox->setChecked(getSelectedInstrument()->sendsBankSelect());
    updateVariationComboBox();

    // Channel
    m_channelValue->setCurrentIndex(
            getSelectedInstrument()->hasFixedChannel() ? 1 : 0);

    // Controller Rotaries

    // Make sure we have the right number of Rotary widgets and
    // they have the proper labels.
    // rename: setupRotaries()?
    setupControllers(md);

    // For each rotary
    for (RotaryInfoVector::iterator rotaryIter = m_rotaries.begin();
            rotaryIter != m_rotaries.end(); ++rotaryIter) {
        MidiByte value = 0;

        try {
            value = getSelectedInstrument()->getControllerValue(rotaryIter->controller);
        } catch (...) {  // unknown controller, try the next one
            continue;
        }

        rotaryIter->rotary->setPosition(static_cast<float>(value));
    }

    RG_DEBUG << "updateWidgets() end";
}

void
MIDIInstrumentParameterPanel::setupControllers(MidiDevice *md)
{
    RG_DEBUG << "setupControllers()";

    if (!md)
        return;

    // To cut down on flicker, we avoid destroying and recreating
    // widgets as far as possible here.  If a label already exists,
    // we just set its text; if a rotary exists, we only replace it
    // if we actually need a different one.

    Composition &comp =
            RosegardenMainWindow::self()->getDocument()->getComposition();

    ControlList list = md->getControlParameters();

    // Sort by IPB position.
    std::sort(list.begin(), list.end(),
              ControlParameter::ControlPositionCmp());

    int count = 0;
    RotaryInfoVector::iterator rotaryIter = m_rotaries.begin();

    // For each controller
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
        if (rotaryIter != m_rotaries.end()) {

            // Update the controller number that is associated with the
            // existing rotary widget.

            rotaryIter->controller = it->getControllerValue();

            // Update the properties of the existing rotary widget.

            rotary = rotaryIter->rotary;

            rotary->setMinimum(it->getMin());
            rotary->setMaximum(it->getMax());
            // If the default is 64, then this is most likely a "centered"
            // control which should show its distance from the 12 o'clock
            // position around the outside.
            rotary->setCentered((it->getDefault() == 64));
            rotary->setKnobColour(knobColour);

            // Update the controller name.
            rotaryIter->label->setText(QObject::tr(it->getName().c_str()));

            // Next Rotary widget
            ++rotaryIter;

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
                                (it->getDefault() == 64));  // centred, see setCentered() above
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

            rotaryIter = m_rotaries.end();
        }

        // Add signal mapping
        //
        m_rotaryMapper->setMapping(rotary,
                                   int(it->getControllerValue()));

        ++count;
    }

    // If there are more rotary widgets than this instrument needs,
    // delete them.
    if (rotaryIter != m_rotaries.end()) {
        for (RotaryInfoVector::iterator it = rotaryIter; it != m_rotaries.end(); ++it) {
            // ??? Instead of deleting and recreating, we could hide the
            //     extras and bring them back when needed.
            delete it->rotary;
            delete it->label;
        }
        m_rotaries.resize(count);
    }
}

void
MIDIInstrumentParameterPanel::showBank(bool show)
{
    // Show/hide all bank-related widgets.
    m_bankLabel->setVisible(show);
    m_bankCheckBox->setVisible(show);
    m_bankComboBox->setVisible(show);
}

void
MIDIInstrumentParameterPanel::updateBankComboBox()
{
    RG_DEBUG << "updateBankComboBox()";

    if (!getSelectedInstrument())
        return;

    MidiDevice *md =
            dynamic_cast<MidiDevice *>(getSelectedInstrument()->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::updateBankComboBox(): No MidiDevice for Instrument " << getSelectedInstrument()->getId() << '\n';
        return;
    }

    int currentBank = -1;
    BankList banks;

    RG_DEBUG << "updateBankComboBox(): Variation type is " << md->getVariationType();

    if (md->getVariationType() == MidiDevice::NoVariations) {

        banks = md->getBanks(getSelectedInstrument()->isPercussion());

        // If there are banks to display, show the bank widgets.
        // Why not showBank(banks.size()>1)?  Because that would hide the
        // bank checkbox which would take away the user's ability to
        // enable/disable bank selects.  If we do away with the checkbox
        // in the future, we should re-evaluate this decision.
        showBank(!banks.empty());

        // Find the selected bank in the MIDI Device's bank list.
        for (unsigned int i = 0; i < banks.size(); ++i) {
            if (getSelectedInstrument()->getProgram().getBank().partialCompare(banks[i])) {
                currentBank = i;
                break;
            }
        }

    } else {

        // Usually in variation mode, the bank widgets will be hidden.
        // E.g. in GM2, the MSB for all banks is 121 with the variations
        // in the LSB numbered 0-9.  If, however, there were another
        // MSB, say 122, with some variations in the LSB, this code would
        // display the Bank combobox to allow selection of the MSB.

        // If the variations are in the LSB, then the banks are in the MSB
        // and vice versa.
        bool useMSB = (md->getVariationType() == MidiDevice::VariationFromLSB);

        MidiByteList bytes;

        if (useMSB) {
            bytes = md->getDistinctMSBs(getSelectedInstrument()->isPercussion());
        } else {
            bytes = md->getDistinctLSBs(getSelectedInstrument()->isPercussion());
        }

        // If more than one bank value is found, show the bank widgets.
        showBank(bytes.size() > 1);

        // Load "banks" with the banks and figure out currentBank.

        if (useMSB) {
            for (unsigned int i = 0; i < bytes.size(); ++i) {
                BankList bl = md->getBanksByMSB
                              (getSelectedInstrument()->isPercussion(), bytes[i]);
                RG_DEBUG << "updateBankComboBox(): Have " << bl.size() << " variations for MSB " << bytes[i];

                if (bl.size() == 0)
                    continue;
                if (getSelectedInstrument()->getMSB() == bytes[i]) {
                    currentBank = banks.size();
                }
                banks.push_back(bl[0]);
            }
        } else {
            for (unsigned int i = 0; i < bytes.size(); ++i) {
                BankList bl = md->getBanksByLSB
                              (getSelectedInstrument()->isPercussion(), bytes[i]);

                RG_DEBUG << "updateBankComboBox(): Have " << bl.size() << " variations for LSB " << bytes[i];

                if (bl.size() == 0)
                    continue;
                if (getSelectedInstrument()->getLSB() == bytes[i]) {
                    currentBank = banks.size();
                }
                banks.push_back(bl[0]);
            }
        }
    }

    // Populate the combobox with bank names.

    // If we need to repopulate m_bankComboBox
    if (banks != m_banks)
    {
        // Update the cache.
        m_banks = banks;

        // Copy from m_banks to m_bankComboBox.
        m_bankComboBox->clear();
        for (BankList::const_iterator i = m_banks.begin();
                i != m_banks.end(); ++i) {
            m_bankComboBox->addItem(QObject::tr(i->getName().c_str()));
        }
    }

    m_bankComboBox->setEnabled(getSelectedInstrument()->sendsBankSelect());

#if 0
// ??? This is a pretty nifty idea, but unfortunately, it requires
//     that we maintain a bogus combobox entry.  For now, we'll go
//     with the simpler "unselected" approach.

    // If the current bank was not found...
    if (currentBank < 0  &&  !banks.empty()) {
        // Format bank MSB:LSB and add to combobox.
        MidiBank bank = getSelectedInstrument()->getProgram().getBank();
        QString bankString = QString("%1:%2").arg(bank.getMSB()).arg(bank.getLSB());
        m_bankComboBox.addItem(bankString);
        currentBank = banks.size();
    }
#endif

    // If the bank wasn't in the Device, show the bank widgets so
    // the user can fix it if they want.
    if (currentBank == -1  &&  !banks.empty())
        showBank(true);

    // Display the current bank.
    m_bankComboBox->setCurrentIndex(currentBank);
}

bool
MIDIInstrumentParameterPanel::hasNoName(const MidiProgram &p)
{
    return (p.getName() == "");
}

void
MIDIInstrumentParameterPanel::updateProgramComboBox()
{
    RG_DEBUG << "updateProgramComboBox()";

    if (!getSelectedInstrument())
        return;

    MidiDevice *md =
            dynamic_cast<MidiDevice *>(getSelectedInstrument()->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::updateProgramComboBox(): No MidiDevice for Instrument " << getSelectedInstrument()->getId() << '\n';
        return;
    }

    RG_DEBUG << "updateProgramComboBox(): variation type is " << md->getVariationType();

    MidiBank bank = getSelectedInstrument()->getProgram().getBank();

    ProgramList programs =
            md->getPrograms0thVariation(getSelectedInstrument()->isPercussion(), bank);

    // Remove the programs that have no name.
    programs.erase(std::remove_if(programs.begin(), programs.end(),
                                  MIDIInstrumentParameterPanel::hasNoName),
                   programs.end());

    // If we've got programs, show the Program widgets.
    // Why not "show = (programs.size()>1)"?  Because that would hide the
    // program checkbox which would take away the user's ability to
    // enable/disable program changes.  If we do away with the checkbox
    // in the future, we should re-evaluate this decision.
    bool show = !programs.empty();
    m_programLabel->setVisible(show);
    m_programCheckBox->setVisible(show);
    m_programComboBox->setVisible(show);

    int currentProgram = -1;

    // Compute the current program.
    for (unsigned i = 0; i < programs.size(); ++i) {
        // If the program change is the same...
        if (getSelectedInstrument()->getProgram().getProgram() == programs[i].getProgram()) {
            currentProgram = i;
            break;
        }
    }

    // If the programs have changed, we need to repopulate the combobox.
    if (!partialCompareWithName(programs, m_programs))
    {
        // Update the cache.
        m_programs = programs;

        // Copy from m_programs to m_programComboBox.
        m_programComboBox->clear();
        for (unsigned i = 0; i < m_programs.size(); ++i) {
            m_programComboBox->addItem(QObject::tr("%1. %2")
                                       .arg(m_programs[i].getProgram() + 1)
                                       .arg(QObject::tr(m_programs[i].getName().c_str())));
        }
    }

    m_programComboBox->setEnabled(getSelectedInstrument()->sendsProgramChange());

#if 0
// ??? This is a pretty nifty idea, but unfortunately, it requires
//     that we maintain a bogus combobox entry.  For now, we'll go
//     with the simpler "unselected" approach.

    // If the current program was not found...
    if (currentProgram < 0  &&  !m_programs.empty()) {
        // Format program change and add to combobox.
        MidiByte programChange = getSelectedInstrument()->getProgram().getProgram();
        m_programComboBox.addItem(QString::number(programChange + 1));
        currentProgram = programs.size();
    }
#endif

    // Display the current program.
    m_programComboBox->setCurrentIndex(currentProgram);
}

void
MIDIInstrumentParameterPanel::showVariation(bool show)
{
    // Show/hide all variation-related widgets.
    m_variationLabel->setVisible(show);
    m_variationCheckBox->setVisible(show);
    m_variationComboBox->setVisible(show);
}

void
MIDIInstrumentParameterPanel::updateVariationComboBox()
{
    RG_DEBUG << "updateVariationComboBox() begin...";

    if (!getSelectedInstrument())
        return;

    MidiDevice *md =
            dynamic_cast<MidiDevice *>(getSelectedInstrument()->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::updateVariationComboBox(): No MidiDevice for Instrument " << getSelectedInstrument()->getId() << '\n';
        return;
    }

    RG_DEBUG << "updateVariationComboBox(): Variation type is " << md->getVariationType();

    if (md->getVariationType() == MidiDevice::NoVariations) {
        showVariation(false);
        return;
    }

    // Get the variations.

    bool useMSB = (md->getVariationType() == MidiDevice::VariationFromMSB);
    MidiByteList variationBanks;

    if (useMSB) {
        MidiByte lsb = getSelectedInstrument()->getLSB();
        variationBanks = md->getDistinctMSBs(getSelectedInstrument()->isPercussion(),
                                         lsb);
        RG_DEBUG << "updateVariationComboBox(): Have " << variationBanks.size() << " variations for LSB " << lsb;

    } else {
        MidiByte msb = getSelectedInstrument()->getMSB();
        variationBanks = md->getDistinctLSBs(getSelectedInstrument()->isPercussion(),
                                         msb);

        RG_DEBUG << "updateVariationComboBox(): Have " << variationBanks.size() << " variations for MSB " << msb;
    }

    // Convert variationBanks to a ProgramList.

    ProgramList variations;

    // For each variation
    for (size_t i = 0; i < variationBanks.size(); ++i) {
        // Assemble the program for the variation.
        MidiBank bank;
        if (useMSB) {
            bank = MidiBank(getSelectedInstrument()->isPercussion(),
                            variationBanks[i],
                            getSelectedInstrument()->getLSB());
        } else {
            bank = MidiBank(getSelectedInstrument()->isPercussion(),
                            getSelectedInstrument()->getMSB(),
                            variationBanks[i]);
        }
        MidiProgram program(bank, getSelectedInstrument()->getProgramChange());

        // Skip any programs without names.
        if (md->getProgramName(program) == "")
            continue;

        variations.push_back(program);
    }

    // Compute the current variation.
    // ??? This might be combined into the previous for loop.

    int currentVariation = -1;

    // For each variation
    for (size_t i = 0; i < variations.size(); ++i) {
        if (getSelectedInstrument()->getProgram().partialCompare(variations[i])) {
            currentVariation = i;
            break;
        }
    }

    // If the variations have changed, repopulate the combobox.
    if (!partialCompareWithName(variations, m_variations)) {
        RG_DEBUG << "updateVariationComboBox(): Repopulating the combobox";

        // Update the cache.
        m_variations = variations;

        // Copy from m_variations to m_variationComboBox.
        m_variationComboBox->clear();
        for (size_t i = 0; i < m_variations.size(); ++i) {
            std::string programName = md->getProgramName(m_variations[i]);

            // Pick the correct bank number.
            MidiBank bank = m_variations[i].getBank();
            MidiByte variationBank = useMSB ? bank.getMSB() : bank.getLSB();

            m_variationComboBox->addItem(QObject::tr("%1. %2")
                                         .arg(variationBank)
                                         .arg(QObject::tr(programName.c_str())));
        }
    }

    // Display the current variation.
    m_variationComboBox->setCurrentIndex(currentVariation);

    // Show the variation widgets in either of two cases:
    //   1. More than one variation is available for this program.
    //   2. The variation was not in the Device and there is a variation
    //      to choose from.
    showVariation(m_variations.size() > 1  ||
                  (currentVariation == -1  &&  !m_variations.empty()));

    m_variationComboBox->setEnabled(getSelectedInstrument()->sendsBankSelect());
}

void
MIDIInstrumentParameterPanel::slotNewDocument(RosegardenDocument *doc)
{
    connect(doc, SIGNAL(documentModified(bool)),
            SLOT(slotDocumentModified(bool)));
}

void
MIDIInstrumentParameterPanel::slotDocumentModified(bool)
{
    RosegardenDocument *doc = RosegardenMainWindow::self()->getDocument();

    // Get the selected Track's Instrument.
    InstrumentId instrumentId =
            doc->getComposition().getSelectedInstrumentId();

    Instrument *instrument = NULL;

    // If an instrument has been selected.
    if (instrumentId != NoInstrument)
        instrument = doc->getStudio().getInstrumentById(instrumentId);

    // If the user has selected a different instrument
    if (getSelectedInstrument() != instrument) {
        // Clear the "Receive external" checkbox.
        m_receiveExternalCheckBox->setChecked(false);
    }

    if (!instrument) {
        setSelectedInstrument(NULL);
        return;
    }

    if (instrument->getType() != Instrument::Midi) {
        setSelectedInstrument(NULL);
        return;
    }

    setSelectedInstrument(instrument);

    updateWidgets();
}

void
MIDIInstrumentParameterPanel::slotInstrumentChanged(Instrument *instrument)
{
    if (!instrument)
        return;

    if (!getSelectedInstrument())
        return;

    // If this isn't a change for the Instrument we are displaying, bail.
    if (getSelectedInstrument()->getId() != instrument->getId())
        return;

    updateWidgets();
}

void
MIDIInstrumentParameterPanel::slotControlChange(Instrument *instrument, int cc)
{
    if (!instrument)
        return;

    if (!getSelectedInstrument())
        return;

    // If this isn't a change for the Instrument we are displaying, bail.
    if (getSelectedInstrument()->getId() != instrument->getId())
        return;

    // Just update the relevant rotary.

    // For each rotary...
    for (RotaryInfoVector::iterator rotaryIter = m_rotaries.begin();
            rotaryIter != m_rotaries.end(); ++rotaryIter) {

        // If this is the one we want, update it.
        if (rotaryIter->controller == cc)
        {
            try {

                MidiByte value =
                        getSelectedInstrument()->getControllerValue(cc);
                rotaryIter->rotary->setPosition(static_cast<float>(value));

            } catch (...) {
                RG_WARNING << "slotControlChange(): WARNING: Encountered unexpected cc " << cc;
            }

            break;
        }

    }
}

void
MIDIInstrumentParameterPanel::slotPercussionClicked(bool checked)
{
    RG_DEBUG << "slotPercussionClicked(" << checked << ")";

    if (!getSelectedInstrument())
        return;

    // Update the Instrument.
    getSelectedInstrument()->setPercussion(checked);
    getSelectedInstrument()->changed();

    RosegardenMainWindow::self()->getDocument()->slotDocumentModified();

    // At this point, the bank will be invalid.  We could select
    // the first valid bank/program for the current mode (percussion
    // or not percussion).  This seems to be the right thing to
    // do for the most common use case of setting up a track for
    // percussion on a new device.  OTOH, the Device should already
    // know which channels are percussion and which aren't.  So, it
    // seems highly unlikely that anyone will ever click the Percussion
    // checkbox.  Probably best just to leave this simple.  The user
    // will normally be presented with a blank bank combobox and they
    // can fix the problem themselves.
}

void
MIDIInstrumentParameterPanel::slotBankClicked(bool checked)
{
    RG_DEBUG << "slotBankClicked()";

    if (!getSelectedInstrument())
        return;

    // Update the Instrument.
    getSelectedInstrument()->setSendBankSelect(checked);
    getSelectedInstrument()->changed();
}

void
MIDIInstrumentParameterPanel::slotProgramClicked(bool checked)
{
    RG_DEBUG << "slotProgramClicked()";

    if (!getSelectedInstrument())
        return;

    // Update the Instrument.
    getSelectedInstrument()->setSendProgramChange(checked);
    getSelectedInstrument()->changed();
}

void
MIDIInstrumentParameterPanel::slotVariationClicked(bool checked)
{
    RG_DEBUG << "slotVariationClicked()";

    // ??? Disabling the sending of Bank Selects in Variations mode seems
    //     very strange.  Variations mode is all about selecting a
    //     variation through different bank selects.  Without the bank
    //     selects, there are no variations.  It's likely that we can get
    //     rid of this checkbox (and always send banks selects) and no one
    //     will notice.

    if (!getSelectedInstrument())
        return;

    // Update the Instrument.
    getSelectedInstrument()->setSendBankSelect(checked);
    getSelectedInstrument()->changed();
}

void
MIDIInstrumentParameterPanel::slotSelectBank(int index)
{
    RG_DEBUG << "slotSelectBank() begin...";

    if (!getSelectedInstrument())
        return;

    MidiDevice *md =
            dynamic_cast<MidiDevice *>(getSelectedInstrument()->getDevice());
    if (!md) {
        std::cerr << "WARNING: MIDIInstrumentParameterPanel::slotSelectBank(): No MidiDevice for Instrument " << getSelectedInstrument()->getId() << '\n';
        return;
    }

    const MidiBank &bank = m_banks[index];

    bool change = false;

    if (md->getVariationType() != MidiDevice::VariationFromLSB) {
        if (getSelectedInstrument()->getLSB() != bank.getLSB()) {
            getSelectedInstrument()->setLSB(bank.getLSB());
            change = true;
        }
    }
    if (md->getVariationType() != MidiDevice::VariationFromMSB) {
        if (getSelectedInstrument()->getMSB() != bank.getMSB()) {
            getSelectedInstrument()->setMSB(bank.getMSB());
            change = true;
        }
    }

    // If no change, bail.
    if (!change)
        return;

    // Make sure the Instrument is valid WRT the Device.

    // If the current bank/program is not valid for this device, fix it.
    if (!getSelectedInstrument()->isProgramValid()) {

        // If we're not in variations mode...
        if (md->getVariationType() == MidiDevice::NoVariations) {

            // ...go with the first program
            ProgramList programList = md->getPrograms(bank);
            if (!programList.empty()) {
                // Switch to the first program in this bank.
                getSelectedInstrument()->setProgram(programList.front());
            } else {
                // No programs for this bank.  Just go with 0.
                getSelectedInstrument()->setProgramChange(0);
            }

        } else {  // We're in variations mode...

            // This is the three-comboboxes (bank/program/variation) case.
            // It's an extremely difficult case to handle, so we're just
            // going to punt and give them the first program/variation in
            // the bank they just selected.

            // Get the variation bank list for this bank
            BankList bankList;
            if (md->getVariationType() == MidiDevice::VariationFromMSB) {
                bankList = md->getBanksByLSB(
                        getSelectedInstrument()->isPercussion(), bank.getLSB());
            } else {
                bankList = md->getBanksByMSB(
                        getSelectedInstrument()->isPercussion(), bank.getMSB());
            }
            if (!bankList.empty()) {
                // Pick the first bank
                MidiBank firstBank = bankList.front();
                // Get the program list
                ProgramList programList = md->getPrograms(firstBank);
                if (!programList.empty()) {
                    // Pick the first program
                    getSelectedInstrument()->setProgram(programList.front());
                }
            }

            // To make the above more complex, we could consider the
            // case where the Program Change happens to be valid for
            // some variation bank in the newly selected bank.  Then
            // go with the 0th variation bank that has that program
            // change.  But I think this is complicated enough.

        }
    }

    getSelectedInstrument()->sendChannelSetup();

    // This is why changed() isn't called within
    // the setters.  If it were, then each of the above changes would
    // result in a change notification going out.  Worst case, that
    // would be three change notifications and the first two would be
    // sent when the Instrument was in an inconsistent state.
    // Rule: Avoid sending change notifications from setters.
    // Why?  It reduces the number of notifications which improves
    // performance.  It avoids sending notifications when an object's
    // state is inconsistent.  It avoids endless loops.
    getSelectedInstrument()->changed();
}

void
MIDIInstrumentParameterPanel::slotExternalProgramChange(int programChange, int bankLSB, int bankMSB )
{
    RG_DEBUG << "slotExternalProgramChange()";

    // If we aren't set to "Receive External", bail.
    if (!m_receiveExternalCheckBox->isChecked())
        return;

    if (!getSelectedInstrument())
        return;

    bool bankChanged = false;

    // MSB Bank Select
    if (bankMSB >= 0) {  // &&  md->getVariationType() != MidiDevice::VariationFromMSB ) {
        // If the MSB is changing
        if (getSelectedInstrument()->getMSB() != bankMSB) {
            getSelectedInstrument()->setMSB(bankMSB);
            bankChanged = true;
        }
    }

    // LSB Bank Select
    if (bankLSB >= 0) { // &&  md->getVariationType() != MidiDevice::VariationFromLSB) {
        // If the LSB is changing
        if (getSelectedInstrument()->getLSB() != bankLSB) {
            getSelectedInstrument()->setLSB(bankLSB);
            bankChanged = true;
        }
    }

    bool pcChanged = false;

    // If the Program Change is changing
    if (getSelectedInstrument()->getProgramChange() !=
                static_cast<MidiByte>(programChange)) {
        getSelectedInstrument()->setProgramChange(static_cast<MidiByte>(programChange));
        pcChanged = true;
    }

    // If nothing changed, bail.
    if (!pcChanged  &&  !bankChanged)
        return;

    // ??? If an unexpected bank/program change comes in, we could
    //     pop up a message box to let the user know the values so
    //     they can fix their device file.  Might be pretty annoying.
    //     Maybe with a "[X] Don't *ever* show this pop-up again"
    //     checkbox.

    // Just one change notification for the three potential changes.
    // See comments in slotSelectBank() for further discussion.
    getSelectedInstrument()->changed();
}

void
MIDIInstrumentParameterPanel::slotSelectProgram(int index)
{
    RG_DEBUG << "slotSelectProgram()";

    if (!getSelectedInstrument())
        return;

    MidiDevice *md =
            dynamic_cast<MidiDevice *>(getSelectedInstrument()->getDevice());
    if (!md)
        return;

    const MidiProgram *prg = &m_programs[index];

    // If there has been no change, bail.
    if (getSelectedInstrument()->getProgramChange() == prg->getProgram())
        return;

    getSelectedInstrument()->setProgramChange(prg->getProgram());

    // In Variations mode, select the 0th variation.

    // In Variations mode, it's very easy to select an "invalid"
    // program change.  I.e. one for which the bank is not valid.  Go
    // from one program/variation to a program that doesn't have that
    // variation.  We need to handle that here by selecting the 0th
    // variation.  That's what the user expects.

    if (md->getVariationType() == MidiDevice::VariationFromMSB) {
        MidiBank bank = getSelectedInstrument()->getProgram().getBank();
        // Get the list of MSB variations.
        BankList bankList = md->getBanksByLSB(
                getSelectedInstrument()->isPercussion(), bank.getLSB());
        if (!bankList.empty()) {
            // Pick the first MSB variation
            getSelectedInstrument()->setMSB(bankList.front().getMSB());
        }
    }
    if (md->getVariationType() == MidiDevice::VariationFromLSB) {
        MidiBank bank = getSelectedInstrument()->getProgram().getBank();
        // Get the list of LSB variations.
        BankList bankList = md->getBanksByMSB(
                getSelectedInstrument()->isPercussion(), bank.getMSB());
        if (!bankList.empty()) {
            // Pick the first LSB variation
            getSelectedInstrument()->setLSB(bankList.front().getLSB());
        }
    }

    getSelectedInstrument()->sendChannelSetup();

    // Just one change notification for the two potential changes.
    // See comments in slotSelectBank() for further discussion.
    getSelectedInstrument()->changed();
}

void
MIDIInstrumentParameterPanel::slotSelectVariation(int index)
{
    RG_DEBUG << "slotSelectVariation()";

    if (!getSelectedInstrument())
        return;

    MidiBank newBank = m_variations[index].getBank();

    bool changed = false;

    // Update bank MSB/LSB as needed.
    if (getSelectedInstrument()->getMSB() != newBank.getMSB()) {
        getSelectedInstrument()->setMSB(newBank.getMSB());
        changed = true;
    }
    if (getSelectedInstrument()->getLSB() != newBank.getLSB()) {
        getSelectedInstrument()->setLSB(newBank.getLSB());
        changed = true;
    }

    if (!changed)
        return;

    getSelectedInstrument()->sendChannelSetup();

    getSelectedInstrument()->changed();
}

// In place of the old sendBankAndProgram, instruments themselves now
// signal the affected channel managers when changed.

void
MIDIInstrumentParameterPanel::slotControllerChanged(int controllerNumber)
{
    RG_DEBUG << "slotControllerChanged(" << controllerNumber << ")";

    if (!getSelectedInstrument())
        return;

    int value = -1;

    // Figure out who sent this signal.
    Rotary *rotary = dynamic_cast<Rotary *>(m_rotaryMapper->mapping(controllerNumber));
    if (rotary)
        value = static_cast<int>(std::floor(rotary->getPosition() + .5));

    if (value == -1) {
        std::cerr << "MIDIInstrumentParameterPanel::slotControllerChanged(): Couldn't get value of rotary for controller " << controllerNumber << '\n';
        return;
    }

    getSelectedInstrument()->setControllerValue(
            static_cast<MidiByte>(controllerNumber),
            static_cast<MidiByte>(value));
    Instrument::getStaticSignals()->
            emitControlChange(getSelectedInstrument(), controllerNumber);
    RosegardenMainWindow::self()->getDocument()->setModified();
}

void
MIDIInstrumentParameterPanel::
slotSelectChannel(int index)
{
    RG_DEBUG << "slotSelectChannel(" << index << ")";

    if (!getSelectedInstrument())
        return;

    // Fixed
    if (index == 1)
        getSelectedInstrument()->setFixedChannel();
    else  // Auto
        getSelectedInstrument()->releaseFixedChannel();

    getSelectedInstrument()->sendChannelSetup();

    // A call to getSelectedInstrument()->changed() is not required as the
    // auto/fixed channel feature has its own notification mechanisms.
}


}
