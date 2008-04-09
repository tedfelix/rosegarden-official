//
//  CompositionWrapper.m
//  rgplayer
//
//  Created by Guillaume Laurent on 3/5/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "CompositionWrapper.h"
#import "CompositionItemWrapper.h"
#include "base/Composition.h"

@implementation CompositionWrapper

-(id)initWithComposition:(Rosegarden::Composition*)aComposition {
    self = [super init];
    composition = aComposition;
    currentSegment = nil;
    currentIndex = 0;
    return self; 
}

-(NSInteger)getNbOfTracks {
    return composition->getNbTracks();
}

-(NSInteger)getNbOfSegmentsForTrack:(Rosegarden::Track*)track {
    
    NSInteger res = 0;
    
    Rosegarden::TrackId trackId = track->getId();
    const Rosegarden::Composition::segmentcontainer& segments = composition->getSegments();
    for(Rosegarden::Composition::const_iterator i = segments.begin(); i != segments.end(); ++i) {
        
        const Rosegarden::Segment* segment = *i;
        if (segment->getTrack() == trackId)
            ++res;
    }
    return res;
}

-(NSInteger)getNbOfEventsForSegment:(Rosegarden::Segment*)segment {
    return segment->size();
}

-(id)getChild:(NSInteger)index ofItem:(id)item {
    if(item == nil) {
        return [CompositionItemWrapper wrapperWithTrack:composition->getTrackById(index)];
    }
    
    if ([item track] != nil) {
        Rosegarden::Track* track = [item track];
        NSInteger count = 0;
        const Rosegarden::Composition::segmentcontainer& segments = composition->getSegments();
        for(Rosegarden::Composition::const_iterator it = segments.begin(); it != segments.end(); ++it) {
            Rosegarden::Segment* s = *it;
            if (s->getTrack() == track->getId()) {
                if (count == index) {
                    return [CompositionItemWrapper wrapperWithSegment:s];
                }
                ++count;                
            }
        }
    }
    
    if ([item segment] != nil) {
        Rosegarden::Segment* segment = [item segment];
        
        if (segment != currentSegment || index < currentIndex) {
            currentIndex = 0;
            currentSegment = segment;            
            currentSegmentIterator = segment->begin();
        }

        for(; currentSegmentIterator != segment->end() && currentIndex < index; ++currentSegmentIterator) {
            // fetch Nth event
            ++currentIndex;            
        }
        
        if (currentSegmentIterator != segment->end()) {
            return [CompositionItemWrapper wrapperWithEvent:(*currentSegmentIterator)];            
        }
    }
    
    // should never happen
    return nil;
    
}

- (NSInteger) numberOfChildrenOfItem:(id)item {
    
    if(item == nil) { // this is the root node, so return number of tracks
        return composition->getTracks().size();
    }
    
    if ([item track] != nil) {
        Rosegarden::Track* track = [item track];
        return [self getNbOfSegmentsForTrack:track];
    }
    
    if ([item segment]) {
        return ([item segment]->size());
    }
    
    // should never happen
    return 0;
}

-(id)itemValue:(id)item forColumn:(NSString*)identifier {
   
    if (item == nil) {
        return @"";
    }
    
    CompositionItemWrapper* wrapper = item;

    Rosegarden::Segment* segment = [wrapper segment];
    Rosegarden::Event* event = [wrapper event];
    
    if (segment != nil) {
        
        if ([identifier isEqualToString:@"trackNumber"]) {
            return [NSString stringWithFormat:@"%d", segment->getTrack()];
        }
        if ([identifier isEqualToString:@"type"]) {
            if (segment->getType() == Rosegarden::Segment::Internal) {
                return @"MIDI";
            }
            return @"Audio";
        }
        if ([identifier isEqualToString:@"duration"]) {
            return [NSString stringWithFormat:@"%d", segment->getEndTime() - segment->getStartTime()];
        }
        
        return @"";
    }
    
    if (event != nil) {
        NSString* res = @"";
        
        if ([identifier isEqualToString:@"type"]) {
            res = [NSString stringWithCString:event->getType().c_str() encoding:[NSString defaultCStringEncoding]];
        } else if ([identifier isEqualToString:@"time"]) {
            res = [NSString stringWithFormat:@"%d", event->getAbsoluteTime()];
        } else if ([identifier isEqualToString:@"duration"]) {
            res = [NSString stringWithFormat:@"%d", event->getDuration()];
        }
        
        
        return res;
    }
    
    // in all other cases, no value should be displayed
    return @"";
}

@synthesize composition;

@end
