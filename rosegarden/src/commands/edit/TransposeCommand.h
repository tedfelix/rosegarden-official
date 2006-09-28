
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_TRANSPOSECOMMAND_H_
#define _RG_TRANSPOSECOMMAND_H_

#include "document/BasicSelectionCommand.h"
#include <qstring.h>
#include <klocale.h>




namespace Rosegarden
{

class EventSelection;


class TransposeCommand : public BasicSelectionCommand
{
public:
    TransposeCommand(int semitones, EventSelection &selection) :
	BasicSelectionCommand(getGlobalName(semitones), selection, true),
	m_selection(&selection), m_semitones(semitones) { }

    static QString getGlobalName(int semitones = 0) {
	switch (semitones) {
	case   1: return i18n("&Up a Semitone");
	case  -1: return i18n("&Down a Semitone");
	case  12: return i18n("Up an &Octave");
	case -12: return i18n("Down an Octa&ve");
	default:  return i18n("&Transpose...");
	}
    }

protected:
    virtual void modifySegment();

private:
    EventSelection *m_selection;// only used on 1st execute (cf bruteForceRedo)
    int m_semitones;
};



}

#endif
