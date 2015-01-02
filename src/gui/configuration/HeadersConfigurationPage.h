
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

#ifndef RG_HEADERSCONFIGURATIONPAGE_H
#define RG_HEADERSCONFIGURATIONPAGE_H

#include "gui/widgets/LineEdit.h"

#include <QWidget>

class QWidget;
class LineEdit;
class QTableWidget;

namespace Rosegarden
{

class RosegardenDocument;

class HeadersConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    HeadersConfigurationPage(QWidget *parent = 0,
	       RosegardenDocument *doc = 0);

public slots:
    void apply();

protected slots:
    void slotAddNewProperty();
    void slotDeleteProperty();
 
protected:
    RosegardenDocument *m_doc;
 
    // Header fields
    LineEdit *m_editDedication;
    LineEdit *m_editTitle;
    LineEdit *m_editSubtitle;
    LineEdit *m_editSubsubtitle;
    LineEdit *m_editPoet;
    LineEdit *m_editComposer;
    LineEdit *m_editMeter;
    LineEdit *m_editOpus;
    LineEdit *m_editArranger;
    LineEdit *m_editInstrument;
    LineEdit *m_editPiece;
    LineEdit *m_editCopyright;
    LineEdit *m_editTagline;

    QTableWidget *m_metadata;
};


}

#endif
