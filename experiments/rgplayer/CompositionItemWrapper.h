//
//  CompositionItemWrapper.h
//  rgplayer
//
//  Created by Guillaume Laurent on 3/7/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

namespace Rosegarden {
    class Track;
    class Segment;
    class Event;
}

@interface CompositionItemWrapper : NSObject {
    Rosegarden::Track* track;
    Rosegarden::Segment* segment;
    Rosegarden::Event* event;
}

@property(readonly) Rosegarden::Track* track;
@property(readonly) Rosegarden::Segment* segment;
@property(readonly) Rosegarden::Event* event;

-(id)init:(Rosegarden::Track*)aTrack segment:(Rosegarden::Segment*)aSegment event:(Rosegarden::Event*)anEvent;
//-(void)finalize;

+(id)wrapperWithTrack:(Rosegarden::Track*)track;
+(id)wrapperWithSegment:(Rosegarden::Segment*)segment;
+(id)wrapperWithEvent:(Rosegarden::Event*)event;
+(void)setupMapTables;

@end
