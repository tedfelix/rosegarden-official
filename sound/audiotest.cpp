#include <iostream>

/*
#include <artsmidi.h>
#include "Sequencer.h"
#include "AudioFile.h"
#include <arts/artsflow.h>
#include <arts/artsmodules.h>
#include <arts/soundserver.h>
#include <arts/connect.h>
*/

//#include <sndfile.h>
#include "PluginManager.h"

using std::cout;
using std::cerr;
using std::endl;

int
main(int argc, char **argv)
{
    Rosegarden::PluginManager *pluginManager = new Rosegarden::PluginManager();
    pluginManager->getenvLADSPAPath();

    // find them
    pluginManager->discoverPlugins();

    Rosegarden::PluginIterator it;
    for (it = pluginManager->begin(); it != pluginManager->end(); it++)
    {
        cout << (*it)->getName() << " (id = " << (*it)->getId() << ")" << endl;
    }

    delete pluginManager;
}


/*
int
main(int argc, char **argv)
{
    cout << "SND FILE TEST" << endl;

    SF_INFO info;

    SNDFILE *file = sf_open_read("/home/bownie/rosegarden/gui/testfiles/audio/909-kick.wav", &info);

    switch (info.format & SF_FORMAT_TYPEMASK)
    {
        case SF_FORMAT_WAV:
            cout << "WAV FORMAT" << endl;
            break;

        default:
            cout << "UNKNOWN TYPE " << info.format << endl;
            break;
    }

    switch(info.format & SF_FORMAT_SUBMASK)
    {
        case SF_FORMAT_PCM:
            cout << "PCM" << endl;
            break;

        default:
            cout << "UNKNOWN SUBTYPE" << endl;
            break;
    }

    if (sf_format_check(&info) == false)
    {
        cout << "FORMAT CHECK FAILED" << endl;
        exit(1);
    }

    cout << "MAX = " << sf_signal_max(file) << endl;
    cout << "CHANNELS = " << info.channels << endl;
    cout << "SECTIONS = " << info.sections << endl;
    cout << "SAMPLE RATE = " << info.samplerate << endl;
    cout << "SAMPLES = " << info.samples << endl;
    cout << "PCM BIT = " << info.pcmbitwidth << endl;

    if (sf_seek(file, 0, SEEK_SET) == -1)
    {
        cout << "FAILED SEEK" << endl;
        exit(1);
    }

    int frames = info.samples / info.channels;
    double *samples = new double[info.channels * frames];

    sf_readf_double(file, samples, frames, true);

    for (int i = 0; i < frames; i++)
    {
        for (int j = 0; j < info.channels; j++)
        {
            cout << "INT = " << samples[(i * info.channels) + j] << endl;
        }
    }

    delete [] samples;

    exit(0);

}
*/


