// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef _CONTROLEDITOR_H_
#define _CONTROLEDITOR_H_

#include <kmainwindow.h>
#include <klistview.h>
#include <kdialogbase.h>

#include <qpushbutton.h>
#include <qstring.h>

#include "MidiDevice.h"
#include "ControlParameter.h"

namespace Rosegarden { class Studio; }
class RosegardenGUIDoc;
class KCommand;
class MultiViewCommandHistory;
class QSpinBox;
class QLineEdit;

class ControlParameterItem : public QListViewItem
{
public:
    ControlParameterItem(int id,
                         QListView *parent,
                         QString str1,
                         QString str2,
                         QString str3,
                         QString str4,
                         QString str5,
                         QString str6,
                         QString str7,
                         QString str8,
                         QString str9):
        QListViewItem(parent, str1, str2, str3, str4, str5, str6, str7, str8),
        m_id(id) { setText(8, str9); }

    int getId() const { return m_id; }

protected:

    int     m_id;
    QString m_string9;
};

class ControlParameterEditDialog : public KDialogBase
{
    Q_OBJECT
public:
    ControlParameterEditDialog(QWidget *parent,
                               Rosegarden::ControlParameter *control,
                               RosegardenGUIDoc *doc);

    Rosegarden::ControlParameter& getControl() { return m_dialogControl; }

public slots: 

    void slotNameChanged(const QString &);
    void slotTypeChanged(int);
    void slotDescriptionChanged(const QString &);
    void slotControllerChanged(int);
    void slotMinChanged(int);
    void slotMaxChanged(int);
    void slotDefaultChanged(int);
    void slotColourChanged(int);
    void slotIPBPositionChanged(int);

protected:
    RosegardenGUIDoc             *m_doc;
    Rosegarden::ControlParameter *m_control;
    Rosegarden::ControlParameter  m_dialogControl;

    QLineEdit                    *m_nameEdit;
    KComboBox                    *m_typeCombo;
    QLineEdit                    *m_description;
    QSpinBox                     *m_controllerBox;
    QSpinBox                     *m_minBox;
    QSpinBox                     *m_maxBox;
    QSpinBox                     *m_defaultBox;
    KComboBox                    *m_colourCombo;
    KComboBox                    *m_ipbPosition;
    QLabel                       *m_hexValue;
};


class ControlEditorDialog : public KMainWindow
{
    Q_OBJECT

public:
    ControlEditorDialog(QWidget *parent,
                        RosegardenGUIDoc *doc,
			Rosegarden::DeviceId device);

    ~ControlEditorDialog();

    void initDialog();

    void addCommandToHistory(KCommand *command);
    MultiViewCommandHistory* getCommandHistory();

    void setModified(bool value);
    void checkModified();

    // reset the document
    void setDocument(RosegardenGUIDoc *doc);

    Rosegarden::DeviceId getDevice() { return m_device; }

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
    Rosegarden::Studio      *m_studio;
    RosegardenGUIDoc        *m_doc;
    Rosegarden::DeviceId     m_device;

    QPushButton             *m_closeButton;

    QPushButton             *m_copyButton;
    QPushButton             *m_pasteButton;

    QPushButton             *m_addButton;
    QPushButton             *m_deleteButton;

    KListView               *m_listView;

    bool                     m_modified;

    Rosegarden::ControlList  m_clipboard; // local clipboard only

    static const char* const ControlEditorConfigGroup;

};

#endif // _CONTROLEDITOR_H_

