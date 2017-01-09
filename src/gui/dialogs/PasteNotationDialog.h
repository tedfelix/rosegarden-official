
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PASTENOTATIONDIALOG_H
#define RG_PASTENOTATIONDIALOG_H

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
    PasteNotationDialog(QWidget *parent);

    PasteEventsCommand::PasteType getPasteType() const;
    static PasteEventsCommand::PasteType getSavedPasteType();

public slots:
    void slotPasteTypeChanged();
    void slotHelpRequested();

protected:
    bool setAsDefault() const;
    virtual void accept();

    //--------------- Data members ---------------------------------

    std::vector<QRadioButton *> m_pasteTypeButtons;
    QCheckBox *m_setAsDefaultButton;

    PasteEventsCommand::PasteType m_defaultType;
};



}

#endif
