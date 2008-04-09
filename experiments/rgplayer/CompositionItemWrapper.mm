//
//  CompositionItemWrapper.mm
//  rgplayer
//
//  Created by Guillaume Laurent on 3/7/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "CompositionItemWrapper.h"

@implementation CompositionItemWrapper

static NSMutableDictionary* trackWrappers = nil;
static NSMutableDictionary* segmentWrappers = nil;
static NSMutableDictionary* eventWrappers = nil;

-(id)init:(Rosegarden::Track*)aTrack segment:(Rosegarden::Segment*)aSegment event:(Rosegarden::Event*)anEvent {
    self = [super init];
    track   = aTrack;
    segment = aSegment;
    event   = anEvent;
    
//    NSLog([NSString stringWithFormat:@"compositionItemWrapper : %@ track = %x, segment = %x, event = %x", self, track, segment, event]);
    
    return self;
} 

//-(void)finalize {
//    NSLog([NSString stringWithFormat:@"CompositionItemWrapper - finalize : %@", self]);
//    [super finalize];
//}

+(id)wrapperWithTrack:(Rosegarden::Track*)aTrack {
    [CompositionItemWrapper setupMapTables];
    
    id trackAsKey = [NSValue valueWithPointer:aTrack];
    
    id wrapper = [trackWrappers objectForKey:trackAsKey];
    
    if (wrapper == nil) {
        wrapper = [[CompositionItemWrapper alloc] init:aTrack segment:nil event:nil];
        [trackWrappers setObject:wrapper forKey:trackAsKey];
    } 
    return (id)wrapper;
}

+(id)wrapperWithSegment:(Rosegarden::Segment*)aSegment {
    [CompositionItemWrapper setupMapTables];
    
    id segmentAsKey = [NSValue valueWithPointer:aSegment];
    id wrapper = [segmentWrappers objectForKey:segmentAsKey];
    
    if (wrapper == nil) {
        wrapper = [[CompositionItemWrapper alloc] init:nil segment:aSegment event:nil];
        [segmentWrappers setObject:wrapper forKey:segmentAsKey];
    }

    return (id)wrapper;
}

+(id)wrapperWithEvent:(Rosegarden::Event*)anEvent {
    [CompositionItemWrapper setupMapTables];
    
    id eventAsKey = [NSValue valueWithPointer:anEvent];
    id wrapper = [eventWrappers objectForKey:eventAsKey];
    
    if (wrapper == nil) {
        wrapper = [[CompositionItemWrapper alloc] init:nil segment:nil event:anEvent];
        [eventWrappers setObject:wrapper forKey:eventAsKey];
    }
    
    return (id)wrapper;
}

+(void)setupMapTables {
    if (trackWrappers == 0) {
        trackWrappers = [NSMutableDictionary dictionaryWithCapacity:64];
    }
    
    if (segmentWrappers == 0) {
        segmentWrappers = [NSMutableDictionary dictionaryWithCapacity:128];
    }
    
    if (eventWrappers == 0) {
        eventWrappers = [NSMutableDictionary dictionaryWithCapacity:1024];
    }
    
    
}

@synthesize track;
@synthesize segment;
@synthesize event;


@end
