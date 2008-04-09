//
//  EventXMLParserDelegate.m
//  rgplayer
//
//  Created by Guillaume Laurent on 3/2/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "EventXMLParserDelegate.h"

#include "base/Event.h"
#include "base/Segment.h"
#include "base/Property.h"

@implementation EventXMLParserDelegate

@synthesize event;

- (id)init:(RGXMLParser*)parentParser {
    parent = parentParser;
    standardKeys = [NSArray arrayWithObjects:@"type", @"duration", @"subordering", @"absoluteTime", @"timeOffset", nil];
    return self;
}

- (void)parser:(NSXMLParser *)parser didStartElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName attributes:(NSDictionary *)attributeDict {
    
    if ([elementName isEqualToString:@"event"]) {
        
        NSString* typeStr = [attributeDict objectForKey:@"type"];
        NSString* durationStr = [attributeDict objectForKey:@"duration"];
        NSString* suborderingStr = [attributeDict objectForKey:@"subordering"];
        NSString* absoluteTimeStr = [attributeDict objectForKey:@"absoluteTime"];
        NSString* timeOffsetStr = [attributeDict objectForKey:@"timeOffset"];
        
        Rosegarden::timeT duration = 0, absoluteTime;
        short subordering = 0;
        
        if (durationStr != nil) {
            duration = [durationStr intValue];
        }
        if (suborderingStr != nil) {
            subordering = [suborderingStr intValue];
        }
        
        absoluteTime = [absoluteTimeStr intValue];
        
        if (timeOffsetStr != nil) {
            absoluteTime += [timeOffsetStr intValue];
        }
        
        event = new Rosegarden::Event([typeStr cStringUsingEncoding:[NSString defaultCStringEncoding]],
                                      absoluteTime, duration, subordering);
        
        for(NSString* key in attributeDict) {
            if ([standardKeys indexOfObjectIdenticalTo:key] == NSNotFound) {
                std::string keyc = [key cStringUsingEncoding:[NSString defaultCStringEncoding]];
                NSString* val = [attributeDict objectForKey:key];
                if ([val isEqualToString:@"true"] || [val isEqualToString:@"false"]) {
                    BOOL b = [val boolValue];
                    event->set<Rosegarden::Bool>(keyc, b);
                } else {
                    NSScanner* scanner = [NSScanner scannerWithString:val];                
                    int iVal = 0;
                    if ([scanner scanInt:&iVal]) {
                        event->set<Rosegarden::Int>(keyc, iVal);
                    } else {
                        event->set<Rosegarden::String>(keyc, [val cStringUsingEncoding:[NSString defaultCStringEncoding]]);
                    }
                    
                }
            } 
        }
        
    } else if ([elementName isEqualToString:@"property"]) {
        NSString* propertyName = [attributeDict objectForKey:@"name"];
        std::string propertyNameC = [propertyName cStringUsingEncoding:[NSString defaultCStringEncoding]];
        
        for(NSString* key in attributeDict) {            
            
            if ([key isEqualToString:@"name"]) 
                continue;

            NSString* val = [attributeDict objectForKey:key];
            
            NSScanner* scanner = [NSScanner scannerWithString:val];
            if ([key isEqualToString:@"int"]) {                                
                int iVal = 0;
                if ([scanner scanInt:&iVal]) {
                    event->set<Rosegarden::Int>(propertyNameC, iVal);
                }                
            } else if ([key isEqualToString:@"bool"]) {
                BOOL b = [val boolValue];
                event->set<Rosegarden::Bool>(propertyNameC, b);
            } else {
                event->set<Rosegarden::String>(propertyNameC, [val cStringUsingEncoding:[NSString defaultCStringEncoding]]);
            }
        }
        
    }
}

- (void)parser:(NSXMLParser *)parser didEndElement:(NSString *)elementName namespaceURI:(NSString *)namespaceURI qualifiedName:(NSString *)qName {
    
    if ([elementName isEqualToString:@"event"]) {
        parent.currentSegment->insert(event);
        event = 0;
    } else if ([elementName isEqualToString:@"segment"]) {
        [parent popDelegate];
    }
}


@end
