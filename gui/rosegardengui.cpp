/***************************************************************************
                          rosegardengui.cpp  -  description
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

// include files for QT
#include <qdir.h>
#include <qprinter.h>
#include <qpainter.h>

// include files for KDE
#include <kstdaccel.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kconfig.h>

#include <kaction.h>
#include <kstdaction.h>

// application specific includes
#include "rosegardengui.h"
#include "rosegardenguiview.h"
#include "rosegardenguidoc.h"
#include "resource.h"


RosegardenGUIApp::RosegardenGUIApp()
    : KMainWindow(0)
{
    config=kapp->config();

    ///////////////////////////////////////////////////////////////////
    // call inits to invoke all other construction parts
    setupActions();
    
    initStatusBar();
    initDocument();
    initView();
	
//     readOptions();

    ///////////////////////////////////////////////////////////////////
    // disable menu and toolbar items at startup
    disableCommand(ID_FILE_SAVE);
    disableCommand(ID_FILE_SAVE_AS);
    disableCommand(ID_FILE_PRINT);
 	
    disableCommand(ID_EDIT_CUT);
    disableCommand(ID_EDIT_COPY);
    disableCommand(ID_EDIT_PASTE);
}

RosegardenGUIApp::~RosegardenGUIApp()
{

}


void RosegardenGUIApp::setupActions()
{
    // setup File menu
    // New Window ?
    KStdAction::openNew(this, SLOT(slotFileNew()),     actionCollection());
    KStdAction::open   (this, SLOT(slotFileOpen()),    actionCollection());
    m_fileRecent = KStdAction::openRecent(this,
                                          SLOT(slotFileOpenRecent(const KURL&)),
                                          actionCollection());
    KStdAction::save  (this, SLOT(slotFileSave()),     actionCollection());
    KStdAction::saveAs(this, SLOT(slotFileSaveAs()),  actionCollection());
    KStdAction::close (this, SLOT(slotFileClose()),    actionCollection());
    KStdAction::print (this, SLOT(slotFilePrint()),         actionCollection());
    KStdAction::quit  (this, SLOT(slotFileQuit()),         actionCollection());

    // setup edit menu
    KStdAction::undo     (this, SLOT(slotEditUndo()),       actionCollection());
    KStdAction::redo     (this, SLOT(slotEditRedo()),       actionCollection());
    KStdAction::cut      (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy     (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste    (this, SLOT(slotEditPaste()),      actionCollection());

    // setup Settings menu
    KStdAction::showToolbar  (this, SLOT(slotToggleToolBar()),   actionCollection());
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()),    actionCollection());

    KStdAction::keyBindings      (this, SLOT(editKeys()),     actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

//     createGUI("rosegardenui.rc");
    createGUI(); // we don't have non-standard actions for the moment
    
}


void RosegardenGUIApp::initStatusBar()
{
    ///////////////////////////////////////////////////////////////////
    // STATUSBAR
    // TODO: add your own items you need for displaying current application status.
    statusBar()->insertItem(i18n(IDS_STATUS_DEFAULT), ID_STATUS_MSG);
}

void RosegardenGUIApp::initDocument()
{
    doc = new RosegardenGUIDoc(this);
    doc->newDocument();
}

void RosegardenGUIApp::initView()
{ 
    ////////////////////////////////////////////////////////////////////
    // create the main widget here that is managed by KTMainWindow's view-region and
    // connect the widget to your document to display document contents.

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::initView()" << endl;

    view = new RosegardenGUIView(this);
    doc->addView(view);
    setCentralWidget(view);	
    setCaption(doc->getTitle());

}

void RosegardenGUIApp::enableCommand(int id_)
{
    ///////////////////////////////////////////////////////////////////
    // enable menu and toolbar functions by their ID's
    menuBar()->setItemEnabled(id_, true);
    toolBar()->setItemEnabled(id_, true);
}

void RosegardenGUIApp::disableCommand(int id_)
{
    ///////////////////////////////////////////////////////////////////
    // disable menu and toolbar functions by their ID's
    menuBar()->setItemEnabled(id_, false);
    toolBar()->setItemEnabled(id_, false);
}

void RosegardenGUIApp::openDocumentFile(const char* _cmdl)
{
    slotStatusMsg(i18n("Opening file..."));
    
    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::openDocumentFile("
                         << _cmdl << ")" << endl;

    doc->openDocument(_cmdl);
    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));

    initView();
}


RosegardenGUIDoc *RosegardenGUIApp::getDocument() const
{
    return doc;
}

void RosegardenGUIApp::saveOptions()
{	
    config->setGroup("General Options");
    config->writeEntry("Geometry", size());
    config->writeEntry("Show Toolbar", toolBar()->isVisible());
    config->writeEntry("Show Statusbar",statusBar()->isVisible());
    config->writeEntry("ToolBarPos", (int) toolBar()->barPos());
//     config->writeEntry("Recent Files", recentFiles);
}


void RosegardenGUIApp::readOptions()
{
	
//     config->setGroup("General Options");

//     // bar status settings
//     bool bViewToolbar = config->readBoolEntry("Show Toolbar", true);
//     viewMenu->setItemChecked(ID_VIEW_TOOLBAR, bViewToolbar);

//     if(!bViewToolbar) {
//         KMessageBox::sorry(0, "Need to re-implement toolbar hide");
//         // enableToolBar(KToolBar::Hide);
//     }
	
//     bool bViewStatusbar = config->readBoolEntry("Show Statusbar", true);
//     viewMenu->setItemChecked(ID_VIEW_STATUSBAR, bViewStatusbar);
//     if(!bViewStatusbar) {
//         KMessageBox::sorry(0, "Need to re-implement statusbar hide");
//         // enableStatusBar(KStatusBar::Hide);
//     }

//     // bar position settings
//     KToolBar::BarPosition toolBarPos;
//     toolBarPos=(KToolBar::BarPosition) config->readNumEntry("ToolBarPos", KToolBar::Top);
//     toolBar()->setBarPos(toolBarPos);
	
//     // initialize the recent file list
//     recentFiles.setAutoDelete(TRUE);
//     config->readListEntry("Recent Files", recentFiles);
	
//     for (int i=0; i < (int) recentFiles.count(); i++) {
//         recentFilesMenu->insertItem(recentFiles.at(i), i);
//     }

//     QSize size=config->readSizeEntry("Geometry");

//     if(!size.isEmpty()) {
//         resize(size);
//     }
}

void RosegardenGUIApp::saveProperties(KConfig *_cfg)
{
    if(doc->getTitle()!=i18n("Untitled") && !doc->isModified()) {
        // saving to tempfile not necessary
    } else {
        QString filename=doc->getAbsFilePath();	
        _cfg->writeEntry("filename", filename);
        _cfg->writeEntry("modified", doc->isModified());
		
        QString tempname = kapp->tempSaveName(filename);
        doc->saveDocument(tempname);
    }
}


void RosegardenGUIApp::readProperties(KConfig* _cfg)
{
    QString filename = _cfg->readEntry("filename", "");
    bool modified = _cfg->readBoolEntry("modified", false);
    if(modified)
        {
            bool canRecover;
            QString tempname = kapp->checkRecoverFile(filename, canRecover);
  	
            if(canRecover) {
                doc->openDocument(tempname);
                doc->setModified();
                QFileInfo info(filename);
                doc->setAbsFilePath(info.absFilePath());
                doc->setTitle(info.fileName());
                QFile::remove(tempname);
            }
        } else {
            if(!filename.isEmpty()) {
                doc->openDocument(filename);
            }
        }

    QString caption=kapp->caption();	
    setCaption(caption+": "+doc->getTitle());
}		

bool RosegardenGUIApp::queryClose()
{
    return doc->saveModified();
}

bool RosegardenGUIApp::queryExit()
{
    saveOptions();
    return true;
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////

void RosegardenGUIApp::slotFileNewWindow()
{
    slotStatusMsg(i18n("Opening a new application window..."));
	
    RosegardenGUIApp *new_window= new RosegardenGUIApp();
    new_window->show();

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotFileNew()
{
    slotStatusMsg(i18n("Creating new document..."));

    if(!doc->saveModified()) {
        // here saving wasn't successful

    } else {	
        doc->newDocument();		

        QString caption=kapp->caption();	
        setCaption(caption+": "+doc->getTitle());
    }

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotFileOpen()
{
    slotStatusMsg(i18n("Opening file..."));

    kdDebug(KDEBUG_AREA) << "slotFileOpen()" << endl;
	
//     if(!doc->saveModified()) {
//         // here saving wasn't successful
//     } else {	
//         QString fileToOpen=KFileDialog::getOpenFileName(QDir::homeDirPath(),
//                                                         i18n("*.xml"),
//                                                         this, i18n("Open File..."));
//         if(!fileToOpen.isEmpty()) {
//             doc->closeDocument();
//             doc->openDocument(fileToOpen);
//             QString caption=kapp->caption();	
//             setCaption(caption+": "+doc->getTitle());
//             m_fileRecent->addFile(fileToOpen);
//         }
//     }

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));

    initView();
}

void RosegardenGUIApp::slotFileOpenRecent(const KURL &url)
{
//     slotStatusMsg(i18n("Opening file..."));
	
//     if(!doc->saveModified()) {
//         // here saving wasn't successful
//     } else {
//         doc->closeDocument();
//         doc->openDocument(recentFiles.at(id_));
//         QString caption=kapp->caption();	
//         setCaption(caption+": "+doc->getTitle());
//     }

//     slotStatusMsg(i18n(IDS_STATUS_DEFAULT));

//     initView();

    kdDebug(KDEBUG_AREA) << "slotFileOpenRecent()" << endl;
    //openURL(url);
}

void RosegardenGUIApp::slotFileSave()
{
    slotStatusMsg(i18n("Saving file..."));
	
    doc->saveDocument(doc->getAbsFilePath());

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotFileSaveAs()
{
    slotStatusMsg(i18n("Saving file with a new filename..."));

    kdDebug(KDEBUG_AREA) << "slotFileSaveAs()" << endl;

//     QString newName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
//                                                  i18n("*.xml"), this, i18n("Save as..."));
//     if(!newName.isEmpty())
//         {
//             QFileInfo saveAsInfo(newName);
//             doc->setTitle(saveAsInfo.fileName());
//             doc->setAbsFilePath(saveAsInfo.absFilePath());
//             doc->saveDocument(newName);
//             m_fileRecent->addFile(newName);

//             QString caption=kapp->caption();	
//             setCaption(caption+": "+doc->getTitle());
//         }

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotFileClose()
{
    slotStatusMsg(i18n("Closing file..."));
	
    close();
    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotFilePrint()
{
    slotStatusMsg(i18n("Printing..."));

    QPrinter printer;

    if (printer.setup(this)) {
        view->print(&printer);
    }

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotFileQuit()
{
    slotStatusMsg(i18n("Exiting..."));
    saveOptions();
    // close the first window, the list makes the next one the first again.
    // This ensures that queryClose() is called on each window to ask for closing
    KMainWindow* w;
    if(memberList) {

        for(w=memberList->first(); w!=0; w=memberList->first()) {
            // only close the window if the closeEvent is accepted. If the user presses Cancel on the saveModified() dialog,
            // the window and the application stay open.
            if(!w->close())
                break;
        }
    }	
    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotEditUndo()
{
    slotStatusMsg(i18n("Undo..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotEditRedo()
{
    slotStatusMsg(i18n("Redo..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotEditCut()
{
    slotStatusMsg(i18n("Cutting selection..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotEditCopy()
{
    slotStatusMsg(i18n("Copying selection to clipboard..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotEditPaste()
{
    slotStatusMsg(i18n("Inserting clipboard contents..."));

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotToggleToolBar()
{
    slotStatusMsg(i18n("Toggle the toolbar..."));

    if (toolBar()->isVisible())
        toolBar()->hide();
    else
        toolBar()->show();

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotToggleStatusBar()
{
    slotStatusMsg(i18n("Toggle the statusbar..."));

    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}


void RosegardenGUIApp::slotStatusMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message permanently
    statusBar()->clear();
    statusBar()->changeItem(text, ID_STATUS_MSG);
}


void RosegardenGUIApp::slotStatusHelpMsg(const QString &text)
{
    ///////////////////////////////////////////////////////////////////
    // change status message of whole statusbar temporary (text, msec)
    statusBar()->message(text, 2000);
}

