#!/usr/bin/perl -w
require 5;
use strict;

# Command line should contain a list of files to look in.
#
# A likely invocation would be
#  perl ./scripts/find-todo.pl */*.{[Ch],cpp}
#
# Unusual or binary files will be ignored.

my $todo_count = 0;
my $file_count = 0;
my $unfinished_count = 0;

sub print_file_maybe
{
    my ($pf, $file) = @_;
    if (! $$pf) {
	print "\n$file:\n";
	$$pf = 1;
	$file_count++;
    }
}

foreach (@ARGV) {

    my $file = $_;
    next if ($file =~ /~$/);
    open FILE, $file or next;
    my $in_todo = 0;
    my $printed_file = 0;
    my $line = 0;

    while (<FILE>) {

	$line++;
	last if (m.^[\200-\377].); # probably a binary file

	if ($in_todo) {

	    if (m, /[/\*](!!!)?\s* (.*) $ ,x) {
		print "          $2\n";
	    } else {
		$in_todo = 0;
	    }

	} else {

	    if (m, /[/\*] !!!\s* (.*) $ ,x) {
		print_file_maybe(\$printed_file, $file);
		print sprintf("%8d", $line), ": $1\n";
		$in_todo = 1;
		$todo_count++;

	    } elsif (m, /[/\*] \Q...\E \s* (.*) $ ,x) {
		print_file_maybe(\$printed_file, $file);
		print sprintf("%8d", $line), ": [U] $1\n";
		$unfinished_count++;
	    }
	}
    }

    close FILE;
}

print "\nTotal: $todo_count problem items, $unfinished_count unfinished " .
    "markers in $file_count files\n\n";

