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

#include <qpushbutton.h>

class RosegardenGUIDoc;

namespace Rosegarden { class Studio; }

class ControlEditorDialog : public KMainWindow
{
    Q_OBJECT

public:
    ControlEditorDialog(QWidget *parent,
                        RosegardenGUIDoc *doc);

    ~ControlEditorDialog();

    void initDialog();

public slots:
    void slotApply();
    void slotReset();
    void slotUpdate();

signals:

protected:
    void setupActions();

    //--------------- Data members ---------------------------------
    Rosegarden::Studio      *m_studio;
    RosegardenGUIDoc        *m_doc;

    QPushButton             *m_closeButton;
    QPushButton             *m_resetButton;
    QPushButton             *m_applyButton;

    KListView               *m_listView;
};

#endif // _CONTROLEDITOR_H_

