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
// framework when indicated through the sequencer.
//
//
//

#include "AudioFile.h"
#include "RealTime.h"
#include <string>
#include <vector>

#ifndef _AUDIOFILEPLAYER_H_
#define _AUDIOFILEPLAYER_H_

namespace Rosegarden
{

class AudioFilePlayer
{
public:
    AudioFilePlayer();
    ~AudioFilePlayer();

    void addAudioFile(const AudioFileType &audioFileType,
                      const string &fileName,
                      const int &id);

    void deleteAudioFile(const int &id);

    // Empty all the files and clear down all the handles
    //
    void clear();

    // Play an audio sample
    //
    bool playAudio(const int &id, const RealTime startIndex,
                   const RealTime duration);

    
private:

    std::vector<AudioFile*> m_audioFiles;

};


}

#endif // _AUDIOFILEPLAYER_H_
