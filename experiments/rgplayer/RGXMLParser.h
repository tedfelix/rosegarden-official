//
//  RGXMLParser.h
//  rgplayer
//
//  Created by Guillaume Laurent on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "base/Event.h"

namespace Rosegarden {
    class Composition;
    class Segment;
}

@interface RGXMLParser : NSXMLParser {

    Rosegarden::Composition* composition;
    Rosegarden::Segment* currentSegment;
    Rosegarden::timeT currentTime;
    Rosegarden::timeT segmentEndMarkerTime;
    BOOL segmentEndMarkerTimeSet;
    NSMutableArray* delegates;
}

@property(readonly) Rosegarden::Segment* currentSegment;
@property(readonly) Rosegarden::Composition* composition;


-(id)initWithData:(NSData*)data;

-(void)pushDelegate:(id)delegate;

-(id)popDelegate;

@end
