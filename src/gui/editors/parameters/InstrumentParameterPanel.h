
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_INSTRUMENTPARAMETERPANEL_H_
#define _RG_INSTRUMENTPARAMETERPANEL_H_

#include "gui/widgets/InstrumentAliasButton.h"
#include <QFrame>
#include <vector>
#include <utility>

class QWidget;
class QLabel;


namespace Rosegarden
{

class RosegardenDocument;
class Instrument;
class Rotary;

typedef std::pair<Rotary *, QLabel *> RotaryPair;
typedef std::vector<std::pair<int, RotaryPair> > RotaryMap;


////////////////////////////////////////////////////////////////////////

class InstrumentParameterPanel : public QFrame
{
    Q_OBJECT
public:
    InstrumentParameterPanel(RosegardenDocument *doc, QWidget* parent);

    virtual ~InstrumentParameterPanel() {};

    virtual void setupForInstrument(Instrument*) = 0;

    void setDocument(RosegardenDocument* doc);

    void showAdditionalControls(bool showThem);

signals:
    void updateAllBoxes();
        
protected:
    //--------------- Data members ---------------------------------
    QLabel                          *m_instrumentLabel;
    Instrument                      *m_selectedInstrument;
    RosegardenDocument              *m_doc;
    InstrumentAliasButton           *m_aliasButton;
};



}

#endif
