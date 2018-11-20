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

#ifndef RG_TEMPOVIEW_H
#define RG_TEMPOVIEW_H

#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/general/ListEditView.h"
#include <QSize>
#include <QString>
#include <vector>
#include "base/Event.h"


class QWidget;
class QTreeWidgetItem;
class QCloseEvent;
class QCheckBox;
class QGroupBox;
class QTreeWidget;


namespace Rosegarden
{

class Segment;
class RosegardenDocument;
class Composition;
class EditTempoController;

/**
 * Tempo and time signature list-style editor.  This has some code
 * in common with EventView, but not enough to make them any more
 * shareable than simply through EditViewBase.  Hopefully this one
 * should prove considerably simpler, anyway.
 */

class TempoView : public ListEditView, public CompositionObserver
{
    Q_OBJECT

    enum Filter
    {
        None          = 0x0000,
        Tempo         = 0x0001,
        TimeSignature = 0x0002
    };

public:
    TempoView(RosegardenDocument *doc, QWidget *parent, EditTempoController *editTempoController, timeT openTime);
    virtual ~TempoView();

    virtual bool applyLayout(int staffNo = -1);

    void refreshSegment(Segment *segment,
                                timeT startTime = 0,
                                timeT endTime = 0) override;

    void updateView() override;

    virtual void setupActions();
    void initStatusBar() override;
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
    void timeSignatureChanged(const Composition *) override;
    void tempoChanged(const Composition *) override;

signals:
    void closing();

public slots:
    // standard slots
    void slotEditCut() override;
    void slotEditCopy() override;
    void slotEditPaste() override;

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
    void slotHelpRequested();
    void slotHelpAbout();

    // on double click on the event list
    //
    void slotPopupEditor(QTreeWidgetItem*, int col = 0);

    // Change filter parameters
    //
    void slotModifyFilter(int);

protected slots:

    void slotSaveOptions() override;

protected:
    void closeEvent(QCloseEvent *) override;

private:
    void readOptions() override;
    void makeInitialSelection(timeT);
    QString makeTimeString(timeT time, int timeMode);
    Segment *getCurrentSegment() override;
    void updateViewCaption() override;

    //--------------- Data members ---------------------------------
    EditTempoController *m_editTempoController;
    QTreeWidget   *m_list;
    int          m_filter;

    static int   m_lastSetFilter;

    QGroupBox   *m_filterGroup;
    QCheckBox      *m_tempoCheckBox;
    QCheckBox      *m_timeSigCheckBox;

    std::vector<int> m_listSelection;

    bool m_ignoreUpdates;
};



}

#endif
