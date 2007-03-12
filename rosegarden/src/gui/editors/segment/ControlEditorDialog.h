
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CONTROLEDITORDIALOG_H_
#define _RG_CONTROLEDITORDIALOG_H_

#include "base/Device.h"
#include "base/MidiDevice.h"
#include <kmainwindow.h>


class QWidget;
class QPushButton;
class QListViewItem;
class QCloseEvent;
class KListView;
class KCommand;


namespace Rosegarden
{

class Studio;
class RosegardenGUIDoc;
class MultiViewCommandHistory;


class ControlEditorDialog : public KMainWindow
{
    Q_OBJECT

public:
    ControlEditorDialog(QWidget *parent,
                        RosegardenGUIDoc *doc,
                        DeviceId device);

    ~ControlEditorDialog();

    void initDialog();

    void addCommandToHistory(KCommand *command);
    MultiViewCommandHistory* getCommandHistory();

    void setModified(bool value);
    void checkModified();

    // reset the document
    void setDocument(RosegardenGUIDoc *doc);

    DeviceId getDevice() { return m_device; }

public slots:
    void slotUpdate();

/*
    void slotEditCopy();
    void slotEditPaste();
*/

    void slotAdd();
    void slotDelete();
    void slotClose();

    void slotEdit();
    void slotEdit(QListViewItem *);

signals:
    void closing();


protected:
    virtual void closeEvent(QCloseEvent *);

    void setupActions();

    //--------------- Data members ---------------------------------
    Studio      *m_studio;
    RosegardenGUIDoc        *m_doc;
    DeviceId     m_device;

    QPushButton             *m_closeButton;

    QPushButton             *m_copyButton;
    QPushButton             *m_pasteButton;

    QPushButton             *m_addButton;
    QPushButton             *m_deleteButton;

    KListView               *m_listView;

    bool                     m_modified;

    ControlList  m_clipboard; // local clipboard only

};


}

#endif
