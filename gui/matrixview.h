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

#ifndef MATRIXVIEW_H
#define MATRIXVIEW_H

#include <vector>

#include <qcanvas.h>
#include <kmainwindow.h>

#include "editionview.h"

namespace Rosegarden { class Segment; }

class RosegardenGUIDoc;

class MatrixCanvasView : public QCanvasView
{
public:
    MatrixCanvasView(QCanvas *viewing=0, QWidget *parent=0,
                     const char *name=0, WFlags f=0);

    ~MatrixCanvasView();

};

class MatrixView : public EditionView
{
    Q_OBJECT
public:
    MatrixView(RosegardenGUIDoc *doc,
                std::vector<Rosegarden::Segment *> segments,
                QWidget *parent);
    ~MatrixView();

    QCanvas* canvas() { return m_canvasView->canvas(); }

public slots:

    /**
     * undo
     */
    virtual void slotEditUndo();

    /**
     * redo
     */
    virtual void slotEditRedo();
    
    /**
     * put the indicationed text/object into the clipboard and remove * it
     * from the document
     */
    virtual void slotEditCut();

    /**
     * put the indicationed text/object into the clipboard
     */
    virtual void slotEditCopy();

    /**
     * paste the clipboard into the document
     */
    virtual void slotEditPaste();

protected:

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void saveOptions();

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    /**
     * create menus and toolbars
     */
    virtual void setupActions();

    /**
     * setup status bar
     */
    virtual void initStatusBar();

    //--------------- Data members ---------------------------------

    MatrixCanvasView* m_canvasView;
};



#endif
