#include <iostream>
#include <artsmidi.h>
#include "Sequencer.h"
/*
#include <arts/audioio.h>
#include <arts/artsflow.h>
#include <arts/artsmodules.h>
#include <arts/objectmanager.h>
*/
#include <arts/soundserver.h>



int
main(int argc, char **argv)
{
    Rosegarden::Sequencer seq;
/*
    Arts::AudioManager obMan();
    Arts::AudioManagerClient *audioManClient;
    Arts::AudioManager artsAudio;
*/
    Arts::AudioManager audioManager =
        Arts::Reference("global:Arts_AudioManager");

    if (audioManager.isNull())
    {
        std::cerr << "Couldn't get Arts::AudioManager" << std::endl;
        exit(1);
    }

    Arts::SoundServer as = Arts::Reference("global:Arts_SoundServer");


    Arts::AudioManagerClient audioClient = Arts::Reference("global:Arts_AudioManagerClient");

/*
    audioManClient = new Arts::AudioManagerClient(Arts::amRecord,
                  string("rosegarden audio manager"), string("rg audio man"));
*/

/*
    audioManClient = new Arts::AudioManagerClient();

    Arts::AudioManagerInfo audioManInfo;
  
    std::cout << "TITLE = " << audioManInfo.title << endl;

*/


/*
    Arts::Synth_AMAN_RECORD *aManRecord;

    aManRecord = new Arts::Synth_AMAN_RECORD(*audioManClient);
    aManRecord->title(string("firstwav.wav"));
*/

    //Arts::Synth_CAPTURE_WAV CaptureWav;

    sleep(10);

    exit(0);

}
