
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

#include "rosegardengui.h"

static const char *description =
I18N_NOOP("RosegardenGUI");
// INSERT A DESCRIPTION FOR YOUR APPLICATION HERE
	
	
static KCmdLineOptions options[] =
{
    { "+[File]", I18N_NOOP("file to open"), 0 },
    { 0, 0, 0 }
    // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{

    KAboutData aboutData( "rosegardengui", I18N_NOOP("RosegardenGUI"),
                          VERSION, description, KAboutData::License_GPL,
                          "(c) 2000, Guillaume Laurent, Chris Cannam, Rich Bown");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Rich Bown",0, "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    KApplication app;
 
    if (app.isRestored()) {

        RESTORE(RosegardenGUIApp);

    } else {

        RosegardenGUIApp *rosegardengui = new RosegardenGUIApp();
        rosegardengui->show();

        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		
        if (args->count()) {
            rosegardengui->openDocumentFile(args->arg(0));
        } else {
            // rosegardengui->openDocumentFile();
        }

        args->clear();
    }

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    return app.exec();
}  
