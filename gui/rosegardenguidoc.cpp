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

// include files for Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>

// include files for KDE
#include <klocale.h>
#include <kmessagebox.h>

// application specific includes
#include "rosegardenguidoc.h"
#include "rosegardengui.h"
#include "rosegardenguiview.h"

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
  /////////////////////////////////////////////////
  // TODO: Add your document opening code here
  /////////////////////////////////////////////////
	
  modified=false;
  return true;
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
  /////////////////////////////////////////////////
  // TODO: Add implementation to delete the document contents
  /////////////////////////////////////////////////

}
