//
//  MyDocument.m
//  rgplayer
//
//  Created by Guillaume Laurent on 2/10/08.
//  Copyright __MyCompanyName__ 2008 . All rights reserved.
//

#import "RGDocument.h"
#import "RGDocumentLoader.h"
#import "CompositionItemWrapper.h"
#import "CompositionWrapper.h"

#include "base/Track.h"
#include "base/Segment.h"
#include "base/Event.h"

#include "coremidiwrapper/CompositionToMusicSequence.h"

@implementation RGDocument

- (id)init
{
    self = [super init];
    return self;
}

- (NSString *)windowNibName
{
    // Override returning the nib file name of the document
    // If you need to use a subclass of NSWindowController or if your document supports multiple NSWindowControllers, you should remove this method and override -makeWindowControllers instead.
    return @"MyDocument";
}

- (void)windowControllerDidLoadNib:(NSWindowController *) aController
{
    [super windowControllerDidLoadNib:aController];
    // Add any code here that needs to be executed once the windowController has loaded the document's window.
    [self updateUI];
}

- (NSData *)dataOfType:(NSString *)typeName error:(NSError **)outError
{
    // Insert code here to write your document to data of the specified type. If the given outError != NULL, ensure that you set *outError when returning nil.

    // You can also choose to override -fileWrapperOfType:error:, -writeToURL:ofType:error:, or -writeToURL:ofType:forSaveOperation:originalContentsURL:error: instead.

    // For applications targeted for Panther or earlier systems, you should use the deprecated API -dataRepresentationOfType:. In this case you can also choose to override -fileWrapperRepresentationOfType: or -writeToFile:ofType: instead.

    if ( outError != NULL ) {
		*outError = [NSError errorWithDomain:NSOSStatusErrorDomain code:unimpErr userInfo:NULL];
	}
	return nil;
}

- (BOOL)readFromURL:(NSURL *)url ofType:(NSString *)typeName error:(NSError **)outError
{
    // Insert code here to read your document from the given data of the specified type.  If the given outError != NULL, ensure that you set *outError when returning NO.

    RGDocumentLoader *docLoader = [[RGDocumentLoader alloc] initWithUrl:url];
    
    BOOL res = [docLoader parse];
    
    if ( res == NO && outError != NULL ) {
		*outError = [NSError errorWithDomain:NSOSStatusErrorDomain code:readErr userInfo:NULL];
	} else {
        compositionWrapper = [[CompositionWrapper alloc] initWithComposition:[docLoader getComposition]];
    }
    return YES;
}

- (void)updateUI
{
    [outlineView reloadData];
}

- (id)outlineView:(NSOutlineView *)outlineView
            child:(NSInteger)index
           ofItem:(id)item {
    
    return [compositionWrapper getChild:index ofItem:item];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    CompositionItemWrapper* wrapper = item;
    
    return (item == nil) || ([wrapper track] != nil) || ([wrapper segment] != nil);
}

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    return [compositionWrapper numberOfChildrenOfItem:item];
}

- (id)outlineView:(NSOutlineView *)outlineView
    objectValueForTableColumn:(NSTableColumn *)tableColumn
           byItem:(id)item {

    NSString* identifier = [tableColumn identifier];
//    NSLog([NSString stringWithFormat:@"objectValueForTableColumn - tableColumn = %@", identifier]);
//    NSLog([NSString stringWithFormat:@"objectValueForTableColumn - item = %@", item]);
    
    return [compositionWrapper itemValue:item forColumn:identifier];
    
}

//
// Actions : play, rewind
//
static OSStatus GetSynthFromGraph (AUGraph & inGraph, AudioUnit &outSynth);
static OSStatus SetUpGraph (AUGraph &inGraph, UInt32 numFrames, Float64 &outSampleRate, bool isOffline);
static void PlayLoop (MusicPlayerW &player, AUGraph &graph, MusicTimeStamp sequenceLength);

- (IBAction)playCompo:(id)sender {
    NSLog(@"play");
    
    CompositionToMusicSequence compositionToMusicSequence;
    
    MusicSequenceW musicSequence = compositionToMusicSequence.convertComposition(*[compositionWrapper composition]);
    
    
    AUGraph graph = 0;
    AudioUnit theSynth = 0;
    OSStatus result;
    Float32 maxCPULoad = .8;
	Float64 srate = 0;
	const char* outputFilePath = 0;
	
	Float32 startTime = 0;
	UInt32 numFrames = 512;
    
    graph = musicSequence.getAUGraph();
    AUGraphOpen(graph);
    
    
    result = GetSynthFromGraph(graph, theSynth);
    result = AudioUnitSetProperty(theSynth,
                                  kAudioUnitProperty_CPULoad,
                                  kAudioUnitScope_Global, 0,
                                  &maxCPULoad, sizeof(maxCPULoad));
    
//    if (shouldUseMIDIEndpoint) 
//    {
//        MIDIClientRef	theMidiClient;
//        MIDIClientCreate(CFSTR("Play Sequence"), NULL, NULL, &theMidiClient);		
//        
//        ItemCount destCount = MIDIGetNumberOfDestinations();
//        if (destCount == 0) {
//            fprintf (stderr, "No MIDI Endpoints to play to.\n");
//            exit(1);
//        }
//        
//        require_noerr (result = MusicSequenceSetMIDIEndpoint (sequence, MIDIGetDestination(0)), fail);
//    } 
//    else 
//    {   
    
    // sound font load
    //
    
//        if (shouldSetBank) {                
//            FSRef soundBankRef;
//            require_noerr (result = FSPathMakeRef ((const UInt8*)bankPath, &soundBankRef, 0), fail);
//            
//            printf ("Setting Sound Bank:%s\n", bankPath);
//            
//            require_noerr (result = AudioUnitSetProperty (theSynth,
//                                                          kMusicDeviceProperty_SoundBankFSRef,
//                                                          kAudioUnitScope_Global, 0,
//                                                          &soundBankRef, sizeof(soundBankRef)), fail);
//        }

    // wav export
    //
//        if (outputFilePath) {
//            // need to tell synth that is going to render a file.
//            UInt32 value = 1;
//            require_noerr (result = AudioUnitSetProperty (theSynth,
//                                                          kAudioUnitProperty_OfflineRender,
//                                                          kAudioUnitScope_Global, 0,
//                                                          &value, sizeof(value)), fail);
//        }
        
        SetUpGraph (graph, numFrames, srate, (outputFilePath != NULL));
        
        result = AUGraphInitialize (graph);
        
//    }
    
    MusicPlayerW player;
    player.setSequence(musicSequence);

    // debug
    
//    MusicSequenceW testSequence;
//    MusicTrack testTrack = testSequence.newTrack();
//    MusicTrackW testTrackW(testTrack);
//
//    MIDINoteMessage message;
//    message.channel = 1;
//    message.note = 61;
//    message.velocity = 83;
//    message.duration = 1.0;
//
//    testTrackW.newMIDINoteEvent(0, &message);
    
//    player.setSequence(testSequence);
    
    // end debug
    
    MusicTrack testTrack = musicSequence.getIndTrack(0);
    MusicEventIterator iter;
    NewMusicEventIterator(testTrack, &iter);
//    MusicEventIteratorNextEvent(iter);
    Boolean hasEvt = false;
    MusicEventIteratorHasCurrentEvent(iter, &hasEvt);
    if (hasEvt) {
        MusicTimeStamp ts;
        MusicEventType et;
        const void* eventData;
        UInt32 dataSize;
        MusicEventIteratorGetEventInfo(iter, &ts, &et, &eventData, &dataSize);
        printf("timeStamp : %d - eventType : %d\n", ts, et);
    }
    
    
	// figure out sequence length
    UInt32 ntracks = musicSequence.getTrackCount();
    MusicTimeStamp sequenceLength = 0;
    for (UInt32 i = 0; i < ntracks; ++i) {
        MusicTrack track;
        MusicTimeStamp trackLength;
        UInt32 propsize = sizeof(MusicTimeStamp);
        track = musicSequence.getIndTrack(i);
        MusicTrackW trackW(track);
        trackW.getProperty(kSequenceTrackProperty_TrackLength, &trackLength, &propsize);
        
        if (trackLength > sequenceLength)
            sequenceLength = trackLength;
    }
	
	// now I'm going to add 8 beats on the end for the reverb/long releases to tail off...
    sequenceLength += 8;
    
    player.setTime(startTime);
    player.preroll();
    
    UInt64 startRunningTime = AudioGetCurrentHostTime ();
    
    player.start();
    
//    if (outputFilePath) 
//        WriteOutputFile (outputFilePath, dataFormat, srate, sequenceLength, shouldPrint, graph, numFrames, player);
//    else
        PlayLoop (player, graph, sequenceLength);
    
    player.stop();
    
}

- (IBAction)rewindCompo:(id)sender {
    NSLog(@"rewind");
}

OSStatus SetUpGraph (AUGraph &inGraph, UInt32 numFrames, Float64 &sampleRate, bool isOffline)
{
	OSStatus result = noErr;
	AudioUnit outputUnit = 0;
	AUNode outputNode;
	
	// the frame size is the I/O size to the device
	// the device is going to run at a sample rate it is set at
	// so, when we set this, we also have to set the max frames for the graph nodes
	UInt32 nodeCount;
	require_noerr (result = AUGraphGetNodeCount (inGraph, &nodeCount), home);
    
	for (int i = 0; i < (int)nodeCount; ++i) 
	{
		AUNode node;
		require_noerr (result = AUGraphGetIndNode(inGraph, i, &node), home);
        
		ComponentDescription desc;
		AudioUnit unit;
		require_noerr (result = AUGraphNodeInfo(inGraph, node, &desc, &unit), home);
		
		if (desc.componentType == kAudioUnitType_Output) 
		{
			if (outputUnit == 0) {
				outputUnit = unit;
				require_noerr (result = AUGraphNodeInfo(inGraph, node, 0, &outputUnit), home);
				
				if (!isOffline) {
					// these two properties are only applicable if its a device we're playing too
					require_noerr (result = AudioUnitSetProperty (outputUnit, 
                                                                  kAudioDevicePropertyBufferFrameSize, 
                                                                  kAudioUnitScope_Output, 0,
                                                                  &numFrames, sizeof(numFrames)), home);
                    
//					require_noerr (result = AudioUnitAddPropertyListener (outputUnit, 
//                                                                          kAudioDeviceProcessorOverload, 
//                                                                          OverloadListenerProc, 0), home);
                    
					// if we're rendering to the device, then we render at its sample rate
					UInt32 theSize;
					theSize = sizeof(sampleRate);
					
					require_noerr (result = AudioUnitGetProperty (outputUnit,
                                                                  kAudioUnitProperty_SampleRate,
                                                                  kAudioUnitScope_Output, 0,
                                                                  &sampleRate, &theSize), home);
				} else {
                    // remove device output node and add generic output
					require_noerr (result = AUGraphRemoveNode (inGraph, node), home);
					desc.componentSubType = kAudioUnitSubType_GenericOutput;
					require_noerr (result = AUGraphAddNode (inGraph, &desc, &node), home);
					require_noerr (result = AUGraphNodeInfo(inGraph, node, NULL, &unit), home);
					outputUnit = unit;
					outputNode = node;
					
					// we render the output offline at the desired sample rate
					require_noerr (result = AudioUnitSetProperty (outputUnit,
                                                                  kAudioUnitProperty_SampleRate,
                                                                  kAudioUnitScope_Output, 0,
                                                                  &sampleRate, sizeof(sampleRate)), home);
				}
				// ok, lets start the loop again now and do it all...
				i = -1;
			}
		}
		else
		{
            // we only have to do this on the output side
            // as the graph's connection mgmt will propogate this down.
			if (outputUnit) {	
                // reconnect up to the output unit if we're offline
				if (isOffline && desc.componentType != kAudioUnitType_MusicDevice) {
					require_noerr (result = AUGraphConnectNodeInput (inGraph, node, 0, outputNode, 0), home);
				}
				
				require_noerr (result = AudioUnitSetProperty (unit,
                                                              kAudioUnitProperty_SampleRate,
                                                              kAudioUnitScope_Output, 0,
                                                              &sampleRate, sizeof(sampleRate)), home);
                
                
			}
		}
		require_noerr (result = AudioUnitSetProperty (unit, kAudioUnitProperty_MaximumFramesPerSlice,
                                                      kAudioUnitScope_Global, 0,
                                                      &numFrames, sizeof(numFrames)), home);
	}
	
home:
	return result;
}

void PlayLoop (MusicPlayerW &player, AUGraph &graph, MusicTimeStamp sequenceLength)
{
	while (1) {
		usleep (2 * 1000 * 1000);
		
//		if (didOverload) {
//			printf ("* * * * * %ld Overloads detected on device playing audio\n", didOverload);
//			overloadTime = AudioConvertHostTimeToNanos (overloadTime - startRunningTime);
//			printf ("\tSeconds after start = %lf\n", double(overloadTime / 1000000000.));
//			didOverload = 0;
//		}
        		
		MusicTimeStamp time = player.getTime();
        
		if (time >= sequenceLength || time == 0)
			break;
	}
	
}

OSStatus GetSynthFromGraph (AUGraph& inGraph, AudioUnit& outSynth)
{	
	UInt32 nodeCount;
	OSStatus result = noErr;
	require_noerr (result = AUGraphGetNodeCount (inGraph, &nodeCount), fail);
	
	for (UInt32 i = 0; i < nodeCount; ++i) 
	{
		AUNode node;
		require_noerr (result = AUGraphGetIndNode(inGraph, i, &node), fail);
        
		ComponentDescription desc;
		require_noerr (result = AUGraphNodeInfo(inGraph, node, &desc, 0), fail);
		
		if (desc.componentType == kAudioUnitType_MusicDevice) 
		{
			require_noerr (result = AUGraphNodeInfo(inGraph, node, 0, &outSynth), fail);
			return noErr;
		}
	}
	
fail:		// didn't find the synth AU
	return -1;
}


@end
