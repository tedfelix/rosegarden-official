//
//  RGDocumentLoader.h
//  rgplayer
//
//  Created by Guillaume Laurent on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
@class RGXMLParser;

namespace Rosegarden {
    class Composition;
}

@interface RGDocumentLoader : NSObject {
    RGXMLParser* parser;
    BOOL parsingDone;
}

-(id)initWithUrl:(NSURL*)url;
-(BOOL)parse;
-(Rosegarden::Composition*)getComposition;

@end
