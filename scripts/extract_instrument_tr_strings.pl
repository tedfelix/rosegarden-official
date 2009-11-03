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

# I will add these in this file, even though they are not related to the names
# in presets.xml.  Let's get EVERY note name translated, and we'll separate the
# name from the octave component in strings like "C-2" so nobody has to
# translate the same damn thing again and again but for the number

print 'QObject::tr("Cb", "note name");
';
print 'QObject::tr("C", "note name");
';
print 'QObject::tr("C#", "note name");
';
print 'QObject::tr("Db", "note name");
';
print 'QObject::tr("D", "note name");
';
print 'QObject::tr("Eb", "note name");
';
print 'QObject::tr("E", "note name");
';
print 'QObject::tr("E#", "note name");
';
print 'QObject::tr("Fb", "note name");
';
print 'QObject::tr("F", "note name");
';
print 'QObject::tr("F#", "note name");
';
print 'QObject::tr("G", "note name");
';
print 'QObject::tr("G#", "note name");
';
print 'QObject::tr("Ab", "note name");
';
print 'QObject::tr("A#", "note name");
';
print 'QObject::tr("A", "note name");
';
print 'QObject::tr("Bb", "note name");
';
print 'QObject::tr("B", "note name");
';
print 'QObject::tr("B#", "note name");
';
