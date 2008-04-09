//
//  RGDocumentLoader.m
//  rgplayer
//
//  Created by Guillaume Laurent on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "RGDocumentLoader.h"
#include "RGXMLParser.h"

@implementation RGDocumentLoader

-(id)initWithUrl:(NSURL*)url {
    
    self = [super init];
    parsingDone = NO;
    
    // collect data from URL by piping it through gzip
    //
    NSData *gzipData = [NSData dataWithContentsOfURL:url];
	
    NSTask *gzipTask = [[NSTask alloc] init];
    [gzipTask setLaunchPath:@"/usr/bin/gzip"];
    NSArray* args = [NSArray arrayWithObjects:@"-d", @"-c", @"-f", @"-", nil ];
    [gzipTask setArguments: args];
	
    NSPipe *inputPipe  = [NSPipe pipe];
    NSPipe *outputPipe = [NSPipe pipe];
    
    [gzipTask setStandardInput:inputPipe];
    [gzipTask setStandardOutput:outputPipe];
    [gzipTask launch];
    [[inputPipe fileHandleForWriting] writeData:gzipData];
    [[inputPipe fileHandleForWriting] closeFile];
    
    NSData *inflatedData = [[outputPipe fileHandleForReading] readDataToEndOfFile];
    
    // we have the unzipped data, now parse it
    //
    parser = [RGXMLParser alloc];
 
    [parser initWithData:inflatedData];
    
    [parser pushDelegate:parser];
    [parser setShouldResolveExternalEntities:YES];    
    
    
    return self;
}

-(BOOL)parse {
    BOOL res;
    
    if (!parsingDone) {
        res = [parser parse];
        parsingDone = YES;
    } 
    return res;
}

-(Rosegarden::Composition*)getComposition {
    return parser.composition;
}

@end
