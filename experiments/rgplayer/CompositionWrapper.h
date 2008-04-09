//
//  CompositionWrapper.h
//  rgplayer
//
//  Created by Guillaume Laurent on 3/5/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "base/Segment.h"

namespace Rosegarden {
    class Composition;
    class Track;
    class Segment;
}

@interface CompositionWrapper : NSObject {
    Rosegarden::Composition* composition;
    Rosegarden::Segment* currentSegment;
    Rosegarden::Segment::const_iterator currentSegmentIterator; // we shouldn't be doing this - ctor won't be invoked - but we don't care in this case
    NSInteger currentIndex;
}

@property(readonly) Rosegarden::Composition* composition;

-(id)initWithComposition:(Rosegarden::Composition*)composition;
-(id)getChild:(NSInteger)index ofItem:(id)item;
-(NSInteger)numberOfChildrenOfItem:(id)item;
-(id)itemValue:(id)item forColumn:(NSString*)identifier;

-(NSInteger)getNbOfTracks;
-(NSInteger)getNbOfSegmentsForTrack:(Rosegarden::Track*)track;
-(NSInteger)getNbOfEventsForSegment:(Rosegarden::Segment*)segment;


@end
