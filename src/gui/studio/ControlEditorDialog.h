
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

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

#include "base/Device.h"  // for DeviceId
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


/// Manage Controllers dialog
/**
 * Launched from the "Controllers..." button on the Manage MIDI Devices dialog
 * (DeviceManagerDialog).
 */
class ControlEditorDialog : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    ControlEditorDialog(QWidget *parent,
                        RosegardenDocument *doc,
                        DeviceId device);
    ~ControlEditorDialog() override;

    DeviceId getDevice()  { return m_device; }

public slots:
    void slotUpdate(bool added);
    void slotUpdate()  { slotUpdate(false); }

    void slotAdd();
    void slotDelete();
    void slotClose();

    void slotEdit(QTreeWidgetItem *, int);
    void slotHelpRequested();
    void slotHelpAbout();

signals:
    void closing();

protected:
    void closeEvent(QCloseEvent *) override;

private:
    RosegardenDocument *m_doc;
    Studio *m_studio;
    DeviceId m_device;

    void setupActions();
    void initDialog();

    QTreeWidget *m_treeWidget;

    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QPushButton *m_closeButton;

    bool m_modified;

    void setModified(bool value);
    void addCommandToHistory(Command *command);

};


}

#endif
