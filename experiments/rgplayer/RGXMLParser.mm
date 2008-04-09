//
//  RGXMLParser.m
//  rgplayer
//
//  Created by Guillaume Laurent on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "RGXMLParser.h"
#import "EventXMLParserDelegate.h"

#include "Composition.h"
#include "Track.h"

@implementation RGXMLParser

@synthesize currentSegment;
@synthesize composition;

-(id)initWithData:(NSData*)data {
    self = [super initWithData:data];
    delegates = [[NSMutableArray alloc] init];
    return self;
}

-(void)pushDelegate:(id)delegate {
    [delegates addObject:delegate];
    [self setDelegate:delegate];
}

-(id)popDelegate {
    if ([delegates count] > 0) {
        id oldDelegate = [delegates lastObject];
        [delegates removeLastObject];
        [self setDelegate:[delegates lastObject]];
        return oldDelegate;
    }
    return nil;
}


- (void)parserDidStartDocument:(NSXMLParser *)parser {
	NSLog(@"start document");
    composition = new Rosegarden::Composition();
}

- (void)parserDidEndDocument:(NSXMLParser *)parser {
	NSLog(@"end document");
}

- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qualifiedName attributes:(NSDictionary *)attributeDict
{
	if ([elementName isEqualToString:@"composition"]) {
        
        //
        // parse composition attributes
        //
        NSString* defaultTempoStr = [attributeDict objectForKey:@"compositionDefaultTempo"];
        if (defaultTempoStr != nil) {
            Rosegarden::tempoT tempo = Rosegarden::tempoT([defaultTempoStr intValue]);
            composition->setCompositionDefaultTempo(tempo);
        } else {
            defaultTempoStr = [attributeDict objectForKey:@"defaultTempo"];
            if (defaultTempoStr != nil) {
                double tempo = [defaultTempoStr doubleValue];
                composition->setCompositionDefaultTempo(Rosegarden::Composition::getTempoForQpm(tempo));
            }
        }
        
        int position = 0;
        NSString* pointerPosStr = [attributeDict objectForKey:@"pointer"];
        if (pointerPosStr != nil) {
            position = [pointerPosStr intValue];
        }
        composition->setPosition(position);
     
        NSString* startMarkerStr = [attributeDict objectForKey:@"startMarker"];
        NSString* endMarkerStr = [attributeDict objectForKey:@"endMarker"];
        if (startMarkerStr != nil) {
            composition->setStartMarker([startMarkerStr intValue]);
        }
        if (endMarkerStr != nil) {
            composition->setEndMarker([endMarkerStr intValue]);
        }
        
    } else if ([elementName isEqualToString:@"track"]) {
        int trackId = -1;
        int position = -1;
        int instrument = -1;
        std::string label;
        bool muted = false;
        
        NSString* trackNbStr = [attributeDict objectForKey:@"id"];
        if (trackNbStr != nil) {
            trackId = [trackNbStr intValue];
        }
        
        NSString* labelStr = [attributeDict objectForKey:@"label"];

        if (labelStr != nil) {
            label = [labelStr cStringUsingEncoding:[NSString defaultCStringEncoding]];
        }
        
        NSString* mutedStr = [attributeDict objectForKey:@"muted"];
        if (mutedStr != nil) {
            if ([mutedStr isEqualToString:@"true"])
                muted = true;
            else
                muted = false;
        }
        
        NSString* positionStr = [attributeDict objectForKey:@"position"];
        if (positionStr != nil) {
            position = [positionStr intValue];
        }
        
        NSString* instrumentStr = [attributeDict objectForKey:@"instrument"];
        if (instrumentStr != nil) {
            instrument = [instrumentStr intValue];
        }
        
        Rosegarden::Track *track = new Rosegarden::Track(trackId,
                                 instrument,
                                 position,
                                 label,
                                 muted);
        
        // track properties affecting newly created segments are initialized
        // to default values in the ctor, so they don't need to be initialized
        // here
        composition->addTrack(track);
        
    } else if ([elementName isEqualToString:@"segment"]) {
        
        segmentEndMarkerTimeSet = false;
        int startTime = 0;
        NSString* startIdxStr = [attributeDict objectForKey:@"start"];
        if (startIdxStr != nil) {
            startTime = [startIdxStr intValue];
        }
        currentTime = startTime;
        
        NSString* segmentType = [attributeDict objectForKey:@"type"];
        if (segmentType != nil && [segmentType isEqualToString:@"audio"]) {
                int audioFileId = [[attributeDict objectForKey:@"file"] intValue];
                
                currentSegment = new Rosegarden::Segment(Rosegarden::Segment::Audio);
                currentSegment->setAudioFileId(audioFileId);
                currentSegment->setStartTime(startTime);
                
        } else {
            currentSegment = new Rosegarden::Segment(Rosegarden::Segment::Internal);
        }

        int track = -1;

        NSString* trackNbStr = [attributeDict objectForKey:@"track"];
        if (trackNbStr != nil) {
            track = [trackNbStr intValue];
        }
        currentSegment->setTrack(track);
                
        
        NSString* repeatStr = [attributeDict objectForKey:@"repeat"];
        if ([repeatStr isEqualToString:@"true"]) {
            currentSegment->setRepeating(true);
        }
        
        NSString* transposeStr = [attributeDict objectForKey:@"transpose"];
        if (transposeStr != nil) {
            currentSegment->setTranspose([transposeStr intValue]);
        }
        
        NSString* label = [attributeDict objectForKey:@"label"];
        if (label != nil) {
            std::string s = [label cStringUsingEncoding:[NSString defaultCStringEncoding]]; 
            currentSegment->setLabel(s);
        }
        
        NSString* triggerIdStr = [attributeDict objectForKey:@"triggerid"];
        NSString* triggerPitchStr = [attributeDict objectForKey:@"triggerbasepitch"];
        NSString* triggerVelociyStr = [attributeDict objectForKey:@"triggerbasevelocity"];
        NSString* triggerRetuneStr = [attributeDict objectForKey:@"triggerretune"];
        NSString* triggerAdjustTimeStr = [attributeDict objectForKey:@"triggeradjusttimes"];
        
        if (triggerIdStr != nil) {
            int pitch = -1;
            if (triggerPitchStr != nil) {
                pitch = [triggerPitchStr intValue];
            }
            int velocity = -1;
            if (triggerVelociyStr != nil) {
                velocity = [triggerVelociyStr intValue];
            }
            Rosegarden::TriggerSegmentRec *rec = composition->addTriggerSegment(currentSegment,
                                                                                [triggerIdStr intValue],
                                                                                pitch, velocity);
            if (rec) {
                if (triggerRetuneStr != nil) {
                    rec->setDefaultRetune([triggerRetuneStr isEqualToString:@"true"]);
                }
                if (triggerAdjustTimeStr != nil) {
                    rec->setDefaultTimeAdjust([triggerAdjustTimeStr cStringUsingEncoding:[NSString defaultCStringEncoding]]);
                }
            }
            currentSegment->setStartTimeDataMember(startTime);
        } else {
            composition->addSegment(currentSegment);
            composition->setSegmentStartTime(currentSegment, startTime);
        }
        
        NSString* endMarkerStr = [attributeDict objectForKey:@"endmarker"];
        if (endMarkerStr != nil) {
            segmentEndMarkerTime = [endMarkerStr intValue];
            segmentEndMarkerTimeSet = true;
        }
        
        EventXMLParserDelegate* eventXMLParserDelegate = [[EventXMLParserDelegate alloc] init:self];
        [self pushDelegate:eventXMLParserDelegate];
        
    }
         
}

- (void)parser:(NSXMLParser *)parser parseErrorOccurred:(NSError *)parseError {
	NSString* errorString = [NSString stringWithFormat:@"Error %i, Description: %@, Line: %i, Column: %i", [parseError code],
                             [[parser parserError] localizedDescription], [parser lineNumber],
                             [parser columnNumber]];
	NSLog(errorString);
}

@end
