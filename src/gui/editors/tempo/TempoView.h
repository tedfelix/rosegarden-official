/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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
#include "gui/general/EditViewBase.h"
#include "base/TimeT.h"

#include <QString>

class QTreeWidgetItem;
class QCloseEvent;
class QCheckBox;
class QGroupBox;
class QTreeWidget;
class QFrame;
class QGridLayout;

#include <vector>


namespace Rosegarden
{


class Segment;
class EditTempoController;


/// Tempo and Time Signature Editor
class TempoView : public EditViewBase, public CompositionObserver
{
    Q_OBJECT

public:
    TempoView(EditTempoController *editTempoController,
              timeT openTime);
    ~TempoView() override;

    // CompositionObserver overrides
    void timeSignatureChanged(const Composition *) override;
    void tempoChanged(const Composition *) override;

signals:

    /// Connected to RosegardenMainWindow::slotTempoViewClosed().
    void closing();

public slots:

    // ??? That these three are pure virtual and empty here is suspicious.
    //     Feels like a case of something in EditViewBase that doesn't
    //     belong there.  Consider pushing everything related to these
    //     down to the derivers.  Would that be a huge burden?
    void slotEditCut() override  { }
    void slotEditCopy() override  { }
    void slotEditPaste() override  { }

    /// Edit > Delete (or the delete key)
    void slotEditDelete();
    /// Edit > Add Tempo Change
    void slotAddTempoChange();
    /// Edit > Add Time Signature Change
    void slotAddTimeSignatureChange();
    /// Edit > Edit Item
    /**
     * See slotPopupEditor().
     */
    void slotEditItem();

    /// Edit > Select All
    void slotSelectAll();
    /// Edit > Clear Selection
    void slotClearSelection();

    /// View > Musical Times
    void slotViewMusicalTimes();
    /// View > Real Times
    void slotViewRealTimes();
    /// View > Raw Times
    void slotViewRawTimes();

    /// Help > Help
    void slotHelpRequested();
    /// Help > About
    void slotHelpAbout();

    /// Double-click entry.
    /**
     * See slotEditItem().
     */
    void slotPopupEditor(QTreeWidgetItem *twi, int column = 0);

    /// Filter check box clicked.
    void slotModifyFilter(int);

protected slots:

    /// Connected to RosegardenDocument::documentModified().
    /**
     * ??? See slotDocumentModified() and combine.
     */
    void slotUpdateWindowTitle(bool modified);

protected:

    // EditViewBase override.
    Segment *getCurrentSegment() override;

    // QWidget override.
    void closeEvent(QCloseEvent *) override;

private slots:

    /// Connected to RosegardenDocument::documentModified().
    /**
     * ??? See slotUpdateWindowTitle() and combine.
     */
    void slotDocumentModified(bool modified);

private:

    QFrame *m_frame;
    QGridLayout *m_mainLayout;

    void initMenu();

    // ??? rename: refreshList(), updateList()
    bool updateList();

    /// Select the Event nearest the playback position pointer.
    void makeInitialSelection(timeT time);

    // ??? toString().  Seems useful.  See if there are multiple copies
    //     and pull out into someplace handy.  Like TimeT.h.
    QString makeTimeString(timeT time, int timeMode);

    /// Used to launch the TempoDialog.
    /**
     * Used to launch the TempoDialog to get a tempo from the user when adding
     * or editing a tempo change.
     *
     * ??? Why keep a copy when it has a self()?  Get rid of this and use
     *     EditTempoController::self().
     */
    EditTempoController *m_editTempoController;

    // Widgets

    // Filter
    QGroupBox *m_filterGroup;
    QCheckBox *m_tempoCheckBox;
    QCheckBox *m_timeSigCheckBox;
    // ??? Why not just a couple of bools?  Then store as two settings in
    //     the .conf file.  That would simplify so many things.
    int m_filter;
    /// State (m_filter) -> Filter Check Boxes.
    /**
     * ??? Inline into only caller, the ctor.
     */
    void updateFilterCheckBoxes();

    // List
    // ??? QTreeWidget seems like overkill.  We never have sub items.
    //     What can we replace this with?
    QTreeWidget *m_list;
    std::vector<int> m_listSelection;

    /// Turn off CompositionObserver handlers.
    /**
     * ??? See if we can get rid of this by simplifying how changes are
     *     handled.
     */
    bool m_ignoreUpdates;
};


}

#endif
