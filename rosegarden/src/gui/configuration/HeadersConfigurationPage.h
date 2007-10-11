
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

#ifndef _RG_HEADERSCONFIGURATIONPAGE_H_
#define _RG_HEADERSCONFIGURATIONPAGE_H_

#include <qvbox.h>

class QVBox;
class QWidget;
class QLineEdit;
class KListView;

namespace Rosegarden
{

class RosegardenGUIDoc;

class HeadersConfigurationPage : public QVBox
{
    Q_OBJECT

public:
    HeadersConfigurationPage(QWidget *parent = 0,
	       RosegardenGUIDoc *doc = 0);

public slots:
    void apply();

protected slots:
    void slotAddNewProperty();
    void slotDeleteProperty();
 
protected:
    RosegardenGUIDoc *m_doc;
 
    // Header fields
    QLineEdit *m_editDedication;
    QLineEdit *m_editTitle;
    QLineEdit *m_editSubtitle;
    QLineEdit *m_editSubsubtitle;
    QLineEdit *m_editPoet;
    QLineEdit *m_editComposer;
    QLineEdit *m_editMeter;
    QLineEdit *m_editOpus;
    QLineEdit *m_editArranger;
    QLineEdit *m_editInstrument;
    QLineEdit *m_editPiece;
    QLineEdit *m_editCopyright;
    QLineEdit *m_editTagline;

    KListView *m_metadata;
};


}

#endif
