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
#include <kio/job.h>
#include <kio/netaccess.h>
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
    : KMainWindow(0),
      m_config(kapp->config()),
      m_fileRecent(0),
      m_view(0),
      m_doc(0)
{
    ///////////////////////////////////////////////////////////////////
    // call inits to invoke all other construction parts
    setupActions();
    
    initStatusBar();
    initDocument();
    initView();
	
    readOptions();

//     ///////////////////////////////////////////////////////////////////
//     // disable menu and toolbar items at startup
//     disableCommand(ID_FILE_SAVE);
//     disableCommand(ID_FILE_SAVE_AS);
//     disableCommand(ID_FILE_PRINT);
 	
//     disableCommand(ID_EDIT_CUT);
//     disableCommand(ID_EDIT_COPY);
//     disableCommand(ID_EDIT_PASTE);
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
    KStdAction::save  (this, SLOT(slotFileSave()),          actionCollection());
    KStdAction::saveAs(this, SLOT(slotFileSaveAs()),        actionCollection());
    KStdAction::close (this, SLOT(slotFileClose()),         actionCollection());
    KStdAction::print (this, SLOT(slotFilePrint()),         actionCollection());
    KStdAction::quit  (this, SLOT(slotFileQuit()),          actionCollection());

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

    // setup Notes menu
    new KAction(i18n("Whole"), 0, this,
                SLOT(slotWhole()), actionCollection(), "whole_note" );

    createGUI("rosegardenui.rc");
    // createGUI(); // we don't have non-standard actions for the moment
    
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
    m_doc = new RosegardenGUIDoc(this);
    m_doc->newDocument();
}

void RosegardenGUIApp::initView()
{ 
    ////////////////////////////////////////////////////////////////////
    // create the main widget here that is managed by KTMainWindow's view-region and
    // connect the widget to your document to display document contents.

    kdDebug(KDEBUG_AREA) << "RosegardenGUIDoc::initView()" << endl;

    m_view = new RosegardenGUIView(this);
    m_doc->addView(m_view);
    setCentralWidget(m_view);	
    setCaption(m_doc->getTitle());

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

    m_doc->saveIfModified();
    m_doc->closeDocument();
    m_doc->openDocument(_cmdl);
    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));

    initView();
}

int RosegardenGUIApp::openFile(const QString& url, int mode)
{

    setCaption(url);
    KURL *u = new KURL( url );

    if (u->isMalformed()) {
        KMessageBox::sorry(this, i18n("This is not a valid filename.\n"));
        return RETRY;
    }

    if (!u->isLocalFile()) {
        KMessageBox::sorry(this, i18n("This is not a local file.\n"));
        return RETRY;
    }

    QFileInfo info(u->path());

    if (!info.exists()) {
        KMessageBox::sorry(this, i18n("The specified file does not exist"));
        return RETRY;
    }

    if (info.isDir()) {
        KMessageBox::sorry(this, i18n("You have specified a directory"));
        return RETRY;
    }

    QFile file(u->path());

    if (!file.open(IO_ReadOnly)) {
        KMessageBox::sorry(this, i18n("You do not have read permission to this file."));
        return RETRY;
    }

    m_doc->closeDocument();
    m_doc->openDocument(u->path());
    initView();

    return OK;
}


RosegardenGUIDoc *RosegardenGUIApp::getDocument() const
{
    return m_doc;
}

void RosegardenGUIApp::saveOptions()
{	
    m_config->setGroup("General Options");
    m_config->writeEntry("Geometry", size());
    m_config->writeEntry("Show Toolbar", toolBar()->isVisible());
    m_config->writeEntry("Show Statusbar",statusBar()->isVisible());
    m_config->writeEntry("ToolBarPos", (int) toolBar()->barPos());

    m_fileRecent->saveEntries(m_config);
}


void RosegardenGUIApp::readOptions()
{
	
    m_config->setGroup("General Options");

//     bool viewStatusbar = m_config->readBoolEntry("Show Statusbar", true);
//     if(viewStatusbar) {
//         enableStatusBar(KStatusBar::Hide);
//     } else {
//         enableStatusBar(KStatusBar::Show);
//     }

//     bool viewToolbar = m_config->readBoolEntry("Show Toolbar", true);
//     if(viewToolbar) {
//         enableToolBar(KToolBar::Hide);
//     } else {
//         enableToolBar(KToolBar::Show);
//     }

//     // bar position settings
//     KToolBar::BarPosition toolBarPos;
//     toolBarPos=(KToolBar::BarPosition) m_config->readNumEntry("ToolBarPos", KToolBar::Top);
//     toolBar()->setBarPos(toolBarPos);
	
    // initialize the recent file list
    //
    m_fileRecent->loadEntries(m_config);

    QSize size(m_config->readSizeEntry("Geometry"));

    if(!size.isEmpty()) {
        resize(size);
    }
}

void RosegardenGUIApp::saveProperties(KConfig *_cfg)
{
    if (m_doc->getTitle()!=i18n("Untitled") && !m_doc->isModified()) {
        // saving to tempfile not necessary
    } else {
        QString filename=m_doc->getAbsFilePath();	
        _cfg->writeEntry("filename", filename);
        _cfg->writeEntry("modified", m_doc->isModified());
		
        QString tempname = kapp->tempSaveName(filename);
        m_doc->saveDocument(tempname);
    }
}


void RosegardenGUIApp::readProperties(KConfig* _cfg)
{
    QString filename = _cfg->readEntry("filename", "");
    bool modified = _cfg->readBoolEntry("modified", false);

    if (modified) {
            bool canRecover;
            QString tempname = kapp->checkRecoverFile(filename, canRecover);
  	
            if (canRecover) {
                m_doc->openDocument(tempname);
                m_doc->setModified();
                QFileInfo info(filename);
                m_doc->setAbsFilePath(info.absFilePath());
                m_doc->setTitle(info.fileName());
                QFile::remove(tempname);
            }
        } else {
            if (!filename.isEmpty()) {
                m_doc->openDocument(filename);
            }
        }

    QString caption=kapp->caption();
    setCaption(caption+": "+m_doc->getTitle());
}		

bool RosegardenGUIApp::queryClose()
{
    return m_doc->saveIfModified();
}

bool RosegardenGUIApp::queryExit()
{
    saveOptions();
    return true;
}

/////////////////////////////////////////////////////////////////////
// SLOT IMPLEMENTATION
/////////////////////////////////////////////////////////////////////

// Not connected to anything at the moment
//
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

    if (!m_doc->saveIfModified()) {
        // here saving wasn't successful

    } else {	
        m_doc->newDocument();		

        QString caption=kapp->caption();	
        setCaption(caption+": "+m_doc->getTitle());
    }

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

int RosegardenGUIApp::openURL(const KURL& url, int mode)
{
    QString netFile = url.url();
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::openURL: " << netFile << endl;

    if (url.isMalformed()) {
        QString string;
        string = i18n( "Malformed URL\n%1").arg(netFile);

        KMessageBox::sorry(this, string);
        return USER_ERROR;
    }

    QString target;

    if (KIO::NetAccess::download(url, target) == false) {
        KMessageBox::error(this, i18n("Cannot download file!"));
        return OS_ERROR;
    }

    int result = openFile(target, 0);
    if (result == OK) {
        setCaption(url.path());
        m_fileRecent->addURL(url);
        //         setGeneralStatusField(i18n("Done"));
    }

    return OK;
}

void RosegardenGUIApp::slotFileOpen()
{
    slotStatusMsg(i18n("Opening file..."));

    while( 1 ) {

        KURL url = KFileDialog::getOpenURL(QString::null, "*.xml", this,
                                           i18n("Open File"));
        if ( url.isEmpty() ) { return; }

        QString tmpfile;
        KIO::NetAccess::download( url, tmpfile );
        int result = openFile( tmpfile, 0 );
        KIO::NetAccess::removeTempFile( tmpfile );

        if (result == OK) {

            setCaption(url.path());
            m_fileRecent->addURL( url );
//             setGeneralStatusField(i18n("Done"));
//             statusbar_slot();
            break;

        } else if (result == RETRY) {
        }
    }
    
    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));

//     initView();
}

void RosegardenGUIApp::slotFileOpenRecent(const KURL &url)
{
    slotStatusMsg(i18n("Opening file..."));
	
//     if (!doc->saveIfModified()) {
//         // here saving wasn't successful
//     } else {
//         doc->closeDocument();
//         doc->openDocument(recentFiles.at(id_));
//         QString caption=kapp->caption();	
//         setCaption(caption+": "+doc->getTitle());
//     }

//     initView();

    openURL(url, 0);

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotFileSave()
{
    slotStatusMsg(i18n("Saving file..."));
	
    m_doc->saveDocument(m_doc->getAbsFilePath());

    slotStatusMsg(i18n(IDS_STATUS_DEFAULT));
}

void RosegardenGUIApp::slotFileSaveAs()
{
    slotStatusMsg(i18n("Saving file with a new filename..."));

    kdDebug(KDEBUG_AREA) << "slotFileSaveAs()" << endl;

//     QString newName=KFileDialog::getSaveFileName(QDir::currentDirPath(),
//                                                  i18n("*.xml"), this, i18n("Save as..."));
//     if (!newName.isEmpty())
//         {
//             QFileInfo saveAsInfo(newName);
//             m_doc->setTitle(saveAsInfo.fileName());
//             m_doc->setAbsFilePath(saveAsInfo.absFilePath());
//             m_doc->saveDocument(newName);
//             m_fileRecent->addFile(newName);

//             QString caption=kapp->caption();	
//             setCaption(caption+": "+m_doc->getTitle());
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
        m_view->print(&printer);
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
    if (memberList) {

        for(w=memberList->first(); w!=0; w=memberList->first()) {
            // only close the window if the closeEvent is accepted. If the user presses Cancel on the saveIfModified() dialog,
            // the window and the application stay open.
            if (!w->close())
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

void RosegardenGUIApp::slotWhole()
{
    kdDebug(KDEBUG_AREA) << "RosegardenGUIApp::slotWhole()\n";
}
