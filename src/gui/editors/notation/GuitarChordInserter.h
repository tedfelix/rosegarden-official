/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_GUITAR_CHORD_INSERTER_H
#define RG_GUITAR_CHORD_INSERTER_H

#include "NotationTool.h"
#include "base/Event.h"


namespace Rosegarden
{

class ViewElement;
class NotationWidget;
class GuitarChordSelectorDialog;
class NotationStaff;

/**
 * This tool will insert guitar chord on mouse click events
*/
class GuitarChordInserter : public NotationTool
{
    Q_OBJECT

    friend class NotationToolBox;

public:
    void ready() override;

    void handleLeftButtonPress(const NotationMouseEvent *) override;

    /**
     * Useful to get the tool name from a NotationTool object
     */ 
    const QString getToolName() override { return ToolName(); }

    bool needsWheelEvents() override { return false; }

    static QString ToolName();

protected slots:
    void slotGuitarChordSelected();
    void slotEraseSelected();
    void slotSelectSelected();
    void slotNotesSelected();

protected:
    GuitarChordSelectorDialog *m_guitarChordSelector;

    GuitarChordInserter(NotationWidget *);

private:
    void handleSelectedGuitarChord(const NotationMouseEvent *e);
    void createNewGuitarChord(const NotationMouseEvent *e);

    bool processDialog(NotationStaff *staff, timeT &insertionTime);
};

}

#endif
