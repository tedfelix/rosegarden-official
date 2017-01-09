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

#ifndef RG_SELECTIONSITUATION_H
#define RG_SELECTIONSITUATION_H

#include "base/PropertyName.h"
#include "base/Selection.h"

#include <QString>

#include <string>

namespace Rosegarden
{
  /*A selection with its situation (from a ruler, etc) */

// @class SelectionSituation Non-gui-related situation data for
// EventParameterDialog and SelectionPropertyCommand.  
// @author Tom Breton (Tehom)
class SelectionSituation
{
    typedef EventSelection::eventcontainer eventcontainer;

 public:
  SelectionSituation(std::string eventType,
		     EventSelection *selection,
		     int currentFlatValue = -1);

  QString getPropertyNameQString(void) const;

 public:
  bool isSuitable(Event *e) const
  { return e->isa(m_eventType); }
  std::pair<int,int> getMinMax(void) const;
  void setValue(Event *e, int value) const;
  void addToValue(Event *e, int increase) const;
  EventSelection *getEventSelection(void) const
  { return m_selection; }
  int getFlatValue(void) const { return m_currentFlatValue; }
  int calcMeanValue(void) const;
  int maxValue(void) const;
      
 protected:
  static PropertyName derivePropertyName(std::string eventType);
  
 private:
  // We are treating this event type...
  const std::string          m_eventType;
  // ...and this property
  const PropertyName         m_property;
  // ...in this selection.  m_selection can't be const pointer
  // because SelectionPropertyCommand passes it as non-const to
  // BasicSelectionCommand.
  EventSelection            *m_selection;
  // A reference value from outside.  Some patterns use it as
  // default. 
  const int                  m_currentFlatValue;
};

}

#endif /* ifndef RG_SELECTIONSITUATION_H */
