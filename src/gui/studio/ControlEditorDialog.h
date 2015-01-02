
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CONTROLEDITORDIALOG_H
#define RG_CONTROLEDITORDIALOG_H

#include "base/Device.h"
#include "base/MidiDevice.h"
#include "gui/general/ActionFileClient.h"

#include <QMainWindow>


class QWidget;
class QPushButton;
class QCloseEvent;
class QTreeWidget;
class QTreeWidgetItem;


namespace Rosegarden
{

class Command;
class Studio;
class RosegardenDocument;
class CommandHistory;


class ControlEditorDialog : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    ControlEditorDialog(QWidget *parent,
                        RosegardenDocument *doc,
                        DeviceId device);

    ~ControlEditorDialog();

    void initDialog();

    void addCommandToHistory(Command *command);
    CommandHistory* getCommandHistory();

    void setModified(bool value);
    void checkModified();

    // reset the document
    void setDocument(RosegardenDocument *doc);

    DeviceId getDevice() { return m_device; }

public slots:
    void slotUpdate(bool added);
    void slotUpdate() { slotUpdate(false); }

/*
    void slotEditCopy();
    void slotEditPaste();
*/

    void slotAdd();
    void slotDelete();
    void slotClose();

    void slotEdit(QTreeWidgetItem *, int);
    void slotHelpRequested();
    void slotHelpAbout();

signals:
    void closing();


protected:
    virtual void closeEvent(QCloseEvent *);

    void setupActions();

    //--------------- Data members ---------------------------------
    Studio                  *m_studio;
    RosegardenDocument      *m_doc;
    DeviceId                 m_device;

    QPushButton             *m_closeButton;

    QPushButton             *m_copyButton;
    QPushButton             *m_pasteButton;

    QPushButton             *m_addButton;
    QPushButton             *m_deleteButton;

    QTreeWidget             *m_treeWidget;

    bool                     m_modified;

    ControlList  m_clipboard; // local clipboard only

};


}

#endif
