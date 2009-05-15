/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NEW_NOTATION_VIEW_H_
#define _RG_NEW_NOTATION_VIEW_H_

#include <vector>
#include <QMainWindow>

#include "gui/general/ActionFileClient.h"
#include "gui/general/SelectionManager.h"
#include "gui/general/EditViewBase.h"

class QWidget;

namespace Rosegarden
{

class RosegardenDocument;
class NotationWidget;
class Segment;
class CommandRegistry;
	
class NewNotationView : public EditViewBase,
                        public SelectionManager
{
    Q_OBJECT

public:
    NewNotationView(RosegardenDocument *doc,
                    std::vector<Segment *> segments,
                    QWidget *parent = 0);

    virtual ~NewNotationView();

    virtual Segment *getCurrentSegment();
    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection* s, bool preview);

    virtual void initStatusBar() { }//!!!
    virtual void updateViewCaption() { }//!!!
    virtual timeT getInsertionTime() { return 0; }//!!!

protected slots:
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotEditDelete();
    void slotEditCutAndClose();
    void slotEditGeneralPaste();

    void slotSetSelectTool();
    void slotSetEraseTool();

private:
    RosegardenDocument *m_document;
    NotationWidget *m_notationWidget;
    CommandRegistry *m_commandRegistry;

    void setupActions();
    void setMenuStates();
};

}

#endif
