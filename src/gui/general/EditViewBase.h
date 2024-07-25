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

#ifndef RG_EDITVIEWBASE_H
#define RG_EDITVIEWBASE_H

#include "base/Event.h"
#include "ActionFileClient.h"

#include <QMainWindow>
#include <QString>

class QCloseEvent;

#include <set>
#include <string>
#include <vector>


namespace Rosegarden
{


class Clipboard;
class Command;
class RosegardenDocument;
class Segment;
class Event;


class EditViewBase : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:

    EditViewBase(const std::vector<Segment *> &segments);

    ~EditViewBase() override;

signals:

    /// File > Save
    void saveFile();

    /// Segment > Edit With > Open in Notation Editor
    void openInNotation(std::vector<Segment *>);
    /// Segment > Edit With > Open in Matrix Editor
    void openInMatrix(std::vector<Segment *>);
    /// Segment > Edit With > Open in Percussion Matrix Editor
    void openInPercussionMatrix(std::vector<Segment *>);
    /// Segment > Edit With > Open in Event List Editor
    void openInEventList(std::vector<Segment *>);
    /// Segment > Edit With > Open in Pitch Tracker
    /**
     * ??? RMVW never connects to this.  This goes nowhere.
     */
    void openInPitchTracker(std::vector<Segment *>);

    /// Tell RMVW we want a Track selected (by TrackId).  See slotToggleSolo().
    void selectTrack(int trackId);
    /// Tell RMW to toggle solo.  See slotToggleSolo().
    void toggleSolo(bool);

public slots:

    /// Handle Composition changes.
    /**
     * Responds to changes in the Composition by updating the modified star
     * in the titlebar.  See updateViewCaption().
     */
    virtual void slotCompositionStateUpdate();

protected:

    /// Display message in status bar.
    /**
     * Wrapper around QStatusBar::showMessage().
     *
     * Changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menu entries.
     */
    void showStatusBarMessage(const QString &text);

    /// Update "View (> Toolbars) > Show Statusbar" checkbox.
    void readOptions();

    /// Set check box for visibility of toolbar.
    void setCheckBoxState(const QString &actionName,
                          const QString &toolbarName);

    /// Create actions for menus and toolbars that are managed by this class.
    void setupBaseActions(bool haveClipboard);

    virtual Segment *getCurrentSegment() = 0;

    /// Composition has changed.  Update the titlebar.
    /**
     * Set the caption of the view's window
     *
     * Called by slotCompositionStateUpdate().  Assumption is that something
     * about the Composition has changed.  Modified state?
     *
     * - TempoView sets the window title and filename.  No modified star.
     * - EventView sets the window title, filename, and modified star.
     * - All others ignore this.
     *
     * ??? Get rid of purity (= 0).  Allow Matrix and Notation to ignore.
     * ??? Can TempoView just set the titlebar in its ctor and ignore this?
     *     Is that what MatrixView and NotationView are doing?
     * ??? If EventView is the only user, can it get this info by some other
     *     route?  Then we can get rid of this.
     */
    virtual void updateViewCaption() = 0;

    /// Override to write things to the conf file at close.
    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     *
     * ??? It looks like TempoView and EventView implement this and
     *     call it on close.  There is no need for EventViewBase's
     *     dtor to call this.  And there is no need for this virtual
     *     function.  Get rid of this.
     */
    virtual void saveOptions()  { }

    /// Assemble a title for the window.
    /**
     * This assembles a combination of the modification star (*), the
     * segments being edited, and the name of the editor into a string
     * suitable for using as the window title.
     */
    QString getTitle(const QString &editorName);

    /// The Segment(s) that are being edited.
    std::vector<Segment *> m_segments;

    /// We need this so that we can attach/detach from the same document.
    /**
     * DO NOT REMOVE!  Prefer RosegardenDocument::currentDocument to this
     * if that makes sense.  Only use this for connection/disconnection.
     */
    RosegardenDocument *m_doc;

protected slots:

    /// Edit > Cut.
    virtual void slotEditCut() = 0;
    /// Edit > Copy.
    virtual void slotEditCopy() = 0;
    /// Edit > Paste.
    virtual void slotEditPaste() = 0;
    /// View > Show Statusbar
    void slotToggleStatusBar();

    /// Update clipboard action state.
    /**
     * A command has happened; check the clipboard in case we
     * need to change action state.
     *
     * ??? rename: slotUpdateClipboardActionState()
     */
    void slotTestClipboard();

    /// Move > Solo.  The "S" button.
    void slotToggleSolo();

    /// Edit > Preferences...
    virtual void slotConfigure();

private slots:

    /// Segment/Edit > Set Start Time...
    void slotSetSegmentStartTime();
    /// Segment/Edit > Set Duration...
    void slotSetSegmentDuration();

    /// Segment > Edit With > Open in Matrix Editor
    void slotOpenInMatrix();
    /// Segment > Edit With > Open in Percussion Matrix Editor
    void slotOpenInPercussionMatrix();
    /// Segment > Edit With > Open in Notation Editor
    void slotOpenInNotation();
    /// Segment > Edit With > Open in Event List Editor
    void slotOpenInEventList();
    /// Segment > Edit With > Open in Pitch Tracker
    void slotOpenInPitchTracker();

    /// File > Close
    void slotCloseWindow();

};


}

#endif
