
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
#include "viewelementsmanager.h"
#include "xmlstorableevent.h"
#include "Event.h"

QList<RosegardenGUIView> *RosegardenGUIDoc::pViewList = 0L;

using Rosegarden::Composition;
using Rosegarden::Track;
using Rosegarden::Event;
using Rosegarden::Int;
using Rosegarden::String;


RosegardenGUIDoc::RosegardenGUIDoc(QWidget *parent, const char *name)
    : QObject(parent, name)
{
    if(!pViewList) {
        pViewList = new QList<RosegardenGUIView>();
    }

    pViewList->setAutoDelete(true);
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

bool RosegardenGUIDoc::saveIfModified()
{
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
                    win->slotFileSaveAs();
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
    if (!fileInfo.isReadable()) {
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
                         << " - m_composition->getNbTracks() : "
                         << m_composition.getNbTracks()
                         << " - m_composition->getNbTimeSteps() : "
                         << m_composition.getNbTimeSteps() << endl;

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

    // output all elements
    //
    // Iterate on tracks
    for (Composition::iterator trks = m_composition.begin();
         trks != m_composition.end(); ++trks) {

        //--------------------------
        fileStream << QString("<track instrument=\"%1\" start=\"%2\">")
            .arg((*trks)->getInstrument())
            .arg((*trks)->getStartIndex()) << endl;

        long currentGroup = -1;

        for (Track::iterator i = (*trks)->begin();
             i != (*trks)->end(); ++i) {

            long group;
            if ((*i)->get<Int>(Track::BeamedGroupIdPropertyName, group)) {
                if (group != currentGroup) {
                    if (currentGroup != -1) fileStream << "</group>" << endl;
                    std::string type = (*i)->get<String>
                        (Track::BeamedGroupTypePropertyName);
                    fileStream << "<group type=\""
                               << type.c_str() << "\">" << endl;
                    currentGroup = group;
                }
            } else if (currentGroup != -1) {
                fileStream << "</group>" << endl;
                currentGroup = -1;
            }

            Track::iterator nextEl = i;
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
                    }

                    fileStream << "</chord>" << endl; //-----

                    if (i == (*trks)->end()) break;
                }
            }
        
            fileStream << XmlStorableEvent::toXmlString(*(*i)) << endl;
        }

        if (currentGroup != -1) {
            fileStream << "</group>" << endl;
        }

        fileStream << "</track>" << endl; //-------------------------

    }
    
    // close the top-level XML tag
    //
    fileStream << "</rosegarden-data>\n";

    kdDebug(KDEBUG_AREA) << endl << "RosegardenGUIDoc::saveDocument() finished"
                         << endl;

    m_modified=false;
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


//            1 unit  = 1/4 clock
//            4 units =   1 clock
//            6 units = 3/2 clocks = 1 hemidemisemiquaver
//           12 units =   3 clocks = 1 demisemiquaver
//           24 units =   6 clocks = 1 semiquaver
//           48 units =  12 clocks = 1 quaver
//           96 units =  24 clocks = 1 crotchet
//          192 units =  48 clocks = 1 minim
//          384 units =  96 clocks = 1 semibreve
//          768 units = 192 clocks = 1 breve


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

    // m_doc.setContent(xmldata);
    
//     QDomElement baseNode = m_doc.documentElement();

//     xmlParseElement(baseNode);

    return ok;
}


void
RosegardenGUIDoc::createNewTrack(TrackItem *p)
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::createNewTrack(item : "
                         << p << ")\n";

    Track *newTrack = new Track(p->getItemNbTimeSteps(), p->getStartIndex());

    // store ptr to new track in track part item
    p->setTrack(newTrack);
    
    m_composition.addTrack(newTrack);

    setModified();
}

