// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include <iostream>

// include files for Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>

// include files for KDE
#include <klocale.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <kapp.h>
#include <kconfig.h>

#include <string>
#include <vector>

#include <unistd.h> // sleep
#include <zlib.h>

// application specific includes
#include "Event.h"
#include "Clipboard.h"
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


QList<RosegardenGUIView> *RosegardenGUIDoc::pViewList = 0L;

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Event;
using Rosegarden::PitchBend;
using Rosegarden::Controller;
using Rosegarden::Int;
using Rosegarden::String;
using Rosegarden::timeT;

using namespace Rosegarden::BaseProperties;


RosegardenGUIDoc::RosegardenGUIDoc(QWidget *parent,
                                   bool useSequencer,
                                   const char *name)
    : QObject(parent, name),
      m_recordSegment(0), m_endOfLastRecordedNote(0),
      m_commandHistory(new MultiViewCommandHistory()),
      m_clipboard(new Rosegarden::Clipboard),
      m_startUpSync(true),
      m_useSequencer(useSequencer),
      m_progressDialogDead(false)
{
    // Try to tell the sequencer that we're alive only if the
    // sequencer hasn't already forced us to sync
    //
    if (m_startUpSync)
        alive();

    if(!pViewList) {
        pViewList = new QList<RosegardenGUIView>();
    }

    pViewList->setAutoDelete(true);

    connect(m_commandHistory, SIGNAL(commandExecuted(KCommand *)),
	    this, SLOT(slotDocumentModified()));

    connect(m_commandHistory, SIGNAL(documentRestored()),
	    this, SLOT(slotDocumentRestored()));

}

RosegardenGUIDoc::~RosegardenGUIDoc()
{
    kdDebug(KDEBUG_AREA) << "~RosegardenGUIDoc()\n";
    delete m_commandHistory; // must be deleted before the Composition is
    delete m_clipboard;
}

void RosegardenGUIDoc::addView(RosegardenGUIView *view)
{
    pViewList->append(view);
}

void RosegardenGUIDoc::removeView(RosegardenGUIView *view)
{
    pViewList->remove(view);
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
    if(pViewList)
        {
            for(w=pViewList->first(); w!=0; w=pViewList->next())
                {
                    if(w!=sender)
                        w->repaint();
                }
        }

}

void RosegardenGUIDoc::setModified(bool m)
{
    m_modified = m;
};

void RosegardenGUIDoc::slotDocumentModified()
{
//    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::slotDocumentModified()" << endl;
    setModified(true);
    emit documentModified();
}

void RosegardenGUIDoc::slotDocumentRestored()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::slotDocumentRestored()" << endl;
    setModified(false);
}

bool RosegardenGUIDoc::saveIfModified()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::saveIfModified()" << endl;
    bool completed=true;

    if (m_modified) {
        RosegardenGUIApp *win=(RosegardenGUIApp *) parent();
        int want_save = KMessageBox::warningYesNoCancel(win,
                                                        i18n("The current file has been modified.\n"
                                                             "Do you want to save it?"),
                                                        i18n("Warning"));
        kdDebug(KDEBUG_AREA) << "want_save = " << want_save << endl;

        switch(want_save)
            {
            case KMessageBox::Yes:
                if (m_title == i18n("Untitled")) {
                    win->fileSaveAs();
                } else {
                    saveDocument(getAbsFilePath());
                };

                deleteContents();
                completed=true;
                break;

            case KMessageBox::No:
                setModified(false);
                deleteContents();
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

void RosegardenGUIDoc::closeDocument()
{
    deleteContents();
}

bool RosegardenGUIDoc::newDocument()
{
    m_modified=false;
    m_absFilePath=QString::null;
    m_title=i18n("Untitled");

    m_commandHistory->clear();

    // synchronise sequencer
    prepareAudio();

    return true;
}

bool RosegardenGUIDoc::openDocument(const QString& filename,
                                    const char* /*format*/ /*=0*/)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::openDocument("
                         << filename << ")" << endl;
    
    if (!filename || filename.isEmpty())
        return false;

    newDocument();

    QFileInfo fileInfo(filename);
    m_title=fileInfo.fileName();

    // Check if file readable with fileInfo ?
    if (!fileInfo.isReadable() || fileInfo.isDir()) {
        QString msg(i18n("Can't open file '"));
        msg += filename;
        msg += "'";
        
        KMessageBox::sorry(0, msg);

        return false;
    }
    
    m_absFilePath=fileInfo.absFilePath();	

    QString errMsg;
    QString fileContents;
    bool okay = readFromFile(filename, fileContents, 0);
    if (!okay) errMsg = "Couldn't read from file";
    else {

	// parse xml file
	RosegardenProgressDialog *progressDlg =
	    new RosegardenProgressDialog(i18n("Reading file..."),
					 0, // i18n("Cancel"),
					 100,
					 (QWidget*)parent());

	okay = xmlParse(fileContents, errMsg, progressDlg);

        if (m_progressDialogDead == false)
	    delete progressDlg;
        else
            m_progressDialogDead = false;
    }

    if (!okay) {
        QString msg(i18n("Error when parsing file '%1' : \"%2\"")
                    .arg(filename)
                    .arg(errMsg));
        
        KMessageBox::sorry(0, msg);

        return false;
    }

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::openDocument() end - "
                         << "m_composition : " << &m_composition
                         << " - m_composition->getNbSegments() : "
                         << m_composition.getNbSegments()
                         << " - m_composition->getDuration() : "
                         << m_composition.getDuration() << endl;

    // We might need a progress dialog when we generate previews.
    //
    RosegardenProgressDialog *progressDlg =
        new RosegardenProgressDialog(i18n("Generating audio previews..."),
                                     i18n("Cancel"),
                                     100,
                                     (QWidget*)parent());

    try
    {
        // generate any audio previews after loading the files
        m_audioFileManager.
            generatePreviews(dynamic_cast<Rosegarden::Progress*>(progressDlg));
    }
    catch(std::string e)
    {
        delete progressDlg;
        progressDlg = 0;
        RosegardenGUIApp *win=(RosegardenGUIApp *) parent();
        KMessageBox::error(win, QString(e.c_str()));
    }

    // Get rid of it - if the operation above has been quick enough
    // then we never see this dialog anyway.
    //
    if (progressDlg) delete progressDlg;

    return true;
}

bool RosegardenGUIDoc::saveDocument(const QString& filename,
                                    const char* /*format*/ /*=0*/)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::saveDocument("
                         << filename << ")\n";

    QString outText;
    QTextStream outStream(&outText, IO_WriteOnly);

    // output XML header
    //
    outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<!DOCTYPE rosegarden-data>\n"
               << "<rosegarden-data>\n";

    // Send out Composition (this includes Tracks, Instruments, Tempo
    // and Time Signature changes and any other sub-objects)
    //
    outStream << QString(strtoqstr(m_composition.toXmlString()))
              << endl << endl;

    outStream << QString(strtoqstr(m_audioFileManager.toXmlString()))
              << endl << endl;

    QString time;

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

        if (segment->isRepeating())
            outStream << "\" repeat=\"true";

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

	        //!!! The SegmentQ-properties need to be backed up despite
	        //being non-persistent (they're non-persistent because we
	        //want to lose them when copying the events, not when
	        //saving them).  What's the best way to do that?  We want
	        //to do it here, not in XmlStorableEvent, because the
	        //XmlStorableEvent _is_ the class of Event that's placed
	        //into the segment when reading, so it doesn't want to be
	        //burdened with a separate record of the unquantized 
	        //values or knowledge about the quantizer.  We probably
	        //just want to make versions of the getFromSource/setToXX
	        //methods public in the Quantizer
    
	        outStream << '\t'
		    << XmlStorableEvent(**i).toXmlString(expectedTime) << endl;

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

    kdDebug(KDEBUG_AREA) << endl << "RosegardenGUIDoc::saveDocument() finished\n";

    m_modified = false;
    m_commandHistory->documentSaved();
    return true;
}

void RosegardenGUIDoc::deleteContents()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::deleteContents()\n";

    deleteViews();

    m_commandHistory->clear();

    m_composition.clear();
    m_audioFileManager.clear();
}

void RosegardenGUIDoc::deleteViews()
{
    // auto-deletion is enabled : GUIViews will be deleted
    pViewList->clear();
}


bool
RosegardenGUIDoc::xmlParse(QString &fileContents, QString &errMsg,
                           RosegardenProgressDialog *progress)
{
    unsigned int elementCount = 0;
    for (size_t i = 0; i < fileContents.length() - 1; ++i) {
	if (fileContents[i] == '<' && fileContents[i+1] != '/') {
	    ++elementCount;
	}
    }

    RoseXmlHandler handler(this, elementCount,
                           dynamic_cast<Rosegarden::Progress*>(progress));
    QXmlInputSource source;
    source.setData(fileContents);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    START_TIMING;
    bool ok = reader.parse(source);
    PRINT_ELAPSED("RosegardenGUIDoc::xmlParse (reader.parse())");

    if (!ok) errMsg = handler.errorString();

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
RosegardenGUIDoc::readFromFile(const QString &file, QString &text,
                               RosegardenProgressDialog *progress)
{
    text = "";
    gzFile fd = gzopen(qstrtostr(file).c_str(), "rb");
    if (!fd) return false;

    static char buffer[1000];

    while (gzgets(fd, buffer, 1000)) {

        // Update gui
        //
        if (progress) progress->processEvents();

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
    if (m_recordSegment == 0 && status == RECORDING_MIDI)
    {
        m_recordSegment = new Segment();
        m_recordSegment->setTrack(m_composition.getRecordTrack());
        m_recordSegment->setStartTime(m_composition.getPosition());
    }


    if (mC.size() > 0)
    { 
        Rosegarden::MappedComposition::iterator i;
        Rosegarden::Event *rEvent = 0;
        timeT duration, absTime;

        // process all the incoming MappedEvents
        //
        for (i = mC.begin(); i != mC.end(); ++i)
        {
            absTime = m_composition.
                          getElapsedTimeForRealTime((*i)->getEventTime());
            duration = m_composition.getElapsedTimeForRealTime((*i)->getDuration());
            rEvent = 0;

            /*
            PitchBend *pB;
            Controller *con;
            */

            switch((*i)->getType())
            {
                case Rosegarden::MappedEvent::MidiNote:
                   // Create and populate a new Event (for the moment
                   // all we get from the Sequencer is Notes)
                   //
                   if ((*i)->getDuration() < Rosegarden::RealTime(0, 0))
                       duration = -1;

                   rEvent = new Event(Rosegarden::Note::EventType,
                                      absTime,
                                      duration);

                   rEvent->set<Int>(PITCH, (*i)->getPitch());
                   rEvent->set<Int>(VELOCITY, (*i)->getVelocity());

                   break;

                case Rosegarden::MappedEvent::MidiPitchBend:
                   rEvent = new Event(Rosegarden::PitchBend::EventType,
                                      absTime);

                   rEvent->set<Int>(PitchBend::MSB, (*i)->getData1());
                   rEvent->set<Int>(PitchBend::LSB, (*i)->getData2());
                   /*
                   pB = new PitchBend((*i)->getData1(), (*i)->getData2());
                   rEvent = new Event(pB->getAsEvent(absTime));
                   */
                   break;

                case Rosegarden::MappedEvent::MidiController:
                   rEvent = new Event(Rosegarden::Controller::EventType,
                                      absTime);

                   rEvent->set<Int>(Controller::DATA1, (*i)->getData1());
                   rEvent->set<Int>(Controller::DATA2, (*i)->getData2());
                   /*
                   con = new Controller(Controller::Pan,
                                        (*i)->getData1(),
                                        (*i)->getData2());
                   rEvent = new Event(con->getAsEvent(absTime));
                   */
                   break;

                case Rosegarden::MappedEvent::MidiProgramChange:
                   std::cerr << "RosegardenGUIDoc::insertRecordedMidi() - "
                             << "got Program Change (unsupported)"
                             << std::endl;
                   break;

                case Rosegarden::MappedEvent::MidiKeyPressure:
                   std::cerr << "RosegardenGUIDoc::insertRecordedMidi() - "
                             << "got Key Pressure (unsupported)"
                             << std::endl;
                   break;

                case Rosegarden::MappedEvent::MidiChannelPressure:
                   std::cerr << "RosegardenGUIDoc::insertRecordedMidi() - "
                             << "got Channel Pressure (unsupported)"
                             << std::endl;
                   break;

                case Rosegarden::MappedEvent::MidiSystemExclusive:
                   std::cerr << "RosegardenGUIDoc::insertRecordedMidi() - "
                             << "got System Exclusive (unsupported)"
                             << std::endl;
                   break;


                case Rosegarden::MappedEvent::MidiNoteOneShot:
                   std::cerr << "RosegardenGUIDoc::insertRecordedMidi() - "
                             << "GOT UNEXPECTED MappedEvent::MidiNoteOneShot"
                             << std::endl;
                   break;

                   // Audio control signals - ignore these
                case Rosegarden::MappedEvent::Audio:
                case Rosegarden::MappedEvent::AudioCancel:
                case Rosegarden::MappedEvent::AudioLevel:
                case Rosegarden::MappedEvent::AudioStopped:
                   break;

                default:
                   std::cerr << "RosegardenGUIDoc::insertRecordedMidi() - "
                             << "GOT UNSUPPORTED MAPPED EVENT"
                             << std::endl;
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
            // If there was a gap between the last note and this one
            // then fill it with rests
            //
            if (absTime > m_endOfLastRecordedNote)
                m_recordSegment->fillWithRests(absTime + duration);

            // Now insert the new event
            //
            Segment::iterator loc = m_recordSegment->insert(rEvent);

            // And now fiddle with it
            //
            SegmentNotationHelper helper(*m_recordSegment);
            if (!helper.isViable(rEvent))
                helper.makeNoteViable(loc);

            // Update our counter
            //
            m_endOfLastRecordedNote = absTime + duration;

        }
    }

    // Only update gui if we're still recording - otherwise we
    // can get a recording SegmentItem hanging around.
    //
    if (status == RECORDING_MIDI)
    {
        // update this segment on the GUI
        RosegardenGUIView *w;
        if(pViewList)
        {
            for(w=pViewList->first(); w!=0; w=pViewList->next())
            {
                w->showRecordingSegmentItem(m_recordSegment);
            }
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
    if(pViewList)
    {
        for(w=pViewList->first(); w!=0; w=pViewList->next())
        {
            w->deleteRecordingSegmentItem();
        }
    }

    // Roll out any NOTE ON/NOTE OFFs we've had to insert.
    // These are indicated by a negative time on the first
    // event.  We can only do this after we've recorded the
    // whole.
    //
    convertToSinglePoint(m_recordSegment);
        
    if (m_recordSegment->getComposition()) {

	// something in the record segment (that's why it was added
	// to the composition)
	m_commandHistory->addCommand
	    (new SegmentRecordCommand(m_recordSegment));
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
        std::cerr <<
        "prepareAudio() - couldn't delete all audio files from sequencer"
                  << std::endl;
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
            std::cerr << "prepareAudio() - couldn't add audio file"
                      << std::endl;
            return;
        }
        else
        {
            QDataStream streamIn(replyData, IO_ReadOnly);
            int result;
            streamIn >> result;
            if (!result)
            {
                std::cerr << "prepareAudio() - failed to add file \"" 
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
RosegardenGUIDoc::alive()
{
    if (m_useSequencer == false)
        return;

    // Just a quick refreshing sleep here to ensure that we don't
    // mask any call back to the sequencer by being to hasty.
    //
    // Probably unnecessary but better safe than sorry.
    //

    while(!kapp->dcopClient()->
           isApplicationRegistered(QCString(ROSEGARDEN_SEQUENCER_APP_NAME)))
    {
        kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::getMappedDevice - "
                             << "waiting for Sequencer to come up\n";

        kapp->processEvents(1000);
        sleep(1); // 1s
    }

    QByteArray data;

    if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                  ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                  "alive()",
                                  data))
    {
        kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::getMappedDevice - "
                             << "can't call the Sequencer\n";
        return;
    }

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
        kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::getMappedDevice - "
                             << "can't get number of devices\n";
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
        kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::getMappedDevice - "
                             << "got unknown returntype from getDevices()\n";
        return;
    }

    // Clear the Studio before population
    //
    m_studio.clear();

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::getMappedDevice - devices = "
                         << devices << endl;

    for (unsigned int i = 0; i < devices; i++)
    {
        kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::getMappedDevice - i = "
                             << i << endl;
        getMappedDevice(i);
    }

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::getMappedDevice - "
                         << "Sequencer alive - Instruments synced\n";

    // Ok, we've sync'd - make sure that this app doesn't
    // drive this sync again by switching our startUpSync
    // flag off.
    //
    m_startUpSync = false;

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
        kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::getMappedDevice() - "
                             << "can't call Sequencer\n";
        return;
    }

    Rosegarden::MappedDevice *mD = new Rosegarden::MappedDevice();
    QDataStream reply(replyData, IO_ReadOnly);

    if (replyType == "Rosegarden::MappedDevice")
    {
        kdDebug(KDEBUG_AREA)  << "RosegardenGUIDoc::getMappedDevice() - "
                              << "got Rosegarden::MappedDevice\n";

        // unfurl
        reply >> mD;
    }
    else
    {
        kdDebug(KDEBUG_AREA)  << "RosegardenGUIDoc::getMappedDevice() - "
                              << "didn't get MappedDevice " 
                              << id << "\n";
        return;
    }

    kdDebug(KDEBUG_AREA)  << "RosegardenGUIDoc::getMappedDevice() - check if we've got this device already\n";

    // See if we've got this device already
    //
    Rosegarden::Device *device = m_studio.getDevice(id);
    Rosegarden::Instrument::InstrumentType type;

    kdDebug(KDEBUG_AREA)  << "RosegardenGUIDoc::getMappedDevice() - device = "
                          << device << endl;

    if (mD->size() == 0)
    {
        kdDebug(KDEBUG_AREA)  << "RosegardenGUIDoc::getMappedDevice() - 0 instrument found\n";
        return;
    }
    

    if(device == 0 && (*(mD->begin())))
    {
        type = (*(mD->begin()))->getType();

        if (type  == Rosegarden::Instrument::Midi)
        {
            device = new Rosegarden::MidiDevice(id, "MIDI device");
            m_studio.addDevice(device);
        }
        else if (type  == Rosegarden::Instrument::Audio)
        {
            device = new Rosegarden::AudioDevice(id, "Audio device");
            m_studio.addDevice(device);
        }
        else
        {
            kdDebug(KDEBUG_AREA)  << "RosegardenGUIDoc::getMappedDevice - "
                                  << "unknown device\n";
        }
    }

    Rosegarden::Instrument *instrument;
    Rosegarden::MappedDeviceIterator it;

    for (it = mD->begin(); it != mD->end(); it++)
    {
        instrument = new Rosegarden::Instrument(
                                             (*it)->getId(),
                                             (*it)->getType(),
                                             (*it)->getName(),
                                             (*it)->getChannel(),
                                             device);
        device->addInstrument(instrument);
    }

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

    // Always fill with rests if we have some events
    //
    if(segment->begin() != segment->end())
        segment->normalizeRests(segment->getStartTime(), segment->getEndTime());

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
        m_recordSegment->setLabel("recorded audio");

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
            std::cerr << "RosegardenGUIDoc::insertRecordedAudio - "
                      << "no audio file" << std::endl;
        }

        // always insert straight away for audio
        m_composition.addSegment(m_recordSegment);
    }

    m_recordSegment->fillWithRests
	(m_composition.getElapsedTimeForRealTime(time));

    // update this segment on the GUI
    RosegardenGUIView *w;
    if(pViewList)
    {
        for(w=pViewList->first(); w!=0; w=pViewList->next())
        {
            w->showRecordingSegmentItem(m_recordSegment);
        }
    }
}

// tidy up the recording SegmentItem etc.
//
void
RosegardenGUIDoc::stopRecordingAudio()
{
    // If we've created nothing then do nothing with it
    //
    if (m_recordSegment == 0)
        return;

    RosegardenGUIView *w;
    if(pViewList)
    {
        for(w=pViewList->first(); w!=0; w=pViewList->next())
        {
            w->deleteRecordingSegmentItem();
        }
    }

    // set the audio end time
    //
    m_recordSegment->setAudioEndTime(
        m_composition.getRealTimeDifference(m_recordSegment->getStartTime(),
                                            m_composition.getPosition()));

    // now add the Segment
    std::cout << "RosegardenGUIDoc::stopRecordingAudio - "
              << "got recorded segment" << std::endl;

    // now move the segment back by the jack record latency
    //
    KConfig* config = kapp->config();
    config->setGroup("Latency Options");

    int recordSec = config->readLongNumEntry("jackrecordlatencysec", 0);
    int recordUSec = config->readLongNumEntry("jackrecordlatencyusec", 0);

    Rosegarden::RealTime jackLatency(recordSec, recordUSec);
    Rosegarden::RealTime adjustedStartTime =
        m_composition.getElapsedRealTime(m_recordSegment->getStartTime()) -
        jackLatency;

    Rosegarden::timeT shiftedStartTime =
        m_composition.getElapsedTimeForRealTime(adjustedStartTime);

    std::cout << "RosegardenGUIDoc::stopRecordingAudio - "
              << "shifted recorded audio segment by "
              <<  m_recordSegment->getStartTime() - shiftedStartTime
              << " clicks" << std::endl;

    m_recordSegment->setStartTime(shiftedStartTime);

    // something in the record segment (that's why it was added
    // to the composition)
    m_commandHistory->addCommand (new SegmentRecordCommand(m_recordSegment));

    // Get the last added audio file - the one we've just recorded
    // and generate a preview of this audio file for population
    // into the resulting SegmentItems.
    //
    Rosegarden::AudioFile *newAudioFile = m_audioFileManager.getLastAudioFile();

    // Create a progress dialog
    //
    RosegardenProgressDialog *progressDlg =
        new RosegardenProgressDialog(i18n("Generating audio preview..."),
                                     i18n("Cancel"),
                                     100,
                                     (QWidget*)parent());

    try
    {
        m_audioFileManager.generatePreview(
                dynamic_cast<Rosegarden::Progress*>(progressDlg),
                newAudioFile->getId());
    }
    catch(std::string e)
    {
        delete progressDlg;
        progressDlg = 0;
        RosegardenGUIApp *win=(RosegardenGUIApp *) parent();
        KMessageBox::error(win, QString(e.c_str()));
    }

    if (progressDlg) delete progressDlg;


    // Update preview
    //
    if(pViewList)
    {
        for(w=pViewList->first(); w!=0; w=pViewList->next())
        {
            w->getTrackEditor()->
                getSegmentCanvas()->updateSegmentItem(m_recordSegment);
        }
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
        std::cerr << "prepareAudio() - couldn't add audio file"
                  << std::endl;
        return;
    }

    // clear down
    m_recordSegment = 0;
}

void
RosegardenGUIDoc::progressDialogDead()
{
    m_progressDialogDead = true;
}

void
RosegardenGUIDoc::slotNewRecordButton()
{
    // Inform the sequencer if it's an audio track
    //
    std::cout << "RosegardenGUIDoc::slotNewRecordButton()" << std::endl;

    // Document modified
    setModified(true);

    // If we're got an audio track then tell someone goddamn
    //
    Rosegarden::Track *recordTrack
        = m_composition.getTrackByIndex(m_composition.getRecordTrack());

    if (recordTrack)
    {
        Rosegarden::Instrument *recordInstr =
            m_studio.getInstrumentById(recordTrack->getInstrument());

        if (recordInstr)
        {
            bool monitorAudio = false;
            if (recordInstr->getType() == Rosegarden::Instrument::Audio)
                monitorAudio = true;

            QByteArray data;
            QDataStream streamOut(data, IO_WriteOnly);

            streamOut << recordInstr->getId();

            if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                          ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                          "setAudioMonitoringInstrument(unsigned int)",
                                          data))
            {
                std::cerr << "RosegardenGUIDoc::slotNewRecordButton - "
                          << "can't set monitoring instrument at sequencer"
                          << std::endl;
            }

            QByteArray data2;
            QDataStream streamOut2(data, IO_WriteOnly);

            streamOut2 << monitorAudio;

            if (!kapp->dcopClient()->send(ROSEGARDEN_SEQUENCER_APP_NAME,
                                          ROSEGARDEN_SEQUENCER_IFACE_NAME,
                                          "setAudioMonitoring(bool)",
                                          data))
            {
                std::cerr << "RosegardenGUIDoc::slotNewRecordButton - "
                          << "can't turn on audio monitoring at sequencer"
                          << std::endl;
            }
        }
    }

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
        std::cerr << "RosegardenGUIDoc::getAudioPlayLatency - "
                  << "Playback failed to contact Rosegarden sequencer"
                  << std::endl;
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
        std::cerr << "RosegardenGUIDoc::getAudioRecordLatency - "
                  << "Playback failed to contact Rosegarden sequencer"
                  << std::endl;
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


