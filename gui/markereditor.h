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


#ifndef _MARKEREDITOR_H_
#define _MARKEREDITOR_H_

#include <kmainwindow.h>
#include <klistview.h>
#include <kdialogbase.h>

#include <qpushbutton.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qspinbox.h>

#include "Composition.h"
#include "Event.h" // for timeT definition

class RosegardenGUIDoc;
class KCommand;
class MultiViewCommandHistory;
class QLabel;
class QAccel;

class MarkerModifyDialog : public KDialogBase
{
    Q_OBJECT
public:
    MarkerModifyDialog(QWidget *parent,
                       int time,
                       const QString &name,
                       const QString &des);

    QString getName() const { return m_nameEdit->text(); }
    QString getDescription() const { return m_desEdit->text(); }
    int getTime() const { return m_timeEdit->value(); }
    int getOriginalTime() const { return m_originalTime; }

protected:
    RosegardenGUIDoc             *m_doc;

    QSpinBox                     *m_timeEdit;
    QLineEdit                    *m_nameEdit;
    QLineEdit                    *m_desEdit;

    int                           m_originalTime;
};



class MarkerEditorDialog : public KMainWindow
{
    Q_OBJECT

public:
    MarkerEditorDialog(QWidget *parent,
                       RosegardenGUIDoc *doc);
    ~MarkerEditorDialog();

    void initDialog();

    void addCommandToHistory(KCommand *command);
    MultiViewCommandHistory* getCommandHistory();

    void setModified(bool value);
    void checkModified();

    // reset the document
    void setDocument(RosegardenGUIDoc *doc);

    // update pointer position
    void updatePosition();

    QAccel* getAccelerators() { return m_accelerators; }

public slots:
    void slotUpdate();

    void slotAdd();
    void slotDelete();
    void slotDeleteAll();
    void slotClose();
    void slotEdit(QListViewItem *);
    void slotItemClicked(QListViewItem *);

signals:
    void closing();
    void jumpToMarker(Rosegarden::timeT);

protected:
    virtual void closeEvent(QCloseEvent *);

    void setupActions();

    //--------------- Data members ---------------------------------
    RosegardenGUIDoc        *m_doc;

    QLabel                  *m_absoluteTime;
    QLabel                  *m_realTime;
    QLabel                  *m_barTime;

    QPushButton             *m_closeButton;


    QPushButton             *m_addButton;
    QPushButton             *m_deleteButton;
    QPushButton             *m_deleteAllButton;

    KListView               *m_listView;

    bool                     m_modified;

    static const char* const MarkerEditorConfigGroup;

    QAccel *m_accelerators;
};

#endif // _MARKEREDITOR_H_

