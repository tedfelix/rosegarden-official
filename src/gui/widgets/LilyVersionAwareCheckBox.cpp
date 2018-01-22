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

#include "LilyVersionAwareCheckBox.h"
#include "document/io/LilyPondExporter.h"

namespace Rosegarden
{


LilyVersionAwareCheckBox::LilyVersionAwareCheckBox(const QString& text, QWidget* parent, int workingVersion) 
  : QCheckBox(text, parent), firstWorkingVersion(workingVersion)
{

}

void LilyVersionAwareCheckBox::checkVersion(int value)
{
  if (value < firstWorkingVersion) this->setDisabled(true);
  else this->setDisabled(false);
}

void LilyVersionAwareCheckBox::slotCheckVersion(int value)
{
  if (value < firstWorkingVersion) this->setDisabled(true);
  else this->setDisabled(false);
}


}
