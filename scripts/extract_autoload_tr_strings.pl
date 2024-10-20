#!/usr/bin/perl -w

# Usage:
#  perl extract_autoload_tr_strings.pl [-info=<outputInfoFile>]  \
#      <inputFile1> [<inputFile2> ...] > <outPutFile.cpp>
#
#   "-info=" specify an optional file where the new translations source
#            and context are written
#   <inputFile1>, ... are the input .xml files
#   The output Qt C++ code is written on stdout


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
 * autoload.rg file, which will allow the basic General MIDI program names and
 * controllers to be translated for non-English users.  We will never be able to
 * translate all of the strings in all of the .rdg files in our library, but
 * being able to translate General MIDI and basic controller names should be
 * very nice for improving the non-English user experience.
 * 
 * The code is prepared for the lupdate program. There is make ts target:
 *
 *   make autoload-ts
 *
 * which is used to extract translation strings and to update ts/*.tr files.
 *
 * This code generated by D. Michael McIntyre modifying a script written by
 * Heikki Junes for extracting strings from presets.xml.  This modification
 * carries a really good idea one step further, and Heikki has made me feel
 * stupid for not thinking of this solution years ago!
 */

};


# If asked for, output a file for writing the list of
# translations with a new context
my $wantInfo = 0;
if ($ARGV[0] =~ /^-info=(.*)$/) {
    open INFO, ">$1" or die "Can't open $1 for writing";
    shift @ARGV;
    $wantInfo = 1;
}


sub output
{
    my ($context, $name, $comment) = @_;
    print 'QT_TRANSLATE_NOOP("', $context, '", "', $name, '");';
    print ' /* ', $comment, " */\n";
    print INFO $context, "\t", $name, "\n" if $wantInfo;
}


my $name = "";
while (<>) {
    my $line = $_;
    my $context = "";
    my $file = $ARGV;

    if ($line =~ /name="([^"]*)"/) {
        $name = $1;

        # strip out some special strings inside name="" fields that should not
        # be translated
        if (($name ne "copyright")        &&
            ($name ne "title")            &&
            ($name ne "subtitle")         &&
            ($name ne "author")           &&
            ($name ne "Michael McIntyre") &&
            ($name ne "colourmap")        &&
            ($name ne "segmentmap")) {

                # get more info about context
                if ($line =~ /<key number=/) {
                    $context = "INSTRUMENT";
                } elsif ($line =~ /<keymapping/) {
                    $context = "INSTRUMENT";
                 } elsif ($line =~ /<program/) {
                    $context = "INSTRUMENT";
                } elsif ($line =~ /<bank name=/) {
                    $context = "INSTRUMENT";
                } elsif ($line =~ /<control name=/) {
                    $context = "MIDI_CONTROLLER";
                } elsif ($line =~ /<colourpair/) {
                    $context = "COLOUR";
                } else {
                    # Keep QObject context
                }

                if ($context) {
                    output $context, $name, $file;
                } else {
                    print 'QObject::tr("', $name, '");';
                    print ' /* ', $file, " */\n";
                }
        }
    }
}


# Some extra strings that didn't get extracted, probably due to a simple bug,
# but should be included (remember folks, I don't speak Prle):

output "LILYPOND", "Copyright (c) xxxx Copyright Holder",
                                "default LilyPond/notation header";

output "LILYPOND", "Not Yet Titled", "default LilyPond/notation header";

output "LILYPOND", "not yet subtitled", "default LilyPond/notation header";

output "LILYPOND", "Unknown", "default LilyPond/notation header";


# In spite of the preceding comment, the following ones seem correctly extracted

# print 'QObject::tr("Pan");                                 /* default MIDI controller */
# ';
# print 'QObject::tr("Chorus");                              /* default MIDI controller */
# ';
# print 'QObject::tr("Volume");                              /* default MIDI controller */
# ';
# print 'QObject::tr("Reverb");                              /* default MIDI controller */
# ';
# print 'QObject::tr("Sustain");                             /* default MIDI controller */
# ';
# print 'QObject::tr("Expression");                          /* default MIDI controller */
# ';
# print 'QObject::tr("Modulation");                          /* default MIDI controller */
# ';
# print 'QObject::tr("PitchBend");                           /* default MIDI controller */
# ';


close INFO;

