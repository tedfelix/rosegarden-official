/*
 *  MusicPlayerW.h
 *  rgplayer
 *
 *  Created by Guillaume Laurent on 3/13/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef RG_MUSIC_PLAYER_W
#define RG_MUSIC_PLAYER_W

#include <AudioToolbox/AudioToolbox.h>
#include "ErrCode.h"

class MusicSequenceW : public ErrCode {
public:
    MusicSequenceW(MusicSequence sequence) : m_sequence(sequence) {
    }
    
    MusicSequenceW() {
        NewMusicSequence(&m_sequence);
    }
    
    ~MusicSequenceW() {
        DisposeMusicSequence(m_sequence);
    }
    
    MusicSequence get() const {
        return m_sequence;
    }
    
    MusicTrack newTrack() {
        MusicTrack track;
        MusicSequenceNewTrack(m_sequence, &track);
        return track;
    }
    
    UInt32 getTrackCount() {
        UInt32 res;
        m_lastErrorCode = MusicSequenceGetTrackCount(m_sequence, &res);
        return res;
    }
    
    MusicTrack getIndTrack(UInt32 idx) {
        MusicTrack res;
        m_lastErrorCode = MusicSequenceGetIndTrack(m_sequence, idx, &res);
        return res;
    }
    
    UInt32 getTrackIndex(MusicTrack track) {
        UInt32 res;
        m_lastErrorCode = MusicSequenceGetTrackIndex(m_sequence, track, &res);
        return res;
    }
    
    MusicTrack getTempoTrack() {
        MusicTrack res;
        m_lastErrorCode = MusicSequenceGetTempoTrack(m_sequence, &res);
        return res;
    }
    
    void setAUGraph(AUGraph graph) {
        m_lastErrorCode = MusicSequenceSetAUGraph(m_sequence, graph);
    }

    AUGraph getAUGraph() {
        AUGraph res;
        m_lastErrorCode = MusicSequenceGetAUGraph(m_sequence, &res);
        return res;
    }
    
    void setMIDIEndpoint(MIDIEndpointRef endPoint) {
        m_lastErrorCode = MusicSequenceSetMIDIEndpoint(m_sequence, endPoint);
    }
    
    void reverse() {
        m_lastErrorCode = MusicSequenceReverse(m_sequence);
    }
    
    Float64 getSecondesForBeats(MusicTimeStamp beats) {
        Float64 res;
        m_lastErrorCode = MusicSequenceGetSecondsForBeats(m_sequence, beats, &res);
        return res;
    }
    
    MusicTimeStamp getBeatsForSeconds(Float64 seconds) {
        MusicTimeStamp res;
        m_lastErrorCode = MusicSequenceGetBeatsForSeconds(m_sequence, seconds, &res);
        return res;
    }
    
    // TODO : set user callback
    
    
    
protected:
    MusicSequence m_sequence;
};


class MusicPlayerW : public ErrCode {
public:
    MusicPlayerW() {
        NewMusicPlayer(&m_player);
    }
    
    ~MusicPlayerW() {
        DisposeMusicPlayer(m_player);
    }
    
    void setSequence(const MusicSequenceW& sequence) {
        m_lastErrorCode = MusicPlayerSetSequence(m_player, sequence.get());
    }
    
    void start() {
        m_lastErrorCode = MusicPlayerStart(m_player);
    }

    void stop() {
        m_lastErrorCode = MusicPlayerStop(m_player);
    }

    void preroll() {
        m_lastErrorCode = MusicPlayerPreroll(m_player);
    }
    
    bool isPlaying() {
        Boolean res;
        m_lastErrorCode = MusicPlayerIsPlaying(m_player, &res);
        return res;
    }
    
    void setTime(MusicTimeStamp time) {
        m_lastErrorCode = MusicPlayerSetTime(m_player, time);
    }
    
    MusicTimeStamp getTime() {
        MusicTimeStamp res;
        m_lastErrorCode = MusicPlayerGetTime(m_player, &res);
        return res;
    }
    
    void setPlayRateScalar(Float64 inScaleRate) {
        m_lastErrorCode = MusicPlayerSetPlayRateScalar(m_player, inScaleRate);
    }

    Float64 getPlayRateScalar() {
        Float64 res;
        m_lastErrorCode = MusicPlayerGetPlayRateScalar(m_player, &res);
        return res;
    }
    
    UInt64 getHostTimeForBeats(MusicTimeStamp inBeats) {
        UInt64 res;
        m_lastErrorCode = MusicPlayerGetHostTimeForBeats(m_player, inBeats, &res);
        return res;
    }
    
    MusicTimeStamp getBeatsForHostTime(UInt64 inHostTime) {
        MusicTimeStamp res;
        m_lastErrorCode = MusicPlayerGetBeatsForHostTime(m_player, inHostTime, &res);
        return res;
    }
    
protected:
    MusicPlayer m_player;
};


#endif