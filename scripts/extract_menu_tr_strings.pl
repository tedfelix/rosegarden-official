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
 * *.rc files which are used in creating the menus.
 * 
 * The code is prepared for  lupdate  program. There is make ts target :
 *
 *   make -f qt4-makefile ts
 *
 * which is used to extract translation strings and to update ts/*.tr files.
 */

};

my $file = $ARGV[0];
my $nextfile = $ARGV[1];
while (<>) {
    if ($ARGV[0]) {
        if ($nextfile ne $ARGV[0]) {
            $file = $nextfile;
            $nextfile = $ARGV[0];
        }
    } else {
        $file = $nextfile;
    }
    my $line = $_;

    my $name = "";
    my $text = "";
    my $shortcut = "";
    my $tooltip = "";

    if ($line =~ /<text>(.*)<\/text>/) {
	$text = $1;
    } elsif ($line =~ /text="([^"]*)"/) {
	$text = $1;
    }
    $text =~ s/\&amp;/&/;

    if ($line =~ /name="([^"]*)"/) {
	$name = $1;
    }

    if ($text) {
        print 'QObject::tr("' . $text . '");';
        print ' /* ' . $file;
        if ($name) { print ' : ' . $name; }
        print " */\n";
    }

    # extract shortcuts
    #
    # NOTE: shortcuts do not have a comment field with them in order to avoid a
    # good amount of complication.  Instead, we write the comments in the code.
    if ($line =~ /shortcut="([^"]*)"/) {
        $shortcut = $1;
        print "\n/* TRANSLATOR: this is a keyboard shortcut for an English QWERTY keyboard.  If you adjust this to suit your language";
        print "\n * DO NOT translate English names for special keys like Ctrl Shift Alt Up Down and so on, because this will break the";
        print "\n * translation.  You should only translate single keys, like switching the meaning of Z and Y to fit better on a QWERTZ";
        print "\n * keyboard.  Treat ASCII characters as single keys.  ; : < > [ ] - = ( ) and so on, even if they require Shift or some";
        print "\n * special combination on your keyboard.*/\n";
        print 'QObject::tr("' . $shortcut . '", "keyboard shortcut");';
        if ($name) { print ' // ' . $name; }
        print "\n\n";
    }

    # extract tooltips
    if ($line =~ /tooltip="([^"]*)"/) {
        $tooltip = $1;
        print "\n/* TRANSLATOR: this is a tooltip.  If it contains a string in () this string is a keyboard shortcut";
        print "\n * and this shortcut SHOULD be translated to your target language, because Qt will not translate";
        print "\n * these strings for you.  You will need to sync these manually. */\n";
        print 'QObject::tr("' . $tooltip . '");';
        print "\n\n";
    }

}
