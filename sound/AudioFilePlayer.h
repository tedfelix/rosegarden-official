// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-
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

// AudioFilePlayer resides in the RosegardenSequencer and accepts
//"add" and "delete" requests from the AudioFileManager at the GUI.
// Sample files can then be preloaded, manipulated or generally
// readied for playback and then executed against the aRTS audio
// framework when indicated through the sequencer.  This class has
// a close relationship with the Sequencer for playback and recording
// but seperates out the management aspect of the Audio files.
//
//
//

#include "AudioFile.h"
#include "RealTime.h"
#include "Sequencer.h"
#include <string>
#include <vector>

#ifndef _AUDIOFILEPLAYER_H_
#define _AUDIOFILEPLAYER_H_

namespace Rosegarden
{


class AudioFilePlayer
{
public:
    AudioFilePlayer(Rosegarden::Sequencer *sequencer);
    ~AudioFilePlayer();

    bool addAudioFile(const string &fileName,
                      const unsigned int &id);

    bool deleteAudioFile(const unsigned int &id);

    // Empty all the files and clear down all the handles
    //
    void clear();

    // Queue up an audio sample for playing
    //
    bool queueAudio(const unsigned int &id, const RealTime startIndex,
                    const RealTime duration);

protected:

    std::vector<AudioFile*>::iterator getAudioFile(const unsigned int &id);
    
private:
    std::vector<AudioFile*> m_audioFiles;
    Rosegarden::Sequencer  *m_sequencer;

};


}

#endif // _AUDIOFILEPLAYER_H_
