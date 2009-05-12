#!/usr/bin/perl -w

use strict;

print qq{
/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
 * This file has been automatically generated and should not be compiled.
 *
 * The purpose of the file is to collect the translation strings from the
 * presets.xml file which are used to provide a sort of built-in
 * encyclopedia of arranging to aid composers.
 * 
 * The code is prepared for the lupdate program. There is make ts target:
 *
 *   make ts
 *
 * which is used to extract translation strings and to update ts/*.tr files.
 */

};

my $file = $ARGV[0];
my $nextfile = $ARGV[1];
my $category_name = "";
my $instrument_name = "";
while (<>) {
    if ($ARGV[0]) {
        if ($nextfile ne $ARGV[0]) {
            $file = $nextfile;
            $nextfile = $ARGV[0];
        }
    }
    my $line = $_;


    if ($line =~ /category name="([^"]*)"/) {
        $category_name = $1;
        $instrument_name = "";

        print 'QObject::tr("' . $category_name . '");';
        print ' /* ' . $file;
        print ' */
';
    } elsif ($line =~ /instrument name="([^"]*)"/) {
	$instrument_name = $1;

        print 'QObject::tr("' . $instrument_name . '");';
        print ' /* ' . $file;
        print ' : ' . $category_name;
        print ' */
';
    }
}

# I will add these in this file, even though they are not related to the names in presets.xml
#
# First, we have always had C-2 through G8 even though people in a lot of the
# world use do re mi names, and even in the world that uses letter names, # and
# b have no meaning as such, and then German has H for Bb and so forth.  Since
# Heikki came up with this way to translate dynamically-created data, let's
# extend the concept further by translating those note names.  I put in some
# temporary code to get the Pitch class to produce the following output:

print 'QObject::tr("C-2");';
print 'QObject::tr("C#-2");';
print 'QObject::tr("D-2");';
print 'QObject::tr("E#-2");';
print 'QObject::tr("E-2");';
print 'QObject::tr("F-2");';
print 'QObject::tr("F#-2");';
print 'QObject::tr("G-2");';
print 'QObject::tr("A#-2");';
print 'QObject::tr("A-2");';
print 'QObject::tr("B#-2"); /* TRANSLATOR Producing these strings for translation has revealed a BUG in Pitch.  Where you see B# you should read that as Bb.  Where you see E# you should read that as Eb.  I suggest using the equivalent of C# Eb F# Ab Bb in your translation, since we are no longer constrained by program logic and we can use the most comfortable names for each "black key" pitch to control what the user sees, regardless of the (sometimes incorrect) string Rosegarden uses internally */';
print 'QObject::tr("B-2");';
print 'QObject::tr("C-1");';
print 'QObject::tr("C#-1");';
print 'QObject::tr("D-1");';
print 'QObject::tr("E#-1");';
print 'QObject::tr("E-1");';
print 'QObject::tr("F-1");';
print 'QObject::tr("F#-1");';
print 'QObject::tr("G-1");';
print 'QObject::tr("A#-1");';
print 'QObject::tr("A-1");';
print 'QObject::tr("B#-1");';
print 'QObject::tr("B-1");';
print 'QObject::tr("C0");';
print 'QObject::tr("C#0");';
print 'QObject::tr("D0");';
print 'QObject::tr("E#0");';
print 'QObject::tr("E0");';
print 'QObject::tr("F0");';
print 'QObject::tr("F#0");';
print 'QObject::tr("G0");';
print 'QObject::tr("A#0");';
print 'QObject::tr("A0");';
print 'QObject::tr("B#0");';
print 'QObject::tr("B0");';
print 'QObject::tr("C1");';
print 'QObject::tr("C#1");';
print 'QObject::tr("D1");';
print 'QObject::tr("E#1");';
print 'QObject::tr("E1");';
print 'QObject::tr("F1");';
print 'QObject::tr("F#1");';
print 'QObject::tr("G1");';
print 'QObject::tr("A#1");';
print 'QObject::tr("A1");';
print 'QObject::tr("B#1");';
print 'QObject::tr("B1");';
print 'QObject::tr("C2");';
print 'QObject::tr("C#2");';
print 'QObject::tr("D2");';
print 'QObject::tr("E#2");';
print 'QObject::tr("E2");';
print 'QObject::tr("F2");';
print 'QObject::tr("F#2");';
print 'QObject::tr("G2");';
print 'QObject::tr("A#2");';
print 'QObject::tr("A2");';
print 'QObject::tr("B#2");';
print 'QObject::tr("B2");';
print 'QObject::tr("C3");';
print 'QObject::tr("C#3");';
print 'QObject::tr("D3");';
print 'QObject::tr("E#3");';
print 'QObject::tr("E3");';
print 'QObject::tr("F3");';
print 'QObject::tr("F#3");';
print 'QObject::tr("G3");';
print 'QObject::tr("A#3");';
print 'QObject::tr("A3");';
print 'QObject::tr("B#3");';
print 'QObject::tr("B3");';
print 'QObject::tr("C4");';
print 'QObject::tr("C#4");';
print 'QObject::tr("D4");';
print 'QObject::tr("E#4");';
print 'QObject::tr("E4");';
print 'QObject::tr("F4");';
print 'QObject::tr("F#4");';
print 'QObject::tr("G4");';
print 'QObject::tr("A#4");';
print 'QObject::tr("A4");';
print 'QObject::tr("B#4");';
print 'QObject::tr("B4");';
print 'QObject::tr("C5");';
print 'QObject::tr("C#5");';
print 'QObject::tr("D5");';
print 'QObject::tr("E#5");';
print 'QObject::tr("E5");';
print 'QObject::tr("F5");';
print 'QObject::tr("F#5");';
print 'QObject::tr("G5");';
print 'QObject::tr("A#5");';
print 'QObject::tr("A5");';
print 'QObject::tr("B#5");';
print 'QObject::tr("B5");';
print 'QObject::tr("C6");';
print 'QObject::tr("C#6");';
print 'QObject::tr("D6");';
print 'QObject::tr("E#6");';
print 'QObject::tr("E6");';
print 'QObject::tr("F6");';
print 'QObject::tr("F#6");';
print 'QObject::tr("G6");';
print 'QObject::tr("A#6");';
print 'QObject::tr("A6");';
print 'QObject::tr("B#6");';
print 'QObject::tr("B6");';
print 'QObject::tr("C7");';
print 'QObject::tr("C#7");';
print 'QObject::tr("D7");';
print 'QObject::tr("E#7");';
print 'QObject::tr("E7");';
print 'QObject::tr("F7");';
print 'QObject::tr("F#7");';
print 'QObject::tr("G7");';
print 'QObject::tr("A#7");';
print 'QObject::tr("A7");';
print 'QObject::tr("B#7");';
print 'QObject::tr("B7");';
print 'QObject::tr("C8");';
print 'QObject::tr("C#8");';
print 'QObject::tr("D8");';
print 'QObject::tr("E#8");';
print 'QObject::tr("E8");';
print 'QObject::tr("F8");';
print 'QObject::tr("F#8");';
print 'QObject::tr("G8");';
