#!/usr/bin/perl -w
use strict;
my $prevLine = "";

while (<>) {
    my $line = $_;

    if ($line =~ /pixels/) {
        $prevLine =~ s/ [G\#][\w\d][^\"]*/ None/;
    }

    print $prevLine;
    $prevLine = $line;
}

print $prevLine;
