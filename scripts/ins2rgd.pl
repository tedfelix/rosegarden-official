#!/usr/bin/perl

#
# ins2rgd.pl
# Takes a Sonar .ins instrument definition file on stdin
# emits an (uncompressed) .rgd, which the user then compresses and places
# in the appropriate directory (usually /usr/share/apps/rosegarden/library).
#
# (C)2005 Scott Doty <scott@sonic.net>
# Permission to copy granted in accordance with the Perl Artistic License.
#
# http://ponzo.net/Rosegarden/
#
# BUGS: This could still use some work -- there are hard-coded values that
#       could be configured...

my $devicename = shift;
my $librarian = shift;
my $librarian_email = shift
  || die qq{Usage: $0 "devicename" "librarian name" "librarian email" < file.ins > file.xml};

my $flag_in_names = undef;	# .Patch Names section
my $flag_in_defs = undef;	# .Instrument Definitions section
my $curbank = undef;		# current bank
my $banks = {};			# the in-memory banks database

# sussed from txt2rgd.py
%converter_table = (
  '&'	=>	'&amp;' ,
  '<'	=>	'&lt;'  ,
  '>'	=>	'&gt;'	,
  '"'	=>	'&quot;',
  "'"	=>	'&apos;'
);

LINE:
while(<>)
  {
s/\s+$//; # might be cr/lf too
  next if (/^\#/);
  next if (/^$/);
  next if (/^\;/);

  if(/^\.Patch Names/i)
    {
    $flag_in_names=1;
    $flag_in_defs=undef;
    next LINE;
    }

  if(/^\.Instrument Definitions/i)
    {
    $flag_in_defs=1;
    $flag_in_names=undef;
    next LINE;
    }


  if($flag_in_names)
    {
    if(m#^\[(.+)\]$#)
      {
      $curbank = $1;
      $banks->{$curbank} = {};
      next LINE;
      }
    if(m#^(\d+)\=(.+)$#)
      {
      my $voicenum = $1;
      my $voicename = $2;
      
      $voicename = doquote($voicename);

      $banks->{$curbank}->{$voicenum} = $voicename;
      }
    }
  

  if($flag_in_defs)
    {
    if(m#Patch\[(\d+)\]=(.+)$#)
      {
      my $banknum = $1;
      my $bankname = $2;
      
      $banknumof{$bankname}=$banknum;
      }
    }

  }

# end of .ins file -- write .rgd xml

# header
print '<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE rosegarden-data>
<rosegarden-data version="4-0.9.8_cvs">
<studio thrufilter="0" recordfilter="0" audioinputpairs="2" mixerdisplayoptions="0" metronomedevice="0">


<device id="0" name="',$devicename,'" direction="play" variation="" type="midi">
<librarian name="',$librarian,'" email="',$librarian_email,'"/>
<metronome instrument="2009" pitch="37" depth="2" barvelocity="120" beatvelocity="100" subbeatvelocity="80"/>
';

# generate list of banks sorted by bank number
@banks = sort { $banknumof{$a} <=> $banknumof{$b} } ( keys %banknumof);

for my $bank (@banks)
  {
  $msb = int( $banknumof{$bank} / 128 );
  $lsb = $banknumof{$bank} - (128*$msb);
  my $percussion;
  
  if ($bank =~ /drum/i)
    {
    $percussion='true';
    }
    else
    {
    $percussion='false';
    }
  
  print qq{<bank name="$bank" percussion="$percussion" msb="$msb" lsb="$lsb">};
  print "\n";

  @voicenums = sort
      { $a <=> $b }
      (keys %{ $banks->{$bank} } );

  for $voicenum (@voicenums)
    {
    $voicename = $banks->{$bank}->{$voicenum};
    print qq{\t<program id="$voicenum" name="$voicename"/>\n};
    }

  print "</bank>\n";

  }

print '
        <controls>
            <control name="Pan" type="controller" description="&lt;none&gt;" min="0" max="127" default="64" controllervalue="10" colourindex="2" ipbposition="0"/>
            <control name="Chorus" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="93" colourindex="3" ipbposition="1"/>
            <control name="Volume" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="7" colourindex="1" ipbposition="2"/>
            <control name="Reverb" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="91" colourindex="3" ipbposition="3"/>
            <control name="Sustain" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="64" colourindex="4" ipbposition="4"/>
            <control name="Expression" type="controller" description="&lt;none&gt;" min="0" max="127" default="100" controllervalue="11" colourindex="2" ipbposition="5"/>
            <control name="Modulation" type="controller" description="&lt;none&gt;" min="0" max="127" default="0" controllervalue="1" colourindex="4" ipbposition="-1"/>
            <control name="PitchBend" type="pitchbend" description="&lt;none&gt;" min="0" max="16383" default="8192" controllervalue="1" colourindex="4" ipbposition="-1"/>
        </controls>
        <instrument id="2000" channel="0" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2001" channel="1" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2002" channel="2" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2003" channel="3" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2004" channel="4" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2005" channel="5" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2006" channel="6" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2007" channel="7" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2008" channel="8" type="midi">
            <bank percussion="false" msb="0" lsb="0"/>
            <program id="0"/>
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2009" channel="9" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2010" channel="10" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2011" channel="11" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2012" channel="12" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2013" channel="13" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2014" channel="14" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

        <instrument id="2015" channel="15" type="midi">
            <pan value="64"/>
            <volume value="100"/>
        </instrument>

    </device>




</studio>


</rosegarden-data>
';

exit 0;

  
sub doquote
{
my $n = shift;

my $retval;

my @chars = split(//,$n);

for my $char (@chars)
  {
  if(exists($converter_table{$char}))
    {
    $retval .= $converter_table{$char};
    }
    else
    {
    $retval .= $char;
    }
  }

$retval;
}
