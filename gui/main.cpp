/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Mon Jun 19 23:41:03 CEST 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
            rosegardengui->openDocumentFile();
        }

        args->clear();
    }

    return app.exec();
}  
