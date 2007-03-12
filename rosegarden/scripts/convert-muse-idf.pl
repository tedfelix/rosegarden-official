#!/usr/bin/perl -w

use strict;

my %banknames;
my %banks;
my $devicename = "";
my $bankname = "";
my $lastbankno = -1;
my $goodbank = 1;

while (<>) {
  chomp;
  if (/<MidiInstrument.*\bname=\"([^\"]*)\"/) {
    $devicename = $1;
  }
  if (/<PatchGroup.*\bname=\"([^\"]*)\"/) {
    if ($lastbankno != -1 and $goodbank and $bankname ne "") {
      $banknames{$lastbankno} = $bankname;
    }
    $bankname = $1;
    $lastbankno = -1;
    $goodbank = 1;
  }
  if (/<Patch /) {
    my $line = $_;
    my ($name, $msb, $lsb, $program);
    if ($line =~ /\bname=\"([^\"]*)/) { $name = $1; }
    if ($line =~ /\bhbank=\"([^\"]*)/) { $msb = $1; }
    if ($line =~ /\blbank=\"([^\"]*)/) { $lsb = $1; }
    if ($line =~ /\bprog=\"([^\"]*)/) { $program = $1; }
    my $bankno = $msb * 128 + $lsb;
    if ($bankno != $lastbankno) {
      if ($lastbankno != -1) {
	$goodbank = 0;
      }
      $lastbankno = $bankno;
    }
    $banks{$bankno}->{$program} = [ $name, $bankname ];
  }
}
if ($lastbankno != -1 and $goodbank and $bankname ne "") {
  $banknames{$lastbankno} = $bankname;
}

print qq{<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE rosegarden-data>
<rosegarden-data version="4-0.9.1">

    <!-- Converted from MusE data by convert-muse-idf.pl -->
    <!-- Warning: Be sure to check bank names, variations and percussion settings -->

    <studio thrufilter="0" recordfilter="0">
    <device id="0" name="$devicename" direction="play" variation="" type="midi">

        <librarian name="" email=""/>
};

for my $bankno (sort { $a <=> $b } keys %banks) {

  my $bankname = "";
  if (exists $banknames{$bankno}) { $bankname = $banknames{$bankno}; }
  my $lsb = $bankno % 128;
  my $msb = ($bankno - $lsb) / 128;

  print qq{
	<bank name="$bankname" percussion="false" msb="$msb" lsb="$lsb">\n};

  my $programs = $banks{$bankno};

  for my $program (sort { $a <=> $b } keys %$programs) {
    print qq{	    <program id="$program" name="$programs->{$program}[0]"};
    if ($bankname ne $programs->{$program}[1]) { print qq{ category="$programs->{$program}[1]"}; }
    print qq{/>\n}; 
  }
  
  print qq{	</bank>\n};
}

print qq{
    </device>

</studio>

</rosegarden-data>
}

