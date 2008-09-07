/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    This file is Copyright 2006
        Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
        D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TrackParameterBox.h"
#include <QLayout>
#include <kapplication.h>

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/general/ClefIndex.h"
#include "document/ConfigGroups.h"
#include "base/AudioPluginInstance.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Composition.h"
#include "base/Device.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/StaffExportTypes.h"
#include "commands/segment/SegmentSyncCommand.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/dialogs/PitchPickerDialog.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/PresetHandlerDialog.h"
#include "gui/widgets/CollapsingFrame.h"
#include "gui/widgets/ColourTable.h"
#include "RosegardenParameterArea.h"
#include "RosegardenParameterBox.h"
#include "sound/PluginIdentifier.h"
#include <kcolordialog.h>
#include <QComboBox>
#include <kconfig.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <ksqueezedtextlabel.h>
#include <ktabwidget.h>
#include <QColor>
#include <QDialog>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QRegExp>
#include <qscrollview.h>
#include <QString>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <qwidgetstack.h>
#include <QCheckBox>


namespace Rosegarden
{

TrackParameterBox::TrackParameterBox( RosegardenGUIDoc *doc,
                                      QWidget *parent)
        : RosegardenParameterBox(i18n("Track"),
                                 i18n("Track Parameters"),
                                 parent),
        m_doc(doc),
        m_highestPlayable(127),
        m_lowestPlayable(0),
        m_selectedTrackId( -1)
{
    QFont font(m_font);
    QFont title_font(m_font);
    QFontMetrics metrics(font);
    int width11 = metrics.width("12345678901");
    int width20 = metrics.width("12345678901234567890");
    int width22 = metrics.width("1234567890123456789012");
    int width25 = metrics.width("1234567890123456789012345");
    setFont(m_font);
    title_font.setBold(true);

    // Set up default expansions for the collapsing elements
    QSettings config ; // was: confq4
    QString groupTemp = config->group();
    QSettings config;
    config.beginGroup( "CollapsingFrame" );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( "CollapsingFrame" );
    //  

    bool expanded = qStrToBool( config.value("trackparametersplayback", "true" ) ) ;
    config.setValue("trackparametersplayback", expanded);
    expanded = qStrToBool( config.value("trackparametersrecord", "false" ) ) ;
    config.setValue("trackparametersrecord", expanded);
    expanded = qStrToBool( config.value("trackparametersdefaults", "false" ) ) ;
    config.setValue("trackparametersdefaults", expanded);
    expanded = qStrToBool( config.value("trackstaffgroup", "false" ) ) ;
    config.setValue("trackstaffgroup", expanded);
    QSettings config;
    config.beginGroup( groupTemp );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( groupTemp );
    //  


    QGridLayout *mainLayout = new QGridLayout(this, 5, 1, 2, 1);

    int row = 0;

    // track label
    //
    m_trackLabel = new KSqueezedTextLabel(i18n("<untitled>"), this);
    m_trackLabel->setAlignment(Qt::AlignCenter);
    //mainLayout->addWidget(m_trackLabel, 0, 0, 0- 0+1, 5- 1, AlignCenter);
    mainLayout->addWidget(m_trackLabel, 0, 0);

    // playback group
    //
    CollapsingFrame *cframe = new CollapsingFrame(i18n("Playback parameters"),
                              this, "trackparametersplayback");
    m_playbackGroup = new QFrame(cframe);
    cframe->setWidget(m_playbackGroup);
    QGridLayout *groupLayout = new QGridLayout(m_playbackGroup, 3, 3, 3, 2);

    // playback group title
    //
    row = 0;

    // playback device
    //
    //    row++;
    QLabel *devLabel = new QLabel(i18n("Device"), m_playbackGroup);
    groupLayout->addWidget(devLabel, row, 0);
    m_playDevice = new QComboBox(m_playbackGroup);
    m_playDevice->setMinimumWidth(width25);
    groupLayout->addWidget(m_playDevice, row, 1, row- row+1, 2);

    // playback instrument
    //
    row++;
    QLabel *insLabel = new QLabel(i18n("Instrument"), m_playbackGroup);
    groupLayout->addWidget(insLabel, row, 0, row- row+1, 1- 1);
    m_instrument = new QComboBox(m_playbackGroup);
    m_instrument->setMaxVisibleItems( 16 );
    m_instrument->setMinimumWidth(width22);
    groupLayout->addWidget(m_instrument, row, 2);

    groupLayout->setColStretch(groupLayout->numCols() - 1, 1);

    mainLayout->addWidget(cframe, 1, 0);

    // record group
    //
    cframe = new CollapsingFrame(i18n("Recording filters"), this,
                                 "trackparametersrecord");
    m_recordGroup = new QFrame(cframe);
    cframe->setWidget(m_recordGroup);
    groupLayout = new QGridLayout(m_recordGroup, 3, 3, 3, 2);

    // recording group title
    //
    row = 0;

    // recording device
    groupLayout->addWidget(new QLabel(i18n("Device"), m_recordGroup), row, 0);
    m_recDevice = new QComboBox(m_recordGroup);
    m_recDevice->setMinimumWidth(width25);
    groupLayout->addWidget(m_recDevice, row, 1, row- row+1, 2);

    // recording channel
    //
    row++;
    groupLayout->addMultiCellWidget(new QLabel(i18n("Channel"), m_recordGroup), row, row, 0, 1);
    m_recChannel = new QComboBox(m_recordGroup);
    m_recChannel->setMaxVisibleItems( 17 );
    m_recChannel->setMinimumWidth(width11);
    groupLayout->addWidget(m_recChannel, row, 2);

    groupLayout->setColStretch(groupLayout->numCols() - 1, 1);

    mainLayout->addWidget(cframe, 2, 0);

    // staff group
    //
    cframe = new CollapsingFrame(i18n("Staff export options"), this,
                                 "staffoptions");
    m_staffGroup = new QFrame(cframe);
    cframe->setWidget(m_staffGroup);
    groupLayout = new QGridLayout(m_staffGroup, 2, 2, 2, 2);

    groupLayout->setColStretch(1, 1);

    row = 0;

    // Notation size (export only)
    //
    // NOTE: This is the only way to get a \small or \tiny inserted before the
    // first note in LilyPond export.  Setting the actual staff size on a
    // per-staff (rather than per-score) basis is something the author of the
    // LilyPond documentation has no idea how to do, so we settle for this,
    // which is not as nice, but actually a lot easier to implement.
    m_staffGrpLbl = new QLabel(i18n("Notation size:"), m_staffGroup);
    groupLayout->addWidget(m_staffGrpLbl, row, 0, AlignLeft);
    m_staffSizeCombo = new QComboBox(m_staffGroup);
    m_staffSizeCombo->setMinimumWidth(width11);
    m_staffSizeCombo->addItem(i18n("Normal"), StaffTypes::Normal);
    m_staffSizeCombo->addItem(i18n("Small"), StaffTypes::Small);
    m_staffSizeCombo->addItem(i18n("Tiny"), StaffTypes::Tiny);

    groupLayout->addWidget(m_staffSizeCombo, row, 1, row- row+1, 2);

    // Staff bracketing (export only at the moment, but using this for GUI
    // rendering would be nice in the future!) //!!! 
    row++;
    m_grandStaffLbl = new QLabel(i18n("Bracket type:"), m_staffGroup);
    groupLayout->addWidget(m_grandStaffLbl, row, 0, AlignLeft);
    m_staffBracketCombo = new QComboBox(m_staffGroup);
    m_staffBracketCombo->setMinimumWidth(width11);
    m_staffBracketCombo->addItem(i18n("-----"), Brackets::None);
    m_staffBracketCombo->addItem(i18n("[----"), Brackets::SquareOn);
    m_staffBracketCombo->addItem(i18n("----]"), Brackets::SquareOff);
    m_staffBracketCombo->addItem(i18n("[---]"), Brackets::SquareOnOff);
    m_staffBracketCombo->addItem(i18n("{----"), Brackets::CurlyOn);
    m_staffBracketCombo->addItem(i18n("----}"), Brackets::CurlyOff);
    m_staffBracketCombo->addItem(i18n("{[---"), Brackets::CurlySquareOn);
    m_staffBracketCombo->addItem(i18n("---]}"), Brackets::CurlySquareOff);

    groupLayout->addWidget(m_staffBracketCombo, row, 1, row- row+1, 2);

    mainLayout->addWidget(cframe, 3, 0);


    // default segment group
    //
    cframe = new CollapsingFrame(i18n("Create segments with"), this,
                                 "trackparametersdefaults");
    m_defaultsGroup = new QFrame(cframe);
    cframe->setWidget(m_defaultsGroup);
    groupLayout = new QGridLayout(m_defaultsGroup, 6, 6, 3, 2);

    groupLayout->setColStretch(1, 1);

    row = 0;

    // preset picker
    m_psetLbl = new QLabel(i18n("Preset"), m_defaultsGroup);
    groupLayout->addWidget(m_psetLbl, row, 0, AlignLeft);

    m_presetLbl = new QLabel(i18n("<none>"), m_defaultsGroup);
    m_presetLbl->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_presetLbl->setFixedWidth(width20);
    groupLayout->addWidget(m_presetLbl, row, 1, row- row+1, 3);

    m_presetButton = new QPushButton(i18n("Load"), m_defaultsGroup);
    groupLayout->addWidget(m_presetButton, row, 4, row- row+1, 5- 5);

    // default clef
    //
    row++;
    m_clefLbl = new QLabel(i18n("Clef"), m_defaultsGroup);
    groupLayout->addWidget(m_clefLbl, row, 0, AlignLeft);
    m_defClef = new QComboBox(m_defaultsGroup);
    m_defClef->setMinimumWidth(width11);
    m_defClef->addItem(i18n("treble"), TrebleClef);
    m_defClef->addItem(i18n("bass"), BassClef);
    m_defClef->addItem(i18n("crotales"), CrotalesClef);
    m_defClef->addItem(i18n("xylophone"), XylophoneClef);
    m_defClef->addItem(i18n("guitar"), GuitarClef);
    m_defClef->addItem(i18n("contrabass"), ContrabassClef);
    m_defClef->addItem(i18n("celesta"), CelestaClef);
    m_defClef->addItem(i18n("old celesta"), OldCelestaClef);
    m_defClef->addItem(i18n("french"), FrenchClef);
    m_defClef->addItem(i18n("soprano"), SopranoClef);
    m_defClef->addItem(i18n("mezzosoprano"), MezzosopranoClef);
    m_defClef->addItem(i18n("alto"), AltoClef);
    m_defClef->addItem(i18n("tenor"), TenorClef);
    m_defClef->addItem(i18n("baritone"), BaritoneClef);
    m_defClef->addItem(i18n("varbaritone"), VarbaritoneClef);
    m_defClef->addItem(i18n("subbass"), SubbassClef);
    /*  clef types in the datbase that are not yet supported must be ignored for
     *  now:
        m_defClef->addItem(i18n("two bar"), TwoBarClef); */
    groupLayout->addWidget(m_defClef, row, 1, row- row+1, 2);

    // default transpose
    //
    m_transpLbl = new QLabel(i18n("Transpose"), m_defaultsGroup);
    groupLayout->addWidget(m_transpLbl, row, 3, row- row+1, 4- 4, AlignRight);
    m_defTranspose = new QComboBox(m_defaultsGroup);

    connect(m_defTranspose, SIGNAL(activated(int)),
            SLOT(slotTransposeIndexChanged(int)));

    int transposeRange = 48;
    for (int i = -transposeRange; i < transposeRange + 1; i++) {
        m_defTranspose->addItem(QString("%1").arg(i));
        if (i == 0)
            m_defTranspose->setCurrentIndex(m_defTranspose->count() - 1);
    }

    groupLayout->addWidget(m_defTranspose, row, 5, row- row+1, 1);

    // highest/lowest playable note
    //
    row++;
    m_rangeLbl = new QLabel(i18n("Pitch"), m_defaultsGroup);
    groupLayout->addWidget(m_rangeLbl, row, 0, row- row+1, 0- 1);

    groupLayout->addWidget(new QLabel(i18n("Lowest"), m_defaultsGroup), row, 1, AlignRight);

    m_lowButton = new QPushButton(i18n("---"), m_defaultsGroup);
    QToolTip::add
        (m_lowButton, i18n("Choose the lowest suggested playable note, using a staff"));
    groupLayout->addWidget(m_lowButton, row, 2, row- row+1, 1);

    groupLayout->addWidget(new QLabel(i18n("Highest"), m_defaultsGroup), row, 3, AlignRight);

    m_highButton = new QPushButton(i18n("---"), m_defaultsGroup);
    QToolTip::add
        (m_highButton, i18n("Choose the highest suggested playable note, using a staff"));
    groupLayout->addWidget(m_highButton, row, 4, row- row+1, 5- 5);

    updateHighLow();

    // default color
    //
    row++;
    m_colorLbl = new QLabel(i18n("Color"), m_defaultsGroup);
    groupLayout->addWidget(m_colorLbl, row, 0, AlignLeft);
    m_defColor = new QComboBox(false, m_defaultsGroup);
    m_defColor->setMaxVisibleItems(20);
    groupLayout->addWidget(m_defColor, row, 1, row- row+1, 5);

    // populate combo from doc colors
    slotDocColoursChanged();

    mainLayout->addWidget(cframe, 4, 0);


    // Configure the empty final row to accomodate any extra vertical space.
    //
//    mainLayout->setColStretch(mainLayout->numCols() - 1, 1);
    mainLayout->setRowStretch(mainLayout->numRows() - 1, 1);

    // Connections
    connect( m_playDevice, SIGNAL(activated(int)),
             this, SLOT(slotPlaybackDeviceChanged(int)));

    connect( m_instrument, SIGNAL(activated(int)),
             this, SLOT(slotInstrumentChanged(int)));

    connect( m_recDevice, SIGNAL(activated(int)),
             this, SLOT(slotRecordingDeviceChanged(int)));

    connect( m_recChannel, SIGNAL(activated(int)),
             this, SLOT(slotRecordingChannelChanged(int)));

    connect( m_defClef, SIGNAL(activated(int)),
             this, SLOT(slotClefChanged(int)));

    // Detect when the document colours are updated
    connect(m_doc, SIGNAL(docColoursChanged()),
            this, SLOT(slotDocColoursChanged()));

    // handle colour combo changes
    connect(m_defColor, SIGNAL(activated(int)),
            SLOT(slotColorChanged(int)));

    connect(m_highButton, SIGNAL(released()),
            SLOT(slotHighestPressed()));

    connect(m_lowButton, SIGNAL(released()),
            SLOT(slotLowestPressed()));

    connect(m_presetButton, SIGNAL(released()),
            SLOT(slotPresetPressed()));

    connect(m_staffSizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotStaffSizeChanged(int)));

    connect(m_staffBracketCombo, SIGNAL(activated(int)),
            this, SLOT(slotStaffBracketChanged(int)));
}

TrackParameterBox::~TrackParameterBox()
{}

void

TrackParameterBox::setDocument( RosegardenGUIDoc *doc )
{
    if (m_doc != doc) {
        RG_DEBUG << "TrackParameterBox::setDocument\n";
        m_doc = doc;
        populateDeviceLists();
    }
}

void
TrackParameterBox::populateDeviceLists()
{
    RG_DEBUG << "TrackParameterBox::populateDeviceLists()\n";
    populatePlaybackDeviceList();
    //populateRecordingDeviceList();
    slotUpdateControls( -1);
    m_lastInstrumentType = -1;
}

void
TrackParameterBox::populatePlaybackDeviceList()
{
    RG_DEBUG << "TrackParameterBox::populatePlaybackDeviceList()\n";
    m_playDevice->clear();
    m_playDeviceIds.clear();
    m_instrument->clear();
    m_instrumentIds.clear();
    m_instrumentNames.clear();

    Studio &studio = m_doc->getStudio();

    // Get the list
    InstrumentList list = studio.getPresentationInstruments();
    InstrumentList::iterator it;
    int currentDevId = -1;

    for (it = list.begin(); it != list.end(); it++) {

        if (! (*it))
            continue; // sanity check

        //QString iname(strtoqstr((*it)->getPresentationName()));
        QString iname(strtoqstr((*it)->getName()));
        QString pname(strtoqstr((*it)->getProgramName()));
        Device *device = (*it)->getDevice();
        DeviceId devId = device->getId();

        if ((*it)->getType() == Instrument::SoftSynth) {
            iname.replace("Synth plugin ", "");
            pname = "";
            AudioPluginInstance *plugin = (*it)->getPlugin
                                          (Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin) {
                pname = strtoqstr(plugin->getProgram());
                QString identifier = strtoqstr(plugin->getIdentifier());
                if (identifier != "") {
                    QString type, soName, label;
                    PluginIdentifier::parseIdentifier
                    (identifier, type, soName, label);
                    if (pname == "") {
                        pname = strtoqstr(plugin->getDistinctiveConfigurationText());
                    }
                    if (pname != "") {
                        pname = QString("%1: %2").arg(label).arg(pname);
                    } else {
                        pname = label;
                    }
                }
            }
        }

        if (devId != (DeviceId)(currentDevId)) {
            currentDevId = int(devId);
            QString deviceName = strtoqstr(device->getName());
            m_playDevice->addItem(deviceName);
            m_playDeviceIds.push_back(currentDevId);
        }

        if (pname != "")
            iname += " (" + pname + ")";
        m_instrumentIds[currentDevId].push_back((*it)->getId());
        m_instrumentNames[currentDevId].append(iname);

    }

    m_playDevice->setCurrentIndex( -1);
    m_instrument->setCurrentIndex( -1);
}

void
TrackParameterBox::populateRecordingDeviceList()
{
    RG_DEBUG << "TrackParameterBox::populateRecordingDeviceList()\n";

    if (m_selectedTrackId < 0)
        return ;
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(m_selectedTrackId);
    if (!trk)
        return ;

    Instrument *inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
    if (!inst)
        return ;

    if (m_lastInstrumentType != (char)inst->getInstrumentType()) {
        m_lastInstrumentType = (char)inst->getInstrumentType();

        m_recDevice->clear();
        m_recDeviceIds.clear();
        m_recChannel->clear();

        if (inst->getInstrumentType() == Instrument::Audio) {

            m_recDeviceIds.push_back(Device::NO_DEVICE);
            m_recDevice->addItem(i18n("Audio"));
            m_recChannel->addItem(i18n("Audio"));

            m_recDevice->setEnabled(false);
            m_recChannel->setEnabled(false);

            // hide these for audio instruments
            m_defaultsGroup->parentWidget()->setShown(false);

        } else { // InstrumentType::Midi and InstrumentType::SoftSynth

            // show these if not audio instrument
            m_defaultsGroup->parentWidget()->setShown(true);

            m_recDeviceIds.push_back(Device::ALL_DEVICES);
            m_recDevice->addItem(i18n("All"));

            DeviceList *devices = m_doc->getStudio().getDevices();
            DeviceListConstIterator it;
            for (it = devices->begin(); it != devices->end(); it++) {
                MidiDevice *dev =
                    dynamic_cast<MidiDevice*>(*it);
                if (dev) {
                    if (dev->getDirection() == MidiDevice::Record
                            && dev->isRecording()) {
                        QString connection = strtoqstr(dev->getConnection());
                        // remove trailing "(duplex)", "(read only)", "(write only)" etc
                        connection.replace(QRegExp("\\s*\\([^)0-9]+\\)\\s*$"), "");
                        m_recDevice->addItem(connection);
                        m_recDeviceIds.push_back(dev->getId());
                    }
                }
            }

            m_recChannel->addItem(i18n("All"));
            for (int i = 1; i < 17; ++i) {
                m_recChannel->addItem(QString::number(i));
            }

            m_recDevice->setEnabled(true);
            m_recChannel->setEnabled(true);
        }
    }

    if (inst->getInstrumentType() == Instrument::Audio) {
        m_recDevice->setCurrentIndex(0);
        m_recChannel->setCurrentIndex(0);
    } else {
        m_recDevice->setCurrentIndex(0);
        m_recChannel->setCurrentIndex((int)trk->getMidiInputChannel() + 1);
        for (unsigned int i = 0; i < m_recDeviceIds.size(); ++i) {
            if (m_recDeviceIds[i] == trk->getMidiInputDevice()) {
                m_recDevice->setCurrentIndex(i);
                break;
            }
        }
    }
}

void
TrackParameterBox::updateHighLow()
{
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    if (!trk)
        return ;

    trk->setHighestPlayable(m_highestPlayable);
    trk->setLowestPlayable(m_lowestPlayable);

    Accidental accidental = Accidentals::NoAccidental;

    Pitch highest(m_highestPlayable, accidental);
    Pitch lowest(m_lowestPlayable, accidental);

    QSettings config ; // was: confq4
    QSettings config;
    config.beginGroup( GeneralOptionsConfigGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( GeneralOptionsConfigGroup );
    //  

    int base = config.value("midipitchoctave", -2).toInt() ;
    bool useSharps = true;
    bool includeOctave = true;

//    m_highButton->setText(i18n("High: %1").arg(highest.getAsString(useSharps, includeOctave, base)));
//    m_lowButton->setText(i18n("Low: %1").arg(lowest.getAsString(useSharps, includeOctave, base)));
    m_highButton->setText(QString("%1").arg(highest.getAsString(useSharps, includeOctave, base)));
    m_lowButton->setText(QString("%1").arg(lowest.getAsString(useSharps, includeOctave, base)));

    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotUpdateControls(int /*dummy*/)
{
    RG_DEBUG << "TrackParameterBox::slotUpdateControls()\n";
    slotPlaybackDeviceChanged( -1);
    slotInstrumentChanged( -1);

    if (m_selectedTrackId < 0)
        return ;
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(m_selectedTrackId);
    if (!trk)
        return ;

    m_defClef->setCurrentIndex(trk->getClef());
    m_defTranspose->setCurrentIndex(QString("%1").arg(trk->getTranspose()), true);
    m_defColor->setCurrentIndex(trk->getColor());
    m_highestPlayable = trk->getHighestPlayable();
    m_lowestPlayable = trk->getLowestPlayable();
    updateHighLow();
    // set this down here because updateHighLow just disabled the label
    m_presetLbl->setText(trk->getPresetLabel());
    m_presetLbl->setEnabled(true);

    m_staffSizeCombo->setCurrentIndex(trk->getStaffSize());
    m_staffBracketCombo->setCurrentIndex(trk->getStaffBracket());
}

void
TrackParameterBox::slotSelectedTrackChanged()
{
    RG_DEBUG << "TrackParameterBox::slotSelectedTrackChanged()\n";
    Composition &comp = m_doc->getComposition();
    TrackId newTrack = comp.getSelectedTrack();
    if ( newTrack != m_selectedTrackId ) {
        m_presetLbl->setEnabled(true);
        m_selectedTrackId = newTrack;
        slotSelectedTrackNameChanged();
        slotUpdateControls( -1);
    }
}

void
TrackParameterBox::slotSelectedTrackNameChanged()
{
    RG_DEBUG << "TrackParameterBox::sotSelectedTrackNameChanged()\n";
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(m_selectedTrackId);
    QString m_trackName = trk->getLabel();
    if (m_trackName.isEmpty())
        m_trackName = i18n("<untitled>");
    else
        m_trackName.truncate(20);
    int trackNum = trk->getPosition() + 1;
    m_trackLabel->setText(i18n("[ Track %1 - %2 ]", trackNum, m_trackName));
}

void
TrackParameterBox::slotPlaybackDeviceChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotPlaybackDeviceChanged(" << index << ")\n";
    DeviceId devId;
    if (index == -1) {
        if (m_selectedTrackId < 0)
            return ;
        Composition &comp = m_doc->getComposition();
        Track *trk = comp.getTrackById(m_selectedTrackId);
        if (!trk)
            return ;
        Instrument *inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
        if (!inst)
            return ;
        devId = inst->getDevice()->getId();
        int pos = -1;
        IdsVector::const_iterator it;
        for ( it = m_playDeviceIds.begin(); it != m_playDeviceIds.end(); ++it) {
            pos++;
            if ((*it) == devId)
                break;
        }
        m_playDevice->setCurrentIndex(pos);
    } else {
        devId = m_playDeviceIds[index];
    }

    m_instrument->clear();
    m_instrument->addItems(m_instrumentNames[devId]);

    populateRecordingDeviceList();

    if (index != -1) {
        m_instrument->setCurrentIndex(0);
        slotInstrumentChanged(0);
    }
}

void
TrackParameterBox::slotInstrumentChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotInstrumentChanged(" << index << ")\n";
    DeviceId devId;
    Instrument *inst;
    if (index == -1) {
        Composition &comp = m_doc->getComposition();
        Track *trk = comp.getTrackById(comp.getSelectedTrack());
        if (!trk)
            return ;
        inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
        if (!inst)
            return ;
        devId = inst->getDevice()->getId();

        int pos = -1;
        IdsVector::const_iterator it;
        for ( it = m_instrumentIds[devId].begin(); it != m_instrumentIds[devId].end(); ++it ) {
            pos++;
            if ((*it) == trk->getInstrument())
                break;
        }
        m_instrument->setCurrentIndex(pos);
    } else {
        devId = m_playDeviceIds[m_playDevice->currentIndex()];
        // set the new selected instrument for the track
        int item = 0;
        std::map<DeviceId, IdsVector>::const_iterator it;
        for ( it = m_instrumentIds.begin(); it != m_instrumentIds.end(); ++it) {
            if ( (*it).first == devId )
                break;
            item += (*it).second.size();
        }
        item += index;
        RG_DEBUG << "TrackParameterBox::slotInstrumentChanged() item = " << item << "\n";
        emit instrumentSelected( m_selectedTrackId, item );
    }
}

void
TrackParameterBox::slotRecordingDeviceChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotRecordingDeviceChanged(" << index << ")" << endl;
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    if (!trk)
        return ;
    Instrument *inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
    if (!inst)
        return ;
    if (inst->getInstrumentType() == Instrument::Audio) {
        //Not implemented yet
    } else {
        trk->setMidiInputDevice(m_recDeviceIds[index]);
    }
}

void
TrackParameterBox::slotRecordingChannelChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotRecordingChannelChanged(" << index << ")" << endl;
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    if (!trk)
        return ;
    Instrument *inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
    if (!inst)
        return ;
    if (inst->getInstrumentType() == Instrument::Audio) {
        //Not implemented yet
    } else {
        trk->setMidiInputChannel(index - 1);
    }
}

void
TrackParameterBox::slotInstrumentLabelChanged(InstrumentId id, QString label)
{
    RG_DEBUG << "TrackParameterBox::slotInstrumentLabelChanged(" << id << ") = " << label << "\n";
    populatePlaybackDeviceList();
    slotUpdateControls( -1);
}

void
TrackParameterBox::showAdditionalControls(bool showThem)
{
    //    m_defaultsGroup->parentWidget()->setShown(showThem);
}

void
TrackParameterBox::slotClefChanged(int clef)
{
    RG_DEBUG << "TrackParameterBox::slotClefChanged(" << clef << ")" << endl;
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    trk->setClef(clef);
    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotTransposeChanged(int transpose)
{
    RG_DEBUG << "TrackParameterBox::slotTransposeChanged(" << transpose << ")" << endl;
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    trk->setTranspose(transpose);
    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotTransposeIndexChanged(int index)
{
    slotTransposeTextChanged(m_defTranspose->text(index));
}

void
TrackParameterBox::slotTransposeTextChanged(QString text)
{
    if (text.isEmpty())
        return ;
    int value = text.toInt();
    slotTransposeChanged(value);
}

void
TrackParameterBox::slotDocColoursChanged()
{
    RG_DEBUG << "TrackParameterBox::slotDocColoursChanged()" << endl;

    m_defColor->clear();
    m_colourList.clear();
    // Populate it from composition.m_segmentColourMap
    ColourMap temp = m_doc->getComposition().getSegmentColourMap();

    unsigned int i = 0;

    for (RCMap::const_iterator it = temp.begin(); it != temp.end(); ++it) {
        QString qtrunc(strtoqstr(it->second.second));
        QPixmap colour(15, 15);
        colour.fill(GUIPalette::convertColour(it->second.first));
        if (qtrunc == "") {
            m_defColor->addItem(colour, i18n("Default"), i);
        } else {
            // truncate name to 15 characters to avoid the combo forcing the
            // whole kit and kaboodle too wide
            if (qtrunc.length() > 15)
                qtrunc = qtrunc.left(12) + "...";
            m_defColor->addItem(colour, qtrunc, i);
        }
        m_colourList[it->first] = i; // maps colour number to menu index
        ++i;
    }

    m_addColourPos = i;
    m_defColor->addItem(i18n("Add New Color"), m_addColourPos);

    m_defColor->setCurrentIndex(0);
}

void
TrackParameterBox::slotColorChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotColorChanged(" << index << ")" << endl;

    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());

    trk->setColor(index);

    if (index == m_addColourPos) {
        ColourMap newMap = m_doc->getComposition().getSegmentColourMap();
        QColor newColour;
        bool ok = false;
        QString newName = KLineEditDlg::getText(i18n("New Color Name"), i18n("Enter new name"),
                                                i18n("New"), &ok);
        if ((ok == true) && (!newName.isEmpty())) {
            KColorDialog box(this, "", true);

            int result = box.getColor(newColour);

            if (result == KColorDialog::Accepted) {
                Colour newRColour = GUIPalette::convertColour(newColour);
                newMap.addItem(newRColour, qstrtostr(newName));
                slotDocColoursChanged();
            }
        }
        // Else we don't do anything as they either didn't give a name�
        // or didn't give a colour
    }
}

void
TrackParameterBox::slotHighestPressed()
{
    RG_DEBUG << "TrackParameterBox::slotHighestPressed()" << endl;

    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    if (!trk)
        return ;

    PitchPickerDialog dialog(0, m_highestPlayable, i18n("Highest playable note"));

    if (dialog.exec() == QDialog::Accepted) {
        m_highestPlayable = dialog.getPitch();
        updateHighLow();
    }

    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotLowestPressed()
{
    RG_DEBUG << "TrackParameterBox::slotLowestPressed()" << endl;

    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    if (!trk)
        return ;

    PitchPickerDialog dialog(0, m_lowestPlayable, i18n("Lowest playable note"));

    if (dialog.exec() == QDialog::Accepted) {
        m_lowestPlayable = dialog.getPitch();
        updateHighLow();
    }

    m_presetLbl->setEnabled(false);
}

void
TrackParameterBox::slotPresetPressed()
{
    RG_DEBUG << "TrackParameterBox::slotPresetPressed()" << endl;

    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(comp.getSelectedTrack());
    if (!trk)
        return ;

    PresetHandlerDialog dialog(this);

    try {
        if (dialog.exec() == QDialog::Accepted) {
            m_presetLbl->setText(dialog.getName());
            trk->setPresetLabel(dialog.getName());
            if (dialog.getConvertAllSegments()) {
            	SegmentSyncCommand* command = new SegmentSyncCommand(
            			comp.getSegments(), comp.getSelectedTrack(),
            			dialog.getTranspose(), dialog.getLowRange(), 
            			dialog.getHighRange(),
            			clefIndexToClef(dialog.getClef()));
                m_doc->getCommandHistory()->addCommand(command);
            }
            m_defClef->setCurrentIndex(dialog.getClef());
            m_defTranspose->setCurrentIndex(QString("%1").arg
                                           (dialog.getTranspose()), true);

            m_highestPlayable = dialog.getHighRange();
            m_lowestPlayable = dialog.getLowRange();
            updateHighLow();
            slotClefChanged(dialog.getClef());
            slotTransposeChanged(dialog.getTranspose());

            // the preceding slots will have set this disabled, so we
            // re-enable it until it is subsequently re-disabled by the
            // user overriding the preset, calling one of the above slots
            // in the normal course
            m_presetLbl->setEnabled(true);
        }
    } catch (Exception e) {
        //!!! This should be a more verbose error to pass along the
        // row/column of the corruption, but I can't be bothered to work
        // that out just at the moment.  Hopefully this code will never
        // execute anyway.
        KMessageBox::sorry(0, i18n("The instrument preset database is corrupt.  Check your installation."));
    }

}

void
TrackParameterBox::slotStaffSizeChanged(int index) 
{
    RG_DEBUG << "TrackParameterBox::sotStaffSizeChanged()" << endl;
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(m_selectedTrackId);

    trk->setStaffSize(index);
}


void
TrackParameterBox::slotStaffBracketChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::sotStaffBracketChanged()" << endl;
    Composition &comp = m_doc->getComposition();
    Track *trk = comp.getTrackById(m_selectedTrackId);

    trk->setStaffBracket(index);
}

QString
TrackParameterBox::getPreviousBox(RosegardenParameterArea::Arrangement arrangement) const
{
    if (arrangement == RosegardenParameterArea::CLASSIC_STYLE) {
        return i18n("Segment");
    } else {
        return "";
    }
}

}
#include "TrackParameterBox.moc"
