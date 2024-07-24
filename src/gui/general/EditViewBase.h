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
    EditViewBase(const std::vector<Segment *> &segments,
                 QWidget *parent);

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

protected slots:

    /// Edit > Cut.
    virtual void slotEditCut() = 0;
    /// Edit > Copy.
    virtual void slotEditCopy() = 0;
    /// Edit > Paste.
    virtual void slotEditPaste() = 0;
    /// View > Show Statusbar
    virtual void slotToggleStatusBar();

    /**
     * A command has happened; check the clipboard in case we
     * need to change state
     */
    virtual void slotTestClipboard();

    /// Move > Solo.  The "S" button.
    virtual void slotToggleSolo();

    virtual void slotOpenInMatrix();
    virtual void slotOpenInPercussionMatrix();
    virtual void slotOpenInNotation();
    virtual void slotOpenInEventList();
    // ??? This appears broken.  It emits a signal that no one connects to.
    virtual void slotOpenInPitchTracker();

protected:

    /**
     * Changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusHelpMsg(const QString &text);

    Clipboard *getClipboard();

    /**
     * @see #setInCtor
     */
    void closeEvent(QCloseEvent *e) override;

    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    /**
     * Helper to set checkboxes for visibility of toolbars
     */
    void setCheckBoxState(const QString &actionName,
                          const QString &toolbarName);

    /**
     * create menus and toolbars
     */
    virtual void setupBaseActions(bool haveClipboard);

    /**
     * setup status bar
     */
    virtual void initStatusBar() = 0;

    /**
     * Abstract method to get current segment
     */
    virtual Segment *getCurrentSegment() = 0;

    /**
     * Set the caption of the view's window
     */
    virtual void updateViewCaption() = 0;

protected slots:
    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void slotSaveOptions();
    virtual void slotConfigure();

protected:

    /// form a suitable title string for the window
    QString getTitle(const QString &view);

    /// The Segment(s) that are being edited.
    std::vector<Segment *> m_segments;

    /// We need this so that we can attach/detach from the same document.
    /**
     * DO NOT REMOVE!  Prefer RosegardenDocument::currentDocument to this
     * if that makes sense.  Only use this for connection/disconnection.
     */
    RosegardenDocument *m_doc;

private slots:

    void slotSetSegmentStartTime();
    void slotSetSegmentDuration();

    /// File > Close
    void slotCloseWindow();

};


}

#endif
