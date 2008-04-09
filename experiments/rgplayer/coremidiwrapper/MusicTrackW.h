/*
 *  MusicTrackW.h
 *  rgplayer
 *
 *  Created by Guillaume Laurent on 3/20/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef RG_MUSIC_TRACK_W
#define RG_MUSIC_TRACK_W

#include <AudioToolbox/AudioToolbox.h>
#include "ErrCode.h"
#include "MusicPlayerW.h"

struct EventData {
    MusicTimeStamp timeStamp;
    MusicEventType eventType;
    const void* eventData;
    UInt32 eventDataSize;
};

class MusicTrackW : public ErrCode {
public:
    MusicTrackW(MusicTrack track);
    MusicTrackW() {
      // do nothing  
    };
    
    MusicSequenceW getSequence();
    
    void setDestNode(AUNode node);
    
    void setDestMIDIEndpoint(MIDIEndpointRef endpoint);
    
    AUNode getDestNode();
    
    MIDIEndpointRef getDestMIDIEndpoint();
    
    void setProperty(UInt32 propertyID, void* data, UInt32 length);
    
    void getProperty(UInt32 propertyID, void* outData, UInt32* length);
    
    void newMIDINoteEvent(MusicTimeStamp timeStamp, const MIDINoteMessage* message);
    
    void newMIDIChannelEvent(MusicTimeStamp timeStamp, const MIDIRawData* rawData);
    
    void newMetaEvent(MusicTimeStamp timeStamp, const MIDIMetaEvent* metaEvent);
    
    void newExtendedNoteEvent(MusicTimeStamp timeStamp, const ExtendedNoteOnEvent* info);
    
    void newExtendedControlEvent(MusicTimeStamp timeStamp, const ExtendedControlEvent* info);
    
    void newParameterEvent(MusicTimeStamp timeStamp, const ParameterEvent* info);
    
    void newExtendedTempoEvent(MusicTimeStamp timeStamp, Float64 BPM);
    
    void newUserEvent(MusicTimeStamp timeStamp, const MusicEventUserData* userData);
    
    void moveEvents(MusicTimeStamp startTime, MusicTimeStamp endTime, MusicTimeStamp moveTime);
    
    MusicTrackW newMusicTrackFrom(MusicTimeStamp sourceStartTime, MusicTimeStamp sourceEndTime);
    
    void clear(MusicTimeStamp startTime, MusicTimeStamp endTime);
    
    void cut(MusicTimeStamp startTime, MusicTimeStamp endTime);
    
    void copyInsert(MusicTimeStamp sourceStartTime, MusicTimeStamp sourceEndTime, MusicTrackW destTrack, MusicTimeStamp destInsertTime);
    
    void merge(MusicTimeStamp sourceStartTime, MusicTimeStamp sourceEndTime, MusicTrackW destTrack, MusicTimeStamp destInsertTime);
    
    
    class Iterator : public ErrCode {
    public:
        Iterator(MusicTrack track, MusicEventIterator it) : mOriginTrack(track), mIterator(it) {
        }
        
        ~Iterator() {
            DisposeMusicEventIterator(mIterator);
        }
        
        void seek(MusicTimeStamp timeStamp);
        
        Iterator& operator++();
        
        // no postfix ++
        
        Iterator& operator--();
        
        // no postifx -- either
        
        EventData operator*();
        
        void setEventInfo(EventData eventData);
        
        void deleteEvent();
        
        void setEventTime(MusicTimeStamp timeStamp);
        
        Boolean hasPreviousEvent();
        
        Boolean hasNextEvent();
        
        Boolean hasCurrentEvent();
        
    protected:
        MusicTrack mOriginTrack;
        MusicEventIterator mIterator;
    };
    
    Iterator iterator();
    
    MusicTrack getTrack() {
        return m_track;
    }
    
protected:
    MusicTrack m_track;
};

#endif
