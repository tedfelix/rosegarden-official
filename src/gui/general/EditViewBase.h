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

#ifndef RG_EDITVIEWBASE_H
#define RG_EDITVIEWBASE_H

#include "ActionFileClient.h"

#include <QMainWindow>
#include <QString>

#include <vector>


namespace Rosegarden
{


class RosegardenDocument;
class Segment;


class EditViewBase : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:

    explicit EditViewBase(
            const std::vector<Segment *> &segments = std::vector<Segment *>());

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
     * Only Notation and Pitch Tracker offer this menu item.
     */
    void openInPitchTracker(std::vector<Segment *>);

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

    /// Create actions for menus and toolbars that are managed by this class.
    void setupBaseActions();

    virtual Segment *getCurrentSegment() = 0;

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

    /// Make sure the solo button matches the current Track's solo state.
    void updateSoloButton();

protected slots:

    /// View > Show Statusbar
    void slotToggleStatusBar();

    /**
     * Called whenever a command has happened.  Makes sure the clipboard-related
     * actions (menu items and toolbar buttons) are appropriately
     * enabled/disabled.
     */
    void slotUpdateClipboardActionState();

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
