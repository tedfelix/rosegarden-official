#include <iostream>
#include <artsmidi.h>
#include "Sequencer.h"
#include <arts/artsflow.h>
#include <arts/artsmodules.h>
#include <arts/soundserver.h>
#include <arts/connect.h>



int
main(int argc, char **argv)
{
    Rosegarden::Sequencer seq;
/*
    Arts::AudioManager obMan();
    Arts::AudioManagerClient *audioManClient;
    Arts::AudioManager artsAudio;
*/
/*
    Arts::AudioManager audioManager =
        Arts::Reference("global:Arts_AudioManager");

    if (audioManager.isNull())
    {
        std::cerr << "Couldn't get Arts::AudioManager" << std::endl;
        exit(1);
    }

    Arts::AudioManagerClient amClient(Arts::amPlay, "aRts play port","Rosegarden Audio Play");

    vector<Arts::AudioManagerInfo> *info = audioManager.clients();
    vector<Arts::AudioManagerInfo>::iterator it;

    for (it = info->begin(); it != info->end(); it++)
    {
        cout << "TITLE = " << (*it).title;
    }


*/

    //Arts::SoundServer as = Arts::Reference("global:Arts_SoundServer");


/*
    Arts::AudioManagerClient audioManagerClient(Arts::amRecord,
              string("Rosegarden audio manager"), string("rg audio man"));
*/


/*
    Arts::Synth_AMAN_RECORD record;
    record.title(string("firstwav.wav"));
*/


/*
    audioManClient = new Arts::AudioManagerClient();

    Arts::AudioManagerInfo audioManInfo;
  
    std::cout << "TITLE = " << audioManInfo.title << endl;


*/

    Arts::SoundServerV2 server = Arts::Reference("global:Arts_SoundServerV2");

    //Arts::AudioManagerClient amClient = Arts::DynamicCast(server.createObject("Arts::AudioManagerClient"));

    //Arts::AudioManagerClient amClient(Arts::amRecord, "aRts record port","Rosegarden Audio Record");

    //amClient.direction(Arts::amRecord);

    Arts::Synth_AMAN_PLAY amanPlay;
    Arts::Synth_AMAN_RECORD amanRecord;
    Arts::Synth_CAPTURE_WAV captureWav;

    if (server.fullDuplex())
    {
        std::cout << "SOUND SERVER is FULL DUPLEX" << endl;
    }

    std::cout << "SAMPLING RATE = " << server.samplingRate() << endl;
    std::cout << "SAMPLE SIZE   = " << server.bits() << " bits" << endl;

    if ( !server.isNull() )
    {
       amanPlay = Arts::DynamicCast(server.createObject("Arts::Synth_AMAN_PLAY"));
       amanPlay.title("Rosegarden Audio Play");
       amanPlay.autoRestoreID("Rosegarden Play");


       //Arts::Synth_BUS_DOWNLINK downlink = Arts::DynamicCast(player._getChild( "uplink" ));
       if (amanPlay.isNull())
       {
           std::cerr << "Cannot create audio play object" << std::endl;
           exit(1);
       }

       // Synth_AMAN_RECORD - sends to a file: /tmp/mcop-USER/capture.wav
       //
       //
       amanRecord = Arts::DynamicCast(server.createObject("Arts::Synth_AMAN_RECORD"));

       if (amanRecord.isNull())
       {
           std::cerr << "Cannot create audio record object" << std::endl;
           exit(1);
       }

       //amanRecord.direction(Arts::amRecord);
       amanRecord.title("Rosegarden Audio Record");
       amanRecord.autoRestoreID("Rosegarden Record");

       captureWav = Arts::DynamicCast(
                server.createObject("Arts::Synth_CAPTURE_WAV"));

       if (captureWav.isNull())
       {
           std::cerr << "Cannot create CAPTURE object" << std::endl;
           exit(1);
       }

       
       Arts::connect(captureWav, "left", amanRecord, "left");
       Arts::connect(captureWav, "right", amanRecord, "right");

/*
       amanRecord.streamInit();
       captureWav.streamInit();
       amanRecord.streamStart();
       captureWav.streamStart();
*/

       amanRecord.start();
       captureWav.start();

       sleep(10);

       amanRecord.stop();
       captureWav.stop();

       Arts::disconnect(captureWav, "left", amanRecord, "left");
       Arts::disconnect(captureWav, "right", amanRecord, "right");

/*
       amanRecord.streamEnd();
       captureWav.streamEnd();
*/

    }




    exit(0);

}
