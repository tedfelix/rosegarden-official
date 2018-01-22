
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SELECTIONPROPERTYCOMMAND_H
#define RG_SELECTIONPROPERTYCOMMAND_H

#include "base/PropertyName.h"
#include "base/parameterpattern/ParameterPattern.h"
#include "document/BasicSelectionCommand.h"
#include <QString>
#include <QCoreApplication>


class Set;


namespace Rosegarden
{

class SelectionPropertyCommand : public BasicSelectionCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::SelectionPropertyCommand)

public:

    SelectionPropertyCommand(ParameterPattern::Result result);

    static QString getGlobalName() { return tr("Set &Property"); }

    virtual void modifySegment();

private:
    ParameterPattern::Result m_result;
};


}

#endif
