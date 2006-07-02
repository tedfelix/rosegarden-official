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
#include <klocale.h>
#include <kcombobox.h>

#include "Device.h"
#include "MidiDevice.h"
#include "Studio.h"
#include "Instrument.h"
#include "AudioPluginInstance.h"
#include "PluginIdentifier.h"

#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "trackparameterbox.h"
#include "rosedebug.h"

TrackParameterBox::TrackParameterBox( RosegardenGUIDoc *doc,
                                      QWidget *parent)
    : RosegardenParameterBox(i18n("Track"), parent),
      m_doc(doc)
{
    QFont font(m_font);
    QFont title_font(m_font);
    QFontMetrics metrics(font);
    int minwidth22 = metrics.width("1234567890123456789012");
    int minwidth25 = metrics.width("1234567890123456789012345");
    setFont(m_font);
    title_font.setBold(true);
    
    QGridLayout *mainLayout = new QGridLayout(this, 9, 4, 4, 2);
    
    // row 0 - track label
    //
    //mainLayout->setRowSpacing(0, 2);
    m_trackLabel = new QLabel(i18n("<untitled>"), this);
    mainLayout->addMultiCellWidget(m_trackLabel, 0, 0, 0, 2, AlignCenter);

    // row 1 - playback group title
    //
    QLabel *plyHeader = new QLabel(i18n("Playback parameters"), this);
    plyHeader->setFont(title_font);
    mainLayout->addMultiCellWidget(plyHeader, 1, 1, 0, 2, AlignLeft);

    // row 2 - playback device
    //
    mainLayout->addWidget(new QLabel(i18n("Device"), this), 2, 0, AlignLeft);
    m_playDevice = new KComboBox(this);
    m_playDevice->setMinimumWidth(minwidth25);
    mainLayout->addMultiCellWidget(m_playDevice, 2, 2, 1, 2, AlignRight);
    
    // row 3 - playback instrument
    //
    mainLayout->addMultiCellWidget(new QLabel(i18n("Instrument"), this), 3, 3, 0, 1, AlignLeft);
    m_instrument = new KComboBox(this);
    m_instrument->setSizeLimit( 16 );
    m_instrument->setMinimumWidth(minwidth22);
    mainLayout->addWidget(m_instrument, 3, 2, AlignRight);

    // row 4 - group separator
    //
    QFrame *separator = new QFrame( this );
    separator->setFrameShape( QFrame::HLine );
    separator->setLineWidth( 1 );
    separator->setMidLineWidth( 2 );
    separator->setFrameShadow( QFrame::Sunken );
    separator->setMinimumHeight( 4 );
    mainLayout->addMultiCellWidget( separator, 4, 4, 0, 2 );
    
    // row 5 - recording group title
    //
    QLabel *recHeader = new QLabel(i18n("Recording filters"), this);
    recHeader->setFont(title_font);
    mainLayout->addMultiCellWidget(recHeader, 5, 5, 0, 2, AlignLeft);
    
    // row 6 - recording device
    //
    mainLayout->addWidget(new QLabel(i18n("Device"), this), 6, 0, AlignLeft);
    m_recDevice = new KComboBox(this);
    m_recDevice->setMinimumWidth(minwidth25);
    mainLayout->addMultiCellWidget(m_recDevice, 6, 6, 1, 2, AlignRight);
    
    // row 7 - recording channel
    //
    mainLayout->addMultiCellWidget(new QLabel(i18n("Channel"), this), 7, 7, 0, 1, AlignLeft);
    m_recChannel = new KComboBox(this);
    m_recChannel->setSizeLimit( 17 );
    mainLayout->addWidget(m_recChannel, 7, 2, AlignRight);
    
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
    populateRecordingDeviceList();
    slotUpdateControls(-1);
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
    m_recDevice->clear();
    m_recDeviceIds.clear();
    m_recChannel->clear();

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
    
    m_recDevice->setEnabled(false);
    m_recChannel->setEnabled(false);
    m_recDevice->setCurrentItem(-1);
    m_recChannel->setCurrentItem(-1);
}


void 
TrackParameterBox::slotUpdateControls(int /*dummy*/)
{
    RG_DEBUG << "TrackParameterBox::slotUpdateControls()\n";        
    slotPlaybackDeviceChanged(-1);
    slotInstrumentChanged(-1);
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
    QString trkName = trk->getLabel();
    if (trkName.isEmpty()) 
       trkName = i18n("<untitled>");
    else
       trkName.truncate(20);
    int trkNum = m_selectedTrackId + 1;
    m_trackLabel->setText(i18n("[ Track#%1 - %2 ]").arg(trkNum).arg(trkName));
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
        Rosegarden::Track *trk = comp.getTrackById(comp.getSelectedTrack());
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
TrackParameterBox::slotInstrumentLabelChanged(Rosegarden::InstrumentId id, QString label)
{
    RG_DEBUG << "TrackParameterBox::slotInstrumentLabelChanged(" << id << ") = " << label << "\n";        
    populatePlaybackDeviceList();
    slotUpdateControls(-1);
}

#include "trackparameterbox.moc"
