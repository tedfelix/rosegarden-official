
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

#include <qdatastream.h>
#include "MappedEvent.h"

namespace Rosegarden
{

// turn a MappedComposition into a data stream
//
QDataStream&
operator<<(QDataStream &dS, const MappedComposition &mC)
{
  
  QCString retString = "thing";
  dS << retString;

/*
  for ( it = mC.begin(); it != mC.end(); ++it )
  {
    cerr << "ELEMENT" << endl;
  }
*/


  cerr << "operator<<" << retString << endl;

  return dS;
}

// turn a data stream into a MappedComposition
//
QDataStream& 
operator>>(QDataStream &dS, MappedComposition &mC)
{
  QCString retString;

  dS >> retString;

  cout << retString << endl;
/*
  while (!dS.atEnd())
  {
    dS >> retString;
    cerr << "operator>>" << retString << endl;
  }
    

  //cerr << "PrintableData = " << dS.isPrintableData() << endl;
*/

  return dS;
}

}


