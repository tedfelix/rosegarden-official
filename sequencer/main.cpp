
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


#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <dcopclient.h>
#include <strstream>
#include <iostream>

#include "rosegardensequencer.h"
#include <MappedComposition.h>

using std::cout;
using std::cerr;
using std::endl;

// the name of our rosegarden gui application
//
#define ROSEGARDEN_GUI_APP "rosegardengui"

static const char *description = I18N_NOOP("RosegardenSequencer");
    
static KCmdLineOptions options[] =
{
    { "+[File]", I18N_NOOP("file to open"), 0 },
    { 0, 0, 0 }
    // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{

  KAboutData aboutData( "rosegardensequencer",
                        I18N_NOOP("RosegardenSequencer"),
                        VERSION, description, KAboutData::License_GPL,
                        "(c) 2000-2001, Guillaume Laurent, Chris Cannam, Richard Bown");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Richard Bown",0, "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication app;

  if (app.isRestored())
  {
    RESTORE(RosegardenSequencerApp);
  }
  else
  {
    RosegardenSequencerApp *rosegardensequencer = new RosegardenSequencerApp();

    // we don't show the sequencer application as we're just taking
    // advantage of DCOP

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        
    if (args->count())
    {
      //rosegardensequencer->openDocumentFile(args->arg(0));
    }
    else
    {
      // rosegardensequencer->openDocumentFile();
    }

    args->clear();
  }

  QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

  DCOPClient *client = kapp->dcopClient();

  QCString realAppId = client->registerAs(kapp->name());
    
  // get list of registered applications from DCOP
  //
  QCStringList dcopApps = client->registeredApplications();

  // Number of matches of our GUI app
  //
  int guiAppInstances  = dcopApps.contains(QCString(ROSEGARDEN_GUI_APP));

  if ( guiAppInstances == 0 )
  {
    cerr << "Rosegarden sequencer cannot start as \""
         << ROSEGARDEN_GUI_APP << "\" is not running"  << endl;
    return(1);
  }

  if ( guiAppInstances > 1 )
  {
    cerr << "Rosegarden sequencer cannot start as too many instances of \"" <<
             ROSEGARDEN_GUI_APP << "\" are running." << endl;
    return(1);
  }

  // Get the GUI App reference
  //
  QValueList<QCString>::Iterator guiApp = dcopApps.find(QCString(ROSEGARDEN_GUI_APP));

  // Call the relevant method on the GUI interface to
  // return the time slice of MappedEvents
  //
  QByteArray data, replyData;
  QCString replyType;
  QDataStream arg(data, IO_WriteOnly);
  arg << 0;
  arg << 1000;

  if (!client->call(ROSEGARDEN_GUI_APP,
                    "RosegardenGUIIface",
                    "getSequencerSlice(int, int)",
                    data, replyType, replyData))
  {
    cerr << "there was some error using DCOP." << endl;
    return(1);
  }
  else
  {
    QDataStream reply(replyData, IO_ReadOnly);
    if (replyType == "QString")
    {
      QString result;
      reply >> result;
    }
    else if (replyType = "Rosegarden::MappedComposition")
    {
      Rosegarden::MappedComposition::iterator it;
      Rosegarden::MappedComposition mappedComp;
       
      reply >> mappedComp;

      cout << "GOT " << mappedComp.size() << " ELEMENTS" << endl;

      for (it = mappedComp.begin(); it != mappedComp.end(); ++it )
      {
        cout << "Pitch = " << (*it)->getPitch() << endl;
        cout << "Time = " << (*it)->getAbsoluteTime() << endl;
        cout << "Duration = " << (*it)->getDuration() << endl;
        cout << "Velocity = " << (*it)->getVelocity() << endl;
        cout << "Instrument = " << (*it)->getInstrument() << endl;
        cout << endl;
      }

    }
    else
    {
      cerr << "doIt returned an unexpected type of reply!" << endl;
      return(1);
    }
  }
/*
  cout << "Number of items in list = " << apps.count() << endl;

  for ( QValueList<QCString>::Iterator i = apps.begin();
        i != apps.end(); ++i )
  {
    cout << *i << endl;
  }
*/

  //app.dcopClient()->setDefaultObject("RosegardenGUIIface");

  return app.exec();
}
