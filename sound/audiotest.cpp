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
    Arts::Synth_AMAN_PLAY amanPlay;
    Arts::Synth_AMAN_RECORD amanRecord;
    Arts::Synth_CAPTURE_WAV captureWav;

    if ( !server.isNull() )
    {
       amanPlay = Arts::DynamicCast(
                server.createObject("Arts::Synth_AMAN_PLAY"));
       cout << "HERE" << endl;

       amanRecord = Arts::DynamicCast(
                server.createObject("Arts::Synth_AMAN_RECORD"));

       //Arts::connect(amanRecord, captureWav);
    }



    sleep(10);

    exit(0);

}
