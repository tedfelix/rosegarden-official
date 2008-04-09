//
//  EventXMLParserDelegate.h
//  rgplayer
//
//  Created by Guillaume Laurent on 3/2/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "RGXMLParser.h"

namespace Rosegarden {
    class Event;    
}

@interface EventXMLParserDelegate : NSObject {
    RGXMLParser* parent;
    Rosegarden::Event* event;
    NSArray* standardKeys;
}

@property(readonly) Rosegarden::Event* event;

- (id)init:(RGXMLParser*)parentParser;
- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict;
- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName;


@end
