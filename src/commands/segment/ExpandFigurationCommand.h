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

#ifndef RG_EXPANDFIGURATIONCOMMAND_H
#define RG_EXPANDFIGURATIONCOMMAND_H

#include "base/Segment.h"
#include "base/Selection.h"
#include "document/Command.h"
#include <QCoreApplication>
#include <QString>

namespace Rosegarden
{
  class Composition;
  typedef long timeT;

 
// @class ExpandFigurationCommand
// @remarks Implements the command "Expand block chords to figurations".  
// @author Tom Breton (Tehom)
class ExpandFigurationCommand : public MacroCommand
{
  Q_DECLARE_TR_FUNCTIONS(Rosegarden::ExpandFigurationCommand)

public:
  ExpandFigurationCommand(SegmentSelection selection);

  ~ExpandFigurationCommand() override;

  static QString getGlobalName() 
  { return tr("Expand Block Chords to Figurations"); }

private:
    
  void initialise(SegmentSelection selection);
  timeT rawStartTimeToExact(timeT raw);

  Composition                   *m_composition;
  // The new segments we make.
  SegmentMultiSet                m_newSegments;
  bool                           m_executed;
};

}

#endif
