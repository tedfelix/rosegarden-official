// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <qtimer.h>
#include <ctime>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <dcopclient.h>
#include <kconfig.h>

#include "rosedebug.h"
#include "rosegardengui.h"

#include "kstartuplogo.h"

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
                          "Copyright 2000 - 2002 Guillaume Laurent, Chris Cannam, Richard Bown\nParts copyright 1994 - 2001 Chris Cannam, Andy Green, Richard Bown, Guillaume Laurent\nLilypond fonts copyright 1997 - 2001 Han-Wen Nienhuys and Jan Nieuwenhuizen");
    aboutData.addAuthor("Guillaume Laurent, Chris Cannam, Richard Bown",0,
                        "glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    KApplication app;

    //
    // Ensure quit on last window close
    // Register main DCOP interface
    //
    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    app.dcopClient()->registerAs(app.name(), false);
    app.dcopClient()->setDefaultObject(ROSEGARDEN_GUI_IFACE_NAME);

    // Show Startup logo
    // (this code borrowed from KDevelop 2.0,
    // (c) The KDevelop Development Team
    //
    KConfig* config = KGlobal::config();
    config->setGroup("General Options");
    KStartupLogo* startLogo = 0L;

    if (config->readBoolEntry("Logo",true) && (!kapp->isRestored() ) )
    {
	kdDebug(KDEBUG_AREA) << "main: Showing startup logo\n";
	startLogo = new KStartupLogo();
	startLogo->show();
    }

    clock_t logoShowTime = clock();

    //
    // Start application
    //
    RosegardenGUIApp *rosegardengui = 0;
 
    if (app.isRestored()) {

        RESTORE(RosegardenGUIApp);

    } else {

        rosegardengui = new RosegardenGUIApp();
        rosegardengui->show();

	// raise start logo
	//
	if (startLogo) {
	    startLogo->raise();
	    startLogo->setHideEnabled(true);
	    QApplication::flushX();
	}

        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		
        if (args->count()) {
            rosegardengui->openDocumentFile(args->arg(0));
        } else {
            // rosegardengui->openDocumentFile();
        }

        args->clear();

    }

    // Now that we've started up, raise start logo
    //
    if (startLogo) {
        startLogo->raise();
	startLogo->setHideEnabled(true);
        QApplication::flushX();
    }

    // Check for sequencer and launch if needed
    //
    rosegardengui->launchSequencer();

    if (startLogo) {
	
	// pause to ensure the logo has been visible for a reasonable
	// length of time, just 'cos it looks a bit silly to show it
	// and remove it immediately

	int visibleFor = (clock() - logoShowTime) * 1000 / CLOCKS_PER_SEC;

	if (visibleFor < 2000) {
	    QTimer::singleShot(2500 - visibleFor, startLogo, SLOT(close()));
	} else {
	    startLogo->close();
	}
    }

    return app.exec();
}  

