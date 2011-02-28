
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

#ifndef _RG_PASTENOTATIONDIALOG_H_
#define _RG_PASTENOTATIONDIALOG_H_

#include "commands/edit/PasteEventsCommand.h"
#include <QDialog>
#include <vector>


class QWidget;
class QRadioButton;
class QCheckBox;


namespace Rosegarden
{



class PasteNotationDialog : public QDialog
{
    Q_OBJECT

public:
    PasteNotationDialog(QWidget *parent,
                        PasteEventsCommand::PasteType defaultType);

    PasteEventsCommand::PasteType getPasteType() const;
    bool setAsDefault() const;

public slots:
    void slotPasteTypeChanged();
    void slotHelpRequested();

protected:

    //--------------- Data members ---------------------------------

    std::vector<QRadioButton *> m_pasteTypeButtons;
    QCheckBox *m_setAsDefaultButton;

    PasteEventsCommand::PasteType m_defaultType;
};



}

#endif
