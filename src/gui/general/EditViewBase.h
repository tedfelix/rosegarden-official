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

#ifndef RG_EDITVIEWBASE_H
#define RG_EDITVIEWBASE_H

#include <set>
#include <string>
#include <vector>
#include "base/Event.h"
#include "ActionFileClient.h"

#include <QMainWindow>
#include <QString>

class QShortcut;
class QCloseEvent;

namespace Rosegarden
{
class Clipboard;
class Command;
class Segment;
class RosegardenDocument;
class Event;

 
class EditViewBase : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    EditViewBase(RosegardenDocument *doc,
                 std::vector<Segment *> segments,
                 QWidget *parent);

    ~EditViewBase() override;

    const RosegardenDocument *getDocument() const { return m_doc; }
    RosegardenDocument *getDocument() { return m_doc; }

    Clipboard *getClipboard();

    /**
     * Return our local shortcut object
     */
    QShortcut* getShortcuts() { return m_shortcuts; }

    /**
     * Let tools know if their current element has gone
     */
    virtual void handleEventRemoved(Event *event);

signals:
    /**
     * Tell the app to save the file.
     */
    void saveFile();

    /** 
     * Reopen the given segments in another sort of editor.
     */
    void openInNotation(std::vector<Segment *>);
    void openInMatrix(std::vector<Segment *>);
    void openInPercussionMatrix(std::vector<Segment *>);
    void openInEventList(std::vector<Segment *>);
    void slotOpenInPitchTracker(std::vector<Segment *>);
    
    /**
     * Tell the main view that the track being edited is the
     * current selected track
     * This is used by #slotToggleSolo
     */
    void selectTrack(int);

    /**
     * Tell the main view that the solo status has changed (the user clicked on the 'solo' toggle)
     */
    void toggleSolo(bool);

    //void windowActivated();

public slots:
    /**
     * close window
     */
    virtual void slotCloseWindow();

    /**
     * put the indicationed text/object into the clipboard and remove * it
     * from the document
     */
    virtual void slotEditCut() = 0;

    /**
     * put the indicationed text/object into the clipboard
     */
    virtual void slotEditCopy() = 0;

    /**
     * paste the clipboard into the document
     */
    virtual void slotEditPaste() = 0;

    /**
     * toggles the main toolbar
     */
//    virtual void slotToggleToolBar();

    /**
     * toggles the statusbar
     */
    virtual void slotToggleStatusBar();

    /** 
     * Changes the statusbar contents for the standard label permanently,
     * used to indicate current actions.
     *
     * @param text the text that is displayed in the statusbar
     */
    virtual void slotStatusMsg(const QString &text);

    /**
     * Changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    virtual void slotStatusHelpMsg(const QString &text);

    /**
     * A command has happened; check the clipboard in case we
     * need to change state
     */
    virtual void slotTestClipboard();

    /**
     * Connected to this view's toolbar 'solo' button
     */
    virtual void slotToggleSolo();

    virtual void slotOpenInMatrix();
    virtual void slotOpenInPercussionMatrix();
    virtual void slotOpenInNotation();
    virtual void slotOpenInEventList();
    virtual void slotOpenInPitchTracker();

    virtual void slotSegmentDeleted(Segment *);
    
    /**
     * Set the start time of the current segment
     */
    void slotSetSegmentStartTime();

    /**
     * Set the duration of the current segment
     */
    void slotSetSegmentDuration();

    /**
     * Global composition updates from the main view (track selection, solo, etc...)
     */
    virtual void slotCompositionStateUpdate();
    
protected:

    /**
     * @see #setInCtor
     */
    void closeEvent(QCloseEvent* e) override;
    
    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    /**
     * Helper to set checkboxes for visibility of toolbars
     */
    void setCheckBoxState(QString actionName, QString toolbarName);

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
    /**
     * Set the page index of the config dialog which corresponds to
     * this editview
     */
    void setConfigDialogPageIndex(int p) { m_configDialogPageIndex = p; }
    int getConfigDialogPageIndex()       { return m_configDialogPageIndex; }

    RosegardenDocument* m_doc;

    /// The Segment(s) that are being edited.
    std::vector<Segment *> m_segments;

    int m_configDialogPageIndex;

    QShortcut *m_shortcuts;
};


}

#endif
