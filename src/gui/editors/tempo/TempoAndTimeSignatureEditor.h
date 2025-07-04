/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_TEMPOANDTIMESIGNATUREEDITOR_H
#define RG_TEMPOANDTIMESIGNATUREEDITOR_H

#include "base/Composition.h"
#include "gui/general/EditViewBase.h"
#include "base/TimeT.h"

class QCloseEvent;
class QCheckBox;
class QGroupBox;
class QTableWidget;
class QTableWidgetItem;
class QFrame;
class QHBoxLayout;

#include <vector>


namespace Rosegarden
{


class Segment;


/// Tempo and Time Signature Editor
/**
 * This "2" version uses QTableWidget instead of QTreeWidget.
 */
class TempoAndTimeSignatureEditor : public EditViewBase, public CompositionObserver
{
    Q_OBJECT

public:
    explicit TempoAndTimeSignatureEditor(timeT openTime);
    ~TempoAndTimeSignatureEditor() override;

    // CompositionObserver overrides
    void timeSignatureChanged(const Composition *) override;
    void tempoChanged(const Composition *) override;

signals:

    /// Connected to RosegardenMainWindow::slotTempoViewClosed().
    void closing();

protected:

    // EditViewBase override.
    Segment *getCurrentSegment() override;

    // QWidget override.
    void closeEvent(QCloseEvent *) override;

private slots:

    /// Connected to RosegardenDocument::documentModified().
    void slotDocumentModified(bool modified);

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
    void slotPopupEditor(int row, int col);

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

    /// Filter check box clicked.
    void slotFilterClicked(bool);

    void slotItemSelectionChanged();

private:

    void initMenu();

    void updateWindowTitle();

    // Widgets

    // Filter
    QGroupBox *m_filterGroup;
    QCheckBox *m_tempoCheckBox;
    QCheckBox *m_timeSigCheckBox;

    enum class Type  { TimeSignature, Tempo };

    // List
    QTableWidget *m_tableWidget;
    void updateTable();
    /// Select the item nearest the playback position pointer.
    void makeInitialSelection(timeT time);
    /// Select an item and make sure it is visible.
    void select(timeT time, Type type);

    /// Launch editor for an entry.
    void popupEditor(timeT time, Type type);

    // New item selection support.  Used by popupEditor() to make sure the
    // new/edited item is selected by updateTable().
    bool m_newItemSelect{false};
    timeT m_newItemMIDITicks{};
    Type m_newItemType{};

};


}

#endif
