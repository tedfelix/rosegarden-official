// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <iostream>
#include <string>
#include <vector>

#include <unistd.h> // sleep
#include <zlib.h>

// include files for Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>
#include <qregexp.h>

// include files for KDE
#include <klocale.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <kapp.h>
#include <kconfig.h>
#include <kstddirs.h>

// application specific includes
#include "Event.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "NotationTypes.h"
#include "MidiTypes.h"
#include "XmlExportable.h"
#include "segmentcommands.h"

#include "MappedDevice.h"
#include "MappedInstrument.h"
#include "MappedRealTime.h"
#include "MidiDevice.h"
#include "AudioDevice.h"
#include "Studio.h"
#include "Midi.h"

#include "constants.h"
#include "editviewbase.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "rosegardenguiview.h"
#include "rosexmlhandler.h"
#include "xmlstorableevent.h"
#include "rosegardendcop.h"
#include "widgets.h"
#include "trackeditor.h"
#include "studiocontrol.h"
#include "sequencemanager.h"
#include "kstartuplogo.h"
#include "AudioPluginInstance.h"


using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Event;
using Rosegarden::PitchBend;
using Rosegarden::Controller;
using Rosegarden::SystemExclusive;
using Rosegarden::ChannelPressure;
using Rosegarden::KeyPressure;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::timeT;

using namespace Rosegarden::BaseProperties;


RosegardenGUIDoc::RosegardenGUIDoc(QWidget *parent,
                                   Rosegarden::AudioPluginManager *pluginManager,
				   bool skipAutoload,
                                   const char *name)
    : QObject(parent, name),
      m_modified(false),
      m_autoSaved(false),
      m_recordSegment(0), m_endOfLastRecordedNote(0),
      m_commandHistory(new MultiViewCommandHistory()),
      m_pluginManager(pluginManager),
      m_autoSavePeriod(0),
      m_beingDestroyed(false)
{
    syncDevices();

    m_viewList.setAutoDelete(false);
    m_editViewList.setAutoDelete(false);

    connect(m_commandHistory, SIGNAL(commandExecuted(KCommand *)),
	    this, SLOT(slotDocumentModified()));

    connect(m_commandHistory, SIGNAL(documentRestored()),
	    this, SLOT(slotDocumentRestored()));

    // autoload a new document
    if (!skipAutoload) performAutoload();

    // now set it up as a "new document"
    newDocument();

}

RosegardenGUIDoc::~RosegardenGUIDoc()
{
    RG_DEBUG << "~RosegardenGUIDoc()\n";
    m_beingDestroyed = true;

    deleteEditViews();
    delete m_commandHistory; // must be deleted before the Composition is
}

unsigned int
RosegardenGUIDoc::getAutoSavePeriod() const
{
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    return config->readUnsignedNumEntry("autosaveinterval", 60);
}

void RosegardenGUIDoc::attachView(RosegardenGUIView *view)
{
    m_viewList.append(view);
}

void RosegardenGUIDoc::detachView(RosegardenGUIView *view)
{
    m_viewList.remove(view);
}

void RosegardenGUIDoc::attachEditView(EditViewBase *view)
{
    m_editViewList.append(view);
}

void RosegardenGUIDoc::detachEditView(EditViewBase *view)
{
    // auto-deletion is disabled, as
    // the editview detaches itself when being deleted
    m_editViewList.remove(view);
}

void RosegardenGUIDoc::deleteEditViews()
{
    // enabled auto-deletion : edit views will be deleted
    m_editViewList.setAutoDelete(true);
    m_editViewList.clear();
}

void RosegardenGUIDoc::setAbsFilePath(const QString &filename)
{
    m_absFilePath=filename;
}

const QString &RosegardenGUIDoc::getAbsFilePath() const
{
    return m_absFilePath;
}

void RosegardenGUIDoc::setTitle(const QString &_t)
{
    m_title=_t;
}

const QString& RosegardenGUIDoc::getTitle() const
{
    return m_title;
}

void RosegardenGUIDoc::slotUpdateAllViews(RosegardenGUIView *sender)
{
    RosegardenGUIView *w;

    for(w=m_viewList.first(); w!=0; w=m_viewList.next()) {
            if(w!=sender)
                w->repaint();
    }
}

void RosegardenGUIDoc::setModified(bool m)
{
    m_modified = m;
    RG_DEBUG << "RosegardenGUIDoc::setModified(" << m << ")\n";
}

void RosegardenGUIDoc::clearModifiedStatus()
{
    setModified(false);
    setAutoSaved(true);
    emit documentModified(false);
}

void RosegardenGUIDoc::slotDocumentModified()
{
    RG_DEBUG << "RosegardenGUIDoc::slotDocumentModified()" << endl;
    setModified(true);
    setAutoSaved(false);
    emit documentModified(true);
}

void RosegardenGUIDoc::slotDocumentRestored()
{
    RG_DEBUG << "RosegardenGUIDoc::slotDocumentRestored()\n";
    setModified(false);
}

QString RosegardenGUIDoc::getAutoSaveFileName()
{
    QString filename = getAbsFilePath();
    if (filename.isEmpty())
        filename = QDir::currentDirPath() + "/" + getTitle();

    QString autoSaveFileName = kapp->tempSaveName(filename);

    return autoSaveFileName;
}

void RosegardenGUIDoc::slotAutoSave()
{
//     RG_DEBUG << "RosegardenGUIDoc::slotAutoSave()\n" << endl;

    if (isAutoSaved() || !isModified()) return;
      
    QString autoSaveFileName = getAutoSaveFileName();

    RG_DEBUG << "RosegardenGUIDoc::slotAutoSave() - doc modified - saving '"
             << getAbsFilePath() << "' as "
             << autoSaveFileName << endl;

    saveDocument(autoSaveFileName, 0, true);
}

bool RosegardenGUIDoc::isRegularDotRGFile()
{
    return getAbsFilePath().right(3).lower() == ".rg";
}

bool RosegardenGUIDoc::saveIfModified()
{
    RG_DEBUG << "RosegardenGUIDoc::saveIfModified()" << endl;
    bool completed=true;

    if (isModified()) {
        RosegardenGUIApp *win=(RosegardenGUIApp *) parent();
        int want_save = KMessageBox::warningYesNoCancel(win,
                                                        i18n("The current file has been modified.\n"
                                                             "Do you want to save it?"),
                                                        i18n("Warning"));
        RG_DEBUG << "want_save = " << want_save << endl;

        switch(want_save) {

        case KMessageBox::Yes:

            if (!isRegularDotRGFile()) {

                RG_DEBUG << "RosegardenGUIDoc::saveIfModified() : new or imported file\n";
                win->fileSaveAs();

            } else {

                RG_DEBUG << "RosegardenGUIDoc::saveIfModified() : regular file\n";
                saveDocument(getAbsFilePath());

            }

            completed=true;
            break;

        case KMessageBox::No:
            setModified(false);

            // delete the autosave file so it won't annoy
            // the user when reloading the file.
            QFile::remove(getAutoSaveFileName());
            completed=true;
            break;	

        case KMessageBox::Cancel:
            completed=false;
            break;

        default:
            completed=false;
            break;
        }
    }

    return completed;
}

void RosegardenGUIDoc::newDocument()
{
    setModified(false);
    setAbsFilePath(QString::null);
    setTitle(i18n("Untitled"));
    m_commandHistory->clear();
}

void RosegardenGUIDoc::performAutoload()
{
    QString autoloadFile =
        KGlobal::dirs()->findResource("appdata", "autoload.rg"); 

    QFileInfo autoloadFileInfo(autoloadFile);

    if (!autoloadFileInfo.isReadable())
    {
        RG_DEBUG << "RosegardenGUIDoc::performAutoload - "
                 << "can't find autoload file - defaulting" << endl;
        return;
    }

    openDocument(autoloadFile);

}


bool RosegardenGUIDoc::openDocument(const QString& filename,
				    bool permanent,
                                    const char* /*format*/ /*=0*/)
{
    RG_DEBUG << "RosegardenGUIDoc::openDocument("
                         << filename << ")" << endl;
    
    if (!filename || filename.isEmpty())
        return false;

    newDocument();

    QFileInfo fileInfo(filename);
    setTitle(fileInfo.fileName());

    // Check if file readable with fileInfo ?
    if (!fileInfo.isReadable() || fileInfo.isDir()) {
	KStartupLogo::hideIfStillThere();
        QString msg(i18n("Can't open file '%1'").arg(filename));
        KMessageBox::sorry(0, msg);
        return false;
    }

    RosegardenProgressDialog progressDlg(i18n("Reading file..."),
                                         100,
                                         (QWidget*)parent());
    progressDlg.setMinimumDuration(500);
    progressDlg.setAutoReset(true); // we're re-using it for the preview generation
    setAbsFilePath(fileInfo.absFilePath());	

    QString errMsg;
    QString fileContents;
    bool cancelled = false;

    bool okay = readFromFile(filename, fileContents);
    if (!okay) errMsg = i18n("Not a Rosegarden-4 file");
    else {

        // parse xml file
	okay = xmlParse(fileContents, errMsg, &progressDlg,
			permanent, cancelled);

    }

    if (!okay) {
	KStartupLogo::hideIfStillThere();
        QString msg(i18n("Error when parsing file '%1': \"%2\"")
                    .arg(filename)
                    .arg(errMsg));

	CurrentProgressDialog::freeze();
        KMessageBox::sorry(0, msg);
	CurrentProgressDialog::thaw();

        return false;

    } else if (cancelled) {
        newDocument();
        return false;
    }

    RG_DEBUG << "RosegardenGUIDoc::openDocument() end - "
             << "m_composition : " << &m_composition
             << " - m_composition->getNbSegments() : "
             << m_composition.getNbSegments()
             << " - m_composition->getDuration() : "
             << m_composition.getDuration() << endl;

    // Ensure a minimum of 64 tracks
    //
//     unsigned int nbTracks = m_composition.getNbTracks();
//     Rosegarden::TrackId maxTrackId = m_composition.getMaxTrackId();
//     Rosegarden::InstrumentId instBase = Rosegarden::MidiInstrumentBase;

//     for(unsigned int i = nbTracks; i < MinNbOfTracks; ++i) {

//         Rosegarden::Track *track;

//         track = new Rosegarden::Track(maxTrackId + 1,          // id
//                                       (i + instBase) % 16,     // instrument
//                                       i,                       // position
//                                       "untitled", 
//                                       false);                  // mute

//         m_composition.addTrack(track);
//         ++maxTrackId;
//     }
    
    // We might need a progress dialog when we generate previews,
    // reuse the previous one
    progressDlg.setLabel(i18n("Generating audio previews..."));

    connect(&m_audioFileManager, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));
    try
    {
        // generate any audio previews after loading the files
        m_audioFileManager.generatePreviews();
    }
    catch(std::string e)
    {
	KStartupLogo::hideIfStillThere();
	CurrentProgressDialog::freeze();
        KMessageBox::error(0, strtoqstr(e));
	CurrentProgressDialog::thaw();
    }

    if (isSequencerRunning())
    {
        // Initialise the whole studio - faders, plugins etc.
        //
        initialiseStudio();

        // Initialise the MIDI controllers (reaches through to MIDI devices
        // to set them up)
        //
        initialiseControllers();
    }

    return true;
}

void RosegardenGUIDoc::clearStudio()
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    if (!kapp->dcopClient()->
            send(ROSEGARDEN_SEQUENCER_APP_NAME,
                 ROSEGARDEN_SEQUENCER_IFACE_NAME,
                 "clearStudio()",
                 data))
    {
        RG_DEBUG << "failed to clear studio" << endl;
        return;
    }

    RG_DEBUG << "cleared studio\n";
}

void RosegardenGUIDoc::initialiseStudio()
{
    RG_DEBUG << "RosegardenGUIDoc::initialiseStudio - "
             << "clearing down and initialising" << endl;

    clearStudio();

    Rosegarden::InstrumentList list = m_studio.getAllInstruments();
    Rosegarden::InstrumentList::iterator it = list.begin();
    int count = 0;

    for (; it != list.end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Instrument::Audio)
        {
            Rosegarden::MappedObjectId mappedId =
                Rosegarden::StudioControl::createStudioObject(
                        Rosegarden::MappedObject::AudioFader);

            // Set the object id against the instrument
            //
            (*it)->setMappedId(mappedId);

            /*
            cout << "SETTING MAPPED OBJECT ID = " << mappedId
                 << " - on Instrument " << (*it)->getId() << endl;
                 */


            // Set the instrument id against this object
            //
            Rosegarden::StudioControl::setStudioObjectProperty(mappedId,
                Rosegarden::MappedObject::Instrument,
                Rosegarden::MappedObjectValue((*it)->getId()));

            // Set the level
            //
            Rosegarden::StudioControl::setStudioObjectProperty(mappedId,
                Rosegarden::MappedAudioFader::FaderLevel,
                Rosegarden::MappedObjectValue((*it)->getVelocity()));

            // Set the record level
            //
            Rosegarden::StudioControl::setStudioObjectProperty(mappedId,
                Rosegarden::MappedAudioFader::FaderRecordLevel,
                Rosegarden::MappedObjectValue((*it)->getRecordLevel()));

            // Set the number of channels
            //
            Rosegarden::StudioControl::setStudioObjectProperty(mappedId,
                Rosegarden::MappedAudioObject::Channels,
                Rosegarden::MappedObjectValue((*it)->getAudioChannels()));

            // Set the pan - 0 based
            //
            Rosegarden::StudioControl::setStudioObjectProperty(mappedId,
                Rosegarden::MappedAudioFader::Pan,
                Rosegarden::MappedObjectValue(float((*it)->getPan())) - 100.0);

            count++;

#ifdef HAVE_LADSPA

            // Initialise all the plugins for this Instrument
            //
            Rosegarden::AudioPluginInstance *plugin;
            int pluginNo = 0;

            while ((plugin = (*it)->getPlugin(pluginNo++)))
            {
                if (plugin->isAssigned())
                {
                    //RG_DEBUG << "PLUGIN " << pluginNo - 1
                             //<< " ASSIGNED on INS "
                             //<< (*it)->getId() << endl;

                    // Create the plugin at the sequencer Studio
                    //
                    Rosegarden::MappedObjectId pluginMappedId =
                        Rosegarden::StudioControl::createStudioObject(
                                Rosegarden::MappedObject::LADSPAPlugin);

                    // Create the back linkage from the instance to the
                    // studio id
                    //
                    plugin->setMappedId(pluginMappedId);

                    //RG_DEBUG << "CREATING PLUGIN ID = " 
                               //<< pluginMappedId << endl;

                    // Set the position
                    Rosegarden::StudioControl::setStudioObjectProperty
                        (pluginMappedId,
                         Rosegarden::MappedObject::Position,
                         Rosegarden::MappedObjectValue(plugin->getPosition()));

                    // Set the id of this instrument on the plugin
                    //
                    Rosegarden::StudioControl::setStudioObjectProperty
                        (pluginMappedId,
                         Rosegarden::MappedObject::Instrument,
                         (*it)->getId());

                    // Set the plugin type id - this will set it up ready
                    // for port settings
                    Rosegarden::StudioControl::setStudioObjectProperty
                        (pluginMappedId,
                         Rosegarden::MappedLADSPAPlugin::UniqueId,
                         Rosegarden::MappedObjectValue(plugin->getId()));

                    // Set the bypass
                    //
                    Rosegarden::StudioControl::setStudioObjectProperty
                        (pluginMappedId,
                         Rosegarden::MappedLADSPAPlugin::Bypassed,
                         Rosegarden::MappedObjectValue(plugin->isBypassed()));

                    Rosegarden::PortInstanceIterator portIt;

                    for (portIt = plugin->begin();
                         portIt != plugin->end(); ++portIt)
                    {
                        Rosegarden::StudioControl::setStudioPluginPort
                            (pluginMappedId,
                             (*portIt)->id,
                             (*portIt)->value);
                        //RG_DEBUG << "SETTING PORT " << (*portIt)->id << " to "
                                 //<< (*portIt)->value << endl;
                    }
                }
            }
#endif // HAVE_LADSPA

        }
    }

    RG_DEBUG << "RosegardenGUIDoc::initialiseStudio - "
             << "initialised studio including " << count
             << " audio faders" << endl;

}


bool RosegardenGUIDoc::saveDocument(const QString& filename,
                                    const char* format,
				    bool autosave)
{
    RG_DEBUG << "RosegardenGUIDoc::saveDocument("
                         << filename << ")\n";

    QString outText;
    QTextStream outStream(&outText, IO_WriteOnly);

    // output XML header
    //
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<!DOCTYPE rosegarden-data>\n"
               << "<rosegarden-data>\n";

    static const QString deviceExport("deviceExport");

    if (format && deviceExport == format)
    {
        // Send out the studio - a self contained command
        //
        outStream << strtoqstr(m_studio.toXmlString()) << endl << endl;
    
        // close the top-level XML tag
        //
        outStream << "</rosegarden-data>\n";

        bool okay = writeToFile(filename, outText);
        if (!okay) return false;
    
        RG_DEBUG << endl << "RosegardenGUIDoc::deviceExport() finished\n";
        return true;
    }

    RosegardenProgressDialog *progressDlg = 0;
    KProgress *progress = 0;

    if (!autosave) {

	progressDlg = new RosegardenProgressDialog(i18n("Saving file..."),
						   100,
						   (QWidget*)parent());
	progress = progressDlg->progressBar();

	progressDlg->setMinimumDuration(500);
	progressDlg->setAutoReset(true);

    } else {

	progress = ((RosegardenGUIApp *)parent())->getProgressBar();
    }

    // Send out Composition (this includes Tracks, Instruments, Tempo
    // and Time Signature changes and any other sub-objects)
    //
    outStream << QString(strtoqstr(getComposition().toXmlString()))
              << endl << endl;

    outStream << QString(strtoqstr(getAudioFileManager().toXmlString()))
              << endl << endl;

    outStream << QString(strtoqstr(getConfiguration().toXmlString()))
              << endl << endl;

    QString time;

    long totalEvents = 0, count = 0;
    for (Composition::iterator segitr = m_composition.begin();
         segitr != m_composition.end(); ++segitr) {
	totalEvents += (*segitr)->size();
    }

    // output all elements
    //
    // Iterate on segments
    for (Composition::iterator segitr = m_composition.begin();
         segitr != m_composition.end(); ++segitr) {

	Segment *segment = *segitr;

        //--------------------------
        outStream << QString("<segment track=\"%1\" start=\"%2\" ") 
                            .arg(segment->getTrack())
                            .arg(segment->getStartTime());

        outStream << "label=\"" <<
	    strtoqstr(Rosegarden::XmlExportable::encode(segment->getLabel()));

        if (segment->isRepeating()) {
            outStream << "\" repeat=\"true";
	}

	if (segment->getTranspose() != 0) {
	    outStream << "\" transpose=\"" << segment->getTranspose();
	}

	if (segment->getDelay() != 0) {
	    outStream << "\" delay=\"" << segment->getDelay();
	}

	if (segment->getRealTimeDelay() != Rosegarden::RealTime(0, 0)) {
	    outStream << "\" rtdelaysec=\"" << segment->getRealTimeDelay().sec 
		      << "\" rtdelayusec=\"" << segment->getRealTimeDelay().usec;
	}

	const Rosegarden::timeT *endMarker = segment->getRawEndMarkerTime();
	if (endMarker) {
	    outStream << "\" endmarker=\"" << *endMarker;
	}

        if (segment->getType() == Rosegarden::Segment::Audio) {
            outStream << "\" type=\"audio\" "
                      << "file=\""
                      << segment->getAudioFileId()
                      << "\">\n";

            // convert out - should do this as XmlExportable really
            // once all this code is centralised
            //
            time.sprintf("%ld.%06ld", segment->getAudioStartTime().sec,
                                      segment->getAudioStartTime().usec);

            outStream << "   <begin index=\""
                      << time
                      << "\"/>\n";

            time.sprintf("%ld.%06ld", segment->getAudioEndTime().sec,
                                      segment->getAudioEndTime().usec);

            outStream << "   <end index=\""
                      << time
                      << "\"/>\n";
        }
        else // Internal type
        {
            outStream << "\">\n";

	    bool inChord = false;
	    timeT chordStart = 0, chordDuration = 0;
	    timeT expectedTime = segment->getStartTime();

            for (Segment::iterator i = segment->begin();
                 i != segment->end(); ++i) {

	        timeT absTime = (*i)->getAbsoluteTime();

                Segment::iterator nextEl = i;
                ++nextEl;

                if (nextEl != segment->end() &&
                    (*nextEl)->getAbsoluteTime() == absTime &&
		    (*i)->getDuration() != 0 &&
		    !inChord) {
		    outStream << "<chord>" << endl;
		    inChord = true;
		    chordStart = absTime;
		    chordDuration = 0;
	        }

	        if (inChord && (*i)->getDuration() > 0)
		    if (chordDuration == 0 || (*i)->getDuration() < chordDuration)
		        chordDuration = (*i)->getDuration();
    
	        outStream << '\t'
			  << strtoqstr((*i)->toXmlString(expectedTime)) << endl;

	        if (nextEl != segment->end() &&
		    (*nextEl)->getAbsoluteTime() != absTime &&
		    inChord) {
		    outStream << "</chord>\n";
		    inChord = false;
		    expectedTime = chordStart + chordDuration;
	        } else if (inChord) {
		    expectedTime = absTime;
	        } else {
		    expectedTime = absTime + (*i)->getDuration();
	        }

		if (++count % 500 == 0) {
		    progress->setValue(count * 100 / totalEvents);
		}
            }

	    if (inChord) {
	        outStream << "</chord>\n";
	    }
        }

        outStream << "</segment>\n"; //-------------------------

    }

    // Put a break in the file
    //
    outStream << endl << endl;

    // Send out the studio - a self contained command
    //
    outStream << QString(strtoqstr(m_studio.toXmlString())) << endl << endl;
    
    // close the top-level XML tag
    //
    outStream << "</rosegarden-data>\n";

    bool okay = writeToFile(filename, outText);
    if (!okay) return false;

    RG_DEBUG << endl << "RosegardenGUIDoc::saveDocument() finished\n";

    if (!autosave) {
        emit documentModified(false);
	setModified(false);
	m_commandHistory->documentSaved();
	delete progressDlg;
    } else {
	progress->setProgress(0);
    }

    setAutoSaved(true);

    return true;
}

bool RosegardenGUIDoc::isSequencerRunning()
{
    RosegardenGUIApp* parentApp = dynamic_cast<RosegardenGUIApp*>(parent());
    if (!parentApp) {
        RG_DEBUG << "RosegardenGUIDoc::isSequencerRunning() : parentApp == 0\n";
        return false;
    }
    
    return parentApp->isSequencerRunning();
}

bool
RosegardenGUIDoc::xmlParse(QString &fileContents, QString &errMsg,
                           RosegardenProgressDialog *progress,
			   bool permanent,
                           bool &cancelled)
{
    cancelled = false;

    unsigned int elementCount = 0;
    for (size_t i = 0; i < fileContents.length() - 1; ++i) {
	if (fileContents[i] == '<' && fileContents[i+1] != '/') {
	    ++elementCount;
	}
    }

    RoseXmlHandler handler(this, elementCount, permanent);

    if (progress) {
	connect(&handler, SIGNAL(setProgress(int)),
		progress->progressBar(), SLOT(setValue(int)));
	connect(&handler, SIGNAL(incrementProgress(int)),
		progress->progressBar(), SLOT(advance(int)));
	connect(progress, SIGNAL(cancelClicked()),
		&handler, SLOT(slotCancel()));
    }
    
    QXmlInputSource source;
    source.setData(fileContents);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    START_TIMING;
    bool ok = reader.parse(source);
    PRINT_ELAPSED("RosegardenGUIDoc::xmlParse (reader.parse())");

    if (!ok) {

        if (handler.isCancelled()) {
            RG_DEBUG << "File Loading Cancelled\n";
	    KStartupLogo::hideIfStillThere();
            KMessageBox::information(0, i18n("File Loading Cancelled"));
            cancelled = true;
            return true;
        } else
            errMsg = handler.errorString();

    } else if (handler.isDeprecated()) {

        QString msg(i18n("This file contains one or more old element types that are now deprecated.\nSupport for these elements may disappear in future versions of Rosegarden.\nWe recommend you re-save this file from this version of Rosegarden,\nto ensure that it can still be re-loaded in future versions."));
        
	KStartupLogo::hideIfStillThere();
	CurrentProgressDialog::freeze();
        KMessageBox::information(0, msg);
	CurrentProgressDialog::thaw();
    }

    return ok;
}


bool
RosegardenGUIDoc::writeToFile(const QString &file, const QString &text)
{
    std::string stext = qstrtostr(text);
    const char *ctext = stext.c_str();
    size_t csize = strlen(ctext);

    gzFile fd = gzopen(qstrtostr(file).c_str(), "wb");
    if (!fd) return false;

    int actual = gzwrite(fd, (void *)ctext, csize);
    gzclose(fd);

    return ((size_t)actual == csize);
}

bool
RosegardenGUIDoc::readFromFile(const QString &file, QString &text)
{
    text = "";
    gzFile fd = gzopen(QFile::encodeName(file), "rb");
    if (!fd) return false;

    static char buffer[1000];

    while (gzgets(fd, buffer, 1000)) {

        // Update gui
        //
        kapp->processEvents();

	text.append(strtoqstr(std::string(buffer)));
	if (gzeof(fd)) {
	    gzclose(fd);
	    return true;
	}
    }
	
    // gzgets only returns false on error
    return false;
}    


// Take a MappedComposition from the Sequencer and turn it into an
// Event-rich, Composition-inserted, mouthwateringly-ripe Segment.
//
void
RosegardenGUIDoc::insertRecordedMidi(const Rosegarden::MappedComposition &mC,
                                     TransportStatus status)
{
    // Just create a new record Segment if we don't have one already.
    // Make sure we don't recreate the m_recordSegment if it's already
    // freed.
    //
    if (m_recordSegment == 0 && status == RECORDING_MIDI) {
        m_recordSegment = new Segment();
        m_recordSegment->setTrack(m_composition.getRecordTrack());
        m_recordSegment->setStartTime(m_composition.getPosition());

        // Set an appropriate segment label
        //
        Rosegarden::Track *track =
            m_composition.getTrackById(m_composition.getRecordTrack());
        std::string label = "";

        if (track) {
            if (track->getLabel() == "") {
                Rosegarden::Instrument *instr =
                    m_studio.getInstrumentById(track->getInstrument());

                if (instr) {
                    label = m_studio.getSegmentName(instr->getId());
                }
            } else {
                label = track->getLabel();
	    }

	    label = qstrtostr(i18n("%1 (recorded)").arg(strtoqstr(label)));
        }

        m_recordSegment->setLabel(label);
    }


    if (mC.size() > 0) { 
            Rosegarden::MappedComposition::iterator i;
            Rosegarden::Event *rEvent = 0;
            timeT duration, absTime;

            // process all the incoming MappedEvents
            //
            for (i = mC.begin(); i != mC.end(); ++i) {
                    absTime = m_composition.
                        getElapsedTimeForRealTime((*i)->getEventTime());

                    /* This is incorrect, unless the tempo at absTime happens to
                       be the same as the tempo at zero and there are no tempo
                       changes within the given duration after either zero or
                       absTime

                       duration = m_composition.getElapsedTimeForRealTime((*i)->getDuration());
                    */
                    duration = m_composition.
                        getElapsedTimeForRealTime((*i)->getEventTime() +
                                                  (*i)->getDuration())
                        - absTime;

                    rEvent = 0;

                    switch((*i)->getType()) {
                        case Rosegarden::MappedEvent::MidiNote:
                            if ((*i)->getDuration() < Rosegarden::RealTime(0, 0))
                                duration = -1;
		    
                            rEvent = new Event(Rosegarden::Note::EventType,
                                               absTime,
                                               duration);

                            rEvent->set<Int>(PITCH, (*i)->getPitch());
                            rEvent->set<Int>(VELOCITY, (*i)->getVelocity());

                            break;

                        case Rosegarden::MappedEvent::MidiPitchBend:
                            rEvent = Rosegarden::PitchBend
                                ((*i)->getData1(), (*i)->getData2()).getAsEvent(absTime);
                            break;

                        case Rosegarden::MappedEvent::MidiController:
                            rEvent = Rosegarden::Controller
                                ((*i)->getData1(), (*i)->getData2()).getAsEvent(absTime);
                            break;

                        case Rosegarden::MappedEvent::MidiProgramChange:
                            RG_DEBUG << "RosegardenGUIDoc::insertRecordedMidi()"
                                     << " - got Program Change (unsupported)"
                                     << endl;
                            break;

                        case Rosegarden::MappedEvent::MidiKeyPressure:
                            rEvent = Rosegarden::KeyPressure
                                ((*i)->getData1(), (*i)->getData2()).getAsEvent(absTime);
                            break;

                        case Rosegarden::MappedEvent::MidiChannelPressure:
                            rEvent = Rosegarden::ChannelPressure
                                ((*i)->getData1()).getAsEvent(absTime);
                            break;

                        case Rosegarden::MappedEvent::MidiSystemExclusive:
                            rEvent = Rosegarden::SystemExclusive
                                ((*i)->getDataBlock()).getAsEvent(absTime);
                            break;
		    
                        case Rosegarden::MappedEvent::MidiNoteOneShot:
                            RG_DEBUG << "RosegardenGUIDoc::insertRecordedMidi() - "
                                         << "GOT UNEXPECTED MappedEvent::MidiNoteOneShot"
                                         << endl;
                            break;
		
                            // Audio control signals - ignore these
                        case Rosegarden::MappedEvent::Audio:
                        case Rosegarden::MappedEvent::AudioCancel:
                        case Rosegarden::MappedEvent::AudioLevel:
                        case Rosegarden::MappedEvent::AudioStopped:
                        case Rosegarden::MappedEvent::AudioGeneratePreview:
                        case Rosegarden::MappedEvent::SystemUpdateInstruments:
                            break;

                        default:
                            RG_DEBUG << "RosegardenGUIDoc::insertRecordedMidi() - "
                                         << "GOT UNSUPPORTED MAPPED EVENT"
                                         << endl;
                            break;
                        }

                    // sanity check
                    //
                    if (rEvent == 0)
                        continue;

                    // Set the start index and then insert into the Composition
                    // (if we haven't before)
                    //
                    if (m_recordSegment->size() == 0 &&
                        !m_composition.contains(m_recordSegment))
                        {
                            m_endOfLastRecordedNote = m_composition.getPosition();
                            m_composition.addSegment(m_recordSegment);
                        }

                    // Now insert the new event
                    //
                    m_recordSegment->insert(rEvent);

                    // Update our counter
                    //
                    m_endOfLastRecordedNote = absTime + duration;

                }
        }

    // Only update gui if we're still recording - otherwise we
    // can get a recording SegmentItem hanging around.
    //
    if (status == RECORDING_MIDI) {
        // update this segment on the GUI
        RosegardenGUIView *w;
        for(w=m_viewList.first(); w!=0; w=m_viewList.next()) {
            w->showRecordingSegmentItem(m_recordSegment);
        }
    }
}


// Tidy up a recorded Segment when we've finished recording
//
void
RosegardenGUIDoc::stopRecordingMidi()
{
    // If we've created nothing then do nothing with it
    //
    if (m_recordSegment == 0)
        return;

    // otherwise do something with it
    //
    RosegardenGUIView *w;
    for(w=m_viewList.first(); w!=0; w=m_viewList.next()) {
        w->deleteRecordingSegmentItem();
    }

    // Roll out any NOTE ON/NOTE OFFs we've had to insert.
    // These are indicated by a negative time on the first
    // event.  We can only do this after we've recorded the
    // whole.
    //
    convertToSinglePoint(m_recordSegment);
        
    // the record segment will have already been added to the
    // composition if there was anything in it; otherwise we
    // don't need to do so
    if (m_recordSegment->getComposition() != 0) {

	// Quantize for notation only -- doesn't affect performance timings
	m_commandHistory->addCommand
	    (new EventQuantizeCommand
	     (*m_recordSegment,
	      m_recordSegment->getStartTime(),
	      m_recordSegment->getEndTime(),
	      "Notation Options",
	      Rosegarden::Quantizer::NotationPrefix,
	      true));

	m_commandHistory->addCommand
	    (new SegmentRecordCommand
	     (m_recordSegment));
    }

    m_recordSegment = 0;
    m_endOfLastRecordedNote = 0;

    slotUpdateAllViews(0);
}


// Make the sequencer aware of the samples we have in the
// current Composition so it can prepare them for playing
//
void
RosegardenGUIDoc::prepareAudio()
{
    if (!isSequencerRunning()) return;

    QCString replyType;
    QByteArray replyData;

    // Clear down the sequencer AudioFilePlayer object
    //
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "clearAllAudioFiles()", data))
    {
        RG_DEBUG << "prepareAudio() - "
                     << "couldn't delete all audio files from sequencer"
                     << endl;
        return;
    }
    
    for (Rosegarden::AudioFileManagerIterator it = m_audioFileManager.begin();
         it != m_audioFileManager.end();
         it++)
    {
        QByteArray data;
        QDataStream streamOut(data, IO_WriteOnly);

        // We have to pass the filename as a QString
        //
        streamOut << QString(strtoqstr((*it)->getFilename()));
        streamOut << (*it)->getId();

        if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                      ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                      "addAudioFile(QString, int)", data, replyType, replyData))
        {
            RG_DEBUG << "prepareAudio() - couldn't add audio file "
                         << (*it)->getFilename()
                         << endl;
            return;
        }
        else
        {
            QDataStream streamIn(replyData, IO_ReadOnly);
            int result;
            streamIn >> result;
            if (!result)
            {
                RG_DEBUG << "prepareAudio() - failed to add file \"" 
                             << (*it)->getFilename() << "\"" << endl;
            }
        }
    }
}

void
RosegardenGUIDoc::setPointerPosition(Rosegarden::timeT t)
{
    m_composition.setPosition(t);
    emit pointerPositionChanged(t);
}

void
RosegardenGUIDoc::setPlayPosition(Rosegarden::timeT t)
{
    emit playPositionChanged(t);
}

void
RosegardenGUIDoc::setLoop(Rosegarden::timeT t0, Rosegarden::timeT t1)
{
    m_composition.setLoopStart(t0);
    m_composition.setLoopEnd(t1);
    emit loopChanged(t0, t1);
}

void
RosegardenGUIDoc::syncDevices()
{
    // Start up the sequencer
    //
    while (isSequencerRunning() &&
	   !kapp->dcopClient()->
            isApplicationRegistered(QCString(ROSEGARDEN_SEQUENCER_APP_NAME)))
    {
        RG_DEBUG << "RosegardenGUIDoc::syncDevices - "
                     << "waiting for Sequencer to come up" << endl;

        kapp->processEvents(1000);
        sleep(1); // 1s
    }

    if (!isSequencerRunning())
        return;

    QByteArray data;
    QByteArray replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    // Get number of devices the sequencer has found
    //
    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getDevices()",
                                  data, replyType, replyData, true))
    {
        RG_DEBUG << "RosegardenGUIDoc::syncDevices - "
                     << "can't get number of devices" << endl;
        return;
    }

    unsigned int devices = 0;

    if (replyType == "unsigned int")
    {
        QDataStream reply(replyData, IO_ReadOnly);
        reply >> devices;
    }
    else
    {
        RG_DEBUG << "RosegardenGUIDoc::syncDevices - "
                     << "got unknown returntype from getDevices()" << endl;
        return;
    }

    RG_DEBUG << "RosegardenGUIDoc::syncDevices - devices = "
                 << devices << endl;

    for (unsigned int i = 0; i < devices; i++)
    {
        
        RG_DEBUG << "RosegardenGUIDoc::syncDevices - i = "
                     << i << endl;

        getMappedDevice(i);
    }

    RG_DEBUG << "RosegardenGUIDoc::syncDevices - "
                 << "Sequencer alive - Instruments synced" << endl;


    //!!! need to force an update on the instrument parameter box if
    //necessary -- how? -- and the instrument dropdown
}


void
RosegardenGUIDoc::getMappedDevice(Rosegarden::DeviceId id)
{
    QByteArray data;
    QByteArray replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);

    arg << (unsigned int)id;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getMappedDevice(unsigned int)",
                                  data, replyType, replyData, false))
    {
        RG_DEBUG << "RosegardenGUIDoc::getMappedDevice() - "
                     << "can't call Sequencer" << endl;
        return;
    }

    Rosegarden::MappedDevice *mD = new Rosegarden::MappedDevice();
    QDataStream reply(replyData, IO_ReadOnly);

    if (replyType == "Rosegarden::MappedDevice")
        // unfurl
        reply >> mD;
    else
        return;

    // See if we've got this device already
    //
    Rosegarden::Device *device = m_studio.getDevice(id);

    if (mD->getId() == Rosegarden::Device::NO_DEVICE)
    {
	if (device) m_studio.removeDevice(id);
	delete mD;
	return;
    }

    if (mD->size() == 0)
    {
	// no instruments is OK for a record device
	if (mD->getType() != Rosegarden::Device::Midi ||
	    mD->getDirection() != Rosegarden::MidiDevice::Record) {

	    RG_DEBUG << "RosegardenGUIDoc::getMappedDevice() - "
			 << "no instruments found" << endl;
	    if (device) m_studio.removeDevice(id);
	    delete mD;
	    return;
	}
    }

    bool hadDeviceAlready = (device != 0);

    if (!hadDeviceAlready) 
    {
        if (mD->getType() == Rosegarden::Device::Midi)
        {
	    device =
		new Rosegarden::MidiDevice
		(id,
		 mD->getName(),
		 mD->getDirection());

            m_studio.addDevice(device);

            RG_DEBUG  << "RosegardenGUIDoc::getMappedDevice - "
                          << "adding MIDI Device \""
                          << device->getName() << "\" id = " << id
			  << endl;
        }
        else if (mD->getType() == Rosegarden::Device::Audio)
        {
            device = new Rosegarden::AudioDevice(id, mD->getName());
            m_studio.addDevice(device);

            RG_DEBUG  << "RosegardenGUIDoc::getMappedDevice - "
                          << "adding audio Device \""
                          << device->getName() << "\" id = " << id << endl;
        }
        else
        {
            RG_DEBUG  << "RosegardenGUIDoc::getMappedDevice - "
                          << "unknown device - \"" << mD->getName()
                          << "\" (type = "
                          << mD->getType() << ")\n";
            return;
        }
    }

    if (hadDeviceAlready) {
	// direction might have changed
	if (mD->getType() == Rosegarden::Device::Midi) {
	    Rosegarden::MidiDevice *midid =
		dynamic_cast<Rosegarden::MidiDevice *>(device);
	    if (midid) midid->setDirection(mD->getDirection());
	}
    }

    std::string connection(mD->getConnection());
    RG_DEBUG << "RosegardenGUIDoc::getMappedDevice - got device on connection \"" << connection << "\"" << endl;
    device->setConnection(connection);

    Rosegarden::Instrument *instrument;
    Rosegarden::MappedDeviceIterator it;

    Rosegarden::InstrumentList existingInstrs(device->getAllInstruments());

    for (it = mD->begin(); it != mD->end(); it++)
    {
	Rosegarden::InstrumentId instrumentId = (*it)->getId();

	bool haveInstrument = false;
	for (Rosegarden::InstrumentList::iterator iit = existingInstrs.begin();
	     iit != existingInstrs.end(); ++iit) {

	    if ((*iit)->getId() == instrumentId) {
		haveInstrument = true;
		break;
	    }
	}

	if (!haveInstrument) {
	    RG_DEBUG << "RosegardenGUIDoc::getMappedDevice: new instr " << (*it)->getId() << endl;
	    instrument = new Rosegarden::Instrument((*it)->getId(),
						    (*it)->getType(),
						    (*it)->getName(),
						    (*it)->getChannel(),
						    device);
	    device->addInstrument(instrument);
	}
    }

    delete mD;
}

// Convert a single-point recorded Segment to a proper Segment
// clear of all the NOTE OFF event types.
//
void
RosegardenGUIDoc::convertToSinglePoint(Rosegarden::Segment *segment)
{
    if (segment == 0) return;

    Rosegarden::Segment::iterator it, oIt;
    Rosegarden::Event *event;

    std::vector<Event *> toInsert;
    std::vector<Segment::iterator> toErase;

    for (it = segment->begin(); it != segment->end(); it++)
    {
        if ((*it)->isa(Rosegarden::Note::EventType) &&
            (*it)->getDuration() == -1)
        {
            // start from here
            oIt = it;

            // no, sorry, next one
            oIt++;

            for (; oIt != segment->end(); oIt++)
            {
                if ((*oIt)->isa(Rosegarden::Note::EventType) &&
                    ((*oIt)->get<Int>(Rosegarden::BaseProperties::PITCH) ==
                     (*it)->get<Int>(Rosegarden::BaseProperties::PITCH)) &&
                     (*oIt)->getDuration() >= 0)
                {
                    event = new Event(Rosegarden::Note::EventType,
                                      (*it)->getAbsoluteTime(),
                                      (*oIt)->getDuration());

                    event->set<Int>(PITCH,
                        (*it)->get<Int>(Rosegarden::BaseProperties::PITCH));

                    event->set<Int>(VELOCITY,
                        (*it)->get<Int>(Rosegarden::BaseProperties::VELOCITY));

                    // replace NOTE ON event with one of proper duration
                    //
                    toErase.push_back(it);
                    toInsert.push_back(event);

                    // remove the NOTE OFF helper
                    //
                    segment->eraseSingle(*oIt);

                    break;
                }
            }
        }
    }

    for (unsigned int i = 0; i < toInsert.size(); ++i) {
        segment->insert(toInsert[i]);
    }

    for (unsigned int i = 0; i < toErase.size(); ++i) {
        segment->erase(toErase[i]);
    }
}

std::string
RosegardenGUIDoc::createNewAudioFile()
{
    return m_audioFileManager.createRecordingAudioFile();
}


void
RosegardenGUIDoc::insertRecordedAudio(const Rosegarden::RealTime &time,
                                      TransportStatus status)
{
    if (status != RECORDING_AUDIO)
        return;

    // Just create a new record Segment if we don't have one already.
    // Make sure we don't recreate the m_recordSegment if it's already
    // freed.
    //
    if (m_recordSegment == 0)
    {
        m_recordSegment = new Segment(Rosegarden::Segment::Audio);
        m_recordSegment->setTrack(m_composition.getRecordTrack());
        m_recordSegment->setStartTime(m_composition.getPosition());
        m_recordSegment->setAudioStartTime(Rosegarden::RealTime(0, 0));

        // Set an appropriate segment label
        //
        Rosegarden::Track *track =
            m_composition.getTrackById(m_composition.getRecordTrack());
        std::string label = "";

        if (track)
        {
            if (track->getLabel() == "")
            {
                Rosegarden::Instrument *instr =
                    m_studio.getInstrumentById(track->getInstrument());

                if (instr)
                {
                    label = instr->getName() + std::string(" ");
                }
            }
            else
                label = track->getLabel() + std::string(" ");

            label += std::string("(recorded audio)");
        }
        m_recordSegment->setLabel(label);
        
        // new audio file will have been pushed to the back of the
        // AudioFileManager queue - fetch it out and get the 
        // AudioFileId
        //
        Rosegarden::AudioFile *audioFile =
            m_audioFileManager.getLastAudioFile();

        if (audioFile)
        {
            m_recordSegment->setAudioFileId(audioFile->getId());
        }
        else
        {
            RG_DEBUG << "RosegardenGUIDoc::insertRecordedAudio - "
                         << "no audio file" << endl;
        }

        // always insert straight away for audio
        m_composition.addSegment(m_recordSegment);
    }

    // Leaving this in causes a massive memory leak at the gui
    // when recording audio:
    //
    // [ 730011 ] "memory leak after recording?"
    //
    // m_recordSegment->fillWithRests
    //   (m_composition.getElapsedTimeForRealTime(time));
    //
    // Leaving it here for informational purposes.  RWB (20030523)

    // update this segment on the GUI
    RosegardenGUIView *w;
    for(w=m_viewList.first(); w!=0; w=m_viewList.next()) {
        w->showRecordingSegmentItem(m_recordSegment);
    }
}

// Tidy up the recording SegmentItem and add the recorded Segment
// to the Composition.  The second part of this process is in 
// finalizeAudioFile - where the preview is generated and complete
// finalization of the audio file add is performed.
//
void
RosegardenGUIDoc::stopRecordingAudio()
{
    // If we've created nothing then do nothing with it
    //
    if (m_recordSegment == 0)
        return;

    RosegardenGUIView *w;
    for(w=m_viewList.first(); w!=0; w=m_viewList.next()) {
        w->deleteRecordingSegmentItem();
    }

    // set the audio end time
    //
    m_recordSegment->setAudioEndTime(
        m_composition.getRealTimeDifference(m_recordSegment->getStartTime(),
                                            m_composition.getPosition()));

    // now add the Segment
    RG_DEBUG << "RosegardenGUIDoc::stopRecordingAudio - "
                 << "got recorded segment" << endl;

    // now move the segment back by the jack record latency
    //
    KConfig* config = kapp->config();
    config->setGroup(Rosegarden::LatencyOptionsConfigGroup);

    int recordSec = config->readLongNumEntry("jackrecordlatencysec", 0);
    int recordUSec = config->readLongNumEntry("jackrecordlatencyusec", 0);

    Rosegarden::RealTime jackLatency(recordSec, recordUSec);
    Rosegarden::RealTime adjustedStartTime =
        m_composition.getElapsedRealTime(m_recordSegment->getStartTime()) -
        jackLatency;

    Rosegarden::timeT shiftedStartTime =
        m_composition.getElapsedTimeForRealTime(adjustedStartTime);

    RG_DEBUG << "RosegardenGUIDoc::stopRecordingAudio - "
                 << "shifted recorded audio segment by "
                 <<  m_recordSegment->getStartTime() - shiftedStartTime
                 << " clicks" << endl;

    m_recordSegment->setStartTime(shiftedStartTime);

}

// Called from the sequencer when all is clear with the newly recorded
// file - this method finalizes the add of the audio file as it should
// now have proper audio file information in the header.
//
void
RosegardenGUIDoc::finalizeAudioFile(Rosegarden::AudioFileId /*id*/)
{
    RG_DEBUG << "RosegardenGUIDoc::finalizeAudioFile" << endl;

    // Get the last added audio file - the one we've just recorded
    // and generate a preview of this audio file for population
    // into the resulting SegmentItems.
    //
    Rosegarden::AudioFile *newAudioFile = m_audioFileManager.getLastAudioFile();

    // Create a progress dialog
    //
    RosegardenProgressDialog progressDlg(i18n("Generating audio preview..."),
                                         100, (QWidget*)parent());

    connect(&m_audioFileManager, SIGNAL(setProgress(int)),
            progressDlg.progressBar(), SLOT(setValue(int)));
            
    try
    {
        m_audioFileManager.generatePreview(newAudioFile->getId());
    }
    catch(std::string e)
    {
	KStartupLogo::hideIfStillThere();
	CurrentProgressDialog::freeze();
        KMessageBox::error(0, strtoqstr(e));
	CurrentProgressDialog::thaw();
    }

    m_commandHistory->addCommand
	(new SegmentRecordCommand(m_recordSegment));

    // Update preview
    //
    RosegardenGUIView *w;
    for(w=m_viewList.first(); w!=0; w=m_viewList.next()) {
        w->getTrackEditor()->
            getSegmentCanvas()->updateSegmentItem(m_recordSegment);
    }

    // update views
    slotUpdateAllViews(0);

    // Now install the file in the sequencer
    //
    // We're playing fast and loose with DCOP here - we just send
    // this request and carry on regardless otherwise the sequencer
    // can just hang our request.  We don't risk a call() and we
    // don't get a return type.  Ugly and hacky but it appears to
    // work for me - so hey.
    //
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);
    streamOut << QString(strtoqstr(newAudioFile->getFilename()));
    streamOut << newAudioFile->getId();
    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "addAudioFile(QString, int)", data))
    {
        RG_DEBUG << "prepareAudio() - couldn't add audio file"
                     << endl;
        return;
    }

    // clear down
    m_recordSegment = 0;

}

void
RosegardenGUIDoc::slotNewRecordButton()
{
    // Inform the sequencer if it's an audio track
    //
    RG_DEBUG << "RosegardenGUIDoc::slotNewRecordButton()" << endl;

    // Document modified
    slotDocumentModified();

    // If we're got an audio track then tell someone goddamn
    //
    Rosegarden::Track *recordTrack
        = m_composition.getTrackById(m_composition.getRecordTrack());

    if (recordTrack)
    {
        Rosegarden::Instrument *recordInstr =
            m_studio.getInstrumentById(recordTrack->getInstrument());

        if (recordInstr)
        {
            if (recordInstr->getType() == Rosegarden::Instrument::Audio)
                setAudioMonitoringState(true, recordInstr->getId());
            else
                setAudioMonitoringState(false, recordInstr->getId());

            // Update the instrument parameter box
            //
            RosegardenGUIView *w;
            for(w=m_viewList.first(); w!=0; w=m_viewList.next())
            {
                w->slotSetRecord(recordInstr->getId(), 
                        (m_composition.getRecordTrack() == 
                         m_composition.getSelectedTrack()));
            }
        }
    }

}

void
RosegardenGUIDoc::setAudioMonitoringState(bool value,
                                          Rosegarden::InstrumentId id)
{
    QByteArray data;
    QDataStream streamOut(data, IO_WriteOnly);

    streamOut << id;

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "setAudioMonitoringInstrument(unsigned int)",
                                  data))
    {
        RG_DEBUG << "RosegardenGUIDoc::setAudioMonitoringState - "
                 << "can't set monitoring instrument at sequencer"
                 << endl;
    }

    QByteArray data2;
    QDataStream streamOut2(data, IO_WriteOnly);

    streamOut2 << long(value);

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "setAudioMonitoring(long int)",
                                  data))
    {
        RG_DEBUG << "RosegardenGUIDoc::setAudioMonitoringState - "
                 << "can't turn on audio monitoring at sequencer"
                 << endl;
    }

    RG_DEBUG << "setAudioMonitoringState - " << value
             << " - instrument = " << id << endl;
}


Rosegarden::RealTime
RosegardenGUIDoc::getAudioPlayLatency()
{
    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getAudioPlayLatency()",
                                  data, replyType, replyData))
    {
        RG_DEBUG << "RosegardenGUIDoc::getAudioPlayLatency - "
                 << "Playback failed to contact Rosegarden sequencer"
                 << endl;
        return Rosegarden::RealTime(0, 0);
    }
    else
    {
        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        Rosegarden::MappedRealTime result;
        streamIn >> result;

        return (result.getRealTime());
    }
}

Rosegarden::RealTime
RosegardenGUIDoc::getAudioRecordLatency()
{

    QByteArray data;
    QCString replyType;
    QByteArray replyData;

    if (!kapp->dcopClient()->call(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "getAudioRecordLatency()",
                                  data, replyType, replyData))
    {
        RG_DEBUG << "RosegardenGUIDoc::getAudioRecordLatency - "
                 << "Playback failed to contact Rosegarden sequencer"
                 << endl;
        return Rosegarden::RealTime(0, 0);
    }
    else
    {
        // ensure the return type is ok
        QDataStream streamIn(replyData, IO_ReadOnly);
        Rosegarden::MappedRealTime result;
        streamIn >> result;

        return (result.getRealTime());
    }
}

// This is like SequenceManager::preparePlayback() but we only do it
// once per file load as it takes a bit longer.
//
void
RosegardenGUIDoc::initialiseControllers()
{
    Rosegarden::InstrumentList list = m_studio.getAllInstruments();
    Rosegarden::MappedComposition mC;
    Rosegarden::MappedEvent *mE;

    Rosegarden::InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Instrument::Midi)
        {
            std::vector<Rosegarden::MidiControlPair> advancedControls;
            advancedControls.
                push_back(Rosegarden::
                        MidiControlPair(Rosegarden::MIDI_CONTROLLER_CHORUS,
                                       (*it)->getChorus()));
            advancedControls.
                push_back(Rosegarden::
                        MidiControlPair(Rosegarden::MIDI_CONTROLLER_REVERB,
                                       (*it)->getReverb()));
            advancedControls.
                push_back(Rosegarden::
                        MidiControlPair(Rosegarden::MIDI_CONTROLLER_RESONANCE,
                                       (*it)->getResonance()));
            advancedControls.
                push_back(Rosegarden::
                        MidiControlPair(Rosegarden::MIDI_CONTROLLER_FILTER,
                                       (*it)->getFilter()));
            advancedControls.
                push_back(Rosegarden::
                        MidiControlPair(Rosegarden::MIDI_CONTROLLER_ATTACK,
                                       (*it)->getAttack()));
            advancedControls.
                push_back(Rosegarden::
                        MidiControlPair(Rosegarden::MIDI_CONTROLLER_RELEASE,
                                       (*it)->getRelease()));
            advancedControls.
                push_back(Rosegarden::
                        MidiControlPair(Rosegarden::MIDI_CONTROLLER_PAN,
                                       (*it)->getPan()));
            advancedControls.
                push_back(Rosegarden::
                        MidiControlPair(Rosegarden::MIDI_CONTROLLER_VOLUME,
                                       (*it)->getVelocity()));


            std::vector<Rosegarden::MidiControlPair>::iterator
                    iit = advancedControls.begin();
            for (; iit != advancedControls.end(); iit++)
            {
                try
                {
                    mE =
                        new Rosegarden::MappedEvent((*it)->getId(),
                                        Rosegarden::MappedEvent::MidiController,
                                        iit->first,
                                        iit->second);
                }
                catch(...)
                {
                    continue;
                }

                mC.insert(mE);
            }
        }
    }

    Rosegarden::StudioControl::sendMappedComposition(mC);
}

// Clear all the plugins from the sequencer and from the Composition
// 
void
RosegardenGUIDoc::clearAllPlugins()
{
    //RG_DEBUG << "clearAllPlugins" << endl;

    Rosegarden::InstrumentList list = m_studio.getAllInstruments();
    Rosegarden::MappedComposition mC;

    Rosegarden::InstrumentList::iterator it = list.begin();
    for (; it != list.end(); it++)
    {
        if ((*it)->getType() == Rosegarden::Instrument::Audio)
        {
            Rosegarden::PluginInstanceIterator pIt = (*it)->beginPlugins();

            for(; pIt != (*it)->endPlugins(); pIt++)
            {
                if ((*pIt)->getMappedId() != -1)
                {
                    if (Rosegarden::StudioControl::
                        destroyStudioObject((*pIt)->getMappedId()) == false)
                    {
                        RG_DEBUG << "RosegardenGUIDoc::clearAllPlugins - "
                                 << "couldn't find plugin instance "
                                 << (*pIt)->getMappedId() << endl;
                    }
                }
                (*pIt)->clearPorts();
            }
            (*it)->emptyPlugins();

            /*
            RG_DEBUG << "RosegardenGUIDoc::clearAllPlugins - "
                     << "cleared " << (*it)->getName() << endl;
            */
        }
    }
}


// Deep-copy the studio from another source.
//
void
RosegardenGUIDoc::copyStudio(const Rosegarden::Studio &studio)
{
    m_studio.copy(studio);
}

// Get the clipboard from the parent application
//
Rosegarden::Clipboard*
RosegardenGUIDoc::getClipboard()
{
    RosegardenGUIApp *app = (RosegardenGUIApp*)parent();
    return app->getClipboard();
}

const unsigned int RosegardenGUIDoc::MinNbOfTracks = 64;
