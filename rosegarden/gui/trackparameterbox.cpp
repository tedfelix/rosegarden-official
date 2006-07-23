// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2006
        Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <qlayout.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qspinbox.h>
#include <qpixmap.h>
#include <qtooltip.h>

#include <klocale.h>
#include <kcombobox.h>
#include <kcolordialog.h>
#include <klineeditdlg.h>
#include <kconfig.h>

#include "Device.h"
#include "MidiDevice.h"
#include "Studio.h"
#include "Instrument.h"
#include "AudioPluginInstance.h"
#include "PluginIdentifier.h"
#include "colours.h"
#include "colourwidgets.h"
#include "NotationTypes.h"
#include "dialogs.h"

#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "trackparameterbox.h"
#include "rosedebug.h"
#include "studiowidgets.h"
#include "constants.h"

TrackParameterBox::TrackParameterBox( RosegardenGUIDoc *doc,
                                      QWidget *parent)
    : RosegardenParameterBox(i18n("Track"), parent),
      m_doc(doc),
      m_highestPlayable(127),
      m_lowestPlayable(0)
{
    QFont font(m_font);
    QFont title_font(m_font);
    QFontMetrics metrics(font);
    int minwidth11 = metrics.width("12345678901");
    int minwidth22 = metrics.width("1234567890123456789012");
    int minwidth25 = metrics.width("1234567890123456789012345");
    int minwidthHigh = metrics.width(i18n("High: ----"));
    setFont(m_font);
    title_font.setBold(true);
    
    // Size of the layout grid:
    //  number of rows = 17
    //  number of columns = 4
    QGridLayout *mainLayout = new QGridLayout(this, 17, 4, 4, 2);

    int row = 0;
    
    // track label
    //
    //mainLayout->setRowSpacing(0, 2);
    m_trackLabel = new QLabel(i18n("<untitled>"), this);
    mainLayout->addMultiCellWidget(m_trackLabel, row, row, 0, 2, AlignCenter);

    // playback group title
    //
    row++;
    QLabel *plyHeader = new QLabel(i18n("Playback parameters"), this);
    plyHeader->setFont(title_font);
    mainLayout->addMultiCellWidget(plyHeader, row, row, 0, 2, AlignLeft);

    // playback device
    //
    row++;
    mainLayout->addWidget(new QLabel(i18n("Device"), this), row, 0, AlignLeft);
    m_playDevice = new KComboBox(this);
    m_playDevice->setMinimumWidth(minwidth25);
    mainLayout->addMultiCellWidget(m_playDevice, row, row, 1, 2, AlignRight);
    
    // playback instrument
    //
    row++;
    mainLayout->addMultiCellWidget(new QLabel(i18n("Instrument"), this), row, row, 0, 1, AlignLeft);
    m_instrument = new KComboBox(this);
    m_instrument->setSizeLimit( 16 );
    m_instrument->setMinimumWidth(minwidth22);
    mainLayout->addWidget(m_instrument, row,  2, AlignRight);

    // group separator 1
    //
    row++;
    QFrame *separator1 = new QFrame( this );
    separator1->setFrameShape( QFrame::HLine );
    separator1->setLineWidth( 1 );
    separator1->setMidLineWidth( 2 );
    separator1->setFrameShadow( QFrame::Sunken );
    separator1->setMinimumHeight( 4 );
    mainLayout->addMultiCellWidget( separator1, row, row, 0, 2 );
    
    // recording group title
    //
    row++;
    QLabel *recHeader = new QLabel(i18n("Recording filters"), this);
    recHeader->setFont(title_font);
    mainLayout->addMultiCellWidget(recHeader, row, row, 0, 2, AlignLeft);
    
    // recording device
    //
    row++;
    mainLayout->addWidget(new QLabel(i18n("Device"), this), row, 0, AlignLeft);
    m_recDevice = new KComboBox(this);
    m_recDevice->setMinimumWidth(minwidth25);
    mainLayout->addMultiCellWidget(m_recDevice, row, row, 1, 2, AlignRight);
    
    // recording channel
    //
    row++;
    mainLayout->addMultiCellWidget(new QLabel(i18n("Channel"), this), row, row, 0, 1, AlignLeft);
    m_recChannel = new KComboBox(this);
    m_recChannel->setSizeLimit( 17 );
    m_recChannel->setMinimumWidth(minwidth11);
    mainLayout->addWidget(m_recChannel, row, 2, AlignRight);
    
    // group separator 2
    //
    row++;
    m_separator2 = new QFrame( this );
    m_separator2->setFrameShape( QFrame::HLine );
    m_separator2->setLineWidth( 1 );
    m_separator2->setMidLineWidth( 2 );
    m_separator2->setFrameShadow( QFrame::Sunken );
    m_separator2->setMinimumHeight( 4 ); 
    mainLayout->addMultiCellWidget( m_separator2, row, row, 0, 2 );

    // default segment segment parameters group title
    //
    row++;
    m_segHeader = new QLabel(i18n("Create segments with:"), this);
    m_segHeader->setFont(title_font);
    mainLayout->addMultiCellWidget( m_segHeader, row, row, 0, 2, AlignLeft);

    // preset picker
    //
    row++;

    /* explanation of intention:
     *
     * [Load Preset]  Label showing the preset that was loaded
     *
     * Loading a preset will have too many levels of choices to fit in the
     * TPB, so it will spin off into a separate dialog that will inject its
     * results into the appropriate TPB controls
     *
     * This label is going to be hugely too wide.  I've picked the worst
     * example so you have some idea what we have to accommodate with this
     * somehow.  Maybe we could limit it to n characters and have the whole
     * text revealed as a tooltip or something?  I'm open to any idea how to
     * make it all less obnoxious.  I'll just get the guts working.
     */

    m_presetButton = new QPushButton(i18n("Load Preset"), this);
    mainLayout->addMultiCellWidget(m_presetButton, row, row, 0, 1, AlignLeft);

    int longestPresetName = metrics.width("Electronic organ (manual) (treble)");
    m_presetLbl = new QLabel(i18n("None"), this);
    m_presetLbl->setMinimumWidth(longestPresetName);
    mainLayout->addMultiCellWidget(m_presetLbl, row, row, 2, 4, AlignRight);
//    m_presetButton->hide();
//    m_presetLbl->hide();
    
    // default clef
    //
    row++;
    m_clefLbl = new QLabel(i18n("Clef"), this);
    mainLayout->addMultiCellWidget(m_clefLbl, row, row, 0, 1, AlignLeft);
    m_defClef = new KComboBox(this);
    m_defClef->setMinimumWidth(minwidth11);
    m_defClef->insertItem(i18n("treble"));
    m_defClef->insertItem(i18n("bass"));
    m_defClef->insertItem(i18n("alto"));
    m_defClef->insertItem(i18n("tenor"));
    mainLayout->addWidget(m_defClef, row, 2, AlignRight);

    // default transpose
    //
    row++;
    m_transpLbl = new QLabel(i18n("Transpose"), this);
    mainLayout->addMultiCellWidget(m_transpLbl, row, row, 0, 1, AlignLeft);
    m_defTranspose = new QSpinBox(this);
    m_defTranspose->setMinValue(-24);
    m_defTranspose->setMaxValue(24);
    m_defTranspose->setValue(0);
    m_defTranspose->setButtonSymbols(QSpinBox::PlusMinus);
    mainLayout->addWidget(m_defTranspose, row, 2, AlignRight);

    // default color
    //
    row++;
    m_colorLbl = new QLabel(i18n("Color"), this);
    mainLayout->addWidget(m_colorLbl, row, 0, AlignLeft);
    m_defColor = new KComboBox(false, this);
    mainLayout->addMultiCellWidget(m_defColor, row, row, 1, 2, AlignRight);

    // populate combo from doc colors
    slotDocColoursChanged();

    // highest/lowest playable note
    //
    row++;
    m_rangeLbl = new QLabel(i18n("Range"), this);
    mainLayout->addWidget(m_rangeLbl, row, 0, AlignLeft);

    m_highButton = new QPushButton(i18n("High: ---"), this);
    QToolTip::add(m_highButton, i18n("Choose the highest suggested playable note, using a staff"));
    mainLayout->addWidget(m_highButton, row, 1);

    m_lowButton = new QPushButton(i18n("Low: ----"), this);
    QToolTip::add(m_lowButton, i18n("Choose the lowest suggested playable note, using a staff"));
    mainLayout->addWidget(m_lowButton, row, 2);

    updateHighLow();
    
    // Configure the empty final row to accomodate any extra vertical space.
    //
    mainLayout->setRowStretch(mainLayout->numRows()-1, 1);

    // Configure the empty final column to accomodate any extra horizontal
    // space.
    mainLayout->setColStretch(mainLayout->numCols()-1, 1);
    
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
    
    connect( m_defTranspose, SIGNAL(valueChanged(int)),
             this, SLOT(slotTransposeChanged(int)));

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
}

TrackParameterBox::~TrackParameterBox() {}

void 
TrackParameterBox::setDocument( RosegardenGUIDoc *doc )
{
    RG_DEBUG << "TrackParameterBox::setDocument\n";
    m_doc = doc;
    populateDeviceLists();
}

void 
TrackParameterBox::populateDeviceLists()
{
    RG_DEBUG << "TrackParameterBox::populateDeviceLists()\n";
    populatePlaybackDeviceList();
    //populateRecordingDeviceList();
    slotUpdateControls(-1);
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

    Rosegarden::Studio &studio = m_doc->getStudio();

    // Get the list
    Rosegarden::InstrumentList list = studio.getPresentationInstruments();
    Rosegarden::InstrumentList::iterator it;
    int currentDevId = -1;

    for (it = list.begin(); it != list.end(); it++) {

        if (! (*it)) continue; // sanity check

        //QString iname(strtoqstr((*it)->getPresentationName()));
        QString iname(strtoqstr((*it)->getName()));
        QString pname(strtoqstr((*it)->getProgramName()));
        Rosegarden::Device *device = (*it)->getDevice();
        Rosegarden::DeviceId devId = device->getId();

        if ((*it)->getType() == Rosegarden::Instrument::SoftSynth) {
            iname.replace("Synth plugin ", "");
            pname = "";
            Rosegarden::AudioPluginInstance *plugin = (*it)->getPlugin
                (Rosegarden::Instrument::SYNTH_PLUGIN_POSITION);
            if (plugin) {
                pname = strtoqstr(plugin->getProgram());
                QString identifier = strtoqstr(plugin->getIdentifier());
                if (identifier != "") {
                    QString type, soName, label;
                    Rosegarden::PluginIdentifier::parseIdentifier
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

        if (devId != (Rosegarden::DeviceId)(currentDevId)) {
            currentDevId = int(devId);
            QString deviceName = strtoqstr(device->getName());
            m_playDevice->insertItem(deviceName);
            m_playDeviceIds.push_back(currentDevId);
        }

        if (pname != "") iname += " (" + pname + ")";
        m_instrumentIds[currentDevId].push_back((*it)->getId());
        m_instrumentNames[currentDevId].append(iname);
        
    }
    
    m_playDevice->setCurrentItem(-1);
    m_instrument->setCurrentItem(-1);
}

void 
TrackParameterBox::populateRecordingDeviceList()
{
    RG_DEBUG << "TrackParameterBox::populateRecordingDeviceList()\n";

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk = comp.getTrackById(m_selectedTrackId);
    if (!trk) return;
    
    Rosegarden::Instrument *inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
    if (!inst) return;
    
    if (m_lastInstrumentType != (char)inst->getInstrumentType()) {
        m_lastInstrumentType = (char)inst->getInstrumentType();

        m_recDevice->clear();
        m_recDeviceIds.clear();
        m_recChannel->clear();
    
        if (inst->getInstrumentType() == Rosegarden::Instrument::Audio) {
        
            m_recDeviceIds.push_back(Rosegarden::Device::NO_DEVICE);
            m_recDevice->insertItem(i18n("Audio"));
            m_recChannel->insertItem(i18n("Audio"));

            m_recDevice->setEnabled(false);
            m_recChannel->setEnabled(false);

        } else { // InstrumentType::Midi and InstrumentType::SoftSynth

            m_recDeviceIds.push_back(Rosegarden::Device::ALL_DEVICES);
            m_recDevice->insertItem(i18n("All"));
            
            Rosegarden::DeviceList *devices = m_doc->getStudio().getDevices();
            Rosegarden::DeviceListConstIterator it;
            for (it = devices->begin(); it != devices->end(); it++)
            {
                Rosegarden::MidiDevice *dev =
                    dynamic_cast<Rosegarden::MidiDevice*>(*it);
                if (dev) {
                    if (dev->getDirection() == Rosegarden::MidiDevice::Record
                        && dev->isRecording()) 
                    {
                        QString connection = strtoqstr(dev->getConnection());
                        // remove trailing "(duplex)", "(read only)", "(write only)" etc
                        connection.replace(QRegExp("\\s*\\([^)0-9]+\\)\\s*$"), "");
                        m_recDevice->insertItem(connection);
                        m_recDeviceIds.push_back(dev->getId());
                    } 
                }
            }
            
            m_recChannel->insertItem(i18n("All"));
            for(int i=1; i<17; ++i) {
               m_recChannel->insertItem(QString::number(i));
            }

            m_recDevice->setEnabled(true);
            m_recChannel->setEnabled(true);
        }
    }
    
    if (inst->getInstrumentType() == Rosegarden::Instrument::Audio) {
        m_recDevice->setCurrentItem(0);
        m_recChannel->setCurrentItem(0);
    } else {
        m_recDevice->setCurrentItem(0);
        m_recChannel->setCurrentItem((int)trk->getMidiInputChannel()+1);
        for(unsigned int i = 0; i < m_recDeviceIds.size(); ++i) {
            if (m_recDeviceIds[i] == trk->getMidiInputDevice()) {
               m_recDevice->setCurrentItem(i);
               break;
            }
        }
    }
}

void
TrackParameterBox::updateHighLow()
{
    // Key of C major and NoAccidental means any "black key" notes will be
    // written as sharps.
    Rosegarden::Accidental accidental = Rosegarden::Accidentals::NoAccidental;
    Rosegarden::Key key = Rosegarden::Key("C major");

    Rosegarden::Pitch highest(m_highestPlayable, accidental);
    Rosegarden::Pitch lowest(m_lowestPlayable, accidental);

    KConfig *config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    int base = config->readNumEntry("midipitchoctave", -2);

    m_highButton->setText(QString("High: %1%2").arg(highest.getNoteName(key)).arg(highest.getOctave(base)));
    m_lowButton->setText(QString("Low: %1%2").arg(lowest.getNoteName(key)).arg(lowest.getOctave(base)));
}

void 
TrackParameterBox::slotUpdateControls(int /*dummy*/)
{
    RG_DEBUG << "TrackParameterBox::slotUpdateControls()\n";        
    slotPlaybackDeviceChanged(-1);
    slotInstrumentChanged(-1);

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk = comp.getTrackById(m_selectedTrackId);
    if (!trk) return;

    m_defClef->setCurrentItem(trk->getClef());
    m_defTranspose->setValue(trk->getTranspose());
    m_defColor->setCurrentItem(trk->getColor());
    m_highestPlayable = trk->getHighestPlayable();
    m_lowestPlayable = trk->getLowestPlayable();
}

void 
TrackParameterBox::slotSelectedTrackChanged()
{
    RG_DEBUG << "TrackParameterBox::slotSelectedTrackChanged()\n";        
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::TrackId newTrack = comp.getSelectedTrack();
    if ( newTrack != m_selectedTrackId ) {
        m_selectedTrackId = newTrack;
        slotSelectedTrackNameChanged();
        slotUpdateControls(-1);
    }
}

void 
TrackParameterBox::slotSelectedTrackNameChanged()
{
    RG_DEBUG << "TrackParameterBox::sotSelectedTrackNameChanged()\n";
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk = comp.getTrackById(m_selectedTrackId);
    QString m_trackName = trk->getLabel();
    if (m_trackName.isEmpty()) 
       m_trackName = i18n("<untitled>");
    else
       m_trackName.truncate(20);
    int m_trackNum = m_selectedTrackId + 1;
    m_trackLabel->setText(i18n("[ Track#%1 - %2 ]").arg(m_trackNum).arg(m_trackName));
}

void 
TrackParameterBox::slotPlaybackDeviceChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotPlaybackDeviceChanged(" << index << ")\n";               
    Rosegarden::DeviceId devId;
    if (index == -1) {
        Rosegarden::Composition &comp = m_doc->getComposition();
        Rosegarden::Track *trk = comp.getTrackById(m_selectedTrackId);
        if (!trk) return;
        Rosegarden::Instrument *inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
        if (!inst) return;
        devId = inst->getDevice()->getId();
        int pos = -1;
        IdsVector::const_iterator it;
        for ( it = m_playDeviceIds.begin(); it != m_playDeviceIds.end(); ++it) {
            pos++;
            if ((*it) == devId) break;
        }
        m_playDevice->setCurrentItem(pos);
    } else {
        devId = m_playDeviceIds[index];
    }
    
    m_instrument->clear();
    m_instrument->insertStringList(m_instrumentNames[devId]);

    populateRecordingDeviceList();
    
    if (index != -1) {
        m_instrument->setCurrentItem(0);
        slotInstrumentChanged(0);
    }    
}

void 
TrackParameterBox::slotInstrumentChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotInstrumentChanged(" << index << ")\n";        
    Rosegarden::DeviceId devId;
    Rosegarden::Instrument *inst;
    if (index == -1) {
        Rosegarden::Composition &comp = m_doc->getComposition();
        Rosegarden::Track *trk  = comp.getTrackById(comp.getSelectedTrack());
        if (!trk) return;
        inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
        if (!inst) return;
        devId = inst->getDevice()->getId();
        
        int pos = -1;
        IdsVector::const_iterator it;
        for ( it = m_instrumentIds[devId].begin(); it != m_instrumentIds[devId].end(); ++it ) {
            pos++;
            if ((*it) == trk->getInstrument()) break;
         }
         m_instrument->setCurrentItem(pos);
    } else {
        devId = m_playDeviceIds[m_playDevice->currentItem()];
        // set the new selected instrument for the track
        int item = 0;
        std::map<Rosegarden::DeviceId, IdsVector>::const_iterator it;
        for( it = m_instrumentIds.begin(); it != m_instrumentIds.end(); ++it) {
           if ( (*it).first == devId ) break;
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
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk  = comp.getTrackById(comp.getSelectedTrack());
    if (!trk) return;
    Rosegarden::Instrument *inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
    if (!inst) return;
    if (inst->getInstrumentType() == Rosegarden::Instrument::Audio) {    
        //Not implemented yet
    } else {
        trk->setMidiInputDevice(m_recDeviceIds[index]);
    }
}

void 
TrackParameterBox::slotRecordingChannelChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotRecordingChannelChanged(" << index << ")" << endl;        
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk  = comp.getTrackById(comp.getSelectedTrack());
    if (!trk) return;
    Rosegarden::Instrument *inst = m_doc->getStudio().getInstrumentById(trk->getInstrument());
    if (!inst) return;
    if (inst->getInstrumentType() == Rosegarden::Instrument::Audio) {    
        //Not implemented yet
    } else {
        trk->setMidiInputChannel(index - 1);
    }
}

void 
TrackParameterBox::slotInstrumentLabelChanged(Rosegarden::InstrumentId id, QString label)
{
    RG_DEBUG << "TrackParameterBox::slotInstrumentLabelChanged(" << id << ") = " << label << "\n";        
    populatePlaybackDeviceList();
    slotUpdateControls(-1);
}

void 
TrackParameterBox::showAdditionalControls(bool showThem)
{
    m_separator2->setShown(showThem);
    m_segHeader->setShown(showThem);
    m_presetLbl->setShown(showThem);
    m_presetButton->setShown(showThem);
    m_clefLbl->setShown(showThem);
    m_defClef->setShown(showThem);
    m_transpLbl->setShown(showThem);
    m_defTranspose->setShown(showThem);
    m_colorLbl->setShown(showThem);
    m_defColor->setShown(showThem);
    m_rangeLbl->setShown(showThem);
    m_highButton->setShown(showThem);
    m_lowButton->setShown(showThem);
}



void
TrackParameterBox::slotClefChanged(int clef)
{
    RG_DEBUG << "TrackParameterBox::slotClefChanged(" << clef << ")" << endl;        
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk  = comp.getTrackById(comp.getSelectedTrack());
    trk->setClef(clef);
}

void
TrackParameterBox::slotTransposeChanged(int transpose)
{
    RG_DEBUG << "TrackParameterBox::slotTransposeChanged(" << transpose << ")" << endl;        
    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk  = comp.getTrackById(comp.getSelectedTrack());
    trk->setTranspose(transpose);
}   

// code copied/adapted from segmentparameterbox.cpp on 16 July 2006
void
TrackParameterBox::slotDocColoursChanged()
{
    RG_DEBUG << "TrackParameterBox::slotDocColoursChanged()" << endl;
	
    m_defColor->clear();
    m_colourList.clear();
    // Populate it from composition.m_segmentColourMap
    Rosegarden::ColourMap temp = m_doc->getComposition().getSegmentColourMap();

    unsigned int i=0;

    for (Rosegarden::RCMap::const_iterator it=temp.begin(); it != temp.end(); ++it) {
        QString qtrunc(strtoqstr(it->second.second));
        QPixmap colour(15,15);
        colour.fill(Rosegarden::GUIPalette::convertColour(it->second.first));
        if (qtrunc == "") {
            m_defColor->insertItem(colour, i18n("Default"), i);
	} else {
	    // truncate name to 15 characters to avoid the combo forcing the
	    // whole kit and kaboodle too wide
	    if (qtrunc.length() > 15) qtrunc = qtrunc.left(12) + "...";
            m_defColor->insertItem(colour, qtrunc, i);
	}
        m_colourList[it->first] = i; // maps colour number to menu index
        ++i;
    }

    m_addColourPos = i;
    m_defColor->insertItem(i18n("Add New Color"), m_addColourPos);

    m_defColor->setCurrentItem(0);
}

// code copied/adapted from segmentparameterbox.cpp on 16 July 2006
void
TrackParameterBox::slotColorChanged(int index)
{
    RG_DEBUG << "TrackParameterBox::slotColorChanged(" << index << ")" << endl;        

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk  = comp.getTrackById(comp.getSelectedTrack());
    trk->setColor(index);

    if (index == m_addColourPos) {
        Rosegarden::ColourMap newMap = m_doc->getComposition().getSegmentColourMap();
        QColor newColour;
        bool ok = false;
        QString newName = KLineEditDlg::getText(i18n("New Color Name"), i18n("Enter new name"),
                                                i18n("New"), &ok);
        if ((ok == true) && (!newName.isEmpty())) {
            KColorDialog box(this, "", true);

            int result = box.getColor(newColour);

            if (result == KColorDialog::Accepted) {
                Rosegarden::Colour newRColour = Rosegarden::GUIPalette::convertColour(newColour);
                newMap.addItem(newRColour, qstrtostr(newName));
                slotDocColoursChanged();
            }
        }
        // Else we don't do anything as they either didn't give a name·
        // or didn't give a colour
    }
}

void
TrackParameterBox::slotHighestPressed()
{
    RG_DEBUG << "TrackParameterBox::slotHighestPressed()" << endl;

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk  = comp.getTrackById(comp.getSelectedTrack());
    if (!trk) return;

    PitchPickerDialog dialog(0, m_highestPlayable, i18n("Highest playable note"));

    if (dialog.exec() == QDialog::Accepted) {
        m_highestPlayable = dialog.getPitch();
	updateHighLow();

	trk->setHighestPlayable(m_highestPlayable);
    }
}

void
TrackParameterBox::slotLowestPressed()
{
    RG_DEBUG << "TrackParameterBox::slotLowestPressed()" << endl;

    Rosegarden::Composition &comp = m_doc->getComposition();
    Rosegarden::Track *trk  = comp.getTrackById(comp.getSelectedTrack());
    if (!trk) return;

    PitchPickerDialog dialog(0, m_lowestPlayable, i18n("Lowest playable note"));

    if (dialog.exec() == QDialog::Accepted) {
        m_lowestPlayable = dialog.getPitch();
	updateHighLow();

	trk->setLowestPlayable(m_lowestPlayable);
    }
}
#include "trackparameterbox.moc"
