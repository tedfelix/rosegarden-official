/***************************************************************************
                          rosegardenguidoc.cpp  -  description
                             -------------------
    begin                : Mon Jun 19 23:41:03 CEST 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <iostream>

// include files for Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>

// include files for KDE
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

// application specific includes
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "rosegardenguiview.h"

#include "Element2.h"

QList<RosegardenGUIView> *RosegardenGUIDoc::pViewList = 0L;

RosegardenGUIDoc::RosegardenGUIDoc(QWidget *parent, const char *name) : QObject(parent, name)
{
    if(!pViewList)
        {
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
    absFilePath=filename;
}

const QString &RosegardenGUIDoc::getAbsFilePath() const
{
    return absFilePath;
}

void RosegardenGUIDoc::setTitle(const QString &_t)
{
    title=_t;
}

const QString &RosegardenGUIDoc::getTitle() const
{
    return title;
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

bool RosegardenGUIDoc::saveModified()
{
    bool completed=true;

    if(modified)
        {
            RosegardenGUIApp *win=(RosegardenGUIApp *) parent();
            int want_save = KMessageBox::warningYesNoCancel(win, i18n("Warning"),
                                                            i18n("The current file has been modified.\n"
                                                                 "Do you want to save it?"));
            switch(want_save)
                {
                case 1:
                    if (title == i18n("Untitled"))
                        {
                            win->slotFileSaveAs();
                        }
                    else
                        {
                            saveDocument(getAbsFilePath());
                        };

                    deleteContents();
                    completed=true;
                    break;

                case 2:
                    setModified(false);
                    deleteContents();
                    completed=true;
                    break;	

                case 3:
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
    /////////////////////////////////////////////////
    // TODO: Add your document initialization code here
    /////////////////////////////////////////////////
    modified=false;
    absFilePath=QDir::homeDirPath();
    title=i18n("Untitled");

    return true;
}

bool RosegardenGUIDoc::openDocument(const QString &filename, const char *format /*=0*/)
{
    QFileInfo fileInfo(filename);
    title=fileInfo.fileName();
    absFilePath=fileInfo.absFilePath();	

    QFile file(filename);

    if ( !file.open( IO_ReadOnly ) ) {
        QString msg(i18n("Can't open file '"));
        msg += filename;
        msg += "'";
        
        KMessageBox::sorry(0, msg);

        return false;
    }
    
    size_t size = file.size();
    char* buffer = new char[ size + 1 ];
    file.readBlock( buffer, size );
    buffer[ size ] = 0;
    file.close();

    QString res(QString::fromUtf8( buffer, size ));
    
    delete[] buffer;

    if (xmlParse(res)) {
        modified=false;
        return true;
    }

    return false;
}

bool RosegardenGUIDoc::saveDocument(const QString &filename, const char *format /*=0*/)
{
    /////////////////////////////////////////////////
    // TODO: Add your document saving code here
    /////////////////////////////////////////////////

    modified=false;
    return true;
}

void RosegardenGUIDoc::deleteContents()
{
    for(ElementList::iterator i = m_elements.begin(); i != m_elements.end(); ++i) {
        delete *i;
    }

    m_elements.clear();
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

class XMLStorableElement : public Element2
{
public:
    XMLStorableElement(const QDomNamedNodeMap &attributes,
                       const QDomNodeList &children);
protected:
    duration noteName2Duration(const QString &noteName);
    void initMap();

    typedef hash_map<string, duration, hashstring, eqstring> namedurationmap;

    static namedurationmap m_noteName2DurationMap;
};

XMLStorableElement::namedurationmap
XMLStorableElement::m_noteName2DurationMap;

Element2::duration
XMLStorableElement::noteName2Duration(const QString &nn)
{
    if (m_noteName2DurationMap.empty())
        initMap();

    string noteName(nn.latin1());
    
    namedurationmap::iterator it(m_noteName2DurationMap.find(noteName));
    
    if (it == m_noteName2DurationMap.end()) {
        // note name doesn't exist
        kdDebug(KDEBUG_AREA) << "Bad note name : " << nn << endl;
        return 0;
    }
    

    return it->second;
}

void
XMLStorableElement::initMap()
{
    if (! m_noteName2DurationMap.empty())
        return;

    m_noteName2DurationMap["64th"]               = 6;
    m_noteName2DurationMap["hemidemisemiquaver"] = 6;

    m_noteName2DurationMap["32nd"]           = 12;
    m_noteName2DurationMap["demisemiquaver"] = 12;

    m_noteName2DurationMap["16th"]       = 24;
    m_noteName2DurationMap["semiquaver"] = 24;

    m_noteName2DurationMap["8th"]    = 48;
    m_noteName2DurationMap["quaver"] = 48;

    m_noteName2DurationMap["quarter"]  = 96;
    m_noteName2DurationMap["crotchet"] = 96;
    
    m_noteName2DurationMap["half"]  = 192;
    m_noteName2DurationMap["minim"] = 192;

    m_noteName2DurationMap["whole"]     = 384;
    m_noteName2DurationMap["semibreve"] = 384;

    // what is the american name ??
    m_noteName2DurationMap["breve"] = 768;
    
}


XMLStorableElement::XMLStorableElement(const QDomNamedNodeMap &attributes,
                                       const QDomNodeList &children)
{
    for (unsigned int i = 0; i < attributes.length(); ++i) {
	QDomAttr n(attributes.item(i).toAttr());

        // special cases first : package, type, duration
	if (n.name() == "package") {

            setPackage(n.value().latin1());

        } else if (n.name() == "type") {

            setType(n.value().latin1());

        } else if (n.name() == "duration") {

            bool isNumeric = true;
            Element2::duration d = n.value().toUInt(&isNumeric);
            if (!isNumeric) {
                // It may be one of the accepted strings : breve, semibreve...
                // whole, half-note, ...
                d = noteName2Duration(n.value());
                if (!d)
                    kdDebug(KDEBUG_AREA) << "Bad duration : " << n.value() << endl;
            }

            kdDebug(KDEBUG_AREA) << "Setting duration to : " << d << endl;
            setDuration(d);

        } else {
            // set property
        }
        
    }

}


bool
RosegardenGUIDoc::xmlParseElement(const QDomElement &base)
{
    QString tag;
    QDomElement domElement(base);

    while ( !domElement.isNull() ) {

        tag = domElement.tagName();

        if (tag == "element") {

            QDomNamedNodeMap attributes(domElement.attributes());
            QDomNodeList children(domElement.childNodes());

            m_elements.push_back(new XMLStorableElement(attributes, children));

        } else if (tag == "rosegarden-data") {

            QDomNodeList children(domElement.childNodes());
	    if (children.length())
                xmlParseElement(children.item(0).toElement());

        } else {
            kdDebug(KDEBUG_AREA) << "xmlParseElement: Unknown tag " << tag
                                 << " - checking children" << endl;
            QDomNodeList children(domElement.childNodes());
	    if (children.length())
                xmlParseElement(children.item(0).toElement());

        }

        domElement = domElement.nextSibling().toElement();
    }
    return true;
}



bool
RosegardenGUIDoc::xmlParse(const QString &xmldata)
{
    m_doc.setContent(xmldata);
    
    QDomElement baseNode = m_doc.documentElement();

    xmlParseElement(baseNode);

    return true;
}
