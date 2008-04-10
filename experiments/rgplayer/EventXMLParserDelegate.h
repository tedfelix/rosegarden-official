//
//  EventXMLParserDelegate.h
//  rgplayer
//
//  Created by Guillaume Laurent on 3/2/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "RGXMLParser.h"

#include "base/Event.h"

@interface EventXMLParserDelegate : NSObject {
    RGXMLParser* parent;
    Rosegarden::Event* currentEvent;
    NSArray* standardKeys;
    Rosegarden::timeT currentTime;
    BOOL inChord;
}

@property(readonly) Rosegarden::Event* currentEvent;
@property(readwrite) BOOL inChord;

- (id)init:(RGXMLParser*)parentParser;
- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict;
- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName;


@end
