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

// From this class we control our sound subsystems - audio
// and MIDI are initialised, playback and recording handles
// are available to the higher levels for sending and 
// retreiving MIDI and audio.
//
//

#ifndef _ROSEGARDEN_SEQUENCER_H_
#define _ROSEGARDEN_SEQUENCER_H_

#include <map>
#include <arts/artsmidi.h>
#include <arts/soundserver.h>
#include "MappedComposition.h"
#include "Midi.h"
#include "MidiRecord.h"
#include "MidiEvent.h"
#include "Track.h"


namespace Rosegarden
{

  // Note Off queues handled by the following classes
  class NoteOffEvent
  {
  public:
    NoteOffEvent() {;}
    NoteOffEvent(const unsigned int &midiTime,
                 const unsigned int &pitch,
                 const Rosegarden::MidiByte &status):
                  _midiTime(midiTime),
                  _pitch(pitch),
                  _status(status) {;}
    ~NoteOffEvent() {;}

    struct NoteOffEventCmp
    {
      bool operator()(NoteOffEvent *nO1, NoteOffEvent *nO2)
      { 
        return nO1->getMidiTime() < nO2->getMidiTime();
      }
    };
    
    unsigned int getMidiTime() { return _midiTime; }
    Rosegarden::MidiByte getPitch() { return _pitch; }

  private: 
    unsigned int _midiTime;
    Rosegarden::MidiByte _pitch;
    Rosegarden::MidiByte _status;

  };

  class NoteOffQueue : public std::multiset<NoteOffEvent *,
                                            NoteOffEvent::NoteOffEventCmp>
  {
  public:
    NoteOffQueue() {;}
    ~NoteOffQueue() {;}
  private:
  };


  class Sequencer
  {
  public:

    typedef enum
    {
      ASYNCHRONOUS_MIDI,
      ASYNCHRONOUS_AUDIO,
      RECORD_MIDI,
      RECORD_AUDIO
    } RecordStatus;

    Sequencer();
    ~Sequencer();

    // Activate a recording state
    //
    void record(const RecordStatus& recordStatus);

    // get a vector of recorded events from aRTS
    inline std::vector<Arts::MidiEvent>* getMidiQueue()
      { return _midiRecordPort.getQueue(); }

    // Our current recording status.  Asynchronous record states
    // mean that we're just accepting asynchronous events but not
    // recording them - they are forwarded onto the GUI for level/
    // activity display purposes.
    //
    RecordStatus recordStatus() { return _recordStatus; }
    
    // set and get tempo
    const double getTempo() const { return _tempo; }
    void setTempo(const double &tempo) { _tempo = tempo; }

    // resolution - required?
    const unsigned int resolution() const { return _ppq; }

    // get the difference in TimeStamps
    Arts::TimeStamp deltaTime(const Arts::TimeStamp &ts1,
                              const Arts::TimeStamp &ts2);

    Arts::TimeStamp aggregateTime(const Arts::TimeStamp &ts1,
                                  const Arts::TimeStamp &ts2);

    // get the TimeStamp from the beginning of playing
    inline Arts::TimeStamp playTime(Arts::TimeStamp const &ts)
      { return (deltaTime(ts, _playStartTime)); }

    // get the TimeStamp from the beginning of recording
    inline Arts::TimeStamp recordTime(Arts::TimeStamp const &ts)
      { return (deltaTime(ts, _recordStartTime)); }

    // See docs/discussion/sequencer_timing.txt for explanation of
    // the maths here.
    //
    //
    inline Rosegarden::timeT convertToInternalTime(const Arts::TimeStamp &timeStamp)
    {
      return (Rosegarden::timeT)
             ((((double)(timeStamp.sec * 1000000 + timeStamp.usec)) *
                                  _ppq * (double) _tempo )  /
                                60000000.0);
    }

    // See docs/discussion/sequencer_timing.txt for explanation of
    // the maths here.
    //
    //
    inline Arts::TimeStamp convertToArtsTimeStamp(const unsigned int &midiTime)
    {
      // We ignore the Time Sigs for the moment
      //
      //
      unsigned int usec = (unsigned int)(((double)60000000.0 *(double)midiTime)/
                                         ((double)_ppq * (double)_tempo));
      unsigned int sec = usec / 1000000;
      usec %= 1000000;

      return (Arts::TimeStamp(sec, usec));
    }


    // process a raw aRTS MIDI event into internal representation
    void processMidiIn(const Arts::MidiCommand &midiCommand,
                       const Arts::TimeStamp &timeStamp);

    // Process MappedComposition into MIDI events and send to aRTS.
    void processMidiOut(Rosegarden::MappedComposition mappedComp,
                        const Rosegarden::timeT &playLatency);

    // Reset internal states ready for new playback to commence
    //
    void initializePlayback(const timeT &position);

    Rosegarden::timeT getSequencerTime();

    bool isPlaying() { return _playing; }

    void stopPlayback();

    // NOTE OFF control
    void processNotesOff(unsigned int midiTime);
    void allNotesOff();

    Rosegarden::timeT getStartPosition() { return _playStartPosition; }

  private:

    void initializeMidi();

    // aRTS devices
    //
    Arts::Dispatcher       _dispatcher;
    Arts::SoundServer      _soundServer;
    Arts::MidiManager      _midiManager;
    Arts::MidiClient       _midiPlayClient;
    Arts::MidiClient       _midiRecordClient;
    RosegardenMidiRecord   _midiRecordPort;
    Arts::MidiPort         _midiPlayPort;

    // TimeStamps mark the real world times of the commencement
    // of play or record.  These are for internal use when calculating
    // how to queue up playback.
    //
    Arts::TimeStamp _playStartTime;
    Arts::TimeStamp _recordStartTime;
    NoteOffQueue _noteOffQueue;

    Rosegarden::timeT _playStartPosition;

    // Current recording status
    RecordStatus _recordStatus;

    bool _startPlayback;
    bool _playing;

    unsigned int _ppq;   // sequencer resolution
    double _tempo; // Beats Per Minute

    Rosegarden::Track *_recordTrack;

    std::map<unsigned int, Event*> _noteOnMap;

  };


}

 

#endif // _ROSEGARDEN_SEQUENCER_H_
