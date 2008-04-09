//
//  MyDocument.h
//  rgplayer
//
//  Created by Guillaume Laurent on 2/10/08.
//  Copyright __MyCompanyName__ 2008 . All rights reserved.
//


#import <Cocoa/Cocoa.h>

@class CompositionWrapper;

namespace Rosegarden {
    class Composition;
}

@interface RGDocument : NSDocument
{
    IBOutlet NSOutlineView *outlineView;
    CompositionWrapper* compositionWrapper;
}

- (void)updateUI;

// Data Source methods
- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item;
- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item;
- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item;
- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;
//- (void)outlineView:(NSOutlineView *)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item;

- (IBAction)playCompo:(id)sender;
- (IBAction)rewindCompo:(id)sender;

@end
