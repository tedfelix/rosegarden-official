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

#ifndef EDITIONVIEW_H
#define EDITIONVIEW_H

#include <vector>

#include <kmainwindow.h>

namespace Rosegarden { class Segment; }

class RosegardenGUIDoc;
class EditTool;
class EditToolBox;

class EditView : public KMainWindow
{
    static const unsigned int ID_STATUS_MSG;

    Q_OBJECT
public:
    EditView(RosegardenGUIDoc *doc,
             std::vector<Rosegarden::Segment *> segments,
             QWidget *parent);
    virtual ~EditView();

    const RosegardenGUIDoc *getDocument() const { return m_document; }
    RosegardenGUIDoc *getDocument() { return m_document; }

    virtual bool applyLayout(int staffNo = -1) = 0;

    /**
     * "Clever" readjustment of the view size
     * If the new size is larger, enlarge to that size plus a margin
     * If it is smaller, only shrink if the reduction is significant
     * (e.g. new size is less than 75% of the old one)
     */
    virtual void readjustViewSize(QSize newSize);

public slots:
    /**
     * close window
     */
    virtual void closeWindow();

//     /**
//      * undo
//      */
//     virtual void slotEditUndo() = 0;

//     /**
//      * redo
//      */
//     virtual void slotEditRedo() = 0;
    
    /**
     * put the indicationed text/object into the clipboard and remove * it
     * from the document
     */
    virtual void slotEditCut() = 0;

    /**
     * put the indicationed text/object into the clipboard
     */
    virtual void slotEditCopy() = 0;

    /**
     * paste the clipboard into the document
     */
    virtual void slotEditPaste() = 0;

    /**
     * toggles the main toolbar
     */
    virtual void slotToggleToolBar();

    /**
     * toggles the statusbar
     */
    virtual void slotToggleStatusBar();

    /** 
     * Changes the statusbar contents for the standard label permanently,
     * used to indicate current actions.
     *
     * @param text the text that is displayed in the statusbar
     */
    virtual void slotStatusMsg(const QString &text);

    /**
     * Changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    virtual void slotStatusHelpMsg(const QString &text);

protected:

    /**
     * Set the current Notation tool (note inserter, rest inserter, eraser...)
     *
     * Called when the user selects a new item on one of the notation toolbars
     * (notes toolbars, rests toolbars...)
     */
    void setTool(EditTool*);

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void saveOptions() = 0;

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions() = 0;

    /**
     * create menus and toolbars
     */
    virtual void setupActions() = 0;

    /**
     * setup status bar
     */
    virtual void initStatusBar() = 0;


    /**
     * Abstract method to get the view size
     * Typically implemented as canvas()->size().
     */
    virtual QSize getViewSize() = 0;

    /**
     * Abstract method to set the view size
     * Typically implemented as canvas()->resize().
     */
    virtual void setViewSize(QSize) = 0;

    //--------------- Data members ---------------------------------

    KConfig* m_config;

    RosegardenGUIDoc* m_document;

    EditTool*    m_tool;
    EditToolBox* m_toolBox;

};

#endif
