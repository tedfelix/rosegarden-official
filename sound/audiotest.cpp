#include <iostream>
#include <artsmidi.h>
#include "Sequencer.h"
#include <arts/artsflow.h>
#include <arts/artsmodules.h>
#include <arts/soundserver.h>
#include <arts/connect.h>


using std::cout;
using std::cerr;
using std::endl;

int
main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "usage: audiotest <.wav file>" << endl;
        exit(1);
    }

    string wavFile = argv[1];

    // start sequencer
    //
    Rosegarden::Sequencer seq;

    Arts::SoundServerV2 server = Arts::Reference("global:Arts_SoundServerV2");

    Arts::Synth_AMAN_PLAY amanPlay;
    Arts::Synth_AMAN_RECORD amanRecord;
    Arts::Synth_CAPTURE_WAV captureWav;
    Arts::Synth_PLAY_WAV playWav;

    if (server.fullDuplex())
        cout << "SOUND SERVER is FULL DUPLEX" << endl;

    cout << "SAMPLING RATE = " << server.samplingRate() << endl;
    cout << "SAMPLE SIZE   = " << server.bits() << " bits" << endl;

    if ( !server.isNull() )
    {
       amanPlay=Arts::DynamicCast(server.createObject("Arts::Synth_AMAN_PLAY"));
       amanPlay.title("Rosegarden Audio Play");
       amanPlay.autoRestoreID("Rosegarden Play");

       if (amanPlay.isNull())
       {
           std::cerr << "Cannot create audio play object" << std::endl;
           exit(1);
       }

       playWav = Arts::DynamicCast(
                server.createObject("Arts::Synth_PLAY_WAV"));

       // set a file name
       //
       playWav.filename(wavFile);

       cout << "PLAYING SAMPLE once" << endl;
       playWav.start();
       amanPlay.start();
       connect(playWav, "left", amanPlay, "left");
       connect(playWav, "right", amanPlay, "right");

       sleep(2);

       disconnect(playWav, "left", amanPlay, "left");
       disconnect(playWav, "right", amanPlay, "right");

       playWav.stop();
       amanPlay.stop();

       cout << "PLAYING SAMPLE twice" << endl;
       playWav.filename(wavFile);
       playWav.start();
       amanPlay.start();
       connect(playWav, "left", amanPlay, "left");
       connect(playWav, "right", amanPlay, "right");

       sleep(2);
       
       disconnect(playWav, "left", amanPlay, "left");
       disconnect(playWav, "right", amanPlay, "right");

       playWav.stop();
       amanPlay.stop();


       // Synth_AMAN_RECORD - sends to a file: /tmp/mcop-USER/capture.wav
       //
       //
       amanRecord = Arts::DynamicCast(server.createObject("Arts::Synth_AMAN_RECORD"));

       if (amanRecord.isNull())
       {
           std::cerr << "Cannot create audio record object" << std::endl;
           exit(1);
       }

       amanRecord.title("Rosegarden Audio Record");
       amanRecord.autoRestoreID("Rosegarden Record");

       captureWav = Arts::DynamicCast(
                server.createObject("Arts::Synth_CAPTURE_WAV"));

       if (captureWav.isNull())
       {
           std::cerr << "Cannot create CAPTURE object" << std::endl;
           exit(1);
       }

/*
       Arts::StereoVolumeControl volumeControl  =
           Arts::DynamicCast(server.createObject("Arts::StereoVolumeControl"));
*/

       cout << "RECORDING 10 seconds of AUDIO" << endl;

       amanRecord.start();
       captureWav.start();
       
       Arts::connect(captureWav, "left", amanRecord, "left");
       Arts::connect(captureWav, "right", amanRecord, "right");

       sleep(10);

       Arts::disconnect(captureWav, "left", amanRecord, "left");
       Arts::disconnect(captureWav, "right", amanRecord, "right");
       amanRecord.stop();
       captureWav.stop();
 
    }

    exit(0);

}
