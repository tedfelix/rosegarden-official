// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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
#include <kmessagebox.h>

#include <string>

// application specific includes
#include "rosedebug.h"
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "rosegardenguiview.h"
#include "rosexmlhandler.h"
#include "xmlstorableevent.h"
#include "Event.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"

QList<RosegardenGUIView> *RosegardenGUIDoc::pViewList = 0L;

using Rosegarden::Composition;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::String;

using namespace Rosegarden::BaseProperties;


RosegardenGUIDoc::RosegardenGUIDoc(QWidget *parent, const char *name)
    : QObject(parent, name)
{
    if(!pViewList) {
        pViewList = new QList<RosegardenGUIView>();
    }

    pViewList->setAutoDelete(true);

    connect(&m_commandHistory, SIGNAL(commandExecuted()),
	    this, SLOT(documentModified()));

    connect(&m_commandHistory, SIGNAL(documentRestored()),
	    this, SLOT(documentRestored()));
}

RosegardenGUIDoc::~RosegardenGUIDoc()
{
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

void RosegardenGUIDoc::documentModified()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::documentModified()" << endl;
    setModified(true);
}

void RosegardenGUIDoc::documentRestored()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::documentRestored()" << endl;
    setModified(false);
}

bool RosegardenGUIDoc::saveIfModified()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::saveIfModified()" << endl;
    bool completed=true;

    if(m_modified) {
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

    return true;
}

bool RosegardenGUIDoc::openDocument(const QString& filename,
                                    const char* /*format*/ /*=0*/)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::openDocument("
                         << filename << ")" << endl;
    
    if (!filename || filename.isEmpty())
        return false;

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

    QFile file(filename);
    QString errMsg;
    bool fileParsedOk = xmlParse(file, errMsg);
    file.close();   

    if (!fileParsedOk) {
        QString msg(i18n("Error when parsing file '%1' : %2")
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

    return true;
}

bool RosegardenGUIDoc::saveDocument(const QString& filename,
                                    const char* /*format*/ /*=0*/)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::saveDocument("
                         << filename << ")" << endl;

    QFile file(filename);
    file.open(IO_WriteOnly);

    QTextStream fileStream(&file);

    // output XML header
    //
    fileStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<!DOCTYPE rosegarden-data>\n"
               << "<rosegarden-data>\n";

    // output reference segment
    fileStream << "<reference-segment>" << endl;
    const Segment *refSegment = m_composition.getReferenceSegment();
    for (Segment::iterator i = refSegment->begin(); i != refSegment->end(); ++i) {
	if (!(*i)->isa(Composition::BarEventType)) {
	    if ((*i)->getAbsoluteTime() > 0) {
		fileStream << "<resync time=\"" << (*i)->getAbsoluteTime()
			   << "\"/>" << endl;
	    }
	    fileStream << XmlStorableEvent::toXmlString(*(*i)) << endl;
	}
    }
    fileStream << "</reference-segment>" << endl;

    // output all elements
    //
    // Iterate on segments
    for (Composition::iterator trks = m_composition.begin();
         trks != m_composition.end(); ++trks) {

        //--------------------------
        fileStream << QString("<segment instrument=\"%1\" start=\"%2\">")
            .arg((*trks)->getInstrument())
            .arg((*trks)->getStartIndex()) << endl;

        long currentGroup = -1;

        for (Segment::iterator i = (*trks)->begin();
             i != (*trks)->end(); ++i) {

            long group;
            if ((*i)->get<Int>(BEAMED_GROUP_ID, group)) {
                if (group != currentGroup) {
                    if (currentGroup != -1) fileStream << "</group>" << endl;
                    std::string type = (*i)->get<String>(BEAMED_GROUP_TYPE);
                    fileStream << "<group type=\"" << type.c_str() << "\"";
		    if (type == GROUP_TYPE_TUPLED) {
			fileStream
			    << " length=\""
			    << (*i)->get<Int>(BEAMED_GROUP_TUPLED_LENGTH)
			    << "\" untupled=\""
			    << (*i)->get<Int>(BEAMED_GROUP_UNTUPLED_LENGTH)
			    << "\" count=\""
			    << (*i)->get<Int>(BEAMED_GROUP_TUPLED_COUNT)
			    << "\"";
		    }
			    
		    fileStream << ">" << endl;
                    currentGroup = group;
                }
            } else if (currentGroup != -1) {
                fileStream << "</group>" << endl;
                currentGroup = -1;
            }

            Segment::iterator nextEl = i;
            ++nextEl;

            if (nextEl != (*trks)->end()) {

                timeT absTime = (*i)->getAbsoluteTime();
            
                if ((*nextEl)->getAbsoluteTime() == absTime) {

                    fileStream << "<chord>" << endl; //------

                    while ((*i)->getAbsoluteTime() == absTime) {
                        fileStream << '\t'
                                   << XmlStorableEvent::toXmlString(*(*i))
                                   << endl;
                        ++i;
                        if (i == (*trks)->end()) break;
                    }

                    fileStream << "</chord>" << endl; //-----

                    if (i == (*trks)->end()) break;
		    --i;
		    continue;
                }
            }
        
            fileStream << XmlStorableEvent::toXmlString(*(*i)) << endl;
        }

        if (currentGroup != -1) {
            fileStream << "</group>" << endl;
        }

        fileStream << "</segment>" << endl; //-------------------------

    }
    
    // close the top-level XML tag
    //
    fileStream << "</rosegarden-data>\n";

    kdDebug(KDEBUG_AREA) << endl << "RosegardenGUIDoc::saveDocument() finished"
                         << endl;

    m_modified=false;
    m_commandHistory.documentSaved();
    return true;
}

void RosegardenGUIDoc::deleteContents()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::deleteContents()" << endl;

    deleteViews();

    m_composition.clear();
}

void RosegardenGUIDoc::deleteViews()
{
    // auto-deletion is enabled : GUIViews will be deleted
    pViewList->clear();
}


bool
RosegardenGUIDoc::xmlParse(QFile &file, QString &errMsg)
{
    // parse xml file
    RoseXmlHandler handler(m_composition);

    QXmlInputSource source(file);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

    bool ok = reader.parse(source);

    if (!ok) errMsg = handler.errorString();

    return ok;
}


void
RosegardenGUIDoc::createNewSegment(SegmentItem *p, int instrument)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewSegment(item : "
                         << p << ")\n";

    Segment *newSegment = new Segment();
    newSegment->setInstrument(instrument);

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewSegment() new segment = "
                         << newSegment << endl;
    
    m_composition.addSegment(newSegment);

    int startBar = p->getStartBar();
    int barCount = p->getItemNbBars();

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewSegment() startBar = " 
			 << startBar << ", barCount = " << barCount << endl;

    timeT startIndex = m_composition.getBarRange(startBar).first;
    timeT duration = m_composition.getBarRange(startBar + barCount).first -
	startIndex;

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewSegment() startIndex = " 
			 << startIndex << ", duration = " << duration << endl;

    newSegment->setStartIndex(startIndex);
    newSegment->setDuration(duration);

    // store ptr to new segment in segment part item
    p->setSegment(newSegment);

    setModified();
}

