// -*- c-basic-offset: 4 -*-

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

#include "rosedebug.h"
#include "rosegardengui.h"

static const char *description =
I18N_NOOP("Rosegarden - A sequencer and musical notation editor");
	
	
static KCmdLineOptions options[] =
{
    { "+[File]", I18N_NOOP("file to open"), 0 },
    { 0, 0, 0 }
    // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{

    KAboutData aboutData( "rosegarden", I18N_NOOP("Rosegarden"),
                          VERSION, description, KAboutData::License_GPL,
                          "(c) 2000-2001, Guillaume Laurent, Chris Cannam, Richard Bown");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Richard Bown",0,
                        "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    KApplication app;

    RosegardenGUIApp *rosegardengui = 0;
 
    if (app.isRestored()) {

        RESTORE(RosegardenGUIApp);

    } else {

        rosegardengui = new RosegardenGUIApp();
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
    app.dcopClient()->registerAs(app.name(), false);
    app.dcopClient()->setDefaultObject(ROSEGARDEN_GUI_IFACE_NAME);

    // Check for sequencer and launch if needed
    //
    rosegardengui->launchSequencer();

    return app.exec();

}  
