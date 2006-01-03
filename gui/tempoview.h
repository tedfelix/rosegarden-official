// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _TEMPOVIEW_H_
#define _TEMPOVIEW_H_

#include "editviewbase.h"
#include "dialogs.h"
#include "Composition.h"
#include <vector>

/**
 * Tempo and time signature list-style editor.  This has some code
 * in common with EventView, but not enough to make them any more
 * shareable than simply through EditViewBase.  Hopefully this one
 * should prove considerably simpler, anyway.
 */

class QListViewItem;
class KListView;
class QButtonGroup;
class QCheckBox;

class TempoView : public EditViewBase, public Rosegarden::CompositionObserver
{
    Q_OBJECT

    enum Filter
    {
	None          = 0x0000,
	Tempo         = 0x0001,
	TimeSignature = 0x0002
    };

public:
    TempoView(RosegardenGUIDoc *doc, QWidget *parent, Rosegarden::timeT);
    virtual ~TempoView();

    static const char* const ConfigGroup;

    virtual bool applyLayout(int staffNo = -1);

    virtual void refreshSegment(Rosegarden::Segment *segment,
				Rosegarden::timeT startTime = 0,
				Rosegarden::timeT endTime = 0);

    virtual void updateView();

    virtual void setupActions();
    virtual void initStatusBar();
    virtual QSize getViewSize(); 
    virtual void setViewSize(QSize);

    // Set the button states to the current filter positions
    //
    void setButtonsToFilter();

    // Menu creation and show
    //
    void createMenu();

    // Composition Observer callbacks
    //
    virtual void timeSignatureChanged(const Rosegarden::Composition *);
    virtual void tempoChanged(const Rosegarden::Composition *);

signals:
    // forwarded from tempo dialog:
    void changeTempo(Rosegarden::timeT,  // tempo change time
                     Rosegarden::tempoT,  // tempo value
                     TempoDialog::TempoDialogAction); // tempo action

public slots:
    // standard slots
    virtual void slotEditCut();
    virtual void slotEditCopy();
    virtual void slotEditPaste();

    // other edit slots
    void slotEditDelete();
    void slotEditInsertTempo();
    void slotEditInsertTimeSignature();
    void slotEdit();

    void slotSelectAll();
    void slotClearSelection();

    void slotMusicalTime();
    void slotRealTime();
    void slotRawTime();

    // on double click on the event list
    //
    void slotPopupEditor(QListViewItem*);

    // Change filter parameters
    //
    void slotModifyFilter(int);

protected slots:

    virtual void slotSaveOptions();

protected:

    virtual void readOptions();
    void makeInitialSelection(Rosegarden::timeT);
    QString makeTimeString(Rosegarden::timeT time, int timeMode);
    virtual Rosegarden::Segment *getCurrentSegment();
    virtual void updateViewCaption();

    //--------------- Data members ---------------------------------
    KListView   *m_list;
    int          m_filter;

    static int   m_lastSetFilter;

    QButtonGroup   *m_filterGroup;
    QCheckBox      *m_tempoCheckBox;
    QCheckBox      *m_timeSigCheckBox;

    static const char* const LayoutConfigGroupName;

    std::vector<int> m_listSelection;

    bool m_ignoreUpdates;
};


#endif

