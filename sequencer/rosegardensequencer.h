
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _ROSEGARDEN_SEQUENCER_H_
#define _ROSEGARDEN_SEQUENCER_H_
 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>

// include files for KDE 
#include <kapp.h>
#include <kmainwindow.h>
#include <kaccel.h>

#include "Event.h"
#include "rosegardensequenceriface.h"

class KURL;
class KRecentFilesAction;

// forward declaration of the RosegardenGUI classes
class RosegardenGUIDoc;
class RosegardenGUIView;

class RosegardenSequencerApp : public KMainWindow,
                               virtual public RosegardenSequencerIface
{
  Q_OBJECT

public:
  RosegardenSequencerApp();
  ~RosegardenSequencerApp();

protected:

public slots:
  virtual void quit();
  virtual bool play(const Rosegarden::timeT &position);
  virtual bool stop();
    
private:

};
 
#endif // _ROSEGARDEN_SEQUENCER_H_
