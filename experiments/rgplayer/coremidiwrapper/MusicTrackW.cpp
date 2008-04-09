/*
 *  MusicTrackW.cpp
 *  rgplayer
 *
 *  Created by Guillaume Laurent on 3/20/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "MusicTrackW.h"
#include "MusicPlayerW.h"

MusicTrackW::MusicTrackW(MusicTrack track) {
    m_track = track;
}

MusicSequenceW MusicTrackW::getSequence() {
    MusicSequence sequence;
    m_lastErrorCode = MusicTrackGetSequence(m_track, &sequence);
    return MusicSequenceW(sequence);
}

void MusicTrackW::setDestNode(AUNode node) {
    m_lastErrorCode = MusicTrackSetDestNode(m_track, node);
}

void MusicTrackW::setDestMIDIEndpoint(MIDIEndpointRef endpoint) {
    m_lastErrorCode = MusicTrackSetDestMIDIEndpoint(m_track, endpoint);
}

AUNode MusicTrackW::getDestNode() {
    AUNode res;
    m_lastErrorCode = MusicTrackGetDestNode(m_track, &res);
    return res;
}

MIDIEndpointRef MusicTrackW::getDestMIDIEndpoint() {
    MIDIEndpointRef res;
    m_lastErrorCode = MusicTrackGetDestMIDIEndpoint(m_track, &res);
    return res;
}

void MusicTrackW::setProperty(UInt32 propertyID, void* data, UInt32 length) {
    m_lastErrorCode = MusicTrackSetProperty(m_track, propertyID, data, length);
}

void MusicTrackW::getProperty(UInt32 propertyID, void* outData, UInt32* length) {
    m_lastErrorCode = MusicTrackGetProperty(m_track, propertyID, outData, length);
}

void MusicTrackW::newMIDINoteEvent(MusicTimeStamp timeStamp, const MIDINoteMessage* message) {
    m_lastErrorCode = MusicTrackNewMIDINoteEvent(m_track, timeStamp, message);
}

void MusicTrackW::newMIDIChannelEvent(MusicTimeStamp timeStamp, const MIDIRawData* rawData) {
    m_lastErrorCode = MusicTrackNewMIDIRawDataEvent(m_track, timeStamp, rawData);
}

void MusicTrackW::newMetaEvent(MusicTimeStamp timeStamp, const MIDIMetaEvent* metaEvent) {
    m_lastErrorCode = MusicTrackNewMetaEvent(m_track, timeStamp, metaEvent);
}

void MusicTrackW::newExtendedNoteEvent(MusicTimeStamp timeStamp, const ExtendedNoteOnEvent* info) {
    m_lastErrorCode = MusicTrackNewExtendedNoteEvent(m_track, timeStamp, info);
}

void MusicTrackW::newExtendedControlEvent(MusicTimeStamp timeStamp, const ExtendedControlEvent* info) {
    m_lastErrorCode = MusicTrackNewExtendedControlEvent(m_track, timeStamp, info);
}

void MusicTrackW::newParameterEvent(MusicTimeStamp timeStamp, const ParameterEvent* info) {
    m_lastErrorCode = MusicTrackNewParameterEvent(m_track, timeStamp, info);
}

void MusicTrackW::newExtendedTempoEvent(MusicTimeStamp timeStamp, Float64 BPM) {
    m_lastErrorCode = MusicTrackNewExtendedTempoEvent(m_track, timeStamp, BPM);
}

void MusicTrackW::newUserEvent(MusicTimeStamp timeStamp, const MusicEventUserData* userData) {
    m_lastErrorCode = MusicTrackNewUserEvent(m_track, timeStamp, userData);
}

void MusicTrackW::moveEvents(MusicTimeStamp startTime, MusicTimeStamp endTime, MusicTimeStamp moveTime) {
    m_lastErrorCode = MusicTrackMoveEvents(m_track, startTime, endTime, moveTime);
}

MusicTrackW MusicTrackW::newMusicTrackFrom(MusicTimeStamp sourceStartTime, MusicTimeStamp sourceEndTime) {
    MusicTrack res;
    m_lastErrorCode = NewMusicTrackFrom(m_track, sourceStartTime, sourceEndTime, &res);
    return MusicTrackW(res);
}

void MusicTrackW::clear(MusicTimeStamp startTime, MusicTimeStamp endTime) {
    m_lastErrorCode = MusicTrackClear(m_track, startTime, endTime);
}

void MusicTrackW::cut(MusicTimeStamp startTime, MusicTimeStamp endTime) {
    m_lastErrorCode = MusicTrackCut(m_track, startTime, endTime);
}

void MusicTrackW::copyInsert(MusicTimeStamp sourceStartTime, MusicTimeStamp sourceEndTime, MusicTrackW destTrack, MusicTimeStamp destInsertTime) {
    m_lastErrorCode = MusicTrackCopyInsert(m_track, sourceStartTime, sourceEndTime, destTrack.getTrack(), destInsertTime);
}

void MusicTrackW::merge(MusicTimeStamp sourceStartTime, MusicTimeStamp sourceEndTime, MusicTrackW destTrack, MusicTimeStamp destInsertTime) {
    m_lastErrorCode = MusicTrackMerge(m_track, sourceStartTime, sourceEndTime, destTrack.getTrack(), destInsertTime);
}

void MusicTrackW::Iterator::seek(MusicTimeStamp timeStamp) {
    m_lastErrorCode = MusicEventIteratorSeek(mIterator, timeStamp);
}

MusicTrackW::Iterator& MusicTrackW::Iterator::operator++() {
    m_lastErrorCode = MusicEventIteratorNextEvent(mIterator);
    return *this;
}

// no postfix ++

MusicTrackW::Iterator& MusicTrackW::Iterator::operator--() {
    m_lastErrorCode = MusicEventIteratorPreviousEvent(mIterator);
    return *this;
}

// no postifx -- either

EventData MusicTrackW::Iterator::operator*() {
    EventData res;
    m_lastErrorCode = MusicEventIteratorGetEventInfo(mIterator, &res.timeStamp, &res.eventType, &res.eventData, &res.eventDataSize);
    return res;
}

void MusicTrackW::Iterator::setEventInfo(EventData eventData) {
    m_lastErrorCode = MusicEventIteratorSetEventInfo(mIterator, eventData.eventType, eventData.eventData);
}

void MusicTrackW::Iterator::deleteEvent() {
    m_lastErrorCode = MusicEventIteratorDeleteEvent(mIterator);
}

void MusicTrackW::Iterator::setEventTime(MusicTimeStamp timeStamp) {
    m_lastErrorCode = MusicEventIteratorSetEventTime(mIterator, timeStamp);
}

Boolean MusicTrackW::Iterator::hasPreviousEvent() {
    Boolean res;
    m_lastErrorCode = MusicEventIteratorHasPreviousEvent(mIterator, &res);
    return res;
}

Boolean MusicTrackW::Iterator::hasNextEvent() {
    Boolean res;
    m_lastErrorCode = MusicEventIteratorHasNextEvent(mIterator, &res);
    return res;
}

Boolean MusicTrackW::Iterator::hasCurrentEvent() {
    Boolean res;
    m_lastErrorCode = MusicEventIteratorHasCurrentEvent(mIterator, &res);
    return res;
}


MusicTrackW::Iterator MusicTrackW::iterator() {
    MusicEventIterator res;
    m_lastErrorCode = NewMusicEventIterator(m_track, &res);
    return Iterator(m_track, res);
}

